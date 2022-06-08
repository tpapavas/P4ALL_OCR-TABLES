#pragma once
#include <vector>
#include "ocr_tables/core/document.h"

namespace ocrt {
	namespace aux {
		void startClock();
		double endClock();

		bool WriteHTML(std::string& filename, const ocrt::Document doc);
	}

	enum FileType {
		IMG = 0,
		PDF = 1
	};
}