#pragma once
#include <iostream>
#include <fstream>
#include "ocr_tables/core/ocr_tabs_engine.h"
#include "ocr_tables/helpers/auxiliary.h"
#include "ocr_tables/core/img_processor.h"
#include "ocr_tables/helpers/drawing_handler.h"
#include "ocr_tables/debug.h"
extern "C" {
	#include <mupdf/fitz.h>
}

using namespace cv;

#pragma warning( disable : 4018 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )

namespace ocrt {
	OCRTabsEngine::OCRTabsEngine() {
		fail = false;
		fail_msg = "";
		//tess.Init("..\\tessdata", "eng");
		tess.Init("tessdata", "ell");
		//tess.Init("tessdata", "eng");

		img_processor = ImageProcessor(test);

		std::setlocale(LC_ALL, "el_GR.UTF-8");
	}

	OCRTabsEngine::OCRTabsEngine(FileType filetype, const std::string& filename) : ocrt::OCRTabsEngine() {
		std::vector<cv::Mat> pages, pages_clean;
		
		ReadImage(pages, pages_clean, filetype, filename, "", false);
	}

	OCRTabsEngine::~OCRTabsEngine() {
		resetAll();
	}

	/**
	 * @brief Sets the Mat img we will work on from input img (Mat)
	 * @param img: img (Mat) to be copied
	*/
	void OCRTabsEngine::SetImage(Mat img) {
		//test=SegmentImage(img);
		test = img.clone();
		initial = test.clone();
	}

	/**
	 * Each input image is OCRed, in order to extract words and bounding boxes
	 * Then, we check for similar lines on the top and bottom of the images 
	 * in order to remove headers/footers
	 */
	void OCRTabsEngine::PrepareMulti1() {
		//RemoveGridLines();
		Page tmp_page;

		tmp_page.height = test.size().height;
		tmp_page.width = test.size().width;
		doc.pages.push_back(tmp_page);

		for (int i = 0; i < doc.words.size(); i++) {
			Line tmp_line;

			tmp_line.word_ids.push_back(i);
			int j;
			for (j = i + 1; j < doc.words.size(); j++) {
				// check for vertically-overlapping bboxes
				if (((doc.words[j-1].box[BOX_TOP] <= doc.words[j].box[BOX_TOP]) && (doc.words[j].box[BOX_TOP] <= doc.words[j-1].box[BOX_BOTTOM])) ||
					((doc.words[j].box[BOX_TOP] <= doc.words[j-1].box[BOX_TOP]) && (doc.words[j-1].box[BOX_TOP] <= doc.words[j].box[BOX_BOTTOM]))) {
					tmp_line.word_ids.push_back(j);
				} else {
					break;
				}
			}
			i = j - 1;
			doc.lines.push_back(tmp_line);
		}
		
		words_.push_back(doc.words);
		lines_.push_back(doc.lines);
		
		//test.release();
		doc.words.clear();
		doc.lines.clear();
	}

	/**
	 * @brief Preprocessing for multiple pages
	 * Concatenate pages (boxes, lines etc) as if there was one large page
	 */
	void OCRTabsEngine::PrepareMulti2() {
		for (int i = 1; i < words_.size(); i++) {
			int move = 0;
			for (int j = 0; j < i; j++) {
				move = move + doc.pages[j].height;
			}
			for (int j=0;j<words_[i].size();j++) {
				words_[i][j].box[BOX_TOP] = words_[i][j].box[BOX_TOP] + move;
				words_[i][j].box[BOX_BOTTOM] = words_[i][j].box[BOX_BOTTOM] + move;
			}
		}

		for (int i = 0; i < words_.size(); i++) {
			for (int j = 0; j < words_[i].size(); j++) {
				Word tmp_word;
				tmp_word.box = words_[i][j].box;
				tmp_word.font_size = words_[i][j].font_size;
				tmp_word.name = words_[i][j].name;
				tmp_word.conf = words_[i][j].conf;
				tmp_word.bold = words_[i][j].bold;
				tmp_word.italic = words_[i][j].italic;
				tmp_word.underscore = words_[i][j].underscore;
				tmp_word.dict = words_[i][j].dict;

				doc.words.push_back(tmp_word);
			}
		}
		words_.clear();
		lines_.clear();
		
		doc.page_bottom = doc.page_right = 0;
		doc.page_left = doc.pages[0].width;

		for (int i = 1; i < doc.pages.size(); i++) {
			doc.page_left = std::max(doc.page_left, doc.pages[i].width);
		}
		doc.page_top = 0;
		for (int i = 0; i < doc.pages.size(); i++) {
			doc.page_top = doc.page_top + doc.pages[i].height;
		}
		for (int i = 0; i < doc.words.size(); i++) {
			if (doc.words[i].box[BOX_LEFT] <= doc.page_left) { doc.page_left = doc.words[i].box[BOX_LEFT]; }
			if (doc.words[i].box[BOX_TOP] <= doc.page_top) { doc.page_top = doc.words[i].box[BOX_TOP]; }
			if (doc.words[i].box[BOX_RIGHT] >= doc.page_right) { doc.page_right = doc.words[i].box[BOX_RIGHT]; }
			if (doc.words[i].box[BOX_BOTTOM] >= doc.page_bottom) { doc.page_bottom = doc.words[i].box[BOX_BOTTOM]; }
		}

		OCR_LOG_MSG("\nPROCESSING OVERALL DOCUMENT\n\n");
	}

