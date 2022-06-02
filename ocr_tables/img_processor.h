#pragma once 
#include "dll_config.h"

//STL
#include <stdio.h>
#include <iostream>
#include <numeric>

//OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

//MUPDF
extern "C" {
	//#include <mupdf\fitz\pixmap.h>
	#include <mupdf\fitz.h>
}

//LEPTONICA
#include <leptonica\allheaders.h>
#define NO_CONSOLE_IO
#define DFLAG 0

////probably problematic////
using namespace std;
//using namespace cv;

#define uget(x,y)    at<unsigned char>(y,x)
#define uset(x,y,v)  at<unsigned char>(y,x)=v
#define fget(x,y)    at<float>(y,x)
#define fset(x,y,v)  at<float>(y,x)=v
#define dget(x,y)    at<double>(y,x)
#define dset(x,y,v)  at<double>(y,x)=v

namespace ocrt {
	enum BinarizationType {
		NIBLACK=0,
		SAUVOLA,
		WOLFJOLION,
		BATAINEH
	};

	struct SegmentationBlocks {
		cv::Mat text, figures, other, vert;

		void Resize(cv::Size& siz) {
			if (siz.width == 0 || siz.height == 0) return;
			if (text.size() != siz) cv::resize(text, text, siz, 0, 0, 4);
			if (figures.size() != siz) cv::resize(figures, figures, siz, 0, 0, 4);
			if (other.size() != siz) cv::resize(other, other, siz, 0, 0, 4);
			if (vert.size() != siz) cv::resize(vert, vert, siz, 0, 0, 4);
		}

		void InvertColors() {
			text = 255 - text;
			figures = 255 - figures;
			other = 255 - other;
			vert = 255 - vert;
		}
	};

	// 'less' for contours
	struct ContourSorter {
		bool operator ()(const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
			cv::Rect ra(cv::boundingRect(a));
			cv::Rect rb(cv::boundingRect(b));

			if (ra.x <= rb.x && ra.y <= rb.y) return true;
			if (ra.y <= rb.y && ra.x <= (rb.x + rb.width)) return true;
			return false;
		}
	};
	
	class ImageProcessor {
	public:
		ImageProcessor();
		ImageProcessor(cv::Mat img);
		~ImageProcessor();

		bool ThresholdImage(cv::Mat& input, cv::Mat& output, BinarizationType type = SAUVOLA, int winx = 20, int winy = 20, double k = 0.2, double dR = 128);
		bool ThresholdImage(cv::Mat& output, BinarizationType type = SAUVOLA, int winx = 20, int winy = 20, double k = 0.2, double dR = 128);
		
		l_int32 DoPageSegmentation(PIX* pixs, SegmentationBlocks& blocks);
		void ExtractTextImage(cv::Mat& input, SegmentationBlocks& blk, cv::Mat& output);
		void ReorderImage(cv::Mat& input, SegmentationBlocks& blk, cv::Mat& output);

		void PrepareAll(cv::Mat& input, cv::Mat& thres, SegmentationBlocks& blocks);
		void PrepareAll(fz_pixmap** fzpxmap, cv::Mat& thres, SegmentationBlocks& blocks);
		void PrepareAll(Pix** px, cv::Mat& thres, SegmentationBlocks& blocks);

		bool mat2pix(cv::Mat& mat, Pix** px);
		bool mat2pixBinary(cv::Mat& mat, Pix** px);
		bool pix2mat(Pix** px, cv::Mat& mat);
		bool pixmap2mat(fz_pixmap** fzpxmap, cv::Mat& mat);

		bool ConstructImage(cv::Mat& image, cv::Mat& filter, int ker_len);
		bool ClearImage(cv::Mat& image, cv::Mat& output);

	private:
		BinarizationType bin_type;
		int winx, winy, k, dR;
		cv::Mat img;
		cv::Mat bin_img;

		double CalcLocalStats(cv::Mat& im, cv::Mat& map_m, cv::Mat& map_s, int winx, int winy, double& mean, double& max_s, double& min_s);
		void ApplyThreshold(cv::Mat im, cv::Mat output, BinarizationType type, int winx, int winy, double k, double dR);
	};
};