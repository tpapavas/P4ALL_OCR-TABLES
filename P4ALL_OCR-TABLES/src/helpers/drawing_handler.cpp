#include "helpers/drawing_handler.h"

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

namespace ocrt {
	DrawingHandler::DrawingHandler() {

	}

	DrawingHandler::~DrawingHandler() {

	}

	void DrawingHandler::DrawBoxes(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		for (int i = 0; i < doc.words.size(); i++) {
			int top = doc.words[i].box[1];
			int bottom = doc.words[i].box[3];
			int left = doc.words[i].box[0];
			int right = doc.words[i].box[2];

			line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);

			std::cout << "\n" << doc.words[i].name << "\t\t" << doc.words[i].conf << "  " << doc.words[i].font_size << " ";

			if (doc.words[i].bold) { std::cout << "bold  "; }
			if (doc.words[i].italic) { std::cout << "italic  "; }
			if (doc.words[i].underscore) { std::cout << "underlined  "; }
			if (doc.words[i].dict) { std::cout << "in dictionary  "; }

			std::cout << doc.words[i].box[0] << "|" << doc.words[i].box[1] << "|" << doc.words[i].box[2] << "|" << doc.words[i].box[3];
			std::cout << "\n";

			imshow("img", test);
			char c = waitKey(0);
		}
	}

	void DrawingHandler::DrawLines(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		for (int i = 0; i < doc.lines.size(); i++) {
			line(test, Point2i(doc.page_left, doc.lines[i].dims[0]), Point2i(doc.page_right, doc.lines[i].dims[0]), Scalar(0, 0, 0), 2);
			line(test, Point2i(doc.page_right, doc.lines[i].dims[0]), Point2i(doc.page_right, doc.lines[i].dims[1]), Scalar(0, 0, 0), 2);
			line(test, Point2i(doc.page_right, doc.lines[i].dims[1]), Point2i(doc.page_left, doc.lines[i].dims[1]), Scalar(0, 0, 0), 2);
			line(test, Point2i(doc.page_left, doc.lines[i].dims[1]), Point2i(doc.page_left, doc.lines[i].dims[0]), Scalar(0, 0, 0), 2);
			imshow("img", test);
			char c = waitKey(0);
		}
	}

	void DrawingHandler::DrawSegments(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		for (int i = 0; i < doc.lines.size(); i++) {
			for (int j = 0; j < doc.lines[i].segments.size(); j++) {
				int top = doc.lines[i].dims[0];
				int bottom = doc.lines[i].dims[1];
				int left = doc.words[doc.lines[i].segments[j][0]].box[0];
				int right = doc.words[doc.lines[i].segments[j][doc.lines[i].segments[j].size() - 1]].box[2];
				line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				imshow("img", test);
				char c = waitKey(0);
			}
		}
	}

	void DrawingHandler::DrawAreas(cv::Mat& test, const Document& doc, std::vector<std::vector<int>>& table_area) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < table_area.size(); i++) {
			int top = doc.lines[table_area[i][0]].dims[0];
			int bottom = doc.lines[table_area[i][table_area[i].size() - 1]].dims[1];
			int left = doc.page_left;
			int right = doc.page_right;
			line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
			line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
			imshow("img", test);
			char c = waitKey(0);
		}
	}

	void DrawingHandler::DrawRows(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < doc.tables.size(); i++) {
			for (int j = 0; j < doc.tables[i].multi_rows.size(); j++) {

				int top = doc.lines[doc.tables[i].multi_rows[j][0]].dims[0];
				int bottom = doc.lines[doc.tables[i].multi_rows[j][doc.tables[i].multi_rows[j].size() - 1]].dims[1];
				int left = doc.page_left;
				int right = doc.page_right;
				line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				imshow("img", test);
				char c = waitKey(0);
			}
		}
	}

	void DrawingHandler::DrawColsPartial(cv::Mat& test, const Document& doc, std::vector<std::vector<std::vector<int>>>& tmp_col) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < tmp_col.size(); i++) {
			for (int j = 0; j < tmp_col[i].size(); j++) {
				int top = doc.words[tmp_col[i][j][0]].box[1];
				int bottom = doc.words[tmp_col[i][j][0]].box[3];
				int left = doc.words[tmp_col[i][j][0]].box[0];
				int right = doc.words[tmp_col[i][j][tmp_col[i][j].size() - 1]].box[2];
				line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
				line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				imshow("img", test);
				char c = waitKey(0);
			}
		}
	}

	void DrawingHandler::DrawCols(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < doc.tables.size(); i++) {
			for (int j = 0; j < doc.tables[i].columns.size(); j++) {
				for (int k = 0; k < doc.tables[j].columns.size(); k++) {
					int tmp = doc.tables[i].columns[j][k][0];
					int bottom = doc.words[tmp].box[3];
					int top = doc.words[tmp].box[1];
					int left = doc.words[tmp].box[0];
					tmp = doc.tables[i].columns[j][k][doc.tables[i].columns[j][k].size() - 1];
					int right = doc.words[tmp].box[2];
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

	void DrawingHandler::DrawGrid(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < doc.col_dims.size(); i++) {
			for (int j = 0; j < doc.row_dims[i].size(); j++) {
				int top = doc.row_dims[i][j][0];
				int bottom = doc.row_dims[i][j][1];
				for (int k = 0; k < doc.col_dims[i].size(); k++) {
					int left = doc.col_dims[i][k][0];
					int right = doc.col_dims[i][k][1];

					line(test, Point2i(left, top), Point2i(right, top), Scalar(0, 0, 0), 2);
					line(test, Point2i(right, top), Point2i(right, bottom), Scalar(0, 0, 0), 2);
					line(test, Point2i(right, bottom), Point2i(left, bottom), Scalar(0, 0, 0), 2);
					line(test, Point2i(left, bottom), Point2i(left, top), Scalar(0, 0, 0), 2);
				}
				for (int k = 0; k < doc.tables[i].multi_rows[j].size(); k++) {
					for (int h = 0; h < doc.lines[doc.tables[i].multi_rows[j][k]].segments.size(); h++) {
						int left = doc.words[doc.lines[doc.tables[i].multi_rows[j][k]].segments[h][0]].box[0];
						int right = doc.words[doc.lines[doc.tables[i].multi_rows[j][k]].segments[h][doc.lines[doc.tables[i].multi_rows[j][k]].segments[h].size() - 1]].box[2];
						int col_left = -1;
						int col_right = -1;

						for (int z = 0; z < doc.col_dims[i].size(); z++) {
							if ((left <= doc.col_dims[i][z][1]) && (col_left == -1)) {
								col_left = z;
							}
							if ((right <= doc.col_dims[i][z][1]) && (col_right == -1)) {
								col_right = z;
							}
						}
						if (col_left != col_right) {
							for (int z = col_left + 1; z <= col_right; z++) {
								int point = doc.col_dims[i][z][0];
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

	void DrawingHandler::DrawGridlessImage(cv::Mat& test) {
		namedWindow("img", cv::WINDOW_NORMAL);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);
		imshow("img", test);
		waitKey(0);
	}

	void DrawingHandler::DrawFootHead(cv::Mat& test, const Document& doc) {
		namedWindow("img", 0);
		float ratio = (float)(std::max(test.cols, test.rows)) / 850;
		resizeWindow("img", (test.cols) / ratio, (test.rows) / ratio);

		for (int i = 0; i < doc.lines.size(); i++) {
			if (doc.lines[i].type == 4) {
				line(test, Point2i(doc.page_left, doc.lines[i].dims[0]), Point2i(doc.page_right, doc.lines[i].dims[0]), Scalar(0, 0, 0), 2);
				line(test, Point2i(doc.page_right, doc.lines[i].dims[0]), Point2i(doc.page_right, doc.lines[i].dims[1]), Scalar(0, 0, 0), 2);
				line(test, Point2i(doc.page_right, doc.lines[i].dims[1]), Point2i(doc.page_left, doc.lines[i].dims[1]), Scalar(0, 0, 0), 2);
				line(test, Point2i(doc.page_left, doc.lines[i].dims[1]), Point2i(doc.page_left, doc.lines[i].dims[0]), Scalar(0, 0, 0), 2);
			}
		}
		imshow("img", test);
		char c = cvWaitKey(0);
	}
}