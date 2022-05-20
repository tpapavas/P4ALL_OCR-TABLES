#include "drawing_handler.h"

#include <iostream>
#include <fstream>
extern "C" {
	#include <mupdf/fitz.h>
}


using namespace cv;
//using namespace std;

#pragma warning( disable : 4018 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )

namespace ocr_tabs {
	/*drawing_handler::drawing_handler() {

	}

	drawing_handler::~drawing_handler() {

	}*/

	void drawing_handler::DrawBoxes(cv::Mat& test, std::vector<std::vector<int>>& boxes, std::vector<char*>& words, std::vector<float>& confs, std::vector<int>& font_size, std::vector<bool>& bold, std::vector<bool>& italic, std::vector<bool>& underscore, std::vector<bool>& dict) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		for (int i = 0; i < boxes.size(); i++) {
			int top = boxes[i][1];
			int bottom = boxes[i][3];
			int left = boxes[i][0];
			int right = boxes[i][2];

			line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);

			std::cout << "\n" << words[i] << "\t\t" << confs[i] << "  " << font_size[i] << " ";

			if (bold[i]) { std::cout << "bold  "; }
			if (italic[i]) { std::cout << "italic  "; }
			if (underscore[i]) { std::cout << "underlined  "; }
			if (dict[i]) { std::cout << "in dictionary  "; }

			std::cout << boxes[i][0] << "|" << boxes[i][1] << "|" << boxes[i][2] << "|" << boxes[i][3];
			std::cout << "\n";

