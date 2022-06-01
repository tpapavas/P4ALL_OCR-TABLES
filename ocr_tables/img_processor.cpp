#pragma once 
#include "img_processor.h"
#include "drawing_handler.h"
#include "debug.h"

/**
 * @brief Outputs a binary image.
 * @param input: input image
 * @param output: binary output image
 * @param k: constant value in range [0.2, 0.5]. Default 0.2.
 * @param dR: dynamic range of standard deviation. Default 128.
 * @return true if everything goes normal.
 */
bool img_processor::ThresholdImage (cv::Mat& input, cv::Mat& output, BinarizationType type, int winx, int winy, double k, double dR) {
	if ((input.rows <= 0) || (input.cols <= 0)) {
		cerr << "*** ERROR: Invalid input Image " << endl;
		return false;
	}
    
	int win_len = (int)(2.0 * input.rows - 1) / 3;
	win_len = std::min(win_len, input.cols - 1);

    // Threshold
	output = cv::Mat(input.rows, input.cols, CV_8U);
	//ApplyThreshold(input, output, type, win_len, win_len, k, dR);

	ApplyThreshold(input, output, type, winx, winy, k, dR);
	//ApplyThreshold(input, output, BATAINEH, 20, 20, 0.2, dR);
	output = 255 * output;
	return true;
}

/**
 * @brief Calculate stats for local neighborhood
 * @param im: input image
 * @param map_m: mean value of image's subwindows
 * @param map_s: standard deviation of image's subwindows
 * @param winx: window width
 * @param winy: window height
 * @param mean: image mean value
 * @param max_s: max sd value
 * @param min_s: min sd value
 * @return 
 */
double img_processor::CalcLocalStats(cv::Mat& im, cv::Mat& map_m, cv::Mat& map_s, int winx, int winy, double& mean, double& max_s, double& min_s) {
	cv::Mat im_sum, im_sum_sq;
	double m, s, sum, sum_sq; 
	int wxh = winx / 2;
	int wyh = winy / 2;
	int x_firstth = wxh;
	int y_lastth = im.rows - wyh - 1;
	int y_firstth = wyh;
	double winarea = winx * winy;
	int num_of_windows = 0;
	
	cv::integral(im, im_sum, im_sum_sq, CV_64F);
	mean = 0;
	max_s = 0;
	for	(int j = y_firstth ; j<=y_lastth; j++) {   
		sum = sum_sq = 0;

		sum = im_sum.dget(winx, j - wyh + winy) - im_sum.dget(winx, j - wyh) - im_sum.dget(0, j - wyh + winy) + im_sum.dget(0, j - wyh);
		sum_sq = im_sum_sq.dget(winx, j - wyh + winy) - im_sum_sq.dget(winx, j - wyh) - im_sum_sq.dget(0, j - wyh + winy) + im_sum_sq.dget(0, j - wyh);

		m  = sum / winarea;
		s = sqrt((sum_sq - m * sum) / winarea);
		if (s > max_s) max_s = s;

		mean += m;
		num_of_windows++;

		//cout << "sum: " << s << " | sum_sq: " << sum_sq << " | m: " << m << " | sd: " << s << endl;
		//cout << im_sum.at<double>(j - wyh + winy, winx) << " | " << im_sum_sq.at<double>(j - wyh + winy, winx) << endl;
		//cin.get();

		map_m.fset(x_firstth, j, m);
		map_s.fset(x_firstth, j, s);

		// Shift the window, add and remove	new/old values to the histogram
		for (int i = 1; i <= im.cols - winx; i++) {
			// Remove the left old column and add the right new column
			sum -= im_sum.dget(i, j - wyh + winy) - im_sum.dget(i, j - wyh) - im_sum.dget(i - 1, j - wyh + winy) + im_sum.dget(i - 1, j - wyh);
			sum += im_sum.dget(i + winx, j - wyh + winy) - im_sum.dget(i + winx, j - wyh) - im_sum.dget(i + winx - 1, j - wyh + winy) + im_sum.dget(i + winx - 1, j - wyh);

			// Remove the left old column and add the right new column
			sum_sq -= im_sum_sq.dget(i, j - wyh + winy) - im_sum_sq.dget(i, j - wyh) - im_sum_sq.dget(i - 1, j - wyh + winy) + im_sum_sq.dget(i - 1, j - wyh);
			sum_sq += im_sum_sq.dget(i + winx, j - wyh + winy) - im_sum_sq.dget(i + winx, j - wyh) - im_sum_sq.dget(i + winx - 1, j - wyh + winy) + im_sum_sq.dget(i + winx - 1, j - wyh);

			m = sum / winarea;
			s = sqrt((sum_sq - m * sum) / winarea);
			if (s > max_s) max_s = s;

			mean += m;
			num_of_windows++;

			map_m.fset(i + wxh, j, m);
			map_s.fset(i + wxh, j, s);
		}
	}

	mean = mean / num_of_windows;

	return max_s;
}

