#pragma once
#include <Windows.h>
#include "ocr_tables/helpers/file_handler.h"

namespace ocrt {
	FileHandler::FileHandler() {

	}

	FileHandler::FileHandler(const std::string& filename) {
		this->filename = filename;
		//ResolveAbsolutePath();
		ReadFileType(filename);
	}
	
	FileHandler::~FileHandler() {

	}

	FileType FileHandler::ReadFileType(const std::string& filename) {
		if (filename.find(".pdf") != std::string::npos) {
			filetype = ocrt::PDF;
			return ocrt::PDF;
		} else {
			filetype = ocrt::IMG;
			return ocrt::IMG;
		}
	}

	// probably windows only
	void FileHandler::ResolveAbsolutePath() {
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);		//get .exe path "<path-to-project>\\<exe-folder>\\<program>.exe"
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");	//remove "<program>.exe" from path	
		std::string currentPath = std::string(buffer).substr(0, pos);
		pos = currentPath.find_last_of("\\/");
		currentPath = std::string(buffer).substr(0, pos);	//remove "<exe-folder>\\" from path; point to root of project "<path-to-project>".
		filepath = currentPath + "\\" + filename;	//get path to input file "<path-to-project>\\<relative-path-to-file>"
	}


}