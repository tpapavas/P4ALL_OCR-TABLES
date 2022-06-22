#pragma once
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include "helpers/auxiliary.h"

using namespace std;

#define SEC_IN_MSECS 1000.0

namespace ocrt {
	namespace aux {
		using high_res_clock = std::chrono::high_resolution_clock;
		using unit = std::chrono::milliseconds;

		std::chrono::time_point<high_res_clock> startTime, endTime;
		bool clockStarted = false;

		/**
		 * @brief "Starts the clock"; Save current time.
		 */
		void startClock() {
			startTime = high_res_clock::now();
		}

		/**
		 * @brief Counts time units passed since last call of startClock().
		 * @return (endTime - startTime)
		 */
		double endClock() {
			endTime = high_res_clock::now();

			return std::chrono::duration_cast<unit>(endTime - startTime).count() / SEC_IN_MSECS;
		}

		/**
		 * @brief Write processed input doc into an HTML file
		 * @param filename: The name of output html file
		 * @param table_columns 
		 * @param multi_rows 
		 * @param line_segments 
		 * @param lines_type 
		 * @param words 
		 * @param font_size 
		 */
		bool WriteHTML(std::string& filename, const ocrt::Document doc) {
			std::cout << "Create HTML...";
			aux::startClock();

			//find average font size
			vector<int> tmp_size;

			//tmp_size = font_size;

			size_t n = doc.words.size() / 2;
			//nth_element(doc.words.begin(), doc.words.begin() + n, doc.words.end());
			int font_size_avg = doc.words[n].font_size;
			float ratio = (float)12 / font_size_avg;
			font_size_avg = 12;
			//tmp_size[0] = 1;
			//tmp_size[1] = 2;

			// create font for words
			vector<vector<string>> font;
			for (int i = 0; i < doc.words.size(); i++) {
				//font_size[i]=font_size[i]*ratio;
				vector<string> tmp;
				//if (font_size[i]<(font_size_avg-3))
				//{
				//	tmp.push_back("<font size=
				// "1\">");
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
				for (int x = 0; x < doc.lines.size(); x++) {
					if (doc.lines[x].type != LineType::TABLE) {
						for (int j = 0; j < doc.lines[x].segments.size(); j++) {
							for (int k = 0; k < doc.lines[x].segments[j].size(); k++) {
								file << font[doc.lines[x].segments[j][k]][0] << doc.words[doc.lines[x].segments[j][k]].name << " " << font[doc.lines[x].segments[j][k]][1];
							}
						}
						file << "<br>";
					}
					else {
						file << "\n</p>\n";
						file << "<table border=\"1\">\n";
						for (int i = 0; i < doc.tables[table_num].multi_rows.size(); i++) {
							file << "<tr>\n";
							vector<vector<vector<int>>> col_tmp;
							for (int j = 0; j < doc.tables[table_num].columns.size(); j++) {
								vector<vector<int>> ctmp;
								col_tmp.push_back(ctmp);
							}
							for (int j = 0; j < doc.tables[table_num].multi_rows[i].size(); j++) {
								for (int k = 0; k < doc.lines[doc.tables[table_num].multi_rows[i][j]].segments.size(); k++) {
									for (int s = 0; s < doc.tables[table_num].columns.size(); s++) {
										if (std::find(doc.tables[table_num].columns[s].begin(), doc.tables[table_num].columns[s].end(), doc.lines[doc.tables[table_num].multi_rows[i][j]].segments[k]) != doc.tables[table_num].columns[s].end()) {
											col_tmp[s].push_back(doc.lines[doc.tables[table_num].multi_rows[i][j]].segments[k]);
										}
									}
								}
							}
							for (int j = 0; j < doc.tables[table_num].columns.size(); j++) {
								file << "<td";
								int colspan = 1;
								if (col_tmp[j].size() == 0) {
									file << ">";
								}
								else {
									for (int h = j + 1; h < doc.tables[table_num].columns.size(); h++) {
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
										file << font[col_tmp[j][k][s]][0] << doc.words[col_tmp[j][k][s]].name << " " << font[col_tmp[j][k][s]][1];
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
						int num_of_lines = doc.lines.size();
						while (x < num_of_lines && doc.lines[x].type == LineType::TABLE) { x++; }
						x--;
						table_num++;
					}
				}
				file << "\n</p>\n</body>\n</html>";
				file.close();

				std::cout << " Done in " << aux::endClock() << "s \n";
			} else {
				std::cout << "Unable to open file";
				return false;
			}

			return true;
		}
	}

}