/**
 * @brief Creates binary image based on a threshold
 * @param im: input image
 * @param output: output binary image
 * @param type: type of Threshold (NIBLACK, SAUVOLA, WOLFJOLION)
 * @param winx: window width
 * @param winy: window height
 * @param k: constant factor in range [0.2, 0.5] 
 * @param dR: dynamic range of standard deviation
*/
void img_processor::ApplyThreshold(cv::Mat im, cv::Mat output, BinarizationType type, int winx, int winy, double k, double dR) {
	double m, s, max_s, min_s;
	double th = 0;
	double min_I, max_I;
	int wxh = winx / 2;
	int wyh = winy / 2;
	int x_firstth = wxh;
	int x_lastth = im.cols - wxh - 1;
	int y_lastth = im.rows - wyh - 1;
	int y_firstth = wyh;
	//int mx, my;
	double mean, s_adaptive;

	// Create local statistics and store them in a double matrices
	cv::Mat map_m = cv::Mat::zeros(im.rows, im.cols, CV_32F);
	cv::Mat map_s = cv::Mat::zeros(im.rows, im.cols, CV_32F);
	max_s = CalcLocalStats(im, map_m, map_s, winx, winy, mean, max_s, min_s);

	minMaxLoc(im, &min_I, &max_I);

	cv::Mat thsurf(im.rows, im.cols, CV_32F);
			
	// Create the threshold surface, including border processing
	// ----------------------------------------------------
	for (int j = y_firstth; j <= y_lastth; j++) {
		// NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
		for (int i = 0; i <= im.cols - winx; i++) {
			m = map_m.fget(i + wxh, j);
			s = map_s.fget(i + wxh, j);

    		// Calculate the threshold
			switch (type) {
			case NIBLACK:
				th = m + k * s;
				break;

			case SAUVOLA:
				th = m * (1 + k * (s / dR - 1));
				break;

			case WOLFJOLION:
				th = m + k * (s / max_s - 1) * (m - min_I);
				break;

			case BATAINEH:
				s_adaptive = (s - min_s) / (max_s - min_s);
				th = m - (m * m - s) / ((mean + s) * (s_adaptive + s));
				break;

			default:
				cerr << "Unknown threshold type in img_processor::ApplyThreshold()\n";
				exit(1);
			}
    		
			thsurf.fset(i + wxh, j, th);

			if (i == 0) {
        		// LEFT BORDER
				for (int i = 0; i <= x_firstth; ++i)
					thsurf.fset(i, j, th);
        		// LEFT-UPPER CORNER
				if (j == y_firstth)
					for (int u = 0; u < y_firstth; ++u)
						for (int i = 0; i <= x_firstth; ++i)
							thsurf.fset(i, u, th);
        		// LEFT-LOWER CORNER
				if (j == y_lastth)
					for (int u = y_lastth + 1; u < im.rows; ++u)
						for (int i = 0; i <= x_firstth; ++i)
							thsurf.fset(i, u, th);
    		}

			// UPPER BORDER
			if (j == y_firstth)
				for (int u = 0; u < y_firstth; ++u)
					thsurf.fset(i + wxh, u, th);
			// LOWER BORDER
			if (j == y_lastth)
				for (int u = y_lastth + 1; u < im.rows; ++u)
					thsurf.fset(i + wxh, u, th);
		}

		// RIGHT BORDER
		for (int i = x_lastth; i < im.cols; ++i)
			thsurf.fset(i, j, th);
  		// RIGHT-UPPER CORNER
		if (j == y_firstth)
			for (int u = 0; u < y_firstth; ++u)
				for (int i = x_lastth; i < im.cols; ++i)
					thsurf.fset(i, u, th);
		// RIGHT-LOWER CORNER
		if (j == y_lastth)
			for (int u = y_lastth + 1; u < im.rows; ++u)
				for (int i = x_lastth; i < im.cols; ++i)
					thsurf.fset(i, u, th);
	}
	//cerr << "surface created" << endl;
	
	//apply threshold to each pixel
	for (int y = 0; y < im.rows; ++y)
		for (int x = 0; x < im.cols; ++x)
			if (im.uget(x, y) >= thsurf.fget(x, y))
				output.uset(x, y, 255);
			else
				output.uset(x, y, 0);
	
}

