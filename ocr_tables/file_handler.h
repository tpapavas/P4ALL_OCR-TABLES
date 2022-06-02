#pragma once

#include <iostream>
#include "auxiliary.h"


namespace ocrt {
	class FileHandler {
	public:
		FileHandler();
		FileHandler(const std::string& filename);
		~FileHandler();

		FileType ReadFileType(const std::string& filename);

		std::string GetFilename() { return filename; }
		FileType GetFiletype() { return filetype; }
	private:
		std::string filename;
		std::string filepath;
		FileType filetype;

		void CreateFilepath();
	};

}