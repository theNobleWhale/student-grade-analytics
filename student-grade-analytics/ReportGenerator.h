#pragma once

#include "Analytics.h"
#include "StudentManager.h"
#include "Trends.h"

#include <map>
#include <string>
#include <vector>

using std::string;

class ReportGenerator {
public:
	void printAnalytics(const Analytics& analytics, const string& title) const;

	/// ASCII bar chart of a grade distribution, e.g.  A | ####### 50311 (11.3%)
	void printGradeChart(
		const std::map<Grade, int>& gradeCounts,
		const string& title,
		int barWidth = 40) const;

	/// Per-semester mean and rolling mean with a bar for each mean.
	void printTrend(
		const std::vector<TrendPoint>& points,
		const string& title,
		int barWidth = 40) const;

	/// Top-N students with rank and percentile.
	void printRankings(
		const std::vector<RankedStudent>& ranked,
		std::size_t topN,
		const string& title) const;
};