/**
 * @brief Binarize and segment image
 * @param input: the image to be processed
 * @param thres: thresholded output image
 * @param blocks: segmentation blocks
 */
void img_processor::PrepareAll(cv::Mat& input, cv::Mat& output, SegmentationBlocks& blocks) {
	OCR_LOG_MSG("Thresholding Image...");
	
	cv::Mat threshed_input, temp, closed;
	//cv::erode(input, temp, cv::Mat(), cv::Point(-1, -1), 1);
	//img_processor::ThresholdImage(temp, output, BinarizationType::BATAINEH);  //get binary image
	ClearImage(input, output);
	ocr_tabs::drawing_handler::DrawGridlessImage(output);

	//cv::Mat se = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 2));
	//cv::morphologyEx(output, closed, cv::MORPH_CLOSE, se, cv::Point(-1, -1), 2);  //new operation #1
	//img_processor::ConstructImage(output, closed, 3);  //new operation #2
	//ocr_tabs::drawing_handler::DrawGridlessImage(output);

	OCR_LOG_MSG("Done!\n");
	OCR_LOG_MSG("Segmenting Page...");

	Pix* px = NULL;
	cv::erode(output, threshed_input, cv::Mat(), cv::Point(-1, -1), 3);
	img_processor::mat2pixBinary(threshed_input, &px);
	img_processor::DoPageSegmentation(px, blocks);

	OCR_LOG_MSG("Done!\n");
	pixDestroy(&px);

	//img_processor::ThresholdImage(input, output);
}

void img_processor::PrepareAll(fz_pixmap** fzpxmap, cv::Mat& thres, SegmentationBlocks& blocks) {
	cv::Mat input;
	pixmap2mat(fzpxmap, input);
	PrepareAll(input, thres, blocks);
}

void img_processor::PrepareAll(Pix** px, cv::Mat& thres, SegmentationBlocks& blocks) {
	cv::Mat input;
	pix2mat(px, input);
	PrepareAll(input, thres, blocks);
}

/**
 * @brief Segment input pix and extract segmentation blocks
 * @param pixs: input pix image
 * @param blocks: output segmentation blocks
 * @return 0 if everything goes normal.
 */
