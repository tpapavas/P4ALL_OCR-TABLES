#include "drawingHandler.h"

#include <iostream>
#include <fstream>
#include "ocr_tabs.h"
#include "imgProcessor.h"
extern "C" {
#include <mupdf/fitz.h>
}


using namespace cv;
//using namespace std;

#pragma warning( disable : 4018 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )

void drawingHandler::DrawBoxes(cv::Mat test, std::vector<std::vector<int>> boxes, std::vector<char*> words, std::vector<float> confs, std::vector<int> font_size, std::vector<bool> bold, std::vector<bool> italic, std::vector<bool> underscore, std::vector<bool> dict) {
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
		
		if (bold[i]) { cout << "bold  "; }
		if (italic[i]) { cout << "italic  "; }
		if (underscore[i]) { cout << "underlined  "; }
		if (dict[i]) { cout << "in dictionary  "; }
		
		std::cout << boxes[i][0] << "|" << boxes[i][1] << "|" << boxes[i][2] << "|" << boxes[i][3];
		std::cout << "\n";
		
		imshow("img", test);
		char c = waitKey(0);
	}
}

void drawingHandler::DrawLines(cv::Mat test, std::vector<std::vector<int>> Lines, int page_left, int page_right, int page_top, int page_bottom, std::vector<int*> Line_dims) {
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