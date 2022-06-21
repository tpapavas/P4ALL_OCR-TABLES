#pragma once

#include <iostream>
#include <leptonica/allheaders.h>

#define OCR_DEBUG 1
#define OCR_PIX_DEBUG 0

//for general logging/debugging
#if OCR_DEBUG == 1
	#define OCR_LOG_ERROR(fail_msg) std::cerr << std::endl << "failCondition: " << fail_msg << std::endl
	#define OCR_LOG_MSG(msg) std::cout << msg
#else
	#define OCR_LOG_ERROR(fail_msg) std::cerr << std::endl << "failCondition: " << fail_msg << std::endl
	#define OCR_LOG_MSG(msg) 
#endif // OCR_DEBUG

//for pix display
#if OCR_DEBUG == 1 && OCR_PIX_DEBUG == 1
	#define OCR_LEPT_DEBUG 1
	#define SHOW_PIX(pixs, title) pixDisplayWithTitle(pixs, pixs->w, pixs->h, title, 1)
	#define PIX_CALL(func, pixs) pixs = func; \
				pixDisplayWithTitle(pixs, pixs->w, pixs->h, #pixs ## "=" ## #func, 1)
#else
	#define OCR_LEPT_DEBUG 0
	#define SHOW_PIX
	#define PIX_CALL(func, pixs) pixs = func
#endif