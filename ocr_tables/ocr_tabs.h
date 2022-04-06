#pragma once
#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <ctime>



 #define TESSDLL_IMPORTS

#include "tesseract/baseapi.h"

//probably problematic
//using namespace cv;
//using namespace std;

class ocr_tabs
{
	public:
			OCRTABS_API ocr_tabs ();
			OCRTABS_API ~ocr_tabs ();
			void SetImage(cv::Mat img);
			void RemoveGridLines(float ratio=1);
			void OCR_Recognize();
			void BoxesAndWords();
			void TextBoundaries();
			void TextLines();
			void HeadersFooters();
			void LineSegments();
			void LineTypes();
			void TableAreas();
			void TableRows();
			void TableColumns();
			void TableMultiRows();
			void ColumnSize();
			void FinalizeGrid();
			void WriteHTML(std::string& filename);
			void PrepareMulti1();
			void PrepareMulti2();
			cv::Mat ImgSeg(cv::Mat img);
			cv::Mat getInitial (){return initial;}

			void DrawBoxes();
			void DrawLines();
			void DrawSegments();
			void DrawAreas();
			void DrawRows();
			void DrawColsPartial();
			void DrawCols();
			void DrawGrid();
			void DrawGridlessImage();
			//void DrawFootHead();

			void ResetImage();
			cv::Mat ImagePreproccesing(cv::Mat img);
			bool ImagePreproccesing_withXML(const std::string& fileXML, std::vector<cv::Mat>& imageRAW, std::vector<cv::Mat>& imageCLN);
			
			bool fail_condition();
			bool OCRTABS_API pdf2html (const std::string& filename);
			bool OCRTABS_API img2html (const std::string& filename);
			bool OCRTABS_API pdf2html_withXML (const std::string& filename, const std::string& filenameXML);
			bool OCRTABS_API img2html_withXML (const std::string& filename, const std::string& filenameXML);
			bool parsePDF(const std::string& filename, std::vector<cv::Mat>& imageList);
			void resetAll();

private:
			cv::Mat test,initial;
			tesseract::TessBaseAPI  tess;
			clock_t start;
			double duration;
			std::vector<char*> words;
			std::vector<std::vector<int>> boxes, Lines, table_area, table_Rows;
			std::vector<std::vector<std::vector<int>>> multi_Rows;
			std::vector<int*> Line_dims;
			std::vector<std::vector<std::vector<int>>> Lines_segments;
			std::vector<std::vector<std::vector<std::vector<int>>>> table_Columns;
			std::vector<std::vector<int*>> col_dims, row_dims;
			std::vector<std::vector<std::vector<int>>> tmp_col;
			std::vector<float> confs;
			std::vector<bool> bold;
			std::vector<bool> dict;
			std::vector<bool> italic;
			std::vector<bool> underscore;
			std::vector<int> font_size;
			int page_left, page_right, page_top, page_bottom;
			int* Lines_type;
			std::vector<std::vector<char*>> words_;
			std::vector<std::vector<std::vector<int>>> Lines_;
			std::vector<std::vector<std::vector<int>>> boxes_;
			std::vector<std::vector<float>> confs_;
			std::vector<std::vector<int>> font_size_;
			std::vector<std::vector<bool>> bold_;
			std::vector<std::vector<bool>> dict_;
			std::vector<std::vector<bool>> italic_;
			std::vector<std::vector<bool>> underscore_;
			std::vector<int> page_height,page_width;
			bool fail;
};