l_int32  img_processor::DoPageSegmentation(PIX* pixs, SegmentationBlocks& blocks) {
	l_int32      zero;
	BOXA* boxatm, * boxahm;
	PIX* pixr;   /* image reduced to 150 ppi */
	PIX* pixhs;  /* image of halftone seed, 150 ppi */
	PIX* pixm;   /* image of mask of components, 150 ppi */
	PIX* pixhm1; /* image of halftone mask, 150 ppi */
	PIX* pixhm2; /* image of halftone mask, 300 ppi */
	PIX* pixht;  /* image of halftone components, 150 ppi */
	PIX* pixnht; /* image without halftone components, 150 ppi */
	PIX* pixi;   /* inverted image, 150 ppi */
	PIX* pixvws; /* image of vertical whitespace, 150 ppi */
	PIX* pixtm1; /* image of closed textlines, 150 ppi */
	PIX* pixtm2; /* image of refined text line mask, 150 ppi */
	PIX* pixtm3; /* image of refined text line mask, 300 ppi */
	PIX* pixtb1; /* image of text block mask, 150 ppi */
	PIX* pixtb2; /* image of text block mask, 300 ppi */
	PIX* pixnon; /* image of non-text or halftone, 150 ppi */
	PIX* pixt1, * pixt2, * pixt3;
	PIXCMAP* cmap;
	PTAA* ptaa;
	l_int32      ht_flag = 0;
	l_int32      ws_flag = 0;
	l_int32      text_flag = 0;
	l_int32      block_flag = 0;

	setLeptDebugOK(OCR_LEPT_DEBUG);

	cv::Size defSize(pixs->w, pixs->h);
	//pixr = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);
	PIX_CALL(pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0), pixr);

	PIX_CALL(pixs, pixs);
	PIX_CALL(pixr, pixr);

	/* Get seed for halftone parts */
	//pixt1 = pixReduceRankBinaryCascade(pixr, 4, 4, 3, 0);
	PIX_CALL(pixReduceRankBinaryCascade(pixr, 4, 4, 3, 0), pixt1);

	//pixt2 = pixOpenBrick(NULL, pixt1, 5, 5);
	PIX_CALL(pixOpenBrick(NULL, pixt1, 5, 5), pixt2);
	//pixhs = pixExpandBinaryPower2(pixt2, 8);
	PIX_CALL(pixExpandBinaryPower2(pixt2, 8), pixhs);

	pixDestroy(&pixt1);
	pixDestroy(&pixt2);

	/* Get mask for connected regions */
	//pixm = pixCloseSafeBrick(NULL, pixr, 4, 4);
	PIX_CALL(pixCloseSafeBrick(NULL, pixr, 4, 4), pixm);
	
	/* Fill seed into mask to get halftone mask */
	//pixhm1 = pixSeedfillBinary(NULL, pixhs, pixm, 4);
	PIX_CALL(pixSeedfillBinary(NULL, pixhs, pixm, 4), pixhm1);
	
	//pixhm2 = pixExpandBinaryPower2(pixhm1, 2);
	PIX_CALL(pixExpandBinaryPower2(pixhm1, 2), pixhm2);
	
	/* Extract halftone stuff */
	//pixht = pixAnd(NULL, pixhm1, pixr);
	PIX_CALL(pixAnd(NULL, pixhm1, pixr), pixht);
	
	/* Extract non-halftone stuff */
	//pixnht = pixXor(NULL, pixht, pixr);
	PIX_CALL(pixXor(NULL, pixht, pixr), pixnht);
	
	pixZero(pixht, &zero);
	//pixDisplayWithTitle(pixht, pixht->w, pixht->h, "ht. zero", 1);

	/* Get bit-inverted image */
	//pixi = pixInvert(NULL, pixnht);
	PIX_CALL(pixInvert(NULL, pixnht), pixi);
	
	/* The whitespace mask will break textlines where there
	 * is a large amount of white space below or above.
	 * We can prevent this by identifying regions of the
	 * inverted image that have large horizontal (bigger than
	 * the separation between columns) and significant
	 * vertical extent (bigger than the separation between
	 * textlines), and subtracting this from the whitespace mask. */
	//pixt1 = pixMorphCompSequence(pixi, "o80.60", 0);
	PIX_CALL(pixMorphCompSequence(pixi, "o80.60", 0), pixt1);

	//pixt2 = pixSubtract(NULL, pixi, pixt1);
	PIX_CALL(pixSubtract(NULL, pixi, pixt1), pixt2);
	pixDestroy(&pixt1);

	/* Identify vertical whitespace by opening inverted image */
	//pixt3 = pixOpenBrick(NULL, pixt2, 5, 1);  /* removes thin vertical lines */
	PIX_CALL(pixOpenBrick(NULL, pixt2, 5, 1), pixt3);

	//pixvws = pixOpenBrick(NULL, pixt3, 1, 200);  /* gets long vertical lines */
	PIX_CALL(pixOpenBrick(NULL, pixt3, 1, 200), pixvws);

	pix2mat(&pixvws, blocks.vert);
	pixDestroy(&pixt2);
	pixDestroy(&pixt3);

	/* Get proto (early processed) text line mask. */
	/* First close the characters and words in the textlines */
	//pixtm1 = pixCloseSafeBrick(NULL, pixnht, 30, 1);
	PIX_CALL(pixCloseSafeBrick(NULL, pixnht, 30, 1), pixtm1);

	/* Next open back up the vertical whitespace corridors */
	//pixtm2 = pixSubtract(NULL, pixtm1, pixvws);
	PIX_CALL(pixSubtract(NULL, pixtm1, pixvws), pixtm2);
	
	/* Do a small opening to remove noise */
	pixOpenBrick(pixtm2, pixtm2, 3, 3);
	//pixDisplayWithTitle(pixtm2, pixtm2->w, pixtm2->h, "tm2. open brick", 1);

	//pixtm3 = pixExpandBinaryPower2(pixtm2, 2);
	PIX_CALL(pixExpandBinaryPower2(pixtm2, 2), pixtm3);

	/* Join pixels vertically to make text block mask */
	//pixtb1 = pixMorphSequence(pixtm2, "c1.10 + o4.1", 0);
	PIX_CALL(pixMorphSequence(pixtm2, "c1.10 + o4.1", 0), pixtb1);

	/* Solidify the textblock mask and remove noise:
	 *  (1) For each c.c., close the blocks and dilate slightly
	 *      to form a solid mask.
	 *  (2) Small horizontal closing between components
	 *  (3) Open the white space between columns, again
	 *  (4) Remove small components */
	//pixt1 = pixMorphSequenceByComponent(pixtb1, "c30.30 + d3.3", 8, 0, 0, NULL);
	PIX_CALL(pixMorphSequenceByComponent(pixtb1, "c30.30 + d3.3", 8, 0, 0, NULL), pixt1);

	pixCloseSafeBrick(pixt1, pixt1, 10, 1);
	//pixDisplayWithTitle(pixt1, pixt1->w, pixt1->h, "t1. close brick", 1);

	//pixt2 = pixSubtract(NULL, pixt1, pixvws);
	PIX_CALL(pixSubtract(NULL, pixt1, pixvws), pixt2);

	pixt3 = pixSelectBySize(pixt2, 25, 5, 8, L_SELECT_IF_BOTH,
		L_SELECT_IF_GTE, NULL);
	PIX_CALL(pixSelectBySize(pixt2, 25, 5, 8, L_SELECT_IF_BOTH, L_SELECT_IF_GTE, NULL), pixt3);

	pix2mat(&pixt3, blocks.text);
	//pixtb2 = pixExpandBinaryPower2(pixt3, 2);
	PIX_CALL(pixExpandBinaryPower2(pixt3, 2), pixtb2);

	pixDestroy(&pixt1);
	pixDestroy(&pixt2);
	pixDestroy(&pixt3);

	/* Identify the outlines of each textblock */
	ptaa = pixGetOuterBordersPtaa(pixtb2);
	//pixt1 = pixRenderRandomCmapPtaa(pixtb2, ptaa, 1, 8, 1);
	PIX_CALL(pixRenderRandomCmapPtaa(pixtb2, ptaa, 1, 8, 1), pixt1);

	cmap = pixGetColormap(pixt1);
	pixcmapResetColor(cmap, 0, 130, 130, 130);  /* set interior to gray */

	ptaaDestroy(&ptaa);
	pixDestroy(&pixt1);

	/* Fill line mask (as seed) into the original */
	//pixt1 = pixSeedfillBinary(NULL, pixtm3, pixs, 8);
	PIX_CALL(pixSeedfillBinary(NULL, pixtm3, pixs, 8), pixt1);
	
	pixOr(pixtm3, pixtm3, pixt1);
	//pixDisplayWithTitle(pixtm3, pixtm3->w, pixtm3->h, "tm3. tm3 OR t1", 1);

	pixDestroy(&pixt1);

	/* Fill halftone mask (as seed) into the original */
	//pixt1 = pixSeedfillBinary(NULL, pixhm2, pixs, 8);
	PIX_CALL(pixSeedfillBinary(NULL, pixhm2, pixs, 8), pixt1);

	pixOr(pixhm2, pixhm2, pixt1);
	//pixDisplayWithTitle(pixhm2, pixhm2->w, pixhm2->h, "hm2. hm2 OR t1", 1);

	pixDestroy(&pixt1);
	pix2mat(&pixhm2, blocks.figures);
	//pixDisplayWithTitle(pixhm2, pixhm2->w, pixhm2->h, "hm2", 1);

	/* Find objects that are neither text nor halftones */
	//pixt1 = pixSubtract(NULL, pixs, pixtm3);  /* remove text pixels */
	PIX_CALL(pixSubtract(NULL, pixs, pixtm3), pixt1);

	//pixnon = pixSubtract(NULL, pixt1, pixhm2);  /* remove halftone pixels */
	PIX_CALL(pixSubtract(NULL, pixt1, pixhm2), pixnon);

	pixDestroy(&pixt1);
	pix2mat(&pixnon, blocks.other);

	/* Write out b.b. for text line mask and halftone mask components */
	boxatm = pixConnComp(pixtm3, NULL, 4);
	boxahm = pixConnComp(pixhm2, NULL, 8);

	//pixDestroy(&pixt1);
	//pixaDestroy(&pixa);

	/* clean up to test with valgrind */
	pixDestroy(&pixr);
	pixDestroy(&pixhs);
	pixDestroy(&pixm);
	pixDestroy(&pixhm1);
	pixDestroy(&pixhm2);
	pixDestroy(&pixht);
	pixDestroy(&pixnht);
	pixDestroy(&pixi);
	pixDestroy(&pixvws);
	pixDestroy(&pixtm1);
	pixDestroy(&pixtm2);
	pixDestroy(&pixtm3);
	pixDestroy(&pixtb1);
	pixDestroy(&pixtb2);
	pixDestroy(&pixnon);
	boxaDestroy(&boxatm);
	boxaDestroy(&boxahm);

	blocks.Resize(defSize);
	//blocks.Resize(blocks.text.size());
	blocks.InvertColors();

	return 0;
}

