#pragma once
#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define TESSDLL_IMPORTS
#include "tesseract/baseapi.h"

#define STD_DOTS_SIZE 3500
#define LOWER_DOTS_LIM 2800 
#define UPPER_DOTS_LIM 4200

#include "helpers/auxiliary.h"
#include "core/img_processor.h"

#define ocrtabs_find(list,element) std::find(list.begin(), list.end(), element) != list.end();

namespace ocrt {
	class OCRTabsEngine {
	friend class OCRTabsAPI;
	
	public:
		OCRTABS_API OCRTabsEngine();
		OCRTABS_API OCRTabsEngine(FileType filetype, const std::string& filename);
		OCRTABS_API ~OCRTabsEngine();

		void RemoveGridLines(float ratio = 1);
		void OCR_Recognize();
		void FindBoxesAndWords();
		void FindTextBoundaries();
		void CreateTextLines();
		void RemoveHeadersFooters();
		void CreateLineSegments();
		void AssignLineTypes();
		void FindTableAreas();
		void AssignRowsToTables();
		void CreateTableColumns();
		void CreateTableMultiRows();
		void FindColumnSize();
		void FinalizeGrid();
		void PrepareMulti1();
		void PrepareMulti2();

		bool ReadImage(std::vector<cv::Mat>&, std::vector<cv::Mat>&, FileType filetype, const std::string& filename, const std::string& filenameXML, bool withXML);
		void SetImage(cv::Mat img);
		void ResetImage();
		cv::Mat SegmentImage(cv::Mat img);
		cv::Mat PreprocessImage(cv::Mat img);
		bool PreprocessImageWithXML(const std::string& fileXML, std::vector<cv::Mat>& imageRAW, std::vector<cv::Mat>& imageCLN);

		bool OCRTABS_API doc2html(FileType filetype, const std::string& filename, const std::string& filenameXML, bool withXML);
		bool parsePDF(const std::string& filename, std::vector<cv::Mat>& imageList);
		void WriteHTML(std::string& filename);
		bool fail_condition();
		void resetAll();

		cv::Mat getInitial() { return initial; }
		cv::Mat getRaw() { return raw; }
		ocrt::Document getDoc() { return doc; }
	private:
		cv::Mat test, initial, raw;
		tesseract::TessBaseAPI tess;

		clock_t start;
		double duration;

		ocrt::Document doc;
		ImageProcessor img_processor;

		std::vector<std::vector<std::vector<int>>> tmp_col;
		std::vector<std::vector<int>> table_area;

		std::vector<std::vector<Word>> words_;
		std::vector<std::vector<Line>> lines_;

		bool fail;
		std::string fail_msg;

		void RemoveFigures();
		void ProcessGeneratedColumns();
		int FindMaxRightBoxInSegment(const std::vector<int>& seg);
		int FindMinLeftBoxInSegment(const std::vector<int>& seg);
		void InsertionSortBoxesInSegment(std::vector<int>& seg);
		template<typename T> bool Find(const std::vector<T>& list, const T& element);

		enum BoxSide {
			BOX_LEFT = 0,
			BOX_TOP = 1,
			BOX_RIGHT = 2,
			BOX_BOTTOM = 3
		};

		enum LineSide {
			LINE_TOP = 0,
			LINE_BOTTOM = 1
		};

		enum SegSide {
			SEG_LEFT = 0,
			SEG_RIGHT = 1
		};
	};
}