	/**
     * @brief Removes grid lines, because tesseract has a problem recognizing words when there are dark, dense gridlines
	 * @param ratio: ratio of input image resize
	 */
	void OCRTabsEngine::RemoveGridLines(float ratio /*=1*/) {
		OCR_LOG_MSG("Remove Grid Lines...");
		aux::startClock();

		Mat dst;

		//threshold( test, dst, 100, 255,1 );
		threshold(test, dst, 200, 255, cv::THRESH_BINARY);  //creates binary img
		//erode(dst,dst,Mat(),Point(-1,-1),2);
		uchar* data = (uchar*)dst.data;
		//cvtColor(test,test,CV_GRAY2BGR);

		//check columns for black lines and remove them
		for (int i = 0; i < dst.cols; i++) {
			for (int j = 0; j < dst.rows; j++) {
				int counter = 0;
				if (data[i + j * dst.cols] == 0) {
					while ((j < (dst.rows - 1)) && (data[i + (j + 1) * dst.cols] == 0)) {
						counter++;
						j++;
					}
				}
				if (counter > 120 * ratio) {
					line(test, Point(i, j - counter), Point(i, j), Scalar(255, 255, 255), 3);
				}
			}
		}
		//check rows (?) for black lines and remove them
		for (int i = 0; i < dst.rows * dst.cols; i++) {
			int counter = 0;
			if (data[i] == 0) {
				while ((i < (dst.rows * dst.cols - 1)) && (data[i + 1] == 0)) {
					counter++;
					i++;
				}
			}
			if (counter > 60 * ratio) {
				line(test, Point((i - counter) % (dst.cols), (int)(i - counter) / dst.cols), Point((i) % (dst.cols), (int)(i) / dst.cols), Scalar(255, 255, 255), 3);
			}
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/**
	 * @brief Tesseract recognizes all data in the image
	 */
	void OCRTabsEngine::OCR_Recognize() {
		//resize(test,test,Size(test.size().width*2,test.size().height*2));
		//tess.Init("..\\tessdata", "eng");
		tess.SetImage((uchar*)test.data, test.size().width, test.size().height, test.channels(), test.step1());
		//tess.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!-*()");
		OCR_LOG_MSG("Recognizing...");
		aux::startClock();

		tess.Recognize(NULL);
		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/**
	 * @brief Retrieve all the recognized words and their bounding boxes from tesseract
	 */
	void OCRTabsEngine::FindBoxesAndWords() {
		//tess.SetPageSegMode( tesseract::PSM_AUTO_OSD);
		//tesseract::PageIterator* ri = tess.AnalyseLayout();
		tesseract::ResultIterator* ri = tess.GetIterator();
		std::cout << "now I'm there" << std::endl;

		OCR_LOG_MSG("Get bounding Boxes...");
		aux::startClock();
		do {
			int left, top, right, bottom;
			left = 0; top = 0; right = 0; bottom = 0;
			ri->BoundingBox(tesseract::RIL_WORD, &left, &top, &right, &bottom);

			// Try to discard "noise" recognized as letters
		//	if ((right-left>=10)/*&&(bottom-top>=3)*/&&(ri->Confidence(tesseract::RIL_WORD)>30))
		//	{
			bool is_bold, is_italic, is_underlined, b, c, d;
			int point_size, id;
			ri->WordFontAttributes(&is_bold, &is_italic, &is_underlined, &b, &c, &d, &point_size, &id);
			//		if (point_size>=2)
			//		{
			Word word;
			word.conf = ri->Confidence(tesseract::RIL_WORD);
			word.name = ri->GetUTF8Text(tesseract::RIL_WORD);
			word.font_size = point_size;
			word.bold = is_bold;
			word.italic = is_italic;
			word.underscore = is_underlined;
			word.dict = ri->WordIsFromDictionary();

			word.box.push_back(left);
			word.box.push_back(top);
			word.box.push_back(right);
			word.box.push_back(bottom);

			doc.words.push_back(word);
			
			//		}
			//	}
		} while (ri->Next(tesseract::RIL_WORD));

		RemoveFigures();

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Find the text boundaries of the whole image
	void OCRTabsEngine::FindTextBoundaries() {
		OCR_LOG_MSG("Find text boundaries...");
		aux::startClock();

		doc.page_bottom = doc.page_right = 0;
		doc.page_left = test.size().width;
		doc.page_top = test.size().height;

		for (int i = 0; i < doc.words.size(); i++) {
			if (doc.words[i].box[BOX_LEFT] <= doc.page_left) { doc.page_left = doc.words[i].box[BOX_LEFT]; }
			if (doc.words[i].box[BOX_TOP] <= doc.page_top) { doc.page_top = doc.words[i].box[BOX_TOP]; }
			if (doc.words[i].box[BOX_RIGHT] >= doc.page_right) { doc.page_right = doc.words[i].box[BOX_RIGHT]; }
			if (doc.words[i].box[BOX_BOTTOM] >= doc.page_bottom) { doc.page_bottom = doc.words[i].box[BOX_BOTTOM]; }
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Create lines by assigning vertically-overlapping word boxes to the same unique line
	void OCRTabsEngine::CreateTextLines() {
		OCR_LOG_MSG("Find lines...");
		aux::startClock();

		for (int i = 0; i < doc.words.size(); i++) {
			Line tmp_line;
			tmp_line.word_ids.push_back(i);
			int j;
			for (j = i + 1; j < doc.words.size(); j++) {
				// check vetically-overlapping bboxes
				// IMPORTANT: counts (probably incorrectly) boxes that slightly overlap in same line
				if (((doc.words[j - 1].box[BOX_TOP] <= doc.words[j].box[BOX_TOP]) && (doc.words[j].box[BOX_TOP] <= doc.words[j - 1].box[BOX_BOTTOM])) ||
					((doc.words[j].box[BOX_TOP] <= doc.words[j - 1].box[BOX_TOP]) && (doc.words[j - 1].box[BOX_TOP] <= doc.words[j].box[BOX_BOTTOM]))) {
					tmp_line.word_ids.push_back(j);
				} else {
					break;
				}
			}
			i = j - 1;
			
			doc.lines.push_back(tmp_line);  //assign them to the same line
		}

		// Find line dimensions (top, bottom, left, right)
		for (int i = 0; i < doc.lines.size(); i++) {
			int* tmp = new int[2];
			tmp[0] = doc.page_bottom;
			tmp[1] = doc.page_top;
			for (int j = 0; j < doc.lines[i].word_ids.size(); j++) {
				if (doc.words[doc.lines[i].word_ids[j]].box[BOX_TOP] <= tmp[LINE_TOP]) { tmp[LINE_TOP] = doc.words[doc.lines[i].word_ids[j]].box[BOX_TOP]; }
				if (doc.words[doc.lines[i].word_ids[j]].box[BOX_BOTTOM] >= tmp[LINE_BOTTOM]) { tmp[LINE_BOTTOM] = doc.words[doc.lines[i].word_ids[j]].box[BOX_BOTTOM]; }
			}
			doc.lines[i].dims = tmp;
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	void OCRTabsEngine::RemoveHeadersFooters() {
		int* header_limit=new int [lines_.size()];
		int* footer_limit=new int [lines_.size()];
		for (int i = 0; i < lines_.size(); i++) {
			header_limit[i] = -1;
			footer_limit[i] = -1;
		}
		for (int i = 0; i < lines_.size() - 1; i++) {
			//search for similar headers/footers in consecutive pages
			for (int k = 0; k < std::min(lines_[i].size(), lines_[i + 1].size()); k++) {
				string tmp1;
				string tmp2;
				for (int j = 0; j < lines_[i][k].word_ids.size(); j++) {
					tmp1.append(words_[i][lines_[i][k].word_ids[j]].name);
				}
				for (int j = 0; j < lines_[i + 1][k].word_ids.size(); j++) {
					tmp2.append(words_[i + 1][lines_[i + 1][k].word_ids[j]].name);
				}
				int identical_counter = 0;
				for (int s = 0; s < std::min(tmp1.length(), tmp2.length()); s++) {
					if (tmp1[s] == tmp2[s]) {
						identical_counter++;
					}
				}
				if ((std::max(tmp1.length(), tmp2.length()) - identical_counter) <= 2) {
					header_limit[i + 1] = k;
					if ((i != 0) && (header_limit[i] < k)) {
						header_limit[i] = k;
					}	
				} else {
					k = std::min(lines_[i].size(), lines_[i + 1].size());
				}
			}
			string tmp1;
			string tmp2;
			for (int j = 0; j < lines_[i][lines_[i].size() - 1].word_ids.size(); j++) {
				tmp1.append(words_[i][lines_[i][lines_[i].size() - 1].word_ids[j]].name);
			}
			for (int j = 0; j < lines_[i + 1][lines_[i + 1].size() - 1].word_ids.size(); j++) {
				tmp2.append(words_[i + 1][lines_[i + 1][lines_[i + 1].size() - 1].word_ids[j]].name);
			}
			int identical_counter = 0;
			for (int s = 0; s < std::min(tmp1.length(), tmp2.length()); s++) {
				if (tmp1[s] == tmp2[s]) {
					identical_counter++;
				}
			}
			if ((std::max(tmp1.length(), tmp2.length()) - identical_counter) <= 2) {
				footer_limit[i] = lines_[i].size() - 1;
				footer_limit[i + 1] = lines_[i + 1].size() - 1;
			}
			if (i < (lines_.size() - 2)) {
				for (int k = 0; k < std::min(lines_[i].size(), lines_[i + 2].size()); k++) {
					tmp1.clear();
					tmp2.clear();
					for (int j = 0; j < lines_[i][k].word_ids.size(); j++) {
						tmp1.append(words_[i][lines_[i][k].word_ids[j]].name);
					}
					for (int j = 0; j < lines_[i + 2][k].word_ids.size(); j++) {
						tmp2.append(words_[i + 2][lines_[i + 2][k].word_ids[j]].name);
					}
					identical_counter = 0;
					for (int s = 0; s < std::min(tmp1.length(), tmp2.length()); s++) {
						if (tmp1[s] == tmp2[s]) {
							identical_counter++;
						}
					}
					if ((std::max(tmp1.length(), tmp2.length()) - identical_counter) <= 2) {
						header_limit[i + 2] = k;
						if ((i != 0) && (header_limit[i] < k)) {
							header_limit[i] = k;
						}		
					} else {
						k = std::min(lines_[i].size(), lines_[i + 2].size());
					}
				}
				tmp1.clear();
				tmp2.clear();
				for (int j = 0; j < lines_[i][lines_[i].size() - 1].word_ids.size(); j++) {
					tmp1.append(words_[i][lines_[i][lines_[i].size() - 1].word_ids[j]].name);
				}
				for (int j = 0; j < lines_[i + 2][lines_[i + 2].size() - 1].word_ids.size(); j++) {
					tmp2.append(words_[i + 2][lines_[i + 2][lines_[i + 2].size() - 1].word_ids[j]].name);
				}
				identical_counter=0;
				for (int s = 0; s < std::min(tmp1.length(), tmp2.length()); s++) {
					if (tmp1[s] == tmp2[s]) {
						identical_counter++;
					}
				}
				if ((std::max(tmp1.length(), tmp2.length()) - identical_counter) <= 2) {
					footer_limit[i] = lines_[i].size() - 1;
					footer_limit[i + 2] = lines_[i + 2].size() - 1;
				}
			}
		}

		/*for (int i = lines_.size() - 1; i >= 0; i--) {
			if (footer_limit[i] != (-1)) {
				for (int j = lines_[i][footer_limit[i]].word_ids.size() - 1; j >= 0; j--) {
					words_[i].erase(words_[i].begin() + lines_[i][footer_limit[i]].word_ids[j]);
				}
			}
			for (int k = header_limit[i]; k >= 0; k--) {
				for (int j = lines_[i][k].word_ids.size() - 1; j >= 0; j--) {
					words_[i].erase(words_[i].begin() + lines_[i][k].word_ids[j]);
				}
			}
		}*/
	}

	/**
	 * @brief Create text Segments for each line.
	 * If the horizontal distance between two word boxes is smaller than a threshold,
	 * they will be considered as a single text segment
	 */
	void OCRTabsEngine::CreateLineSegments() {
		OCR_LOG_MSG("Find line segments...");
		aux::startClock();

		float ratio = 0.6 * 2; //1.2

		for (int i = 0; i < doc.lines.size(); i++) {
			/*if (Lines[i].size() == 1) {
				vector<vector<int>> segments;
				vector<int> tmp;
				tmp.push_back(Lines[i][0]);
				segments.push_back(tmp);
				Lines_segments.push_back(segments);
			} else {*/
			float hor_thresh = (doc.lines[i].dims[LINE_BOTTOM] - doc.lines[i].dims[LINE_TOP]) * ratio;
			vector<vector<int>> segments;
			vector<int> tmp;
			vector<vector<int>> segments_dims;
			vector<int> tmp_seg_dims;
			tmp.push_back(doc.lines[i].word_ids[0]);
			for (int j = 1; j < doc.lines[i].word_ids.size(); j++) {
				if (doc.words[doc.lines[i].word_ids[j]].box[BOX_LEFT] - doc.words[doc.lines[i].word_ids[j - 1]].box[BOX_RIGHT] <= hor_thresh) {
					tmp.push_back(doc.lines[i].word_ids[j]);
				} else {
					/*cout << endl << "BOXES" << endl;
					for (int k = 0; k < tmp.size(); k++) {
						cout << "[" << boxes[tmp[k]][BOX_LEFT] << "," << boxes[tmp[k]][BOX_RIGHT] << "] ";
					}*/
					tmp_seg_dims.push_back(doc.words[FindMinLeftBoxInSegment(tmp)].box[BOX_LEFT]);
					tmp_seg_dims.push_back(doc.words[FindMaxRightBoxInSegment(tmp)].box[BOX_RIGHT]);
					segments.push_back(tmp);
					segments_dims.push_back(tmp_seg_dims);
					tmp.clear();
					tmp_seg_dims.clear();
					tmp.push_back(doc.lines[i].word_ids[j]);
				}
			}

			tmp_seg_dims.push_back(doc.words[FindMinLeftBoxInSegment(tmp)].box[BOX_LEFT]);
			tmp_seg_dims.push_back(doc.words[FindMaxRightBoxInSegment(tmp)].box[BOX_RIGHT]);
			doc.lines[i].segments.push_back(tmp);
			doc.lines[i].segments_dims.push_back(tmp_seg_dims);
			
			/*cout << endl << endl;
			for (int k = 0; k < line_segments_dims.size(); k++) {
				for (int l = 0; l < line_segments_dims[k].size(); l++) {
					cout << "[" << line_segments_dims[k][l][SEG_LEFT] << "," << line_segments_dims[k][l][SEG_RIGHT] << "] ";
				}
			}*/
			//}s
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/**
	 * Type 1 - TEXT : Lines with a single long segment (longer than half of the length of the line)
	 * Type 2 - TABLE : Lines with multiple segments
	 * Type 3 - UNKNOWN : Lines with a single short segment
	 */
	void OCRTabsEngine::AssignLineTypes() {
		OCR_LOG_MSG("Find line types...");
		aux::startClock();

		float sum = 0;
		for (int i = 0; i < doc.lines.size(); i++) {
			if (doc.lines[i].segments.size() > 1) { doc.lines[i].type = LineType::TABLE; }
			else {
				int seg_left = doc.words[doc.lines[i].segments[0][0]].box[BOX_LEFT];
				int seg_right = doc.words[doc.lines[i].segments[0][doc.lines[i].segments[0].size() - 1]].box[BOX_RIGHT];
				//if ((seg_right-seg_left)>=(page_right-page_left)/2){Lines_type[i]=1;}
				if ((seg_right - seg_left) >= (float)(doc.page_right - doc.page_left) / 2.5) { doc.lines[i].type = LineType::TEXT; }
				else if (seg_left >= (doc.page_right - doc.page_left) / 4) { doc.lines[i].type = LineType::TABLE; }
				else { doc.lines[i].type = LineType::UNKNOWN; }
			}

			if (i > 1 && doc.lines[i].type == LineType::TABLE && doc.lines[i - 2].type == LineType::TABLE) { doc.lines[i - 1].type = LineType::TABLE; }

			/*if (Lines_segments[i].size()==2)
			{
				int seg_left1=boxes[Lines_segments[i][0][0]][0];
				int seg_right1=boxes[Lines_segments[i][0][Lines_segments[i][0].size()-1]][2];
				int seg_left2=boxes[Lines_segments[i][1][0]][0];
				int seg_right2=boxes[Lines_segments[i][1][Lines_segments[i][1].size()-1]][2];
				if (((seg_right1-seg_left1)>=(page_right-page_left)/2)||
					((seg_right2-seg_left2)>=(page_right-page_left)/2))
				{Lines_type[i]=3;}
			}*/
			sum = sum + doc.lines[i].dims[LINE_BOTTOM] - doc.lines[i].dims[LINE_TOP];
		}
		sum = (float)sum / doc.lines.size();

		/*// FOOTER/HEADER - Line height must be smaller than thw average line height

		// Header must begin from the first line, can have a max of 5 lines, header-lines must be consequential
		float ratio=0.95;
		for (int i=0;i<(std::min((int)Lines.size(),2));i++)
		{
			if ((Line_dims[i][LINE_BOTTOM]-Line_dims[i][LINE_TOP])<=ratio*sum)
			{
				if (i==0)
				{
					Lines_type[i]=4;
				}
				else if (Lines_type[i-1]==4)
				{
					Lines_type[i]=4;
				}
				else {i=Lines.size();}
			}
		}

		// Footer must begin from the last line, footer-lines must be consequential

		for (int i=(Lines.size()-1);i>=0;i--)
		{
			if ((Line_dims[i][LINE_BOTTOM]-Line_dims[i][LINE_TOP])<=ratio*sum)
			{
				if (i==Lines.size()-1)
				{
					Lines_type[i]=4;
				}
				else if (Lines_type[i+1]==4)
				{
					Lines_type[i]=4;
				}
				else {i=-1;}
			}
		}
		*/

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/**
	 * @brief Find areas that can potentially be real tables.
	 * Such areas include consequential type-2 and type-3 lines.
	 */
	void OCRTabsEngine::FindTableAreas() {
		OCR_LOG_MSG("Find table areas...");
		aux::startClock();
		vector<int> tmp;

		if (doc.lines[0].type != LineType::TEXT) { tmp.push_back(0); }
		for (int i = 1; i < doc.lines.size(); i++) {
			if ((doc.lines[i].type != LineType::TEXT) && (doc.lines[i].type != 4)) {
				if ((doc.lines[i - 1].type != LineType::TEXT) && (doc.lines[i - 1].type != 4) && ((doc.lines[i].dims[0] - doc.lines[i - 1].dims[1]) <= 3 * (doc.lines[i].dims[1] - doc.lines[i].dims[0]))) {
					tmp.push_back(i);
				} else {
					if ((tmp.size() > 1) || ((tmp.size() == 1) && (doc.lines[tmp[0]].type == LineType::TABLE))) {
						table_area.push_back(tmp);
					}
					tmp.clear();
					tmp.push_back(i);
				}
			}
		}
		if ((tmp.size() > 1) || ((tmp.size() == 1) && (doc.lines[tmp[0]].type == LineType::TABLE))) {
			table_area.push_back(tmp);
		}
		tmp.clear();

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/**
	 * @brief Assign Rows to tables. Each table must start with a type-2 line.
	 * So if there are initially type-3 without a type-2 line over them,
	 * they are not assigned to the table, unless they are not left-aligned
	 */
	void OCRTabsEngine::AssignRowsToTables() {
		OCR_LOG_MSG("Find table rows...");
		aux::startClock();

		for (int i = 0; i < table_area.size(); i++) {
			Table tmp_table;
			bool t_flag = false;
			for (int j = 0; j < table_area[i].size(); j++) {
				if ((doc.lines[table_area[i][j]].type == LineType::TABLE) || t_flag) {
					t_flag = true;
					tmp_table.rows.push_back(table_area[i][j]);
				}
			}
			if (tmp_table.rows.size() > 0) { doc.tables.push_back(tmp_table); }
		}

		// Tables that end up with just one Row are dismissed
		for (int i = 0; i < doc.tables.size(); i++) {
			if (doc.tables[i].rows.size() < 2) {
				doc.lines[doc.tables[i].rows[0]].type = LineType::TEXT;
				doc.tables[i].rows.erase(doc.tables[i].rows.begin() + i);
			}
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Assign Columns to each table
	void OCRTabsEngine::CreateTableColumns() {
		OCR_LOG_MSG("Find table columns...");
		aux::startClock();

		for (int i = 0; i < doc.tables.size(); i++) {
			// Find a segment to initalize columns
			// First we select the left-most segment
			// Then we find all the segments that almost vertically align (on the left) with this segment, and we find their average length
			// Then we choose as a column creator the segment that is closest to the average length (or slightly bigger). 
			// We proceed by assigning to the final column all the segments that horizontally overlap with the column creator
			// Next we select the left-most segment that is not a part of the previous columns 
			// and we repeat the same process to create the next column
			// Some large segments can be assigned to more than one columns
			vector<vector<int>> column_creator;  //segment used as column generator
			int limit = -1;
			vector<int> min_seg;
			min_seg = doc.lines[doc.tables[0].rows[0]].segments[0];  //left-most segment
			bool end_of_table = false;

			//FIND COLUMN GENERATORS
			while (!end_of_table) {
				duration = aux::endClock(); /*(std::clock() - start) / CLOCKS_PER_SEC;*/
				
				if (duration > 30) {
					end_of_table = true;
					i = doc.tables.size();
					fail = true;
					fail_msg = "Find Table Columns task failed";
					break;
				}

				//find left-most segment in current table area
				for (int j = 0; j < doc.tables[i].rows.size(); j++) {
					for (int k = 0; k < doc.lines[doc.tables[i].rows[j]].segments.size(); k++) {
						int line_id = doc.tables[i].rows[j];
						//int first_box_id = line_segments[line_id][k][0];
						//int last_box_id = line_segments[line_id][k][line_segments[line_id][k].size() - 1];
						////int left = boxes[line_segments[table_rows[i][j]][k][0]][BOX_LEFT];
						////int right = boxes[line_segments[table_rows[i][j]][k][line_segments[table_rows[i][j]][k].size() - 1]][BOX_RIGHT];
						//int left = boxes[first_box_id][BOX_LEFT];
						//int right = boxes[last_box_id][BOX_RIGHT];
						int left = doc.lines[line_id].segments_dims[k][SEG_LEFT];
						int right = doc.lines[line_id].segments_dims[k][SEG_RIGHT];
						int min_left = doc.words[FindMinLeftBoxInSegment(min_seg)].box[BOX_LEFT];

						if ((left <= min_left)/*words[min_seg[0]][BOX_LEFT])*/ && (left > limit)) {
							//min_seg = line_segments[table_rows[i][j]][k];
							min_seg = doc.lines[line_id].segments[k];
						}
					}
				}
				//cout << "min segment #" << min_seg[0] << "  " << words[min_seg[0]][BOX_LEFT] << "," << words[min_seg[0]][BOX_RIGHT] << endl;

				float avg_seg_len = 0;
				int counter = 0;
				int hor_thresh = (doc.lines[doc.tables[i].rows[(int)doc.tables[i].rows.size() / 2]].dims[LINE_BOTTOM] - doc.lines[doc.tables[i].rows[(int)doc.tables[i].rows.size() / 2]].dims[LINE_TOP]) * 2.6;
				for (int j = 0; j < doc.tables[i].rows.size(); j++) {
					for (int k = 0; k < doc.lines[doc.tables[i].rows[j]].segments.size(); k++) {
						int line_id = doc.tables[i].rows[j];
						/*int first_box_id = line_segments[line_id][k][0];
						int last_box_id = line_segments[line_id][k][line_segments[line_id][k].size() - 1];

						int left = words[first_box_id][BOX_LEFT];
						int right = words[last_box_id][BOX_RIGHT];*/
						int left = doc.lines[line_id].segments_dims[k][SEG_LEFT];
						int right = doc.lines[line_id].segments_dims[k][SEG_RIGHT];
						int min_left = doc.words[FindMinLeftBoxInSegment(min_seg)].box[BOX_LEFT];

						// calc avg length of segments horizontally-aligned with min_seg
						if ((abs(left - min_left/*words[min_seg[0]][BOX_LEFT]*/) <= hor_thresh) &&
							/*((right-left)<=(words[min_seg[min_seg.size()-1]][2]-words[min_seg[0]][0]))&&*/
							(left > limit)) {
							//min_seg=Lines_segments[table_Rows[i][j]][k];
							avg_seg_len = avg_seg_len + (right - left);
							counter++;
						}
					}
				}
				//int fin_left = words[min_seg[0]][BOX_LEFT];
				int fin_left = doc.words[FindMinLeftBoxInSegment(min_seg)].box[BOX_LEFT];
				avg_seg_len = (float)avg_seg_len / counter;
				float overlap_ratio = 1.2;
				avg_seg_len = avg_seg_len * overlap_ratio;
				//cout << "avg seg len: " << avg_seg_len << endl;
				
				for (int j = 0; j < doc.tables[i].rows.size(); j++) {
					for (int k = 0; k < doc.lines[doc.tables[i].rows[j]].segments.size(); k++) {
						int line_id = doc.tables[i].rows[j];
						/*int left_box_id = line_segments[line_id][k][0];
						int right_box_id = line_segments[line_id][k][line_segments[line_id][k].size() - 1];

						int left = words[left_box_id][BOX_LEFT];
						int right = words[right_box_id][BOX_RIGHT];*/
						int left = doc.lines[line_id].segments_dims[k][SEG_LEFT];
						int right = doc.lines[line_id].segments_dims[k][SEG_RIGHT];
						int min_left = doc.words[FindMinLeftBoxInSegment(min_seg)].box[BOX_LEFT];
						int min_right = doc.words[FindMaxRightBoxInSegment(min_seg)].box[BOX_RIGHT];

						// Select the segment that is closest to the avg length
						if ((abs(left - fin_left) <= hor_thresh) &&
							(abs((right - left) - avg_seg_len) <= abs((min_right-min_left/*words[min_seg[min_seg.size() - 1]][BOX_RIGHT] - words[min_seg[0]][BOX_LEFT]*/) - avg_seg_len)) &&
							(left > limit)) {
							min_seg = doc.lines[line_id].segments[k];
						}
					}
				}

				//cout << "column creator #" << min_seg[0] << "  " << words[min_seg[0]][BOX_LEFT] << "," << words[min_seg[min_seg.size() - 1]][BOX_RIGHT] << endl;
				column_creator.push_back(min_seg);
				int max_box_id = FindMaxRightBoxInSegment(min_seg);  //there aren't sorted
				//limit = words[min_seg[min_seg.size() - 1]][BOX_RIGHT];  //set left-most limit for remaining segments
				limit = doc.words[max_box_id].box[BOX_RIGHT];
				min_seg.clear();
				
				// find new min_seg initial value for new limit
				for (int j = 0; j < doc.tables[i].rows.size(); j++) {
					for (int k = 0; k < doc.lines[doc.tables[i].rows[j]].segments.size(); k++) {
						//int left = words[line_segments[table_rows[i][j]][k][0]][BOX_LEFT];
						int left = doc.words[FindMinLeftBoxInSegment(doc.lines[doc.tables[i].rows[j]].segments[k])].box[BOX_LEFT];
						if (left > limit) {
							min_seg = doc.lines[doc.tables[i].rows[j]].segments[k];
							break;
						}
					}
				}
				/*cout << "new limit: " << limit << endl;
				cout << "NEW min segment #" << min_seg[0] << "  " << words[min_seg[0]][BOX_LEFT] << "," << words[min_seg[0]][BOX_RIGHT] << endl << endl;*/

				if (min_seg.size() == 0) { end_of_table = true; }
			}
			tmp_col.push_back(column_creator);
		}

		if (fail) return;
		
		// Assign all the segments that horizontally overlap with the column generator to a new column.
		for (int i = 0; i < tmp_col.size(); i++) {
			vector<vector<vector<int>>> t_col;
			for (int j = 0; j < tmp_col[i].size(); j++) {
				//int col_left = words[tmp_col[i][j][0]][BOX_LEFT]; //left side of first box of column creator (segment)
				//int col_right = words[tmp_col[i][j][tmp_col[i][j].size() - 1]][BOX_RIGHT]; //right side of last box of column creator (segment)
				int col_left = doc.words[FindMinLeftBoxInSegment(tmp_col[i][j])].box[BOX_LEFT];
				int col_right = doc.words[FindMaxRightBoxInSegment(tmp_col[i][j])].box[BOX_RIGHT];
				
				vector<vector<int>> t_seg;
				std::cout << std::endl << "this column: (" << col_left << "," << col_right << ")" << std::endl;
				for (int k = 0; k < doc.tables[i].rows.size(); k++) {
					int line_id = doc.tables[i].rows[k];
					for (int z = 0; z < doc.lines[line_id].segments.size(); z++) {
						//int seg_left = words[line_segments[line_id][z][0]][BOX_LEFT];
						//int seg_right = words[line_segments[line_id][z][line_segments[line_id][z].size() - 1]][BOX_RIGHT];
						int seg_left = doc.lines[line_id].segments_dims[z][SEG_LEFT];
						int seg_right = doc.lines[line_id].segments_dims[z][SEG_RIGHT];

						/*if (((seg_right >= col_left) && (seg_right <= col_right)) ||
							((seg_left >= col_left) && (seg_left <= col_right)) ||
							((seg_left <= col_left) && (seg_right >= col_right))) {
							t_seg.push_back(line_segments[line_id][z]);

							cout << seg_left << " " << seg_right << endl;
						}*/
						if ((seg_right >= col_left) && (seg_left <= col_right)) {
							t_seg.push_back(doc.lines[line_id].segments[z]);

							std::cout << seg_left << " " << seg_right << std::endl;
						}
					}
				}
				std::cout << std::endl;

				//if (t_seg.size() > 0)
					t_col.push_back(t_seg);
			}
			doc.tables[i].columns = t_col;  //CAREFUL
		}

		ProcessGeneratedColumns();
	}

	// Create table rows that include more than one lines
	void OCRTabsEngine::CreateTableMultiRows() {
		OCR_LOG_MSG("Find table multiple-rows...");
		aux::startClock();

		// if a table line does not have a segment in the first column, and there is one-to-one column correspondence 
		// with the line above it, 
		// it is merged with the one above it
		// this does not apply for the first line of the table
		for (int i = 0; i < doc.tables.size(); i++) {
			vector<vector<int>> tmp_multi_row;
			vector<int> tmp_multi_lines;
			tmp_multi_lines.push_back(doc.tables[i].rows[0]);
			for (int j = 1; j < doc.tables[i].rows.size(); j++) {
				bool has_seg_in_1st_col = false;
				if (std::find(doc.tables[i].columns[0].begin(), doc.tables[i].columns[0].end(), doc.lines[doc.tables[i].rows[j]].segments[0]) != doc.tables[i].columns[0].end()) {
					has_seg_in_1st_col = true;
				}
				if (!has_seg_in_1st_col) {
					bool exist_all = true;
					for (int k = 0; k < doc.lines[doc.tables[i].rows[j]].segments.size(); k++) {
						bool exist = false;
						for (int s = 0; s < doc.tables[i].columns.size(); s++) {
							bool exist1 = false;  //true if k seg of j line is in s column
							bool exist0 = false;  //true if z seg of j-1 line is in s column
							for (int h = 0; h < doc.tables[i].columns[s].size(); h++) {
								if (doc.lines[doc.tables[i].rows[j]].segments[k] == doc.tables[i].columns[s][h]) {
									exist1 = true;
								}
								for (int z = 0; z < doc.lines[doc.tables[i].rows[j - 1]].segments.size(); z++) {
									if (doc.lines[doc.tables[i].rows[j - 1]].segments[z] == doc.tables[i].columns[s][h]) {
										exist0 = true;
										if ((k < doc.lines[doc.tables[i].rows[j]].segments.size() - 1) &&
											/*(words[line_segments[table_rows[i][j - 1]][z][line_segments[table_rows[i][j - 1]][z].size() - 1]][BOX_RIGHT] >= words[line_segments[table_rows[i][j]][k + 1][0]][BOX_LEFT])*/
											doc.lines[doc.tables[i].rows[j - 1]].segments_dims[z][SEG_RIGHT] >= doc.lines[doc.tables[i].rows[j]].segments_dims[k + 1][SEG_LEFT]) {
											exist0 = false;
										}
									}
								}
							}
							if ((exist1) && (exist0)) { s = doc.tables[i].columns.size(); exist = true; }
						}
						exist_all = (exist_all) && (exist);
					}
					if (exist_all) {
						tmp_multi_lines.push_back(doc.tables[i].rows[j]);
					} else {
						tmp_multi_row.push_back(tmp_multi_lines);
						tmp_multi_lines.clear();
						tmp_multi_lines.push_back(doc.tables[i].rows[j]);
					}
				} else {
					tmp_multi_row.push_back(tmp_multi_lines);
					tmp_multi_lines.clear();
					tmp_multi_lines.push_back(doc.tables[i].rows[j]);
				}
			}
			tmp_multi_row.push_back(tmp_multi_lines);
			doc.tables[i].multi_rows = tmp_multi_row;
			tmp_multi_lines.clear();
			tmp_multi_row.clear();

			// if a line type-3 line, has its segment assigned ONLY to the first column,
			// and the line below it is type-2, and it has a segment in the fist column which is more to the
			// right than the segment of the first line THEN these two lines are merged together
			for (int j = 0; j < doc.tables[i].multi_rows.size() - 1; j++) {
				int line_id = doc.tables[i].multi_rows[j][0];
				int next_line_id = doc.tables[i].multi_rows[j + 1][0];
				if ((doc.tables[i].multi_rows[j].size() == 1) &&
					(doc.lines[line_id].type == LineType::UNKNOWN) && (doc.lines[line_id].segments.size() == 1) &&
					(doc.lines[next_line_id].type == LineType::TABLE)) {
					bool found1 = false;  //true if j line's single segment is in first col
					bool found2 = false;  //true if (j+1) line's seg is in first col
					if (std::find(doc.tables[i].columns[0].begin(), doc.tables[i].columns[0].end(), doc.lines[line_id].segments[0]) != doc.tables[i].columns[0].end()) {
						found1 = true;
					}
					if (std::find(doc.tables[i].columns[0].begin(), doc.tables[i].columns[0].end(), doc.lines[next_line_id].segments[0]) != doc.tables[i].columns[0].end()) {
						found2 = true;
					}
					//int tmp = line_segments[multi_rows[i][j][0]][0][0];
					//float lft1 = words[tmp][BOX_LEFT];
					//tmp = line_segments[multi_rows[i][j + 1][0]][0][0];
					//float lft2 = words[tmp][BOX_LEFT];
					float lft1 = doc.lines[line_id].segments_dims[0][SEG_LEFT];
					float lft2 = doc.lines[next_line_id].segments_dims[0][SEG_LEFT];
					if ((found1) && (found2) && ((lft2 - lft1) >= 0.6 * (doc.lines[next_line_id].dims[LINE_BOTTOM] - doc.lines[next_line_id].dims[LINE_TOP]))) {
						for (int z = 0; z < doc.tables[i].multi_rows[j + 1].size(); z++) {
							doc.tables[i].multi_rows[j].push_back(doc.tables[i].multi_rows[j + 1][z]);
						}
						doc.tables[i].multi_rows.erase(doc.tables[i].multi_rows.begin() + j + 1);  //remove (j+1) row
					}
				}
			}

			// if last row is single, unknown line with one segment and that segment is in 1st column,
			// remove this line from table and turn into textline. also delete 1st column (?).
			int last_line_id = doc.tables[i].multi_rows[doc.tables[i].multi_rows.size() - 1][0];
			if ((doc.tables[i].multi_rows[doc.tables[i].multi_rows.size() - 1].size() == 1) &&
				(doc.lines[last_line_id].type == LineType::UNKNOWN) && (doc.lines[last_line_id].segments.size() == 1) &&
				(doc.lines[last_line_id].segments[0] == doc.tables[i].columns[0][doc.tables[i].columns[0].size() - 1])) {
				doc.lines[last_line_id].type = LineType::TEXT;
				doc.tables[i].multi_rows.erase(doc.tables[i].multi_rows.begin() + doc.tables[i].multi_rows.size() - 1);
				doc.tables[i].columns[0].erase(doc.tables[i].columns[0].begin() + doc.tables[i].columns[0].size() - 1);
			}

			//Finalize line types. Change all lines within the table to type-2
			for (int j = 0; j < doc.tables[i].multi_rows.size(); j++) {
				for (int k = 0; k < doc.tables[i].multi_rows[j].size(); k++) {
					doc.lines[doc.tables[i].multi_rows[j][k]].type = LineType::TABLE;
				}
			}
		}

		//Recheck and discard single and double row tables 
		for (int i = doc.tables.size() - 1; i >= 0; i--) {
			if ((doc.tables[i].multi_rows.size() < 2) && (doc.tables[i].multi_rows[0].size() < 2 || doc.tables[i].columns[0].size() < 2)) {
				for (unsigned s = 0; s < doc.tables[i].multi_rows[0].size(); s++) { doc.lines[doc.tables[i].multi_rows[0][s]].type = LineType::TEXT; }
				doc.tables[i].multi_rows.erase(doc.tables[i].multi_rows.begin() + i);
				doc.tables[i].columns.erase(doc.tables[i].columns.begin() + i);
			} else if ((doc.tables[i].multi_rows.size() < 3) && (doc.tables[i].multi_rows[0].size() < 2) && (doc.tables[i].multi_rows[1].size() < 2)) {
				for (unsigned s = 0; s < doc.tables[i].multi_rows[0].size(); s++) { doc.lines[doc.tables[i].multi_rows[0][s]].type = LineType::TEXT; }
				for (unsigned s = 0; s < doc.tables[i].multi_rows[1].size(); s++) { doc.lines[doc.tables[i].multi_rows[1][s]].type = LineType::TEXT; }
				doc.tables[i].multi_rows.erase(doc.tables[i].multi_rows.begin() + i);
				doc.tables[i].columns.erase(doc.tables[i].columns.begin() + i);
			}
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/**
	 * @brief Find the sizes of the columns.
	 * Each columns spans from the leftest single segment to the rightest
	 * single segment (single segment = assigned to only one column)
	 */
	void OCRTabsEngine::FindColumnSize() {
		OCR_LOG_MSG("Find column sizes...");
		aux::startClock();

		for (int i = 0; i < doc.tables.size(); i++) {
			vector<int*> dims;
			for (int j = 0; j < doc.tables[i].columns.size(); j++) {
				int* tmp = new int[2];
				tmp[0] = doc.page_right;
				tmp[1] = doc.page_left;
				for (int k = 0; k < doc.tables[i].columns[j].size(); k++) {
					bool flag_left = true;
					bool flag_right = true;
					if (j >= 1) {
						if (std::find(doc.tables[i].columns[j - 1].begin(), doc.tables[i].columns[j - 1].end(), doc.tables[i].columns[j][k]) != doc.tables[i].columns[j - 1].end()) {
							flag_left = false;
						}
					}
					if (j < doc.tables[i].columns.size() - 1) {
						if (std::find(doc.tables[i].columns[j + 1].begin(), doc.tables[i].columns[j + 1].end(), doc.tables[i].columns[j][k]) != doc.tables[i].columns[j + 1].end()) {
							flag_right = false;
						}
					}
					if ((doc.words[doc.tables[i].columns[j][k][0]].box[0] <= tmp[0]) && (flag_left)) {
						tmp[0] = doc.words[doc.tables[i].columns[j][k][0]].box[0];
					}
					if ((doc.words[doc.tables[i].columns[j][k][doc.tables[i].columns[j][k].size() - 1]].box[2] >= tmp[1]) && (flag_right)) {
						tmp[1] = doc.words[doc.tables[i].columns[j][k][doc.tables[i].columns[j][k].size() - 1]].box[2];
					}
				}
				dims.push_back(tmp);
			}
			doc.col_dims.push_back(dims);
		}

		// I'M NOT SURE ABOUT ERASEs
		for (int i = doc.tables.size() - 1; i >= 0; i--) {
			if (doc.tables[i].columns.size() == 2) {
				int colSizeA = doc.col_dims[i][0][1] - doc.col_dims[i][0][0];
				int colSizeB = doc.col_dims[i][1][1] - doc.col_dims[i][1][0];
				if (colSizeB >= 10 * colSizeA) {
					doc.tables[i].columns.erase(doc.tables[i].columns.begin() + i);
					for (int j = 0; j < doc.tables[i].multi_rows.size(); j++) {
						for (int k = 0; k < doc.tables[i].multi_rows[j].size(); k++) {
							doc.lines[doc.tables[i].multi_rows[j][k]].type = ocrt::LineType::TEXT;
						}
					}
					//table_Rows.erase(table_Rows.begin()+i);
					doc.tables[i].multi_rows.erase(doc.tables[i].multi_rows.begin() + i);
					doc.col_dims.erase(doc.col_dims.begin() + i);
				}
			}
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	/** 
	 * Find the final row and column sizes in order to create the table
	 * merge cells that contain multi segments
	 * widen columns and rows so that there is no white space between them
	 */ 
	void OCRTabsEngine::FinalizeGrid() {
		OCR_LOG_MSG("Finalize grid...");
		aux::startClock();

		for (int i = 0; i < doc.col_dims.size(); i++) {
			for (int j = 1; j < doc.col_dims[i].size(); j++) {
				doc.col_dims[i][j][0] = (doc.col_dims[i][j][0] + doc.col_dims[i][j - 1][1]) / 2;
				doc.col_dims[i][j - 1][1] = doc.col_dims[i][j][0];
			}
		}
		for (int i = 0; i < doc.tables.size(); i++) {
			vector<int*> dims;
			for (int j = 0; j < doc.tables[i].multi_rows.size(); j++) {
				int* tmp = new int[2];
				tmp[0] = doc.lines[doc.tables[i].multi_rows[j][0]].dims[0];
				tmp[1] = doc.lines[doc.tables[i].multi_rows[j][doc.tables[i].multi_rows[j].size() - 1]].dims[1];
				dims.push_back(tmp);
				if (j > 0) {
					dims[j][0] = (dims[j][0] + dims[j - 1][1]) / 2;
					int low = dims[j][0];
					dims[j - 1][1] = low;
				}
			}
			doc.row_dims.push_back(dims);
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Resets the image to initial state.
	void OCRTabsEngine::ResetImage() {
		test = initial.clone();
		//resize(test,test,Size(test.size().width*2,test.size().height*2));
	}

	Mat OCRTabsEngine::SegmentImage(Mat img) {
		//Search for multi column text
		OCR_LOG_MSG("Segment Image...");
		aux::startClock();
		Mat dst;
		threshold(img, img, 200, 255, 0);
		namedWindow("img", 0);
		vector<double> hor, ver;
		test = img;

		uchar* data = (uchar*)img.data;

		for (int i = 0; i < img.rows; i++) {
			double sum = 0;
			for (int j = 0; j < img.cols; j++) {
				sum += data[j + i * img.cols];
			}
			hor.push_back(sum / img.cols);
		}
		for (int i = 0; i < img.cols; i++) {
			double sum = 0;
			for (int j = 0; j < img.rows; j++) {
				sum += data[i + j * img.cols];
			}
			ver.push_back(sum / img.rows);
		}


		cvtColor(img, img, cv::COLOR_GRAY2BGR);
		for (int i = 0; i < hor.size(); i++) {
			line(img, Point2i(0, i), Point2i(hor[i] * 2 - 400, i), Scalar(0, 0, 255), 2);
		}
		for (int i = 0; i < ver.size(); i++) {
			line(img, Point2i(i, 0), Point2i(i, ver[i] * 2 - 400), Scalar(0, 255, 0), 2);
		}

		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		imshow("img", img);
		waitKey(0);

		/*img=255-img;
		cv::distanceTransform(img, dst, CV_DIST_L2, 3);
				test=dst;
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
		resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
		imshow("img",test);
		cvWaitKey(0);*/

		dst = img.clone();
		threshold(img, img, 200, 255, 0);
		erode(img, img, Mat(), Point(-1, -1), 10);
		/*erode(img,img,Mat(),Point(1,1),20);
		img=255-img;

		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;

		findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
		Mat SegMap = Mat::zeros(img.size().height, img.size().width,  CV_8UC1);
			if (contours.size()){
				int idx = 0;
				for( ; idx >= 0; idx = hierarchy[idx][0] )
				{
					if (contours[idx].size()>=1)
					{	Scalar color = Scalar( 255,255,255 );
						drawContours( SegMap, contours, idx, color, -1, 4, hierarchy, 0);
					}
				}
			}

		test=SegMap;
			namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
		resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
		imshow("img",test);
		cvWaitKey(0);*/

		img = 255 - img;
		data = (uchar*)img.data;
		vector<int> red;
		/*cvtColor(dst,dst,CV_GRAY2BGR);


		for (int i=0;i<img.cols;i++)
		{
			double sum=0;
			for (int j=0;j<img.rows;j++)
			{
				sum += data[i+j*img.cols];
			}
			if (sum<50*255)
			{
				line(dst,Point2i(0,i),Point2i(img.cols-1,i),Scalar(0,0,255),2);
			}
		}

		test=dst;
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
		resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
		imshow("img",test);
		cvWaitKey(0);*/

		for (int i = 0; i < img.cols; i++) {
			double sum = 0;
			for (int j = 0; j < img.rows; j++) {
				sum += data[i + j * img.cols];
			}
			if (sum < 255 * 25) {
				red.push_back(i);
			}
		}
		vector<int> text_columns;
		for (int i = 1; i < red.size(); i++) {
			if (((red[i] - red[i - 1]) > 1)) {
				text_columns.push_back(red[i - 1]);
				text_columns.push_back(red[i]);
			}
		}
		bool cols = true;
		float col_size_thresh = 0.1;
		if ((text_columns.size() > 3) && (text_columns.size() % 2 == 0)) {
			for (int i = 0; i < text_columns.size() - 2; i = i + 2) {
				for (int j = i + 2; j < text_columns.size() - 1; j = j + 2) {
					int tmp = abs((text_columns[i + 1] - text_columns[i]) - (text_columns[j + 1] - text_columns[j]));
					if (tmp >= col_size_thresh * (text_columns[i + 1] - text_columns[i])) {
						cols = false;
					}
				}
			}
		}
		else {
			cols = false;
		}
		if (cols) {
			Mat single_col = Mat::ones(dst.rows * (text_columns.size() / 2), (text_columns[1] - text_columns[0]) * (1 + col_size_thresh), dst.type()) * 255;
			for (int i = 0; i < text_columns.size(); i = i + 2) {
				Mat tmp(single_col, Rect(0, i * dst.rows / 2, text_columns[i + 1] - text_columns[i], dst.rows));
				Mat tmp2(dst, Rect(text_columns[i], 0, text_columns[i + 1] - text_columns[i], dst.rows));
				tmp2.copyTo(tmp);
			}

			std::cout << " Done in " << aux::endClock() << "s \n";
			return single_col;
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
		return dst;
	}

	Mat OCRTabsEngine::PreprocessImage(Mat img) {
		OCR_LOG_MSG("Process Image...");
		aux::startClock();

		test = cv::Mat(img);

		float ratio;
		int max_width_height = std::max(img.size().width, img.size().height);

		/*if (((std::max(img.size().width, img.size().height)) < 4200) && ((std::max(img.size().width, img.size().height)) > 2800)) {
			ratio = 1;
		} else if (img.size().width > img.size().height) {
			ratio = (float)img.size().width / 3500;
		} else {
			ratio = (float)img.size().height / 3500;
		}*/
		if (max_width_height < UPPER_DOTS_LIM && max_width_height > LOWER_DOTS_LIM) {
			ratio = 1;
		} else {
			ratio = (float)max_width_height / STD_DOTS_SIZE;
		}

		RemoveGridLines(ratio);
		SegmentationBlocks blk;
		cv::Mat clean, clean2;
		img_processor.PrepareAll(img, clean, blk);

		//if (ratio >= 0.8) cv::erode(clean, clean, cv::Mat(), cv::Point(-1, -1), 1);
		//drawing_handler::DrawGridlessImage(clean);
		img_processor.ExtractTextImage(clean, blk, clean2);
		//img_processor::ExtractTextImage(img, blk, clean2);
		cv::Size orgSiz = img.size();
		int max_org_width_height = std::max(orgSiz.width, orgSiz.height);
		int min_org_width_height = std::min(orgSiz.width, orgSiz.height);
		int min_max_wh_ratio = min_org_width_height / (float)max_org_width_height;

		img_processor.ReorderImage(clean2, blk, img);

		/*if (((std::max(orgSiz.width, orgSiz.height)) < 4200) && ((std::max(orgSiz.width, orgSiz.height)) > 2800)) {
			std::cout << " Done in " << aux::endClock() << "s \n";
			return (img);
		}
		if (orgSiz.width > orgSiz.height) {
			resize(img, img, cv::Size(3500, img.size().height / ((float)img.size().width / 3500)));
		} else {
			resize(img, img, cv::Size(img.size().width / ((float)orgSiz.height / 3500), 3500 * (float)img.size().height / orgSiz.height));
		}*/
		if ((max_org_width_height < UPPER_DOTS_LIM) && (max_org_width_height > LOWER_DOTS_LIM)) {
			std::cout << " Done in " << aux::endClock() << "s \n";
			return (img);
		}
		if (orgSiz.width > orgSiz.height) {
			resize(img, img, cv::Size(STD_DOTS_SIZE, (img.size().height / (float)img.size().width) * STD_DOTS_SIZE));
		} else {
			resize(img, img, cv::Size((img.size().width / (float)orgSiz.height) * STD_DOTS_SIZE, STD_DOTS_SIZE * (float)img.size().height / orgSiz.height));
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
		return (img);
	}

	void OCRTabsEngine::WriteHTML(std::string& filename) {
		aux::WriteHTML(filename, doc);
	}

	bool OCRTabsEngine::fail_condition() { 
		return fail; 
	}

	bool OCRTabsEngine::ReadImage(std::vector<cv::Mat>& pages, std::vector<cv::Mat>& pages_clean, FileType filetype, const std::string& filename, const std::string& filenameXML, bool withXML) {
		switch (filetype) {
		case FileType::PDF:
			if (!parsePDF(filename, pages)) return false;
			break;

		case FileType::IMG:
			pages.push_back(cv::imread(filename, cv::IMREAD_GRAYSCALE));
			if (pages[0].empty()) { OCR_LOG_ERROR("File not available"); return false; }
			break;

		default:
			return false;
		}
		if (withXML) {
			//// NOT WORKING ////
			if (!PreprocessImageWithXML(filenameXML, pages, pages_clean)) { OCR_LOG_ERROR("Preprocessing with XML failed"); return false; }
			pages.clear();
			pages = pages_clean;  //not sure about that copying
		}

		raw = pages[0].clone();
		return true;
	}

	bool OCRTABS_API OCRTabsEngine::doc2html(FileType filetype, const std::string& filename, const std::string& filenameXML, bool withXML) {
		resetAll();
		std::vector<cv::Mat> pages, pages_clean;
		
		if (!ReadImage(pages, pages_clean, filetype, filename, filenameXML, withXML))
			return false;

		for (int i = 0; i < pages.size(); i++) {
			raw = pages[i].clone();
			Mat tmp = PreprocessImage(pages[i]);
			SetImage(tmp);
			//RemoveGridLines();
			OCR_Recognize();
			FindBoxesAndWords();
			PrepareMulti1();
		}
		RemoveHeadersFooters();
		PrepareMulti2();
		//FindTextBoundaries(); ////maybe is needed here////
		CreateTextLines();
		CreateLineSegments();
		AssignLineTypes();
		FindTableAreas();
		AssignRowsToTables();
		CreateTableColumns(); ////
		if (fail_condition()) {
			OCR_LOG_ERROR(fail_msg);
			return false;
		}
		CreateTableMultiRows();
		FindColumnSize();	 ////
		FinalizeGrid(); ////

		std::string outputFilename = filename;
		outputFilename.append(withXML ? "XML.html" : ".html");
		WriteHTML(outputFilename);

		return true;
	}

	// Resets everything
	void OCRTabsEngine::resetAll() {
		test = cv::Mat();
		initial = cv::Mat();
		doc.words.clear();
		doc.tables.clear();
		doc.pages.clear();
		table_area.clear();
		doc.col_dims.clear();
		doc.row_dims.clear();
		tmp_col.clear();
		words_.clear();
		lines_.clear();
	}

	bool OCRTabsEngine::parsePDF(const std::string& filename, std::vector<cv::Mat>& imageList) {
		fz_context* ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
		// Register document handlers for the default file types we support.
		fz_register_document_handlers(ctx);
		// Open the PDF, XPS or CBZ document.
		fz_document* doc = fz_open_document(ctx, filename.c_str());
		// Retrieve the number of pages (not used in this example).
		int pagecount = fz_count_pages(ctx, doc);
		if (pagecount < 1) {
			fz_drop_document(ctx, doc);
			fz_drop_context(ctx);
			return false;
		}
		int rotation = 0;
		float zoom = 400;
		for (unsigned i = 0; i < pagecount; i++) {
			fz_page* page = fz_load_page(ctx, doc, i);
			fz_matrix transform = fz_rotate(rotation);
			fz_matrix no_transform = fz_rotate(rotation);
			/*cout << "[" << transform.a << " " << transform.b << " " << transform.c << " " << transform.d << " " << transform.e << " " << transform.f << "]" << endl;*/
			transform = fz_pre_scale(transform, zoom / 100.0f, zoom / 100.0f);
			fz_rect bounds = fz_bound_page(ctx, page);
			bounds = fz_transform_rect(bounds, transform);
			fz_irect bbox = fz_round_rect(bounds);
			fz_pixmap* pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, NULL, 1);
			fz_clear_pixmap_with_value(ctx, pix, 0xff);
			fz_device* dev = fz_new_draw_device(ctx, no_transform, pix);
			fz_run_page(ctx, page, dev, transform, NULL);
			fz_save_pixmap_as_png(ctx, pix, "out.png");
			fz_drop_device(ctx, dev);
			cv::Mat input;
			img_processor.pixmap2mat(&pix, input);
			imageList.push_back(input.clone());
			fz_drop_pixmap(ctx, pix);
			fz_drop_page(ctx, page);
		}
		fz_drop_document(ctx, doc);
		fz_drop_context(ctx);
		return true;
	}

	bool OCRTabsEngine::PreprocessImageWithXML(const std::string& fileXML, std::vector<cv::Mat>& imageRAW, std::vector<cv::Mat>& imageCLN) {
		std::ifstream file(fileXML);
		if (!file.is_open()) return false;
		for (int i = 0; i < imageRAW.size(); i++) {
			imageCLN.push_back(cv::Mat(imageRAW[i].size(), imageRAW[i].type()));
			imageCLN[i].setTo(cv::Scalar(255));

			cv::Mat mask;
			bool inText = false;
			float resiz = -1;
			std::string str;
			while (std::getline(file, str)) {
				if (resiz == -1) {
					std::cout << "here" << std::endl;
					std::size_t foundW = str.find("<page width=");
					if (foundW != std::string::npos) {
						resiz = (float)imageRAW[i].cols / std::stoi(str.substr(foundW + 13, str.find("height=") - 2 - foundW - 13));
						mask = cv::Mat::zeros(imageRAW[i].rows / resiz, imageRAW[i].cols / resiz, CV_8UC1);
					}
				}
				else if (str.find("<line baseline") != std::string::npos && inText) {
					std::cout << "not here" << std::endl;
					std::size_t found_l = str.find(" l=\"");
					std::size_t found_t = str.find(" t=\"");
					std::size_t found_r = str.find(" r=\"");
					std::size_t found_b = str.find(" b=\"");
					std::size_t found_f = str.find("\"><formatting");

					int left = std::stoi(str.substr(found_l + 4, found_t - 1 - found_l - 4));
					int top = std::stoi(str.substr(found_t + 4, found_r - 1 - found_t - 4));
					int right = std::stoi(str.substr(found_r + 4, found_b - 1 - found_r - 4));
					int bottom = std::stoi(str.substr(found_b + 4, found_f - found_b - 4));

					mask(cv::Rect(left, top, right - left, bottom - top)).setTo(1);
				} else if (str.find("<block blockType=\"Text") != std::string::npos || str.find("<block blockType=\"Table") != std::string::npos) {
					std::cout << "not here" << std::endl;
					inText = true;
				} else if (str.find("</block>") != std::string::npos) {
					std::cout << "not here" << std::endl;
					inText = false;
				} else if (str.find("</page>") != std::string::npos) {
					std::cout << "not here" << std::endl;
					cv::resize(mask, mask, imageRAW[i].size());
					imageRAW[i].copyTo(imageCLN[i], mask);

					//img_processor::ThresholdImage(imageCLN[i], imageCLN[i]);
					//cv::erode(imageCLN[i],imageCLN[i],cv::Mat(), cv::Point(-1,-1), 1);

					break;
				}
			}
		}
		file.close();
		return true;
	}

	/**
	 * @brief Removes possible figures.
	 * Search for word "figure". When found check the words above. if the previous 4 words are not "in dictionary", 
	 * then they are part of images that have been recognised as text, and they are removed. 
	 * The line with the word "figure" is also removed as it is probably a caption of the figure.
	 */
	void OCRTabsEngine::RemoveFigures() {
		for (int i = doc.words.size() - 1; i >= 0; i--) {
			string tmp = doc.words[i].name;
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
			if (tmp.find("figure") != std::string::npos) {
				for (int j = i - 1; j >= 3; j--) {
					bool image = (doc.words[j].dict && doc.words[j - 1].dict && doc.words[j - 2].dict && doc.words[j - 3].dict);
					if (image) {
						if (j >= (i - 5)) {
							j = 0;
						}
						else {
							int ln_index = i;
							for (int k = i + 1; k < doc.words.size(); k++) {
								if (!(((doc.words[k].box[BOX_TOP] <= doc.words[i].box[BOX_TOP]) && (doc.words[i].box[BOX_TOP] <= doc.words[k].box[BOX_BOTTOM])) ||
									((doc.words[i].box[BOX_TOP] <= doc.words[k].box[BOX_TOP]) && (doc.words[k].box[BOX_TOP] <= doc.words[i].box[BOX_BOTTOM])))) {
									ln_index = k - 1;
									k = doc.words.size();
								}
							}
							for (int k = ln_index; k >= j + 1; k--) {
								doc.words.erase(doc.words.begin() + k);
								/*words.erase(words.begin() + k);
								confs.erase(confs.begin() + k);
								font_size.erase(font_size.begin() + k);
								bold.erase(bold.begin() + k);
								italic.erase(italic.begin() + k);
								underscore.erase(underscore.begin() + k);
								dict.erase(dict.begin() + k);*/
							}
							i = j;
							j = 0;
						}
					}
				}
			}
		}
	}

	/**
	 * @brief Process generated columns.
	 * Remove, merge etc
	 */
	void OCRTabsEngine::ProcessGeneratedColumns() {
		if (fail) return;

		// remove empty columns
		for (int i = 0; i < doc.tables.size(); i++) {
			for (int j = 0; j < doc.tables[i].columns.size(); j++) {
				if (doc.tables[i].columns[j].size() == 0) {
					std::cout << "empty column" << std::endl;
					doc.tables[i].columns.erase(doc.tables[i].columns.begin() + j);
					j--;
				}
				else if (doc.tables[i].columns[j].size() == 1) {
					std::cout << "unary column" << std::endl;
				}
			}
		}

		// If we find a column where the unique segments  (segments that are assigned to only one column)
		// are less than the multiple segments (segments assigned to more than 1 column), we merge this column with
		// the immediatelly previous one
		for (int i = 0; i < doc.tables.size(); i++) {
			for (int j = 1; j < doc.tables[i].columns.size(); j++) {
				int counter_single = 0;
				int counter_multi = 0;
				for (int k = 0; k < doc.tables[i].columns[j].size(); k++) {
					if (std::find(doc.tables[i].columns[j - 1].begin(), doc.tables[i].columns[j - 1].end(), doc.tables[i].columns[j][k]) != doc.tables[i].columns[j - 1].end()) {
						counter_multi++;
					} else {
						counter_single++;
					}
				}

				if (counter_multi >= counter_single) {
					for (int k = 0; k < doc.tables[i].columns[j].size(); k++) {
						bool multi = false;
						if (std::find(doc.tables[i].columns[j - 1].begin(), doc.tables[i].columns[j - 1].end(), doc.tables[i].columns[j][k]) == doc.tables[i].columns[j - 1].end()) {
							doc.tables[i].columns[j - 1].push_back(doc.tables[i].columns[j][k]);
						}
					}
					doc.tables[i].columns.erase(doc.tables[i].columns.begin() + j);
					j--;
				}
			}
		}

		// Type-3 lines (UNKNOWN) that are in the end of a table, and their single segment is assigned to more than one columns,
		// are removed from the table
		for (int i = 0; i < doc.tables.size(); i++) {
			for (int j = doc.tables[i].rows.size() - 1; j >= 0; j--) {
				//cout << table_Rows[0].size()<<"     "<<table_Rows[1].size()<<"\n";
				//cout << i<<"     "<<j<<"\n";
				if (doc.lines[doc.tables[i].rows[j]].type == LineType::UNKNOWN) {
					vector<int> xcol;
					for (int k = 0; k < doc.tables[i].columns.size(); k++) {
						if (doc.tables[i].columns[k].size() > 0) {
							//mysterious if statement (?)
							if (doc.lines[doc.tables[i].rows[j]].segments[0] == doc.tables[i].columns[k][doc.tables[i].columns[k].size() - 1]) {
								xcol.push_back(k);
							}
						}
					}
					if (xcol.size() > 1) {
						doc.lines[doc.tables[i].rows[j]].type = LineType::TEXT;
						doc.tables[i].rows.erase(doc.tables[i].rows.begin() + j);
						for (int k = 0; k < xcol.size(); k++) {
							doc.tables[i].columns[xcol[k]].erase(doc.tables[i].columns[xcol[k]].begin() + doc.tables[i].columns[xcol[k]].size() - 1);
						}
					} else {
						j = -1;
					}
				} else {
					j = -1;
				}
			}
		}

		//If a column has only one segment, which is on the 1st row (possibly missaligned table header) and 
		// the column on its left doesnot have a segment in the same row, then 
		// merge these columns
		for (int i = 0; i < doc.tables.size(); i++) {
			for (int j = 1; j < doc.tables[i].columns.size(); j++) {
				if (doc.tables[i].columns[j].size() == 1) {
					bool col_has_in_1st_row = false;
					bool prev_col_has_in_1st_row = false;
					if (std::find(doc.lines[doc.tables[i].rows[0]].segments.begin(), doc.lines[doc.tables[i].rows[0]].segments.end(), doc.tables[i].columns[j][0]) != doc.lines[doc.tables[i].rows[0]].segments.end()) {
						col_has_in_1st_row = true;
					}
					if (std::find(doc.lines[doc.tables[i].rows[0]].segments.begin(), doc.lines[doc.tables[i].rows[0]].segments.end(), doc.tables[i].columns[j - 1][0]) != doc.lines[doc.tables[i].rows[0]].segments.end()) {
						prev_col_has_in_1st_row = true;
					}
					if ((col_has_in_1st_row) && (!prev_col_has_in_1st_row)) {
						for (int k = 0; k < doc.tables[i].columns[j - 1].size(); k++) {
							doc.tables[i].columns[j].push_back(doc.tables[i].columns[j - 1][k]);
						}
						doc.tables[i].columns.erase(doc.tables[i].columns.begin() + j - 1);
					}
				}
			}
		}

		// Tables that end up having only one column, are discarded and treated as simple text
		for (int i = doc.tables.size() - 1; i >= 0; i--) {
			if (doc.tables[i].columns.size() < 2) {
				doc.tables[i].columns.erase(doc.tables[i].columns.begin() + i);
				for (int j = 0; j < doc.tables[i].rows.size(); j++) {
					doc.lines[doc.tables[i].rows[j]].type = LineType::TEXT;
				}
				doc.tables[i].rows.erase(doc.tables[i].rows.begin() + i);
				doc.tables.erase(doc.tables.begin() + i);
			}
		}

		//Check and remove scrambled tables i.e tables that seem to have a table format but in reality are random segments generated by
		// big white spaces bwtween words (justified text aligment)

		//bool scrambled_table=false;
		//int scramLim = 0;
		//for (int i=table_Columns.size()-1;i>=0;i--)
		//{
		//	if (scrambled_table)
		//	{	
		//		table_Columns.erase(table_Columns.begin()+i+1);
		//		for (int k=0;k<table_Rows[i+1].size();k++)
		//		{
		//			Lines_type[table_Rows[i+1][k]]=1;
		//		}
		//		table_Rows.erase(table_Rows.begin()+i+1);
		//	}
		//	scrambled_table=false;
		//	for (int j=0;j<table_Columns[i].size();j++)
		//	{
		//		bool scrambled_col=false;
		//		for (int k=0;k<table_Columns[i][j].size()-1;k++)
		//		{
		//			int left1=boxes[table_Columns[i][j][k][0]][0];
		//			int left2=boxes[table_Columns[i][j][k+1][0]][0];
		//			int right1=boxes[table_Columns[i][j][k][table_Columns[i][j][k].size()-1]][2];
		//			int right2=boxes[table_Columns[i][j][k+1][table_Columns[i][j][k+1].size()-1]][2];	
		//			if ((abs(left1-left2)<=scramLim)||(abs(right1-right2)<=scramLim))
		//			{
		//				scrambled_col=true;
		//			}
		//			if (k<table_Columns[i][j].size()-2){
		//			left2=boxes[table_Columns[i][j][k+2][0]][0];
		//			right2=boxes[table_Columns[i][j][k+2][table_Columns[i][j][k+2].size()-1]][2];	
		//			if ((abs(left1-left2)<=scramLim)||(abs(right1-right2)<=scramLim))
		//			{
		//				scrambled_col=true;
		//			}
		//			}
		//		}
		//		if (!scrambled_col)
		//		{
		//			scrambled_table=true;
		//			j=table_Columns[i].size();
		//		}
		//	}
		//}
		//if (scrambled_table)
		//{	
		//	table_Columns.erase(table_Columns.begin()+0);
		//	for (int k=0;k<table_Rows[0].size();k++)
		//	{
		//		Lines_type[table_Rows[0][k]]=1;
		//	}
		//	table_Rows.erase(table_Rows.begin()+0);
		//}

		//If all the columns of a table (besides the 1st one) have more empty cells than cells with data then
		//this table is discarded (simple formatted text)
		for (int i = doc.tables.size() - 1; i >= 0; i--) {
			bool almost_empty = true;
			for (int j = 1; j < doc.tables[i].columns.size(); j++) {
				if (doc.tables[i].columns[j].size() >= doc.tables[i].rows.size() / 2) {
					almost_empty = false;
					j = doc.tables[i].columns.size();
				}
			}
			if (almost_empty) {
				doc.tables[i].columns.erase(doc.tables[i].columns.begin() + i);
				for (int k = 0; k < doc.tables[i].rows.size(); k++) {
					doc.lines[doc.tables[i].rows[k]].type = LineType::TEXT;
				}
				doc.tables[i].rows.erase(doc.tables[i].rows.begin() + i);
				doc.tables.erase(doc.tables.begin() + i);
			}
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	int OCRTabsEngine::FindMaxRightBoxInSegment(const std::vector<int>& seg) {
		int max_right = -1;
		int max_box_id = -1;
		for (int i = 0; i < seg.size(); i++) {
			if (doc.words[seg[i]].box[BOX_RIGHT] > max_right) {
				max_right = doc.words[seg[i]].box[BOX_RIGHT];
				max_box_id = seg[i];
			}
		}

		return max_box_id;
	}

	int OCRTabsEngine::FindMinLeftBoxInSegment(const std::vector<int>& seg) {
		int min_left = doc.words[seg[0]].box[BOX_LEFT];
		int min_box_id = seg[0];
		for (int i = 1; i < seg.size(); i++) {
			if (doc.words[seg[i]].box[BOX_LEFT] < min_left) {
				min_left = doc.words[seg[i]].box[BOX_RIGHT];
				min_box_id = seg[i];
			}
		}

		return min_box_id;
	}

	void OCRTabsEngine::InsertionSortBoxesInSegment(std::vector<int>& seg) {
		for (int i = 1; i < seg.size(); i++) {
			int key = doc.words[seg[i]].box[BOX_LEFT];
			int key_id = seg[i];

			int j = i - 1;
			while (j >= 0 && key < doc.words[seg[j]].box[BOX_LEFT]) {
				seg[j + 1] = seg[j];
				j--;
			}

			seg[j + 1] = key_id;
		}
	}

	template<typename T> bool OCRTabsEngine::Find(const std::vector<T>& list, const T& element) {
		return std::find(list.begin(), list.end(), element) != list.end();
	}
}