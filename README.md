## P4ALL_OCR-TABLES (version 1.1)
A module that exports scanned documents (image or .pdf files) to .html, recognizing tabular structures. A description of the recognition algorithm can be found here: [Extraction of Tabular Data from Document Images](https://doi.org/10.1145/3058555.3058581)

See [CHANGES.md](docs/CHANGES.md) for changes on version 1.1.

### Contents
- [Repo](#repo)
- [Dependencies](#dependencies)
- [Building](#building)
- [DLL Usage](#dll-usage)
- [Standalone App usage](#standalone-app-usage)
- [WebService App usage](#webservice-app-usage)
- [Current limitations (to be updated in next version)](#current-limitations-to-be-updated-in-next-version)
- [Citation](#citation)
- [Funding Acknowledgement](#funding-acknowledgement)

### Repo
The repository contains 5 directories

1) [ocr_tables](P4ALL_OCR-TABLES/src) : This includes the source code that generates the OCR_TABLES.dll.

2) [App](App) : This includes the source code for a sample Qt-based app to test the module.

3) [tessdata](P4ALL_OCR-TABLES/tessdata) : This includes the traindata necessary for the OCR engine. The tessdata folder must be in the same directory as the executable.

4) [test files](P4ALL_OCR-TABLES/test%20files) : This includes some sample files to test the module.

5) [WebService](WebService) : This includes the .php files for a sample webservice implementation.

### Dependencies
The following libraries were used to build and test the module. Older subversions may also be compatible.

[OpenCV 4.5.5](http://opencv.org/) : Used by the ocr_tables module for image processing.\
opencv_core.lib, opencv_highgui.lib, opencv_imgproc.lib.

[MuPDF 1.19.0](http://mupdf.com/) : Used by the ocr_tables module for pdf processing.\
libmupdf.lib, libthirdparty.lib.\
Also set /NODEFAULTLIB:"libcmt.lib".

[Tesseract-OCR 4.1](https://github.com/tesseract-ocr/tesseract) : Used by the ocr_tables module for OCR.\
tesseract41.lib

[Leptonica 1.82.0](http://www.leptonica.com/) : Used by Tesseract-OCR for image processing, and for Document Image Analysis  
leptonica-1.82.0.lib

[Qt 5.15.2](http://www.qt.io/download-open-source/) : Used to build the sample App.\
qtmain.lib, Qt5Core.lib, Qt5Gui.lib, Qt5Widgets.lib.\
Also don't forget to copy Qt's "platforms" directory in the same folder as the executable.

See [INSTALL_DEPS.md](docs/INSTALL_DEPS.md) for installation instuctions on dependencies.

### Building
#### Windows 
For VisualStudio 2019, just run **CreateSolution.bat** to create Solution/Project files.

You can also run from repo directory
```
vendor\bin\premake\premake5.exe [action]
```
where [action] can get values for **Visual Studio** or **make** (use vs2019 for Visual Studio 2019). \
See full list in Premake Docs [Using Premake](https://premake.github.io/docs/Using-Premake).

### DLL Usage
```
//include header file
#include <iostream>
#include "ocr_tabs_api.h"

int main(int argc, char *argv[]) {
  std::string filename = "test files/001.png";

  // Extract tables and creates html file
  // with OCRTabsAPI object
  ocrt::OCRTabsAPI tabs_api(filename);
  tabs_api.ExtractTables();  

  // or with OCRTabsEngine object  
  ocrt::OCRTabsEngine tab = ocrt::OCRTabsEngine();
  tab.doc2html(ocrt::IMG, filename, "", false);
}
```

### Standalone App usage
Load an image or pdf file using the "LOAD" button. After the processing is finished an html file is create at "filename" + .html which can be opened using the "SHOW" button

### WebService App usage
Define "_SERVICE" to build the App in console mode, without GUI. The application takes as an argument a filename (image or .pdf) and produces the .html file. In this mode it can be called from a webservice

### Current limitations (to be updated in next version)
The module works best for single column horizontal text, for both single and multi-page documents.
Support for multi-column text and in-text images has been added in the updated version, however text/image segmentation may sometimes fail.
Non-manhattan document layouts and vertical text are not supported yet.

### Citation
Please cite the following paper in your publications if it helps your research:

    @inproceedings{vasileiadis2017extraction,
      author = {Vasileiadis, Manolis and Kaklanis, Nikolaos and Votis, Konstantinos and Tzovaras, Dimitrios},
      booktitle = {Proceedings of the 14th Web for All Conference on The Future of Accessible Work},
      title = {Extraction of Tabular Data from Document Images},
      pages={24},
      organization={ACM},
      year = {2017}
    }  

### Funding Acknowledgement
The research leading to these results has received funding from the European
Union's Seventh Framework Programme (FP7) under grant agreement No.610510
