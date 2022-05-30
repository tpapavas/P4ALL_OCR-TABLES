#pragma once

#include <leptonica\allheaders.h>

#define OCR_DEBUG 1
#if OCR_DEBUG == 1
    #define LEPT_DEBUG 1
	#define SHOW_PIX(pixs, title) pixDisplayWithTitle(pixs, pixs->w, pixs->h, title, 1)
	#define PIX_CALL(func, pixs) pixs = func; \
		pixDisplayWithTitle(pixs, pixs->w, pixs->h, #pixs ## "=" ## #func, 1)
#else
	#define LEPT_DEBUG 0
	#define SHOW_PIX
	#define PIX_CALL(func, pixs) pixs = func
#endif // OCR_DEBUG
