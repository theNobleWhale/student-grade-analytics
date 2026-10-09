#pragma once

#include "GradeRecord.h"
#include "StudentManager.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

/// Average performance in one semester plus the rolling average up to it.
struct TrendPoint {
	std::string semester;
	int records;
	double mean;
	double rollingMean;
};

enum class TrendDirection {
	Improving,
	Declining,
	Stable,
	Insufficient // fewer than two semesters of data
};

std::string trendToString(TrendDirection direction);

/// Groups records by semester (chronological: "2025S1" < "2025S2" < "2026S1"),
/// computes each semester's mean score, and runs those means through a
/// deque-backed RollingAverage with the given window size.
std::vector<TrendPoint> computeTrend(
	const std::vector<GradeRecord>& records,
	std::size_t window = 2);

/// Compares the newest semester's mean against the rolling average of the
/// semesters before it. A difference within +/- tolerance is "Stable".
TrendDirection classifyTrend(
	const std::vector<TrendPoint>& points,
	double tolerance = 1.0);

/// How many students are improving / declining / stable.
std::map<TrendDirection, int> countStudentTrends(
	const StudentManager& studentManager,
	std::size_t window = 2,
	double tolerance = 1.0);