/**
 * @brief Extracts image that contains only text areas, based on input blocks.
 * It basically clears non text areas
 * @param input: input image
 * @param blk: segmentation blocks of input image 
 * @param output: output text image 
 */
void img_processor::ExtractTextImage(cv::Mat& input, SegmentationBlocks& blk, cv::Mat& output) {
	blk.text.setTo(0, blk.figures == 255);
	blk.text.setTo(0, blk.vert == 255);
	cv::dilate(blk.text, blk.text, cv::Mat(), cv::Point(-1, -1), 5);
	cv::dilate(blk.figures, blk.figures, cv::Mat(), cv::Point(-1, -1), 10);
	output = input.clone();
	output.setTo(255, blk.figures == 255);
	output.setTo(255, blk.text == 0);
	//output.setTo(255, blk.other==255);
}

void img_processor::ReorderImage(cv::Mat& input, SegmentationBlocks& blk, cv::Mat& output) {
	cv::Mat mask(blk.text > 100);

	cv::Mat vertSpaces;
	cv::reduce(mask, vertSpaces, 0, CV_REDUCE_SUM, CV_32FC1);  //CV_REDUCE_SUM
	float* data = (float*)vertSpaces.data;

	std::vector<int> emptyCols, trueEmptyCols;

	emptyCols.push_back(0);
	for (unsigned i = 1; i < vertSpaces.cols - 1; i++) {
		if (data[i] == 0 && data[i + 1] != 0) emptyCols.push_back(i);
	}
	emptyCols.push_back(vertSpaces.cols - 1);

	for (unsigned i = 0; i < emptyCols.size() - 1; i++)	{
		if (emptyCols[i] != emptyCols[i + 1]) {
			cv::Rect rct(emptyCols[i], 0, emptyCols[i + 1] - emptyCols[i], 1);
			if (cv::sum(vertSpaces(rct))[0] != 0) {
				if (trueEmptyCols.empty()) trueEmptyCols.push_back(emptyCols[i]);
				trueEmptyCols.push_back(emptyCols[i + 1]);
			}
		}
	}

	//too many columns, probably full page matrix
	if (trueEmptyCols.size() > 5) {
		output = input.clone();
		return;
	}

	//if columns are uniform then possibly they are text columns. else either matrix or simply formatted text
	if (trueEmptyCols.size() > 2) {
		float avg_width = 0, var_width = 0;
		for (unsigned i = 0; i < trueEmptyCols.size() - 1; i++) {
			avg_width += trueEmptyCols[i + 1] - trueEmptyCols[i];
		}
		avg_width /= trueEmptyCols.size() - 1;
		for (unsigned i = 0; i < trueEmptyCols.size() - 1; i++) {
			var_width += std::pow((trueEmptyCols[i + 1] - trueEmptyCols[i] - avg_width), 2);
		}
		var_width /= trueEmptyCols.size() - 1;
		var_width = std::sqrt(var_width);

		//probably multi column text
		if (var_width < 100) {
			output = cv::Mat((input.rows + 10) * (trueEmptyCols.size() - 1), input.cols, CV_8UC1);
			output.setTo(255);
			for (unsigned i = 0; i < trueEmptyCols.size() - 1; i++) {
				cv::Rect rt(0, i * (input.rows + 10), trueEmptyCols[i + 1] - trueEmptyCols[i], input.rows);
				cv::Rect rt2(trueEmptyCols[i], 0, trueEmptyCols[i + 1] - trueEmptyCols[i], input.rows);
				cv::Mat tmp2 = input(rt2);
				tmp2.copyTo(output(rt));
			}
			return;
		}
		output = input.clone();
		return;
	}

	//if non of the above works, check for subareas between empty rows
	cv::Mat horSpaces;
	cv::reduce(mask, horSpaces, 1, CV_REDUCE_SUM, CV_32FC1);
	data = (float*)horSpaces.data;

	std::vector<int> emptyRows, trueEmptyRows;

	emptyRows.push_back(0);
	for (unsigned i = 1; i < horSpaces.rows - 1; i++) {
		if (data[i] == 0 && data[i + 1] != 0) emptyRows.push_back(i);
	}
	emptyRows.push_back(horSpaces.rows - 1);

	for (unsigned i = 0; i < emptyRows.size() - 1; i++) {
		if (emptyRows[i] != emptyRows[i + 1]) {
			cv::Rect rct(0, emptyRows[i], 1, emptyRows[i + 1] - emptyRows[i]);
			if (cv::sum(horSpaces(rct))[0] != 0) {
				if (trueEmptyRows.empty()) trueEmptyRows.push_back(emptyRows[i]);
				trueEmptyRows.push_back(emptyRows[i + 1]);
			}
		}
	}

	//too many rows, probably full page matrix
	if (trueEmptyRows.size() > 6) {
		output = input.clone();
		return;
	}

	// no page segmentation available. single column text
	if (trueEmptyRows.size() < 2) {
		output = input.clone();
		return;
	}

	// search for columned text between empty lines
	std::vector <cv::Mat> outputParts;
	for (unsigned s = 0; s < trueEmptyRows.size() - 1; s++) {
		bool hasColumns = false;
		cv::Rect rcts(0, trueEmptyRows[s], input.cols, trueEmptyRows[s + 1] - trueEmptyRows[s]);
		cv::Mat part = mask(rcts);
		std::vector<int> localEmptyCols, localTrueEmptyCols;
		if (rcts.height > (float)input.rows / 10) {
			cv::Mat localVertSpaces;
			cv::reduce(part, localVertSpaces, 0, CV_REDUCE_SUM, CV_32FC1);
			data = (float*)localVertSpaces.data;

			localEmptyCols.push_back(0);
			for (unsigned i = 1; i < localVertSpaces.cols - 1; i++) {
				if (data[i] == 0 && data[i + 1] != 0) localEmptyCols.push_back(i);
			}
			localEmptyCols.push_back(localVertSpaces.cols - 1);

			for (unsigned i = 0; i < localEmptyCols.size() - 1; i++) {
				if (localEmptyCols[i] != localEmptyCols[i + 1]) {
					cv::Rect rct(localEmptyCols[i], 0, localEmptyCols[i + 1] - localEmptyCols[i], 1);
					if (cv::sum(localVertSpaces(rct))[0] != 0) {
						if (localTrueEmptyCols.empty()) localTrueEmptyCols.push_back(localEmptyCols[i]);
						localTrueEmptyCols.push_back(localEmptyCols[i + 1]);
					}
				}
			}

			if (localTrueEmptyCols.size() > 2 && localTrueEmptyCols.size() < 5) {
				float avg_width = 0, var_width = 0;
				for (unsigned i = 0; i < localTrueEmptyCols.size() - 1; i++) {
					avg_width += localTrueEmptyCols[i + 1] - localTrueEmptyCols[i];
				}
				avg_width /= localTrueEmptyCols.size() - 1;
				for (unsigned i = 0; i < localTrueEmptyCols.size() - 1; i++) {
					var_width += std::pow((localTrueEmptyCols[i + 1] - localTrueEmptyCols[i] - avg_width), 2);
				}
				var_width /= localTrueEmptyCols.size() - 1;
				var_width = std::sqrt(var_width);

				if (var_width < 100) {
					hasColumns = true;
				}
			}
		}

		if (!hasColumns) {
			outputParts.push_back(input(rcts).clone());
		} else {
			cv::Mat prt((part.rows + 10)* (localTrueEmptyCols.size() - 1), input.cols, CV_8UC1);
			prt.setTo(255);
			for (unsigned i = 0; i < localTrueEmptyCols.size() - 1; i++) {
				cv::Rect rt(0, i* (part.rows + 10), localTrueEmptyCols[i + 1] - localTrueEmptyCols[i], part.rows);
				cv::Rect rt2(localTrueEmptyCols[i], 0, localTrueEmptyCols[i+1]-localTrueEmptyCols[i], part.rows);
				cv::Mat tmp2 =(input(rcts))(rt2);
				tmp2.copyTo(prt(rt));
			}
			outputParts.push_back(prt.clone());
		}
	}
	output = outputParts[0].clone();
	for (unsigned i = 1; i < outputParts.size(); i++) {
		cv::vconcat(output,outputParts[i],output);
	}
}

