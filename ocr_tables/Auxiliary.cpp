#pragma once
#include <chrono>
#include "auxiliary.h"

#define SEC_IN_MSECS 1000.0

using high_res_clock = std::chrono::high_resolution_clock;
using unit = std::chrono::milliseconds;

std::chrono::time_point<high_res_clock> startTime, endTime;
bool clockStarted = false;

namespace ocr_tabs {
	namespace aux {
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
	}
}