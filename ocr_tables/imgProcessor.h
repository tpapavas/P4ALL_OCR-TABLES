#pragma once 
#include "dll_config.h"

//STL
#include <stdio.h>
#include <iostream>
#include <numeric>

//OPENCV
//#include <cv.h> //DEPRECATED
//#include <highgui.h> //DEPRECATED
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>


//MUPDF
extern "C" {

	//#include <mupdf\fitz\pixmap.h>
	#include <mupdf\fitz.h>
}

//LEPTONICA
//#include "allheaders.h"	//not working
#include <leptonica\allheaders.h>
#define NO_CONSOLE_IO
#define DFLAG        0

////probably problematic////
using namespace std;
//using namespace cv;


#define uget(x,y)    at<unsigned char>(y,x)
#define uset(x,y,v)  at<unsigned char>(y,x)=v;
#define fget(x,y)    at<float>(y,x)
#define fset(x,y,v)  at<float>(y,x)=v;

namespace imgProcessor {
	enum NiblackVersion {
		NIBLACK=0,
		SAUVOLA,
		WOLFJOLION,
	};

	struct segmentationBlocks {
		cv::Mat text, figures, other, vert;

		void resize(cv::Size& siz) {
			if (siz.width == 0 || siz.height == 0) return;
			if (text.size() != siz) cv::resize(text, text, siz,0,0,4);
			if (figures.size() != siz) cv::resize(figures, figures, siz,0,0,4);
			if (other.size() != siz) cv::resize(other, other, siz,0,0,4);
			if (vert.size() != siz) cv::resize(vert, vert, siz,0,0,4);
		}

		void invertColors() {
			text = 255-text;
			figures=255-figures;
			other=255-other;
			vert=255-vert;
		}
	};

	// 'less' for contours
	struct contour_sorter {
		bool operator ()(const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
			cv::Rect ra(cv::boundingRect(a));
			cv::Rect rb(cv::boundingRect(b));

			if (ra.x <= rb.x && ra.y <= rb.y) return true;
			if (ra.y <= rb.y && ra.x <= (rb.x + rb.width)) return true;
			return false;
		}
	};	

	bool mat2pix (cv::Mat& mat, Pix** px);
	bool mat2pixBinary (cv::Mat& mat, Pix** px);
	bool pix2mat (Pix** px, cv::Mat& mat);
	bool pixmap2mat (fz_pixmap** fzpxmap, cv::Mat& mat);

	bool thresholdImg (cv::Mat& input, cv::Mat& output, double k = 0.2, double dR = 128);
	double calcLocalStats (cv::Mat &im, cv::Mat &map_m, cv::Mat &map_s, int winx, int winy);
	void NiblackSauvolaWolfJolion (cv::Mat im, cv::Mat output, NiblackVersion version, int winx, int winy, double k, double dR);
	
	l_int32 DoPageSegmentation(PIX *pixs, segmentationBlocks& blocks);

	void prepareAll(cv::Mat& input, cv::Mat& thres, segmentationBlocks& blocks);
	void prepareAll(fz_pixmap** fzpxmap, cv::Mat& thres, segmentationBlocks& blocks);
	void prepareAll(Pix** px, cv::Mat& thres, segmentationBlocks& blocks);

	void getTextImage(cv::Mat& input, segmentationBlocks& blk, cv::Mat& output);

	void reorderImage(cv::Mat& input, segmentationBlocks& blk, cv::Mat& output);

};