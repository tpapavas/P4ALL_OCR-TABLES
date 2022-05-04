#pragma once
#include <iostream>
#include <fstream>
#include "ocr_tabs.h"
#include "auxiliary.h"
#include "imgProcessor.h"
#include "drawingHandler.h"
extern "C" {
	#include <mupdf/fitz.h>
}

using namespace cv;
//using namespace std;

#pragma warning( disable : 4018 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )

namespace ocr_tabs {
	OCRTabsEngine::OCRTabsEngine() {
		fail = false;
		fail_msg = "";
		lines_type = NULL;
		//tess.Init("..\\tessdata", "eng");
		//tess.Init("tessdata", "Greek");
		tess.Init("tessdata", "eng");
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
		page_height.push_back(test.size().height);
		page_width.push_back(test.size().width);

		for (int i = 0; i < boxes.size(); i++) {
			vector<int> tmp;
			tmp.push_back(i);
			int j;
			for (j = i + 1; j < boxes.size(); j++) {
				// check for vertically-overlapping bboxes
				if (((boxes[j-1][BOX_TOP] <= boxes[j][BOX_TOP]) && (boxes[j][BOX_TOP] <= boxes[j-1][BOX_BOTTOM])) ||
					((boxes[j][BOX_TOP] <= boxes[j-1][BOX_TOP]) && (boxes[j-1][BOX_TOP] <= boxes[j][BOX_BOTTOM]))) { 
					tmp.push_back(j);
				} else {
					break;
				}
			}
			i = j - 1;
			lines.push_back(tmp);
		}
		
		boxes_.push_back(boxes);
		words_.push_back(words);
		confs_.push_back(confs);
		lines_.push_back(lines);
		font_size_.push_back(font_size);
		bold_.push_back(bold);
		italic_.push_back(italic);
		underscore_.push_back(underscore);
		dict_.push_back(dict);
		
		test.release();
		confs.clear();
		boxes.clear();
		words.clear();
		lines.clear();
		font_size.clear();
		bold.clear();
		italic.clear();
		underscore.clear();
		dict.clear();
	}

	/**
	 * @brief Preprocessing for multiple pages
	 * Concatenate pages (boxes, lines etc) as if there was one large page
	 */
	void OCRTabsEngine::PrepareMulti2() {
		for (int i = 1; i < boxes_.size(); i++) {
			int move = 0;
			for (int j = 0; j < i; j++) {
				move=move+page_height[j];
			}
			for (int j=0;j<boxes_[i].size();j++) {
				boxes_[i][j][BOX_TOP] = boxes_[i][j][BOX_TOP] + move;
				boxes_[i][j][BOX_BOTTOM] = boxes_[i][j][BOX_BOTTOM] + move;
			}
		}

		for (int i = 0; i < boxes_.size(); i++) {
			for (int j = 0; j < boxes_[i].size(); j++) {
				boxes.push_back(boxes_[i][j]);
				font_size.push_back(font_size_[i][j]);
				words.push_back(words_[i][j]);
				confs.push_back(confs_[i][j]);
				bold.push_back(bold_[i][j]);
				italic.push_back(italic_[i][j]);
				underscore.push_back(underscore_[i][j]);
				dict.push_back(dict_[i][j]);
			}
		}
		confs_.clear();
		boxes_.clear();
		words_.clear();
		lines_.clear();
		font_size_.clear();
		bold_.clear();
		italic_.clear();
		underscore_.clear();
		dict_.clear();
	
		page_bottom = page_right = 0;
		page_left = page_width[0];
		for (int i = 1; i < page_width.size(); i++) {
			page_left = std::max(page_left, page_width[i]);
		}
		page_top = 0;
		for (int i = 0; i < page_height.size(); i++) {
			page_top = page_top + page_height[i];
		}
		for (int i = 0; i < boxes.size(); i++) {
			if (boxes[i][BOX_LEFT] <= page_left) { page_left = boxes[i][BOX_LEFT]; }
			if (boxes[i][BOX_TOP] <= page_top) { page_top = boxes[i][BOX_TOP]; }
			if (boxes[i][BOX_RIGHT] >= page_right) { page_right = boxes[i][BOX_RIGHT]; }
			if (boxes[i][BOX_BOTTOM] >= page_bottom) { page_bottom = boxes[i][BOX_BOTTOM]; }
		}

		std::cout << "\nPROCESSING OVERALL DOCUMENT\n\n";
	}

	/**
     * @brief Removes grid lines, because tesseract has a problem recognizing words when there are dark, dense gridlines
	 * @param ratio: ratio of input image resize
	 */
	void OCRTabsEngine::RemoveGridLines(float ratio /*=1*/) {
		Mat dst;
		std::cout << "Remove Grid Lines...";
		//start = clock();
		aux::startClock();

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

		//cv::namedWindow("asd",CV_WINDOW_NORMAL);
		//cv::imshow("asd", test);
		//cv::waitKey(0);

		//duration = (std::clock() - start) / CLOCKS_PER_SEC;
		//std::cout << " Done in " << duration << "s \n";
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
		std::cout << "Recognizing...";
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

		std::cout << "Get bounding Boxes...";
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
			confs.push_back(ri->Confidence(tesseract::RIL_WORD));
			words.push_back(ri->GetUTF8Text(tesseract::RIL_WORD));
			font_size.push_back(point_size);
			bold.push_back(is_bold);
			italic.push_back(is_italic);
			underscore.push_back(is_underlined);

			vector <int> tmp;
			tmp.push_back(left);
			tmp.push_back(top);
			tmp.push_back(right);
			tmp.push_back(bottom);
			boxes.push_back(tmp);
			dict.push_back(ri->WordIsFromDictionary());
			//		}
			//	}
		} while (ri->Next(tesseract::RIL_WORD));

