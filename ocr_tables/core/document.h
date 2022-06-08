#pragma once
#include <vector>

namespace ocrt {
	enum LineType {
		TEXT = 1,
		TABLE = 2,
		UNKNOWN = 3
	};

	struct Word {
		char* name;
		bool bold;
		bool dict;
		bool italic;
		bool underscore;
		int font_size;
		float conf;
		std::vector<int> box;
	};

	struct Page {
		int height, width;
	};

	struct Table {
		std::vector<int> rows;
		std::vector<std::vector<std::vector<int>>> columns;
		std::vector<std::vector<int>> multi_rows;
	};

	struct Line {
		std::vector<int> word_ids;
		int* dims;
		std::vector<std::vector<int>> segments, segments_dims;
		ocrt::LineType type;
	};

	struct Document {
		std::vector<Word> words;
		std::vector<Line> lines;
		std::vector<Table> tables;
		std::vector<Page> pages;

		int page_left, page_right, page_top, page_bottom;

		std::vector<std::vector<int*>> col_dims, row_dims;
	};
}