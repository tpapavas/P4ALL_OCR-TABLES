#pragma once

#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <ctime>



#define TESSDLL_IMPORTS

#include "tesseract/baseapi.h"

//typedef std::vector<std::vector<std::vector<std::vector<int>>>> vec3d_int;
namespace ocr_tabs {
	class drawingHandler {
	public:
		OCRTABS_API drawingHandler();
		OCRTABS_API ~drawingHandler();

		void DrawBoxes(cv::Mat test, std::vector<std::vector<int>> boxes, std::vector<char*> words, std::vector<float> confs, std::vector<int> font_size, std::vector<bool> bold, std::vector<bool> italic, std::vector<bool> underscore, std::vector<bool> dict);
		void DrawLines(cv::Mat test, std::vector<std::vector<int>> Lines, int page_left, int page_right, int page_top, int page_bottom, std::vector<int*> Line_dims);
		void DrawSegments(cv::Mat test, std::vector<std::vector<int>> Lines, std::vector<std::vector<std::vector<int>>> Lines_segments, std::vector<int*> Line_dims, std::vector<std::vector<int>> boxes);
		void DrawAreas(cv::Mat test, std::vector<std::vector<int>> table_area, std::vector<int*> Line_dims, int page_left, int page_right);
		void DrawRows(cv::Mat test, std::vector<std::vector<std::vector<int>>> multi_Rows, std::vector<int*> Line_dims, int page_left, int page_right);
		void DrawColsPartial(cv::Mat test, std::vector<std::vector<int>> boxes, std::vector<std::vector<std::vector<int>>> tmp_col);
		void DrawCols(cv::Mat test, std::vector<std::vector<int>> boxes, std::vector<std::vector<std::vector<int>>> tmp_col, std::vector<std::vector<std::vector<std::vector<int>>>> table_Columns);
		void DrawGrid(cv::Mat test, std::vector<std::vector<int>> boxes, std::vector<std::vector<int*>> row_dims, std::vector<std::vector<int*>> col_dims, std::vector<std::vector<std::vector<int>>> multi_Rows, std::vector<std::vector<std::vector<int>>> Lines_segments);
		void DrawGridlessImage(cv::Mat test);
		void DrawFootHead(cv::Mat test, int* Lines_type, std::vector<std::vector<int>> Lines, std::vector<int*> Line_dims, int page_left, int page_right);
	};
}