		RemoveFigures();

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Find the text boundaries of the whole image
	void OCRTabsEngine::FindTextBoundaries() {
		std::cout << "Find text boundaries...";
		aux::startClock();

		page_bottom = page_right = 0;
		page_left = test.size().width;
		page_top = test.size().height;

		for (int i = 0; i < boxes.size(); i++) {
			if (boxes[i][BOX_LEFT] <= page_left) { page_left = boxes[i][BOX_LEFT]; }
			if (boxes[i][BOX_TOP] <= page_top) { page_top = boxes[i][BOX_TOP]; }
			if (boxes[i][BOX_RIGHT] >= page_right) { page_right = boxes[i][BOX_RIGHT]; }
			if (boxes[i][BOX_BOTTOM] >= page_bottom) { page_bottom = boxes[i][BOX_BOTTOM]; }
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Create lines by assigning vertically-overlapping word boxes to the same unique line
	void OCRTabsEngine::CreateTextLines() {
		std::cout << "Find lines...";
		aux::startClock();

		for (int i = 0; i < boxes.size(); i++) {
			vector<int> tmp;
			tmp.push_back(i);
			int j;
			for (j = i + 1; j < boxes.size(); j++) {
				// check vetically-overlapping bboxes
				if (((boxes[j - 1][BOX_TOP] <= boxes[j][BOX_TOP]) && (boxes[j][BOX_TOP] <= boxes[j - 1][BOX_BOTTOM])) ||
					((boxes[j][BOX_TOP] <= boxes[j - 1][BOX_TOP]) && (boxes[j - 1][BOX_TOP] <= boxes[j][BOX_BOTTOM]))) {
					tmp.push_back(j);
				} else {
					break;
				}
			}
			i = j - 1;
			lines.push_back(tmp);  //assign them to the same line
		}

		// Find line dimensions (top, bottom, left, right)
		for (int i = 0; i < lines.size(); i++) {
			int* tmp = new int[2];
			tmp[0] = page_bottom;
			tmp[1] = page_top;
			for (int j = 0; j < lines[i].size(); j++) {
				if (boxes[lines[i][j]][BOX_TOP] <= tmp[LINE_TOP]) { tmp[LINE_TOP] = boxes[lines[i][j]][BOX_TOP]; }
				if (boxes[lines[i][j]][BOX_BOTTOM] >= tmp[LINE_BOTTOM]) { tmp[LINE_BOTTOM] = boxes[lines[i][j]][BOX_BOTTOM]; }
			}
			line_dims.push_back(tmp);
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
				for (int j = 0; j < lines_[i][k].size(); j++) {
					tmp1.append(words_[i][lines_[i][k][j]]);
				}
				for (int j = 0; j < lines_[i + 1][k].size(); j++) {
					tmp2.append(words_[i + 1][lines_[i + 1][k][j]]);
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
			for (int j = 0; j < lines_[i][lines_[i].size() - 1].size(); j++) {
				tmp1.append(words_[i][lines_[i][lines_[i].size() - 1][j]]);
			}
			for (int j = 0; j < lines_[i + 1][lines_[i + 1].size() - 1].size(); j++) {
				tmp2.append(words_[i + 1][lines_[i + 1][lines_[i + 1].size() - 1][j]]);
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
					for (int j = 0; j < lines_[i][k].size(); j++) {
						tmp1.append(words_[i][lines_[i][k][j]]);
					}
					for (int j = 0; j < lines_[i + 2][k].size(); j++) {
						tmp2.append(words_[i + 2][lines_[i + 2][k][j]]);
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
				for (int j = 0; j < lines_[i][lines_[i].size() - 1].size(); j++) {
					tmp1.append(words_[i][lines_[i][lines_[i].size() - 1][j]]);
				}
				for (int j = 0; j < lines_[i + 2][lines_[i + 2].size() - 1].size(); j++) {
					tmp2.append(words_[i + 2][lines_[i + 2][lines_[i + 2].size() - 1][j]]);
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

		for (int i = lines_.size() - 1; i >= 0; i--) {
			if (footer_limit[i] != (-1)) {
				for (int j = lines_[i][footer_limit[i]].size() - 1; j >= 0; j--) {
					words_[i].erase(words_[i].begin() + lines_[i][footer_limit[i]][j]);
					boxes_[i].erase(boxes_[i].begin() + lines_[i][footer_limit[i]][j]);
					confs_[i].erase(confs_[i].begin() + lines_[i][footer_limit[i]][j]);
					font_size_[i].erase(font_size_[i].begin() + lines_[i][footer_limit[i]][j]);
					italic_[i].erase(italic_[i].begin() + lines_[i][footer_limit[i]][j]);
					bold_[i].erase(bold_[i].begin() + lines_[i][footer_limit[i]][j]);
					underscore_[i].erase(underscore_[i].begin() + lines_[i][footer_limit[i]][j]);
					dict_[i].erase(dict_[i].begin() + lines_[i][footer_limit[i]][j]);
				}
			}
			for (int k = header_limit[i]; k >= 0; k--) {
				for (int j = lines_[i][k].size() - 1; j >= 0; j--) {
					words_[i].erase(words_[i].begin() + lines_[i][k][j]);
					boxes_[i].erase(boxes_[i].begin() + lines_[i][k][j]);
					confs_[i].erase(confs_[i].begin() + lines_[i][k][j]);
					font_size_[i].erase(font_size_[i].begin() + lines_[i][k][j]);
					bold_[i].erase(bold_[i].begin() + lines_[i][k][j]);
					italic_[i].erase(italic_[i].begin() + lines_[i][k][j]);
					underscore_[i].erase(underscore_[i].begin() + lines_[i][k][j]);
					dict_[i].erase(dict_[i].begin() + lines_[i][k][j]);
				}
			}
		}
	}

	/**
	 * @brief Create text Segments for each line.
	 * If the horizontal distance between two word boxes is smaller than a threshold,
	 * they will be considered as a single text segment
	 */
	void OCRTabsEngine::CreateLineSegments() {
		std::cout << "Find line segments...";
		aux::startClock();

		float ratio = 0.6 * 2; //1.2

		for (int i = 0; i < lines.size(); i++) {
			/*if (Lines[i].size() == 1) {
				vector<vector<int>> segments;
				vector<int> tmp;
				tmp.push_back(Lines[i][0]);
				segments.push_back(tmp);
				Lines_segments.push_back(segments);
			} else {*/
			float hor_thresh = (line_dims[i][LINE_BOTTOM] - line_dims[i][LINE_TOP]) * ratio;
			vector<vector<int>> segments;
			vector<int> tmp;
			tmp.push_back(lines[i][0]);
			for (int j = 1; j < lines[i].size(); j++) {
				if (boxes[lines[i][j]][0] - boxes[lines[i][j - 1]][2] <= hor_thresh) {
					tmp.push_back(lines[i][j]);
				} else {
					segments.push_back(tmp);
					tmp.clear();
					tmp.push_back(lines[i][j]);
				}
			}
			segments.push_back(tmp);
			line_segments.push_back(segments);
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
		std::cout << "Find line types...";
		aux::startClock();
		lines_type = new int[lines.size()];

		float sum = 0;
		for (int i = 0; i < lines.size(); i++) {
			if (line_segments[i].size() > 1) { lines_type[i] = LineType::TABLE; }
			else {
				int seg_left = boxes[line_segments[i][0][0]][BOX_LEFT];
				int seg_right = boxes[line_segments[i][0][line_segments[i][0].size() - 1]][BOX_RIGHT];
				//if ((seg_right-seg_left)>=(page_right-page_left)/2){Lines_type[i]=1;}
				if ((seg_right - seg_left) >= (float)(page_right - page_left) / 2.5) { lines_type[i] = LineType::TEXT; }
				else if (seg_left >= (page_right - page_left) / 4) { lines_type[i] = LineType::TABLE; }
				else { lines_type[i] = LineType::UNKNOWN; }
			}

			if (i > 1 && lines_type[i] == LineType::TABLE && lines_type[i - 2] == LineType::TABLE) { lines_type[i - 1] = LineType::TABLE; }

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
			sum = sum + line_dims[i][LINE_BOTTOM] - line_dims[i][LINE_TOP];
		}
		sum = (float)sum / lines.size();

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
		std::cout << "Find table areas...";
		aux::startClock();
		vector<int> tmp;

		if (lines_type[0] != LineType::TEXT) { tmp.push_back(0); }
		for (int i = 1; i < lines.size(); i++) {
			if ((lines_type[i] != LineType::TEXT) && (lines_type[i] != 4)) {
				if ((lines_type[i - 1] != LineType::TEXT) && (lines_type[i - 1] != 4) && ((line_dims[i][0] - line_dims[i - 1][1]) <= 3 * (line_dims[i][1] - line_dims[i][0]))) {
					tmp.push_back(i);
				} else {
					if ((tmp.size() > 1) || ((tmp.size() == 1) && (lines_type[tmp[0]] == LineType::TABLE))) {
						table_area.push_back(tmp);
					}
					tmp.clear();
					tmp.push_back(i);
				}
			}
		}
		if ((tmp.size() > 1) || ((tmp.size() == 1) && (lines_type[tmp[0]] == LineType::TABLE))) {
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
		std::cout << "Find table rows...";
		aux::startClock();

		for (int i = 0; i < table_area.size(); i++) {
			bool t_flag = false;
			vector<int> tmp;
			for (int j = 0; j < table_area[i].size(); j++) {
				if ((lines_type[table_area[i][j]] == LineType::TABLE) || t_flag) {
					t_flag = true;
					tmp.push_back(table_area[i][j]);
				}
			}
			if (tmp.size() > 0) { table_rows.push_back(tmp); }
		}

		// Tables that end up with just one Row are dismissed
		for (int i = 0; i < table_rows.size(); i++) {
			if (table_rows[i].size() < 2) {
				lines_type[table_rows[i][0]] = LineType::TEXT;
				table_rows.erase(table_rows.begin() + i);
			}
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	// Assign Columns to each table
	void OCRTabsEngine::CreateTableColumns() {
		std::cout << "Find table columns...";
		aux::startClock();

		for (int i = 0; i < table_rows.size(); i++) {
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
			min_seg = line_segments[table_rows[0][0]][0];  //left-most segment
			bool end_of_table = false;

			//FIND COLUMN GENERATORS
			while (!end_of_table) {
				duration = aux::endClock(); /*(std::clock() - start) / CLOCKS_PER_SEC;*/
				
				if (duration > 30) {
					end_of_table = true;
					i = table_rows.size();
					fail = true;
					fail_msg = "Find Table Columns task failed";
					break;
				}

				//find left-most segment in current table area
				for (int j = 0; j < table_rows[i].size(); j++) {
					for (int k = 0; k < line_segments[table_rows[i][j]].size(); k++) {
						int line_id = table_rows[i][j];
						int first_box_id = line_segments[line_id][k][0];
						int last_box_id = line_segments[line_id][k][line_segments[line_id][k].size() - 1];
						//int left = boxes[line_segments[table_rows[i][j]][k][0]][BOX_LEFT];
						//int right = boxes[line_segments[table_rows[i][j]][k][line_segments[table_rows[i][j]][k].size() - 1]][BOX_RIGHT];
						int left = boxes[first_box_id][BOX_LEFT];
						int right = boxes[last_box_id][BOX_RIGHT];

						if ((left <= boxes[min_seg[0]][BOX_LEFT]) && (left > limit)) {
							//min_seg = line_segments[table_rows[i][j]][k];
							min_seg = line_segments[line_id][k];
						}
					}
				}

				float avg_seg_len = 0;
				int counter = 0;
				int hor_thresh = (line_dims[table_rows[i][(int)table_rows[i].size() / 2]][LINE_BOTTOM] - line_dims[table_rows[i][(int)table_rows[i].size() / 2]][LINE_TOP]) * 2.6;
				for (int j = 0; j < table_rows[i].size(); j++) {
					for (int k = 0; k < line_segments[table_rows[i][j]].size(); k++) {
						int line_id = table_rows[i][j];
						int first_box_id = line_segments[line_id][k][0];
						int last_box_id = line_segments[line_id][k][line_segments[line_id][k].size() - 1];

						int left = boxes[first_box_id][BOX_LEFT];
						int right = boxes[last_box_id][BOX_RIGHT];

						// calc avg length of segments horizontally-aligned with min_seg
						if ((abs(left - boxes[min_seg[0]][BOX_LEFT]) <= hor_thresh) &&
							/*((right-left)<=(boxes[min_seg[min_seg.size()-1]][2]-boxes[min_seg[0]][0]))&&*/
							(left > limit)) {
							//min_seg=Lines_segments[table_Rows[i][j]][k];
							avg_seg_len = avg_seg_len + (right - left);
							counter++;
						}
					}
				}
				int fin_left = boxes[min_seg[0]][BOX_LEFT];
				avg_seg_len = (float)avg_seg_len / counter;
				float overlap_ratio = 1.2;
				avg_seg_len = avg_seg_len * overlap_ratio;
				for (int j = 0; j < table_rows[i].size(); j++) {
					for (int k = 0; k < line_segments[table_rows[i][j]].size(); k++) {
						int line_id = table_rows[i][j];
						int left_box_id = line_segments[line_id][k][0];
						int right_box_id = line_segments[line_id][k][line_segments[line_id][k].size() - 1];

						int left = boxes[left_box_id][BOX_LEFT];
						int right = boxes[right_box_id][BOX_RIGHT];

						// Select the segment that is closest to the avg length
						if ((abs(left - fin_left) <= hor_thresh) &&
							(abs((right - left) - avg_seg_len) <= abs((boxes[min_seg[min_seg.size() - 1]][BOX_RIGHT] - boxes[min_seg[0]][BOX_LEFT]) - avg_seg_len)) &&
							(left > limit)) {
							min_seg = line_segments[line_id][k];
						}
					}
				}

				column_creator.push_back(min_seg);
				int max_box_id = FindMaxBoxInSegment(min_seg);  //maybe there aren't sorted
				//limit = boxes[min_seg[min_seg.size() - 1]][BOX_RIGHT];  //set left-most limit for remaining segments
				limit = boxes[min_seg[max_box_id]][BOX_RIGHT];
				min_seg.clear();
				
				// find new min_seg initial value for new limit
				for (int j = 0; j < table_rows[i].size(); j++) {
					for (int k = 0; k < line_segments[table_rows[i][j]].size(); k++) {
						int left = boxes[line_segments[table_rows[i][j]][k][0]][BOX_LEFT];
						if (left > limit) {
							min_seg = line_segments[table_rows[i][j]][k];
							break;
						}
					}
				}

				if (min_seg.size() == 0) { end_of_table = true; }
			}
			tmp_col.push_back(column_creator);
		}

		if (fail) return;
		
		// Assign all the segments that horizontally overlap with the column generator to a new column.
		for (int i = 0; i < tmp_col.size(); i++) {
			vector<vector<vector<int>>> t_col;
			for (int j = 0; j < tmp_col[i].size(); j++) {
				int col_left = boxes[tmp_col[i][j][0]][BOX_LEFT]; //left side of first box of column creator (segment)
				int col_right = boxes[tmp_col[i][j][tmp_col[i][j].size() - 1]][BOX_RIGHT]; //right side of last box of column creator (segment)
				vector<vector<int>> t_seg;
				for (int k = 0; k < table_rows[i].size(); k++) {
					int line_id = table_rows[i][k];
					for (int z = 0; z < line_segments[line_id].size(); z++) {
						int seg_left = boxes[line_segments[line_id][z][0]][BOX_LEFT];
						int seg_right = boxes[line_segments[line_id][z][line_segments[line_id][z].size() - 1]][BOX_RIGHT];
						/*if (((seg_right >= col_left) && (seg_right <= col_right)) ||
							((seg_left >= col_left) && (seg_left <= col_right)) ||
							((seg_left <= col_left) && (seg_right >= col_right))) {
							t_seg.push_back(line_segments[line_id][z]);
						}*/
						if ((seg_right >= col_left) && (seg_left <= col_right)) {
							t_seg.push_back(line_segments[line_id][z]);
						}
					}
				}

				t_col.push_back(t_seg);
			}
			table_columns.push_back(t_col);
		}

		ProcessGeneratedColumns();
	}

	// Create table rows that include more than one lines
	void OCRTabsEngine::CreateTableMultiRows() {
		cout << "Find table multiple-rows...";
		aux::startClock();

		// if a table line does not have a segment in the first column, and there is one-to-one column correspondence 
		// with the line above it, 
		// it is merged with the one above it
		// this does not apply for the first line of the table
		for (int i = 0; i < table_rows.size(); i++) {
			vector<vector<int>> tmp_multi_row;
			vector<int> tmp_multi_lines;
			tmp_multi_lines.push_back(table_rows[i][0]);
			for (int j = 1; j < table_rows[i].size(); j++) {
				bool found = false;
				if (std::find(table_columns[i][0].begin(), table_columns[i][0].end(), line_segments[table_rows[i][j]][0]) != table_columns[i][0].end()) {
					found = true;
				}
				if (!found) {
					bool exist_all = true;
					for (int k = 0; k < line_segments[table_rows[i][j]].size(); k++) {
						bool exist = false;
						for (int s = 0; s < table_columns[i].size(); s++) {
							bool exist1 = false; bool exist0 = false;
							for (int h = 0; h < table_columns[i][s].size(); h++) {
								if (line_segments[table_rows[i][j]][k] == table_columns[i][s][h]) {
									exist1 = true;
								}
								for (int z = 0; z < line_segments[table_rows[i][j - 1]].size(); z++) {
									if (line_segments[table_rows[i][j - 1]][z] == table_columns[i][s][h]) {
										exist0 = true;
										if ((k < line_segments[table_rows[i][j]].size() - 1) &&
											(boxes[line_segments[table_rows[i][j - 1]][z][line_segments[table_rows[i][j - 1]][z].size() - 1]][BOX_RIGHT] >= boxes[line_segments[table_rows[i][j]][k + 1][0]][BOX_LEFT])) {
											exist0 = false;
										}
									}
								}
							}
							if ((exist1) && (exist0)) { s = table_columns[i].size(); exist = true; }
						}
						exist_all = (exist_all) && (exist);
					}
					if (exist_all) {
						tmp_multi_lines.push_back(table_rows[i][j]);
					} else {
						tmp_multi_row.push_back(tmp_multi_lines);
						tmp_multi_lines.clear();
						tmp_multi_lines.push_back(table_rows[i][j]);
					}
				} else {
					tmp_multi_row.push_back(tmp_multi_lines);
					tmp_multi_lines.clear();
					tmp_multi_lines.push_back(table_rows[i][j]);
				}
			}
			tmp_multi_row.push_back(tmp_multi_lines);
			multi_rows.push_back(tmp_multi_row);
			tmp_multi_lines.clear();
			tmp_multi_row.clear();

			// if a line is type-3, its segment is assigned ONLY to the first column,
			// the line below it is type-2, and it has a segment in the fist column which is more to the
			// right than the segment of the first line THEN these two lines are merged together
			for (int j = 0; j < multi_rows[i].size() - 1; j++) {
				if ((multi_rows[i][j].size() == 1) &&
					(lines_type[multi_rows[i][j][0]] == LineType::UNKNOWN) && (line_segments[multi_rows[i][j][0]].size() == 1) &&
					(lines_type[multi_rows[i][j + 1][0]] == LineType::TABLE)) {
					bool found1 = false;
					bool found2 = false;
					if (std::find(table_columns[i][0].begin(), table_columns[i][0].end(), line_segments[multi_rows[i][j][0]][0]) != table_columns[i][0].end()) {
						found1 = true;
					}
					if (std::find(table_columns[i][0].begin(), table_columns[i][0].end(), line_segments[multi_rows[i][j + 1][0]][0]) != table_columns[i][0].end()) {
						found2 = true;
					}
					int tmp = line_segments[multi_rows[i][j][0]][0][0];
					float lft1 = boxes[tmp][BOX_LEFT];
					tmp = line_segments[multi_rows[i][j + 1][0]][0][0];
					float lft2 = boxes[tmp][BOX_LEFT];
					if ((found1) && (found2) && (lft2 >= lft1 + 0.6 * (line_dims[multi_rows[i][j + 1][0]][1] - line_dims[multi_rows[i][j + 1][0]][0]))) {
						for (int z = 0; z < multi_rows[i][j + 1].size(); z++) {
							multi_rows[i][j].push_back(multi_rows[i][j + 1][z]);
						}
						multi_rows[i].erase(multi_rows[i].begin() + j + 1);
					}
				}
			}
			if ((multi_rows[i][multi_rows[i].size() - 1].size() == 1) &&
				(lines_type[multi_rows[i][multi_rows[i].size() - 1][0]] == 3) && (line_segments[multi_rows[i][multi_rows[i].size() - 1][0]].size() == 1) &&
				(line_segments[multi_rows[i][multi_rows[i].size() - 1][0]][0] == table_columns[i][0][table_columns[i][0].size() - 1])) {
				lines_type[multi_rows[i][multi_rows[i].size() - 1][0]] = LineType::TEXT;
				multi_rows[i].erase(multi_rows[i].begin() + multi_rows[i].size() - 1);
				table_columns[i][0].erase(table_columns[i][0].begin() + table_columns[i][0].size() - 1);
			}

			//Finalize Line Types. Change all Lines within the table to type-2
			for (int j = 0; j < multi_rows[i].size(); j++) {
				for (int k = 0; k < multi_rows[i][j].size(); k++) {
					lines_type[multi_rows[i][j][k]] = LineType::TABLE;
				}
			}
		}

		//Recheck and discard single and double row tables 
		for (int i = multi_rows.size() - 1; i >= 0; i--) {
			if ((multi_rows[i].size() < 2) && (multi_rows[i][0].size() < 2 || table_columns[i][0].size() < 2)) {
				for (unsigned s = 0; s < multi_rows[i][0].size(); s++) { lines_type[multi_rows[i][0][s]] = LineType::TEXT; }
				multi_rows.erase(multi_rows.begin() + i);
				table_columns.erase(table_columns.begin() + i);
			} else if ((multi_rows[i].size() < 3) && (multi_rows[i][0].size() < 2) && (multi_rows[i][1].size() < 2)) {
				for (unsigned s = 0; s < multi_rows[i][0].size(); s++) { lines_type[multi_rows[i][0][s]] = LineType::TEXT; }
				for (unsigned s = 0; s < multi_rows[i][1].size(); s++) { lines_type[multi_rows[i][1][s]] = LineType::TEXT; }
				multi_rows.erase(multi_rows.begin() + i);
				table_columns.erase(table_columns.begin() + i);
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
		std::cout << "Find column sizes...";
		aux::startClock();

		for (int i = 0; i < table_columns.size(); i++) {
			vector<int*> dims;
			for (int j = 0; j < table_columns[i].size(); j++) {
				int* tmp = new int[2];
				tmp[0] = page_right;
				tmp[1] = page_left;
				for (int k = 0; k < table_columns[i][j].size(); k++) {
					bool flag_left = true;
					bool flag_right = true;
					if (j >= 1) {
						if (std::find(table_columns[i][j - 1].begin(), table_columns[i][j - 1].end(), table_columns[i][j][k]) != table_columns[i][j - 1].end()) {
							flag_left = false;
						}
					}
					if (j < table_columns[i].size() - 1) {
						if (std::find(table_columns[i][j + 1].begin(), table_columns[i][j + 1].end(), table_columns[i][j][k]) != table_columns[i][j + 1].end()) {
							flag_right = false;
						}
					}
					if ((boxes[table_columns[i][j][k][0]][0] <= tmp[0]) && (flag_left)) {
						tmp[0] = boxes[table_columns[i][j][k][0]][0];
					}
					if ((boxes[table_columns[i][j][k][table_columns[i][j][k].size() - 1]][2] >= tmp[1]) && (flag_right)) {
						tmp[1] = boxes[table_columns[i][j][k][table_columns[i][j][k].size() - 1]][2];
					}
				}
				dims.push_back(tmp);
			}
			col_dims.push_back(dims);
		}

		for (int i = table_columns.size() - 1; i >= 0; i--) {
			if (table_columns[i].size() == 2) {
				int colSizeA = col_dims[i][0][1] - col_dims[i][0][0];
				int colSizeB = col_dims[i][1][1] - col_dims[i][1][0];
				if (colSizeB >= 10 * colSizeA) {
					table_columns.erase(table_columns.begin() + i);
					for (int j = 0; j < multi_rows[i].size(); j++) {
						for (int k = 0; k < multi_rows[i][j].size(); k++) {
							lines_type[multi_rows[i][j][k]] = 1;
						}
					}
					//table_Rows.erase(table_Rows.begin()+i);
					multi_rows.erase(multi_rows.begin() + i);
					col_dims.erase(col_dims.begin() + i);
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
		std::cout << "Finalize grid...";
		aux::startClock();

		for (int i = 0; i < col_dims.size(); i++) {
			for (int j = 1; j < col_dims[i].size(); j++) {
				col_dims[i][j][0] = (col_dims[i][j][0] + col_dims[i][j - 1][1]) / 2;
				col_dims[i][j - 1][1] = col_dims[i][j][0];
			}
		}
		for (int i = 0; i < multi_rows.size(); i++) {
			vector<int*> dims;
			for (int j = 0; j < multi_rows[i].size(); j++) {
				int* tmp = new int[2];
				tmp[0] = line_dims[multi_rows[i][j][0]][0];
				tmp[1] = line_dims[multi_rows[i][j][multi_rows[i][j].size() - 1]][1];
				dims.push_back(tmp);
				if (j > 0) {
					dims[j][0] = (dims[j][0] + dims[j - 1][1]) / 2;
					int low = dims[j][0];
					dims[j - 1][1] = low;
				}
			}
			row_dims.push_back(dims);
		}

		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	void OCRTabsEngine::DrawBoxes() {
		drawingHandler::DrawBoxes(test, boxes, words, confs, font_size, bold, italic, underscore, dict);
	}

	void OCRTabsEngine::DrawLines() {
		drawingHandler::DrawLines(test, lines, page_left, page_right, page_top, page_bottom, line_dims);
	}

	void OCRTabsEngine::DrawSegments() {
		drawingHandler::DrawSegments(test, lines, line_segments, line_dims, boxes);
	}

	void OCRTabsEngine::DrawAreas() {
		drawingHandler::DrawAreas(test, table_area, line_dims, page_left, page_right);
	}

	/**
	 * Draw rows on a cv window
	 */
	void OCRTabsEngine::DrawRows() {
		drawingHandler::DrawRows(test, multi_rows, line_dims, page_left, page_right);
	}

	void OCRTabsEngine::DrawColsPartial() {
		drawingHandler::DrawColsPartial(test, boxes, tmp_col);
	}

	void OCRTabsEngine::DrawCols() {
		drawingHandler::DrawCols(test, boxes, tmp_col, table_columns);
	}

	void OCRTabsEngine::DrawGrid() {
		drawingHandler::DrawGrid(test, boxes, row_dims, col_dims, multi_rows, line_segments);
	}

	void OCRTabsEngine::DrawGridlessImage() {
		drawingHandler::DrawGridlessImage(test);
	}

	//void ocr_tabs::DrawFootHead() {
	//	drawingHandler::DrawFootHead(test, Lines_type, Lines, Line_dims, page_left, page_right);
	//} 

	// Resets the image to initial state.
	void OCRTabsEngine::ResetImage() {
		test = initial.clone();
		//resize(test,test,Size(test.size().width*2,test.size().height*2));
	}

	Mat OCRTabsEngine::SegmentImage(Mat img) {
		//Search for multi column text
		std::cout << "Segment Image...";
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


		cvtColor(img, img, cv::COLOR_GRAY2BGR);  //CV_GRAY2BGR
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
		std::cout << "Process Image...";
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
		imgProcessor::segmentationBlocks blk;
		cv::Mat clean, clean2;
		imgProcessor::prepareAll(img, clean, blk);

		if (ratio >= 0.8) cv::erode(clean, clean, cv::Mat(), cv::Point(-1, -1), 1);
		//cv::imshow("asd", clean);
		//cv::waitKey(0);
		imgProcessor::getTextImage(clean, blk, clean2);
		//imgProcessor::getTextImage(img, blk, clean2);
		cv::Size orgSiz = img.size();
		int max_org_width_height = std::max(orgSiz.width, orgSiz.height);
		int min_org_width_height = std::min(orgSiz.width, orgSiz.height);
		int min_max_wh_ratio = min_org_width_height / (float)max_org_width_height;

		imgProcessor::reorderImage(clean2, blk, img);

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
		aux::WriteHTML(filename, table_columns, multi_rows, line_segments, lines_type, words, font_size);
	}

	bool OCRTabsEngine::fail_condition() { 
		return fail; 
	}

	bool OCRTABS_API OCRTabsEngine::doc2html(FileType filetype, const std::string& filename, const std::string& filenameXML, bool withXML) {
		resetAll();
		std::vector<cv::Mat> pages, pages_clean;
		switch (filetype) {
		case FileType::PDF:
			if (!parsePDF(filename, pages)) return false;
			break;

		case FileType::IMG:
			pages.push_back(cv::imread(filename, cv::IMREAD_GRAYSCALE));  //CV_LOAD_IMAGE_GRAYSCALE	
			if (pages[0].empty()) { std::cout << "File not available" << std::endl; return false; }
			break;

		default:
			return false;
		}
		if (withXML) {
			//// NOT WORKING ////
			if (!PreprocessImageWithXML(filenameXML, pages, pages_clean)) { cout << "Preprocessing with XML failed" << endl; return false; }
			pages.clear();
			pages = pages_clean;  //not sure about that copying
		}
		for (int i = 0; i < pages.size(); i++) {
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
			std::cout << std::endl << "failCondition: " << fail_msg << std::endl;
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
		words.clear();
		boxes.clear();
		lines.clear();
		table_area.clear();
		table_rows.clear();
		multi_rows.clear();
		line_dims.clear();
		line_segments.clear();
		table_columns.clear();
		col_dims.clear();
		row_dims.clear();
		tmp_col.clear();
		confs.clear();
		bold.clear();
		dict.clear();
		italic.clear();
		underscore.clear();
		font_size.clear();
		if (lines_type != NULL) delete lines_type;
		words_.clear();
		lines_.clear();
		boxes_.clear();
		confs_.clear();
		font_size_.clear();
		bold_.clear();
		dict_.clear();
		italic_.clear();
		underscore_.clear();
		page_height.clear();
		page_width.clear();
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
			//fz_rotate(&transform, rotation);
			fz_pre_scale(transform, zoom / 100.0f, zoom / 100.0f);
			//fz_pre_scale(&transform, zoom / 100.0f, zoom / 100.0f);
			fz_rect bounds = fz_bound_page(ctx, page);
			//fz_bound_page(ctx, page, &bounds);
			fz_transform_rect(bounds, transform);
			//fz_transform_rect(&bounds, &transform);
			fz_irect bbox = fz_round_rect(bounds);
			//fz_round_rect(&bbox, &bounds);
			fz_pixmap* pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, NULL, 1);
			//fz_pixmap* pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox);
			fz_clear_pixmap_with_value(ctx, pix, 0xff);
			//fz_device* dev = fz_new_draw_device(ctx, pix);
			fz_device* dev = fz_new_draw_device(ctx, transform, pix);
			fz_run_page(ctx, page, dev, transform, NULL);
			fz_drop_device(ctx, dev);
			cv::Mat input;
			imgProcessor::pixmap2mat(&pix, input);
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
					cout << "here" << endl;
					std::size_t foundW = str.find("<page width=");
					if (foundW != std::string::npos) {
						resiz = (float)imageRAW[i].cols / std::stoi(str.substr(foundW + 13, str.find("height=") - 2 - foundW - 13));
						mask = cv::Mat::zeros(imageRAW[i].rows / resiz, imageRAW[i].cols / resiz, CV_8UC1);
					}
				}
				else if (str.find("<line baseline") != std::string::npos && inText) {
					cout << "not here" << endl;
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
					cout << "not here" << endl;
					inText = true;
				} else if (str.find("</block>") != std::string::npos) {
					cout << "not here" << endl;
					inText = false;
				} else if (str.find("</page>") != std::string::npos) {
					cout << "not here" << endl;
					cv::resize(mask, mask, imageRAW[i].size());
					imageRAW[i].copyTo(imageCLN[i], mask);

					//imgProcessor::thresholdImg(imageCLN[i], imageCLN[i]);
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
		for (int i = boxes.size() - 1; i >= 0; i--) {
			string tmp = words[i];
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
			if (tmp.find("figure") != std::string::npos) {
				for (int j = i - 1; j >= 3; j--) {
					bool image = (dict[j] && dict[j - 1] && dict[j - 2] && dict[j - 3]);
					if (image) {
						if (j >= (i - 5)) {
							j = 0;
						}
						else {
							int ln_index = i;
							for (int k = i + 1; k < boxes.size(); k++) {
								if (!(((boxes[k][BOX_TOP] <= boxes[i][BOX_TOP]) && (boxes[i][BOX_TOP] <= boxes[k][BOX_BOTTOM])) ||
									((boxes[i][BOX_TOP] <= boxes[k][BOX_TOP]) && (boxes[k][BOX_TOP] <= boxes[i][BOX_BOTTOM])))) {
									ln_index = k - 1;
									k = boxes.size();
								}
							}
							for (int k = ln_index; k >= j + 1; k--) {
								boxes.erase(boxes.begin() + k);
								words.erase(words.begin() + k);
								confs.erase(confs.begin() + k);
								font_size.erase(font_size.begin() + k);
								bold.erase(bold.begin() + k);
								italic.erase(italic.begin() + k);
								underscore.erase(underscore.begin() + k);
								dict.erase(dict.begin() + k);
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
		for (int i = 0; i < table_columns.size(); i++) {
			for (int j = 0; j < table_columns[i].size(); j++) {
				if (table_columns[i][j].size() == 0) {
					table_columns[i].erase(table_columns[i].begin() + j);
					j--;
				}
			}
		}

		// If we find a column where the unique segments  (segments that are assigned to only one column)
		// are less than the multiple segemnts (segments assigned to more than 1 column), we merge this column with
		// the immediatelly previous one
		for (int i = 0; i < table_columns.size(); i++) {
			for (int j = 1; j < table_columns[i].size(); j++) {
				int counter_single = 0;
				int counter_multi = 0;
				for (int k = 0; k < table_columns[i][j].size(); k++) {
					bool multi = false;
					if (std::find(table_columns[i][j - 1].begin(), table_columns[i][j - 1].end(), table_columns[i][j][k]) != table_columns[i][j - 1].end()) {
						multi = true;
					}
					if (multi) {
						counter_multi++;
					}
					else {
						counter_single++;
					}
				}
				if (counter_multi >= 1 * counter_single) {
					for (int k = 0; k < table_columns[i][j].size(); k++) {
						bool multi = false;
						if (std::find(table_columns[i][j - 1].begin(), table_columns[i][j - 1].end(), table_columns[i][j][k]) != table_columns[i][j - 1].end()) {
							multi = true;
						}
						if (!multi) { table_columns[i][j - 1].push_back(table_columns[i][j][k]); }
					}
					table_columns[i].erase(table_columns[i].begin() + j);
					j--;
				}
			}
		}

		// Type-3 lines that are in the end of a table, and their single segment is assigned to more than one columns,
		// are removed from the table
		for (int i = 0; i < table_rows.size(); i++) {
			for (int j = table_rows[i].size() - 1; j >= 0; j--) {
				//cout << table_Rows[0].size()<<"     "<<table_Rows[1].size()<<"\n";
				//cout << i<<"     "<<j<<"\n";
				if (lines_type[table_rows[i][j]] == LineType::UNKNOWN) {
					vector<int> xcol;
					for (int k = 0; k < table_columns[i].size(); k++) {
						if (table_columns[i][k].size() > 0) {
							if (line_segments[table_rows[i][j]][0] == table_columns[i][k][table_columns[i][k].size() - 1]) {
								xcol.push_back(k);
							}
						}
					}
					if (xcol.size() > 1) {
						lines_type[table_rows[i][j]] = LineType::TEXT;
						table_rows[i].erase(table_rows[i].begin() + j);
						for (int k = 0; k < xcol.size(); k++) {
							table_columns[i][xcol[k]].erase(table_columns[i][xcol[k]].begin() + table_columns[i][xcol[k]].size() - 1);
						}
					} else {
						j = -1;
					}
				} else {
					j = -1;
				}
			}
		}

		//If a column has only one segment, which is on the 1st row (possibly missaligned table header) and the column on its left doesnot have a segment
		// in the same row, then merge these columns
		for (int i = 0; i < table_columns.size(); i++) {
			for (int j = 1; j < table_columns[i].size(); j++) {
				if (table_columns[i][j].size() == 1) {
					bool found1 = false;
					bool found2 = false;
					if (std::find(line_segments[table_rows[i][0]].begin(), line_segments[table_rows[i][0]].end(), table_columns[i][j][0]) != line_segments[table_rows[i][0]].end()) {
						found1 = true;
					}
					if (std::find(line_segments[table_rows[i][0]].begin(), line_segments[table_rows[i][0]].end(), table_columns[i][j - 1][0]) != line_segments[table_rows[i][0]].end()) {
						found2 = true;
					}
					if ((found1) && (!found2)) {
						for (int k = 0; k < table_columns[i][j - 1].size(); k++) {
							table_columns[i][j].push_back(table_columns[i][j - 1][k]);
						}
						table_columns[i].erase(table_columns[i].begin() + j - 1);
					}
				}
			}
		}

		// Tables that end up having only one column, are discarded and treated as simple text
		for (int i = table_columns.size() - 1; i >= 0; i--) {
			if (table_columns[i].size() < 2) {
				table_columns.erase(table_columns.begin() + i);
				for (int j = 0; j < table_rows[i].size(); j++) {
					lines_type[table_rows[i][j]] = LineType::TEXT;
				}
				table_rows.erase(table_rows.begin() + i);
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
		for (int i = table_columns.size() - 1; i >= 0; i--) {
			bool almost_empty = false;
			for (int j = 1; j < table_columns[i].size(); j++) {
				if (table_columns[i][j].size() < table_rows[i].size() / 2) {
					almost_empty = true;
				}
				else {
					almost_empty = false;
					j = table_columns[i].size();
				}
			}
			if (almost_empty) {
				table_columns.erase(table_columns.begin() + i);
				for (int k = 0; k < table_rows[i].size(); k++) {
					lines_type[table_rows[i][k]] = LineType::TEXT;
				}
				table_rows.erase(table_rows.begin() + i);
			}
		}

		//duration = (std::clock() - start) / CLOCKS_PER_SEC;
		//std::cout << " Done in " << duration << "s \n";
		std::cout << " Done in " << aux::endClock() << "s \n";
	}

	int OCRTabsEngine::FindMaxBoxInSegment(const std::vector<int>& seg) {
		int max_right = -1;
		int max_box_id = -1;
		for (int i = 0; i < seg.size(); i++) {
			if (boxes[seg[i]][BOX_RIGHT] > max_right) {
				max_right = boxes[seg[i]][BOX_RIGHT];
				max_box_id = i;
			}
		}

		return max_box_id;
	}
}