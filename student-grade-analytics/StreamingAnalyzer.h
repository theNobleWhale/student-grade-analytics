#pragma once

#include "CSVReader.h"
#include "ReportTypes.h"

#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

/// Counter-style histogram for integer scores 0..100.
///
/// Because scores are bounded, mean, median, mode, percentile, min/max and
/// grade counts can all be computed *exactly* from 101 counters, so memory
/// stays constant no matter how many scores are added.
class ScoreHistogram {
private:
	std::array<long long, 101> m_counts{};
	long long m_total = 0;
	long long m_sum = 0;

	int kthSmallest(long long k) const; // zero-based

public:
	void add(int score); // throws std::out_of_range outside 0..100

	long long total() const { return m_total; }
	long long countOf(int score) const;

	/// % of scores at or below `score` (same definition as
	/// Analytics::calculatePercentile).
	double percentile(int score) const;

	ScoreStats toStats() const;
};

struct StreamingResult {
	ScoreHistogram overall;
	std::vector<std::pair<std::string, ScoreHistogram>> byCourse;
	std::vector<std::pair<std::string, ScoreHistogram>> bySemester;
	LoadResult load;
};

/// Analyses a grades file row by row without ever storing the records.
/// Memory is O(courses + semesters), not O(rows) - the "generator" approach
/// for files too large to load. Student IDs are not cross-checked because
/// that would require holding every student in memory.
StreamingResult analyzeGradesStreaming(
	const std::filesystem::path& gradesFile,
	const std::vector<Course>& courses,
	const CSVReader& reader = CSVReader{});
