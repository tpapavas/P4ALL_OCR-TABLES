
bool OCRTabsEngine::pdf2html(const std::string& filename, const std::string& filenameXML, bool withXML) {
	resetAll();
	std::vector<cv::Mat> pages, pages_clean;
	if (!parsePDF(filename, pages)) return false;
	if (withXML) {
		//// NOT WORKING ////
		if (!ImagePreproccesing_withXML(filenameXML, pages, pages_clean)) { cout << "Preprocessing with XML failed\n"; return false; }
		pages.clear();
		pages = pages_clean;  //not sure about that copying
	}
	// if (pages.size() == 1) {
	//	test = ImagePreproccesing(pages[0]);
	//	SetImage(test);
	//	//RemoveGridLines();
	//	OCR_Recognize();
	//	BoxesAndWords();
	//	TextBoundaries();
	//} else {
	//	for (int i = 0; i < pages.size(); i++) {
	//		Mat tmp = ImagePreproccesing(pages[i]);
	//		SetImage(tmp);
	//		//RemoveGridLines();
	//		OCR_Recognize();
	//		BoxesAndWords();
	//		PrepareMulti1();
	//	}
	//	HeadersFooters();
	//	PrepareMulti2();
	//}
	for (int i = 0; i < pages.size(); i++) {
		Mat tmp = ImagePreproccesing(pages[i]);
		SetImage(tmp);
		//RemoveGridLines();
		OCR_Recognize();
		BoxesAndWords();
		PrepareMulti1();
	}
	HeadersFooters();
	PrepareMulti2();

	TextLines();
	LineSegments();
	LineTypes();
	TableAreas();
	TableRows();
	TableColumns(); ////
	if (fail_condition()) {
		std::cout << "\nfailCondition: " << fail_msg << std::endl;
		return false;
	}
	TableMultiRows();
	ColumnSize();	 ////
	FinalizeGrid(); ////
	std::string outputFilename = filename;
	outputFilename.append(withXML ? "XML.html" : ".html");
	WriteHTML(outputFilename);

	return true;
}

bool OCRTabsEngine::img2html(const std::string& filename, const std::string& filenameXML, bool withXML) {
	resetAll();
	//Mat test = imread(filename, IMREAD_GRAYSCALE);
	//if (test.empty()) { cout << "File not available\n"; return false; }
	std::vector<cv::Mat> imageList, imageClean;
	imageList.push_back(cv::imread(filename, cv::IMREAD_GRAYSCALE));  //CV_LOAD_IMAGE_GRAYSCALE
	if (imageList[0].empty()) { cout << "File not available" << endl; return false; }
	if (withXML) {
		if (!ImagePreproccesing_withXML(filenameXML, imageList, imageClean)) { cout << "Preprocessing with XML failed\n"; return false; }
		imageList.clear();
		imageList = imageClean;  //not sure it's working.
	}
	Mat test = ImagePreproccesing(imageList[0]);
	SetImage(test);
	//RemoveGridLines();
	OCR_Recognize();
	BoxesAndWords();
	TextBoundaries();
	TextLines();
	LineSegments();
	LineTypes();
	TableAreas();
	TableRows();
	TableColumns(); ////
	if (fail_condition()) {
		std::cout << "\nfailCondition: " << fail_msg << std::endl;
		return false;
	}
	TableMultiRows();
	ColumnSize();	 ////
	FinalizeGrid(); ////
	std::string outputFilename = filename;
	outputFilename.append(withXML ? "XML.html" : ".html");
	WriteHTML(outputFilename);

	return true;
}


bool OCRTabsEngine::pdf2html_withXML(const std::string& filename, const std::string& filenameXML) {
	resetAll();
	std::vector<cv::Mat> pages, pages_clean;
	if (!parsePDF(filename, pages)) return false;
	if (!ImagePreproccesing_withXML(filenameXML, pages, pages_clean)) { cout << "Preprocessing with XML failed\n"; return false; }
	if (pages.size() == 1) {
		cout << "in here " << endl;
		SetImage(pages_clean[0]);
		//RemoveGridLines();
		OCR_Recognize();
		BoxesAndWords();
		TextBoundaries();
	} else {
		for (int i = 0; i < pages.size(); i++) {
			SetImage(pages_clean[i]);
			//RemoveGridLines();
			PrepareMulti1();
		}
		HeadersFooters();
		PrepareMulti2();
	}
	TextLines();
	LineSegments();
	LineTypes();
	TableAreas();
	TableRows();
	TableColumns(); ////
	if (fail_condition()) {
		std::cout << "\nfailCondition\n";
		return false;
	}
	TableMultiRows();
	ColumnSize();	 ////
	FinalizeGrid(); ////
	std::string outputFilename = filename;
	outputFilename.append("XML.html");
	WriteHTML(outputFilename);
	return true;
}

bool OCRTabsEngine::img2html_withXML(const std::string& filename, const std::string& filenameXML) {
	resetAll();
	std::vector<cv::Mat> imageList, imageClean;
	imageList.push_back(cv::imread(filename, cv::IMREAD_GRAYSCALE));  //CV_LOAD_IMAGE_GRAYSCALE	
	if (imageList[0].empty()) { cout << "File not available\n"; return false; }
	if (!ImagePreproccesing_withXML(filenameXML, imageList, imageClean)) { cout << "Preprocessing with XML failed\n"; return false; }
	SetImage(imageClean[0]);
	OCR_Recognize();
	BoxesAndWords();
	TextBoundaries();
	TextLines();
	LineSegments();
	LineTypes();
	TableAreas();
	TableRows();
	TableColumns(); ////
	if (fail_condition()) {
		std::cout <<"\nfailCondition\n";
		return false;
	}
	TableMultiRows();
	ColumnSize();	 ////
	FinalizeGrid(); ////
	std::string outputFilename = filename;
	outputFilename.append("XML.html");
	WriteHTML(outputFilename);
	return true;
} 

