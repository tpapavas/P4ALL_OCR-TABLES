#pragma once
#include <vector>

namespace ocrt {
	namespace aux {
		void startClock();
		double endClock();

		bool WriteHTML(std::string& filename, const std::vector<std::vector<std::vector<std::vector<int>>>>& table_columns, const std::vector<std::vector<std::vector<int>>>& multi_rows, const std::vector<std::vector<std::vector<int>>>& line_segments, const int* lines_type, const std::vector<char*>& words, const std::vector<int>& font_size);
	}

	enum FileType {
		IMG = 0,
		PDF = 1
	};

	enum LineType {
		TEXT = 1,
		TABLE = 2,
		UNKNOWN = 3
	};
}