#pragma once
#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <ctime>
#include "core/document.h"


//typedef std::vector<std::vector<std::vector<std::vector<int>>>> vec3d_int;
namespace ocrt {
	class DrawingHandler {
	public:
		DrawingHandler();
		~DrawingHandler();
		void DrawBoxes(cv::Mat& test, const Document& doc);
		void DrawLines(cv::Mat& test, const Document& doc);
		void DrawSegments(cv::Mat& test, const Document& doc);
		void DrawAreas(cv::Mat& test, const Document& doc, std::vector<std::vector<int>>& table_area);
		void DrawRows(cv::Mat& test, const Document& doc);
		void DrawColsPartial(cv::Mat& test, const Document& doc, std::vector<std::vector<std::vector<int>>>& tmp_col);
		void DrawCols(cv::Mat& test, const Document& doc);
		void DrawGrid(cv::Mat& test, const Document& doc);
		void DrawGridlessImage(cv::Mat& test);
		void DrawFootHead(cv::Mat& test, const Document& doc);
	};
}