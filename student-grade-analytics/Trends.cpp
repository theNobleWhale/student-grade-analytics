#include "Trends.h"

#include "RollingAverage.h"

using std::map, std::string, std::vector;

string trendToString(TrendDirection direction)
{
	switch (direction)
	{
	case TrendDirection::Improving: return "improving";
	case TrendDirection::Declining: return "declining";
	case TrendDirection::Stable: return "stable";
	case TrendDirection::Insufficient: return "insufficient_data";
	}

	return "unknown";
}

vector<TrendPoint> computeTrend(
	const vector<GradeRecord>& records, std::size_t window)
{
	struct Accumulator {
		double sum = 0.0;
		int count = 0;
	};

	// std::map keeps semesters sorted; operator[] default-constructs the
	// accumulator on first use (the defaultdict pattern).
	map<string, Accumulator> bySemester;

	for (const GradeRecord& record : records)
	{
		Accumulator& acc = bySemester[record.m_semester];
		acc.sum += record.m_score;
		acc.count++;
	}

	RollingAverage rolling(window);
	vector<TrendPoint> points;
	points.reserve(bySemester.size());

	for (const auto& [semester, acc] : bySemester)
	{
		double mean = acc.sum / acc.count;

		points.push_back({
			semester,
			acc.count,
			mean,
			rolling.add(mean) });
	}

	return points;
}

TrendDirection classifyTrend(
	const vector<TrendPoint>& points, double tolerance)
{
	if (points.size() < 2)
		return TrendDirection::Insufficient;

	// Newest semester vs. the rolling average that ended one semester earlier.
	double delta =
		points.back().mean - points[points.size() - 2].rollingMean;

	if (delta > tolerance) return TrendDirection::Improving;
	if (delta < -tolerance) return TrendDirection::Declining;

	return TrendDirection::Stable;
}

map<TrendDirection, int> countStudentTrends(
	const StudentManager& studentManager,
	std::size_t window,
	double tolerance)
{
	map<TrendDirection, int> counts;

	for (const auto& [studentId, student] : studentManager.getStudents())
	{
		if (student.getGrades().empty())
			continue;

		counts[classifyTrend(
			computeTrend(student.getGrades(), window),
			tolerance)]++;
	}

	return counts;
}
