#pragma once
#include <iostream>
#include <fstream>
#include "ocr_tables/ocr_tabs_api.h"
#include "ocr_tables/helpers/auxiliary.h"
#include "ocr_tables/helpers/drawing_handler.h"
#include "debug.h"
extern "C" {
#include <mupdf/fitz.h>
}

namespace ocrt {
	OCRTABS_API OCRTabsAPI::OCRTabsAPI() {

	}

	OCRTABS_API OCRTabsAPI::OCRTabsAPI(const std::string& filename) {
		file_handler = FileHandler(filename);
	}
	
	OCRTABS_API OCRTabsAPI::~OCRTabsAPI() {

	}

	void OCRTabsAPI::OCRTabsEngineCall(Function function) {
		switch (function) {
		case Function::RemoveGridLines:
			tabs_engine.RemoveGridLines();
			break;
			
		case Function::FindBoxesAndWords:
			tabs_engine.FindBoxesAndWords();
			break;

		case Function::FindTextBoundaries:
			tabs_engine.FindTextBoundaries();
			break;

		case Function::CreateTextLines:
			tabs_engine.CreateTextLines();
			break;

		case Function::OCR_Recognize:
			tabs_engine.OCR_Recognize();
			break;

		case Function::RemoveHeadersFooters:
			tabs_engine.RemoveHeadersFooters();
			break;

		case Function::CreateLineSegments:
			tabs_engine.CreateLineSegments();
			break;

		case Function::AssignLineTypes:
			tabs_engine.AssignLineTypes();
			break;

		case Function::FindTableAreas:
			tabs_engine.FindTableAreas();
			break;

		case Function::AssignRowsToTables:
			tabs_engine.AssignRowsToTables();
			break;

		case Function::CreateTableColumns:
			tabs_engine.CreateTableColumns();
			break;

		case Function::CreateTableMultiRows:
			tabs_engine.CreateTableMultiRows();
			break;

		case Function::FindColumnSize:
			tabs_engine.FindColumnSize();
			break;

		case Function::FinalizeGrid:
			tabs_engine.FinalizeGrid();
			break;

		case Function::PrepareMulti1:
			tabs_engine.PrepareMulti1();
			break;

		case Function::PrepareMulti2:
			tabs_engine.PrepareMulti2();
			break;

		case Function::SegmentImage:
			//tabs_engine.SegmentImage();
			break;
		}
	}
	
	void OCRTabsAPI::Draw(DrawMode mode) {
		switch (mode) {
		case DrawMode::Boxes:
			drawing_handler.DrawBoxes(tabs_engine.test, tabs_engine.boxes, tabs_engine.words, tabs_engine.confs, tabs_engine.font_size, tabs_engine.bold, tabs_engine.italic, tabs_engine.underscore, tabs_engine.dict);
			break;

		case DrawMode::Lines:
			drawing_handler.DrawLines(tabs_engine.test, tabs_engine.lines, tabs_engine.page_left, tabs_engine.page_right, tabs_engine.page_top, tabs_engine.page_bottom, tabs_engine.line_dims);
			break;

		case DrawMode::Segments:
			drawing_handler.DrawSegments(tabs_engine.test, tabs_engine.lines, tabs_engine.line_segments, tabs_engine.line_dims, tabs_engine.boxes);
			break;

		case DrawMode::Areas:
			drawing_handler.DrawAreas(tabs_engine.test, tabs_engine.table_area, tabs_engine.line_dims, tabs_engine.page_left, tabs_engine.page_right);
			break;

		case DrawMode::Rows:
			drawing_handler.DrawRows(tabs_engine.test, tabs_engine.multi_rows, tabs_engine.line_dims, tabs_engine.page_left, tabs_engine.page_right);
			break;

		case DrawMode::ColsPartial:
			drawing_handler.DrawColsPartial(tabs_engine.test, tabs_engine.boxes, tabs_engine.tmp_col);
			break;

		case DrawMode::Cols:
			drawing_handler.DrawCols(tabs_engine.test, tabs_engine.boxes, tabs_engine.tmp_col, tabs_engine.table_columns);
			break;

		case DrawMode::Grid:
			drawing_handler.DrawGrid(tabs_engine.test, tabs_engine.boxes, tabs_engine.row_dims, tabs_engine.col_dims, tabs_engine.multi_rows, tabs_engine.line_segments);
			break;

		case DrawMode::GridlessImage:
			drawing_handler.DrawGridlessImage(tabs_engine.test);
			break;

		case DrawMode::FootHead:
			drawing_handler.DrawFootHead(tabs_engine.test, tabs_engine.lines_type, tabs_engine.lines, tabs_engine.line_dims, tabs_engine.page_left, tabs_engine.page_right);
			break;
		}
	}

	bool OCRTabsAPI::ExtractTables(const std::string& filename) {
		return tabs_engine.doc2html(file_handler.ReadFileType(filename), filename, "", false);
	}

	bool OCRTabsAPI::ExtractTables() {
		return ExtractTables(file_handler.GetFilename());
	}
}