bool img_processor::mat2pix(cv::Mat& mat, Pix** px) {
	if ((mat.type() != CV_8UC1) || (mat.empty())) return false;

	*px = pixCreate(mat.cols, mat.rows, 8);
	uchar* data = mat.data;

	for (int i = 0; i < mat.rows; i++) {
		unsigned idx = i * mat.cols;
		for (int j = 0; j < mat.cols; j++) {
			pixSetPixel(*px, j, i, (l_uint32)data[idx + j]);
		}
	}
	return true;
}

/**
 * @brief Creates a binary pix from mat
 * @param mat: input mat
 * @param px: output pix
 * @return
 */
bool img_processor::mat2pixBinary(cv::Mat& mat, Pix** px) {
	if ((mat.type() != CV_8UC1) || (mat.empty())) return false;

	*px = pixCreate(mat.cols, mat.rows, 1);
	uchar* data = mat.data;

	for (int i = 0; i < mat.rows; i++) {
		unsigned idx = i * mat.cols;
		for (int j = 0; j < mat.cols; j++) {
			pixSetPixel(*px, j, i, 1 - (l_uint32)(data[idx + j] / 255));
		}
	}
	return true;
}

bool img_processor::pix2mat(Pix** px, cv::Mat& mat) {
	if (((*px)->d != 8 && (*px)->d != 1)
		|| ((*px)->w < 1)) return false;

	mat = cv::Mat((*px)->h, (*px)->w, CV_8UC1);
	uchar* data = mat.data;

	if ((*px)->d == 8) {
		for (int i = 0; i < mat.rows; i++) {
			unsigned idx = i * mat.cols;
			for (int j = 0; j < mat.cols; j++) {
				l_uint32 val;
				pixGetPixel(*px, j, i, &val);
				data[idx + j] = (uchar)val;
			}
		}
	}
	else {
		for (int i = 0; i < mat.rows; i++) {
			unsigned idx = i * mat.cols;
			for (int j = 0; j < mat.cols; j++) {
				l_uint32 val;
				pixGetPixel(*px, j, i, &val);
				data[idx + j] = (uchar)(255 - val * 255);
			}
		}
	}
	return true;
}

