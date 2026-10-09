#include "StreamingAnalyzer.h"

#include "ScoreInput.h"

#include <map>
#include <stdexcept>
#include <unordered_set>

using std::string;

void ScoreHistogram::add(int score)
{
	if (score < 0 || score > 100)
		throw std::out_of_range("Score must be between 0 and 100");

	m_counts[score]++;
	m_total++;
	m_sum += score;
}

long long ScoreHistogram::countOf(int score) const
{
	if (score < 0 || score > 100) return 0;
	return m_counts[score];
}

int ScoreHistogram::kthSmallest(long long k) const
{
	long long seen = 0;

	for (int score = 0; score <= 100; ++score)
	{
		seen += m_counts[score];

		if (k < seen)
			return score;
	}

	return 100;
}

double ScoreHistogram::percentile(int score) const
{
	if (m_total == 0) return 0.0;

	long long atOrBelow = 0;

	for (int s = 0; s <= score && s <= 100; ++s)
		atOrBelow += m_counts[s];

	return static_cast<double>(atOrBelow)
		/ static_cast<double>(m_total) * 100.0;
}

ScoreStats ScoreHistogram::toStats() const
{
	ScoreStats stats;

	if (m_total == 0)
		return stats;

	stats.records = static_cast<int>(m_total);
	stats.mean = static_cast<double>(m_sum) / static_cast<double>(m_total);

	if (m_total % 2 == 0)
		stats.median =
			(kthSmallest(m_total / 2 - 1) + kthSmallest(m_total / 2)) / 2.0;
	else
		stats.median = kthSmallest(m_total / 2);

	long long highestFrequency = 0;
	long long passed = 0;
	bool haveLowest = false;

	for (int score = 0; score <= 100; ++score)
	{
		long long count = m_counts[score];

		if (count == 0) continue;

		if (!haveLowest)
		{
			stats.lowest = score;
			haveLowest = true;
		}

		stats.highest = score;

		if (count > highestFrequency) highestFrequency = count;
		if (score >= 60) passed += count;

		stats.gradeCounts[scoreToGrade(score)] += static_cast<int>(count);
	}

	// Same rule as Analytics::calculateModes: no mode if nothing repeats.
	if (highestFrequency > 1)
		for (int score = 0; score <= 100; ++score)
			if (m_counts[score] == highestFrequency)
				stats.modes.push_back(score);

	stats.passRate = static_cast<double>(passed)
		/ static_cast<double>(m_total) * 100.0;

	return stats;
}

StreamingResult analyzeGradesStreaming(
	const std::filesystem::path& gradesFile,
	const std::vector<Course>& courses,
	const CSVReader& reader)
{
	std::unordered_set<string> knownCourses;

	for (const Course& course : courses)
		knownCourses.insert(course.m_courseCode);

	StreamingResult result;

	// std::map = sorted keys; operator[] creates a histogram on first use.
	std::map<string, ScoreHistogram> byCourse;
	std::map<string, ScoreHistogram> bySemester;

	auto onMalformed = [&](std::size_t line, const string& message)
		{
			++result.load.rowsRead;
			result.load.recordError(
				"line " + std::to_string(line) + ": " + message);
		};

	reader.forEachRow(
		gradesFile,
		{ "courseCode", "score", "semester" },
		[&](const CsvRow& row, std::size_t line)
		{
			++result.load.rowsRead;

			if (knownCourses.find(row["courseCode"]) == knownCourses.end())
			{
				result.load.recordError(
					"line " + std::to_string(line)
					+ ": Course not found: " + row["courseCode"]);
				return;
			}

			std::optional<int> score = toScore(ScoreInput{ row["score"] });

			if (!score || row["semester"].empty())
			{
				result.load.recordError(
					"line " + std::to_string(line)
					+ ": Invalid score or semester");
				return;
			}

			result.overall.add(*score);
			byCourse[row["courseCode"]].add(*score);
			bySemester[row["semester"]].add(*score);
			++result.load.rowsLoaded;
		},
		onMalformed);

	result.byCourse.assign(byCourse.begin(), byCourse.end());
	result.bySemester.assign(bySemester.begin(), bySemester.end());

	return result;
}
