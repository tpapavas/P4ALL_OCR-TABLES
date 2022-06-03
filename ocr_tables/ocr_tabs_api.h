#pragma once
#include "ocr_tables/dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "ocr_tables/helpers/auxiliary.h"
#include "ocr_tables/helpers/drawing_handler.h"
#include "ocr_tables/helpers/file_handler.h"
#include "ocr_tables/core/ocr_tabs_engine.h"
#include "ocr_tables/core/img_processor.h"

#define STD_DOTS_SIZE 3500
#define LOWER_DOTS_LIM 2800 
#define UPPER_DOTS_LIM 4200


#define ocrtabs_find(list,element) std::find(list.begin(), list.end(), element) != list.end();

namespace ocrt {
	enum Function {
		RemoveGridLines,
		FindBoxesAndWords,
		FindTextBoundaries,
		CreateTextLines,
		OCR_Recognize,
		RemoveHeadersFooters,
		CreateLineSegments,
		AssignLineTypes,
		FindTableAreas,
		AssignRowsToTables,
		CreateTableColumns,
		CreateTableMultiRows,
		FindColumnSize,
		FinalizeGrid,
		PrepareMulti1,
		PrepareMulti2,
		SegmentImage
	};

	enum DrawMode {
		Boxes, 
		Lines, 
		Segments, 
		Areas, 
		Rows, 
		ColsPartial, 
		Cols, 
		Grid, 
		GridlessImage,
		FootHead
	};

	class OCRTabsAPI {
	public:
		OCRTABS_API OCRTabsAPI();
		OCRTABS_API OCRTabsAPI(const std::string& filename);
		OCRTABS_API ~OCRTabsAPI();
		
		void OCRTabsEngineCall(Function function);
		void Draw(DrawMode mode);

		void SetImage(cv::Mat img);

		bool OCRTABS_API ExtractTables(const std::string& filename);
		bool OCRTABS_API ExtractTables();

	private:
		OCRTabsEngine tabs_engine;
		DrawingHandler drawing_handler;
		FileHandler file_handler;

		bool fail_condition();
	};
}