bool img_processor::pixmap2mat(fz_pixmap** fzpxmap, cv::Mat& mat) {
	if ((*fzpxmap)->w < 1) return false;
	mat = cv::Mat((*fzpxmap)->h, (*fzpxmap)->w, CV_8UC1);
	uchar* data = mat.data;

	for (unsigned i = 0; i < (*fzpxmap)->h; i++) {
		//cout << i << ", ";
		unsigned idxMat = i * mat.cols;
		unsigned idxPixmap = i * 4 * mat.cols;
		for (unsigned j = 0; j < (*fzpxmap)->w; j++) {
			data[idxMat + j] = ((*fzpxmap)->samples[idxPixmap + 4 * j] + (*fzpxmap)->samples[idxPixmap + 4 * j + 1] + (*fzpxmap)->samples[idxPixmap + 4 * j + 2]) / 3;
		}
	}
	return true;
}


// I Have to find what to do in frame 
bool img_processor::ConstructImage(cv::Mat& image, cv::Mat& filter, int ker_len) {
	int indx, ker_indx;
	int img_width = image.size().width;
	int img_height = image.size().height;
	int cnt_black;
	int half_ker = ker_len / 2;
	int local_blacks_limit = sqrt(ker_len);
	cv::Mat output = filter.clone();

	for (int i = half_ker; i < img_width - 2 - half_ker; i++) {
		indx = i * img_height;
		for (int j = half_ker; j < img_height - 1 - half_ker; j++) {
			if (image.data[indx+j] == 0 && filter.data[indx+j] == 255) {
				cnt_black = 0;
				for (int k = i - half_ker; k < (i + half_ker + 1); k++) {
					for (int l = j - half_ker; l < (j + half_ker + 1); l++) {
						ker_indx = k * img_height;
						cnt_black += (filter.data[ker_indx + l] == 0) ? 1 : 0;
					}
				}
				if (cnt_black > local_blacks_limit) {
					output.data[indx + j] = 0;
				}
			} /*else if (image.data[indx + j] == 0 && filter.data[indx + j] == 255) {
				output.data[indx + j] = 255;
			}*/
		}
	}

	image = output.clone();

	return true;
}

bool img_processor::ClearImage(cv::Mat& image, cv::Mat& output) {
	cv::Mat temp_er, temp_th, closed, se;
	BinarizationType bin_type = BinarizationType::SAUVOLA;
	int window = 20; 
	int kernel = 3;
	double k = 0.45;

	cv::erode(image, temp_er, cv::Mat(), cv::Point(-1, -1), 1);
	img_processor::ThresholdImage(temp_er, temp_th, bin_type, window, window, k);
	se = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 3));
	cv::morphologyEx(temp_th, output, cv::MORPH_CLOSE, se, cv::Point(-1, -1), 2);

	return true;
}