#define _SERVICE

#ifndef _SERVICE
#include "ocrApp.h"
#include <QtWidgets/QApplication>
#endif
#include <iostream>

#ifdef _SERVICE //dont forget to change the subsystem
#include <Windows.h>
#include "ocr_tabs_api.h"
//#define OCR_DEBUG 1
#include "debug.h"

int main(int argc, char *argv[]) {
	if (argc>1) {
		std::string filename(argv[1]);
		if (filename.empty()) { 
			return 1;
		}

		ocrt::OCRTabsAPI tabs_api(filename);
		if (tabs_api.ExtractTables()) 
			return 0;
		else 
			return 1;
	}
	return 0;
}

#else

#ifdef _CONSOLE
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
#else 
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char*, int nShowCmd)
{
	int argc=0;
	QApplication a(argc,0);
#endif

	ocrApp w;
	//w.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	w.show();
	return a.exec();
}

#endif
