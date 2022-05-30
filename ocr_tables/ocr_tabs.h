#pragma once
#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "auxiliary.h"

#define TESSDLL_IMPORTS
#define STD_DOTS_SIZE 3500
#define LOWER_DOTS_LIM 2800 
#define UPPER_DOTS_LIM 4200

#include "tesseract/baseapi.h"

#define ocrtabs_find(list,element) std::find(list.begin(), list.end(), element) != list.end();

//class ocr_tabs
namespace ocr_tabs {
	class OCRTabsEngine {
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

	private:
		cv::Mat test, initial, raw;
		tesseract::TessBaseAPI  tess;
		
		clock_t start;
		double duration;

		std::vector<char*> words;
		std::vector<std::vector<int>> boxes, lines, table_area, table_rows;
		std::vector<std::vector<std::vector<int>>> multi_rows;
		std::vector<int*> line_dims;
		std::vector<std::vector<std::vector<int>>> line_segments;
		std::vector<std::vector<std::vector<int>>> line_segments_dims;
		std::vector<std::vector<std::vector<std::vector<int>>>> table_columns;
		std::vector<std::vector<int*>> col_dims, row_dims;
		std::vector<std::vector<std::vector<int>>> tmp_col;
		std::vector<float> confs;
		std::vector<bool> bold;
		std::vector<bool> dict;
		std::vector<bool> italic;
		std::vector<bool> underscore;
		std::vector<int> font_size;
		int page_left, page_right, page_top, page_bottom;
		int* lines_type;

		std::vector<std::vector<char*>> words_;
		std::vector<std::vector<std::vector<int>>> lines_;
		std::vector<std::vector<std::vector<int>>> boxes_;
		std::vector<std::vector<float>> confs_;
		std::vector<std::vector<int>> font_size_;
		std::vector<std::vector<bool>> bold_;
		std::vector<std::vector<bool>> dict_;
		std::vector<std::vector<bool>> italic_;
		std::vector<std::vector<bool>> underscore_;
		std::vector<int> page_height, page_width;
		
		bool fail;
		std::string fail_msg;

		void RemoveFigures();
		void ProcessGeneratedColumns();
		int FindMaxRightBoxInSegment(const std::vector<int>& seg);
		int FindMinLeftBoxInSegment(const std::vector<int>& seg);
		void InsertionSortBoxesInSegment(std::vector<int>& seg);
		template<typename T> bool Find(const std::vector<T>& list, const T& element);

		enum BoxSide {
			BOX_LEFT=0,
			BOX_TOP=1,
			BOX_RIGHT=2,
			BOX_BOTTOM=3
		};

		enum LineSide {
			LINE_TOP=0,
			LINE_BOTTOM=1
		};

		enum SegSide {
			SEG_LEFT=0,
			SEG_RIGHT=1
		};
	};
}