void OCRTabsEngine::WriteHTML(std::string& filename) {
	std::cout << "Create HTML...";
	aux::startClock();

	//find average font size
	vector<int> tmp_size;

	tmp_size = font_size;

	size_t n = tmp_size.size() / 2;
	nth_element(tmp_size.begin(), tmp_size.begin() + n, tmp_size.end());
	int font_size_avg = tmp_size[n];
	float ratio = (float)12 / font_size_avg;
	font_size_avg = 12;
	tmp_size[0] = 1;
	tmp_size[1] = 2;

	// create font for words
	vector<vector<string>> font;
	for (int i = 0; i < font_size.size(); i++) {
		//font_size[i]=font_size[i]*ratio;
		vector<string> tmp;
		//if (font_size[i]<(font_size_avg-3))
		//{
		//	tmp.push_back("<font size=\"1\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg-2))
		//{
		//	tmp.push_back("<font size=\"2\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg+2))
		//{
		tmp.push_back("");
		tmp.push_back("");
		//}
		//else if (font_size[i]<(font_size_avg+4))
		//{
		//	tmp.push_back("<font size=\"4\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg+6))
		//{
		//	tmp.push_back("<font size=\"5\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg+8))
		//{
		//	tmp.push_back("<font size=\"6\">");
		//	tmp.push_back("</font>");
		//}
		//else
		//{
		//	tmp.push_back("<font size=\"7\">");
		//	tmp.push_back("</font>");
		//}
		/*if (bold[i])
		{
			tmp[0].append("<b>");
			tmp[1].append("</b>");
		}
		if (italic[i])
		{
			tmp[0].append("<i>");
			tmp[1].append("</i>");
		}
		if (underscore[i])
		{
			tmp[0].append("<u>");
			tmp[1].append("</u>");
		}*/
		font.push_back(tmp);
	}

	// write html file
	std::ofstream file(filename);
	int table_num = 0;
	if (file.is_open()) {
		file << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n<html>\n<body>\n";
		file << "<p>\n";
		for (int x = 0; x < line_segments.size(); x++) {
			if (lines_type[x] != LineType::TABLE) {
				for (int j = 0; j < line_segments[x].size(); j++) {
					for (int k = 0; k < line_segments[x][j].size(); k++) {
						file << font[line_segments[x][j][k]][0] << words[line_segments[x][j][k]] << " " << font[line_segments[x][j][k]][1];
					}
				}
				file << "<br>";
			} else {
				file << "\n</p>\n";
				file << "<table border=\"1\">\n";
				for (int i = 0; i < multi_rows[table_num].size(); i++) {
					file << "<tr>\n";
					vector<vector<vector<int>>> col_tmp;
					for (int j = 0; j < table_columns[table_num].size(); j++) {
						vector<vector<int>> ctmp;
						col_tmp.push_back(ctmp);
					}
					for (int j = 0; j < multi_rows[table_num][i].size(); j++) {
						for (int k = 0; k < line_segments[multi_rows[table_num][i][j]].size(); k++) {
							for (int s = 0; s < table_columns[table_num].size(); s++) {
								if (std::find(table_columns[table_num][s].begin(), table_columns[table_num][s].end(), line_segments[multi_rows[table_num][i][j]][k]) != table_columns[table_num][s].end()) {
									col_tmp[s].push_back(line_segments[multi_rows[table_num][i][j]][k]);
								}
							}
						}
					}
					for (int j = 0; j < table_columns[table_num].size(); j++) {
						file << "<td";
						int colspan = 1;
						if (col_tmp[j].size() == 0) {
							file << ">";
						} else {
							for (int h = j + 1; h < table_columns[table_num].size(); h++) {
								if (col_tmp[h].size() != 0) {
									for (int aa = 0; aa < col_tmp[j].size(); aa++) {
										if (std::find(col_tmp[h].begin(), col_tmp[h].end(), col_tmp[j][aa]) != col_tmp[h].end()) {
											colspan++;
											aa = col_tmp[j].size();
										}
									}
								}
							}
							if (colspan > 1) {
								file << " colspan=\"" << colspan << "\"";
								for (int h = j + 1; h < j + colspan; h++) {
									for (int aa = 0; aa < col_tmp[h].size(); aa++) {
										bool found = false;
										if (std::find(col_tmp[j].begin(), col_tmp[j].end(), col_tmp[h][aa]) != col_tmp[j].end()) {
											found = true;
										}
										if (!found) { col_tmp[j].push_back(col_tmp[h][aa]); }
									}
								}
							}
							file << ">";
						}
						for (int k = 0; k < col_tmp[j].size(); k++) {
							for (int s = 0; s < col_tmp[j][k].size(); s++) {
								file << font[col_tmp[j][k][s]][0] << words[col_tmp[j][k][s]] << " " << font[col_tmp[j][k][s]][1];
							}
							if (col_tmp[j].size() > (k + 1)) {
								file << "<br>";
							}
						}
						file << "</td>\n";
						j = j + colspan - 1;
					}
					file << "</tr>\n";
				}
				file << "</table>\n<p>\n";
				while (lines_type[x] == 2) { x++; }
				x--;
				table_num++;
			}
		}
		file << "\n</p>\n</body>\n</html>";
		file.close();

		cout << " Done in " << aux::endClock() << "s \n";
	}
	else {
		std::cout << "Unable to open file";
	}
}