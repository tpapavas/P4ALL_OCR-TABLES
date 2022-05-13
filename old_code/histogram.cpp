cv::Mat hist; float range[] = { 0, 256 };
	int histSize = 256; const float* histRange = { range };
	cv::calcHist(&input, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    // Threshold
    output = cv::Mat(input.rows, input.cols, CV_8U);
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound((double)hist_w / histSize);

	cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0, 0));
	normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());
	for (int i = 1; i < histSize; i++)
	{
		cv::line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
			cv::Point(bin_w * (i), hist_h - cvRound(hist.at<float>(i))),
			cv::Scalar(255, 0, 0), 2, 8, 0);
	}
	cv::namedWindow("calcHist Demo", cv::WINDOW_AUTOSIZE);
	imshow("calcHist Demo", histImage);