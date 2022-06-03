#pragma once

#include <iostream>
#include "ocr_tables/helpers/auxiliary.h"


namespace ocrt {
	class FileHandler {
	public:
		FileHandler();
		FileHandler(const std::string& filename);
		~FileHandler();

		FileType ReadFileType(const std::string& filename);

		std::string getFilename() { return filename; }
		FileType getFiletype() { return filetype; }
	private:
		std::string filename;  //relative path
		std::string filepath;  //absolute path
		FileType filetype;

		void ResolveAbsolutePath();
	};

}