			imshow("img", test);
			char c = waitKey(0);
		}
	}

	void drawing_handler::DrawLines(cv::Mat& test, std::vector<std::vector<int>>& Lines, int page_left, int page_right, int page_top, int page_bottom, std::vector<int*>& Line_dims) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		for (int i = 0; i < Lines.size(); i++) {
			line(test, Point2i(page_left, Line_dims[i][0]), Point2i(page_right, Line_dims[i][0]), Scalar(0, 0, 0), 2);
			line(test, Point2i(page_right, Line_dims[i][0]), Point2i(page_right, Line_dims[i][1]), Scalar(0, 0, 0), 2);
			line(test, Point2i(page_right, Line_dims[i][1]), Point2i(page_left, Line_dims[i][1]), Scalar(0, 0, 0), 2);
			line(test, Point2i(page_left, Line_dims[i][1]), Point2i(page_left, Line_dims[i][0]), Scalar(0, 0, 0), 2);
			imshow("img", test);
			char c = waitKey(0);
		}
	}

	void drawing_handler::DrawSegments(cv::Mat& test, std::vector<std::vector<int>>& Lines, std::vector<std::vector<std::vector<int>>>& Lines_segments, std::vector<int*>& Line_dims, std::vector<std::vector<int>>& boxes) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		for (int i = 0; i < Lines.size(); i++) {
			for (int j = 0; j < Lines_segments[i].size(); j++) {
				int top = Line_dims[i][0];
				int bottom = Line_dims[i][1];
				int left = boxes[Lines_segments[i][j][0]][0];
				int right = boxes[Lines_segments[i][j][Lines_segments[i][j].size() - 1]][2];
				line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				imshow("img", test);
				char c = waitKey(0);
			}
		}
	}

	void drawing_handler::DrawAreas(cv::Mat& test, std::vector<std::vector<int>>& table_area, std::vector<int*>& Line_dims, int page_left, int page_right) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < table_area.size(); i++) {
			int top = Line_dims[table_area[i][0]][0];
			int bottom = Line_dims[table_area[i][table_area[i].size() - 1]][1];
			int left = page_left;
			int right = page_right;
			line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
			imshow("img", test);
			char c = waitKey(0);
		}
	}

	void drawing_handler::DrawRows(cv::Mat& test, std::vector<std::vector<std::vector<int>>>& multi_Rows, std::vector<int*>& Line_dims, int page_left, int page_right) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < multi_Rows.size(); i++) {
			for (int j = 0; j < multi_Rows[i].size(); j++) {

				int top = Line_dims[multi_Rows[i][j][0]][0];
				int bottom = Line_dims[multi_Rows[i][j][multi_Rows[i][j].size() - 1]][1];
				int left = page_left;
				int right = page_right;
				line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				imshow("img", test);
				char c = waitKey(0);
			}
		}
	}

	void drawing_handler::DrawColsPartial(cv::Mat& test, std::vector<std::vector<int>>& boxes, std::vector<std::vector<std::vector<int>>>& tmp_col) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < tmp_col.size(); i++) {
			for (int j = 0; j < tmp_col[i].size(); j++) {
				int top = boxes[tmp_col[i][j][0]][1];
				int bottom = boxes[tmp_col[i][j][0]][3];
				int left = boxes[tmp_col[i][j][0]][0];
				int right = boxes[tmp_col[i][j][tmp_col[i][j].size() - 1]][2];
				line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				imshow("img", test);
				char c = waitKey(0);
			}
		}
	}

	void drawing_handler::DrawCols(cv::Mat& test, std::vector<std::vector<int>>& boxes, std::vector<std::vector<std::vector<int>>>& tmp_col, std::vector<std::vector<std::vector<std::vector<int>>>>& table_Columns) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < table_Columns.size(); i++) {
			for (int j = 0; j < table_Columns[i].size(); j++) {
				for (int k = 0; k < table_Columns[i][j].size(); k++) {
					int tmp = table_Columns[i][j][k][0];
					int bottom = boxes[tmp][3];
					int top = boxes[tmp][1];
					int left = boxes[tmp][0];
					tmp = table_Columns[i][j][k][table_Columns[i][j][k].size() - 1];
					int right = boxes[tmp][2];
					line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
					line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
					line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
					line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
					imshow("img", test);
					char c = waitKey(0);
				}
			}
		}
	}

	void drawing_handler::DrawGrid(cv::Mat& test, std::vector<std::vector<int>>& boxes, std::vector<std::vector<int*>>& row_dims, std::vector<std::vector<int*>>& col_dims, std::vector<std::vector<std::vector<int>>>& multi_Rows, std::vector<std::vector<std::vector<int>>>& Lines_segments) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < col_dims.size(); i++) {
			for (int j = 0; j < row_dims[i].size(); j++) {
				int top = row_dims[i][j][0];
				int bottom = row_dims[i][j][1];
				for (int k = 0; k < col_dims[i].size(); k++) {
					int left = col_dims[i][k][0];
					int right = col_dims[i][k][1];

					line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
					line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
					line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
					line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				}
				for (int k = 0; k < multi_Rows[i][j].size(); k++) {
					for (int h = 0; h < Lines_segments[multi_Rows[i][j][k]].size(); h++) {
						int left = boxes[Lines_segments[multi_Rows[i][j][k]][h][0]][0];
						int right = boxes[Lines_segments[multi_Rows[i][j][k]][h][Lines_segments[multi_Rows[i][j][k]][h].size() - 1]][2];
						int col_left = -1;
						int col_right = -1;

						for (int z = 0; z < col_dims[i].size(); z++) {
							if ((left <= col_dims[i][z][1]) && (col_left == -1)) {
								col_left = z;
							}
							if ((right <= col_dims[i][z][1]) && (col_right == -1)) {
								col_right = z;
							}
						}
						if (col_left != col_right) {
							for (int z = col_left + 1; z <= col_right; z++) {
								int point = col_dims[i][z][0];
								line(test, Point2i(point, top + (bottom - top) * 0.1), Point2i(point, bottom), Scalar(255, 255, 255), 2);
							}
						}
					}
				}
			}
		}

		imshow("img", test);
		char c = waitKey(0);
	}

	void drawing_handler::DrawGridlessImage(cv::Mat& test) {
		namedWindow("img", cv::WINDOW_NORMAL);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		imshow("img", test);
		waitKey(0);
	}

	void drawing_handler::DrawFootHead(cv::Mat& test, int* Lines_type, std::vector<std::vector<int>>& Lines, std::vector<int*>& Line_dims, int page_left, int page_right) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < Lines.size(); i++) {
			if (Lines_type[i] == 4) {
				line(test, Point2i(page_left, Line_dims[i][0]), Point2i(page_right, Line_dims[i][0]), Scalar(0, 0, 0), 2);
				line(test, Point2i(page_right, Line_dims[i][0]), Point2i(page_right, Line_dims[i][1]), Scalar(0, 0, 0), 2);
				line(test, Point2i(page_right, Line_dims[i][1]), Point2i(page_left, Line_dims[i][1]), Scalar(0, 0, 0), 2);
				line(test, Point2i(page_left, Line_dims[i][1]), Point2i(page_left, Line_dims[i][0]), Scalar(0, 0, 0), 2);
			}
		}
		imshow("img", test);
		char c = cvWaitKey(0);
	}
}