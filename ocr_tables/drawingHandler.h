#pragma once

#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <ctime>



#define TESSDLL_IMPORTS

#include "tesseract/baseapi.h"

class drawingHandler {
	public:
		OCRTABS_API drawingHandler();
		OCRTABS_API ~drawingHandler();

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
};

