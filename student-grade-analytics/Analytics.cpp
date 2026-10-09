#include "Analytics.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

using std::cout, std::endl, std::map, std::vector;

Analytics::Analytics(const vector<GradeRecord>& records)
	: m_records(records)
{
}

map<Grade, int> Analytics::countGrades() const
{
	map<Grade, int> gradeCounts;

	for (const GradeRecord& record : m_records)
	{
		Grade grade = scoreToGrade(record.m_score);
		gradeCounts[grade]++;
	}

	return gradeCounts;
}

double Analytics::calculateMean() const
{
	if (m_records.empty())
		return 0.0;

	double total = 0.0;

	for (const GradeRecord& record : m_records)
		total += record.m_score;

	return total / m_records.size();
}

double Analytics::calculateMedian() const
{
	if (m_records.empty())
		return 0.0;

	vector<int> scores;

	for (const GradeRecord& record : m_records)
		scores.push_back(record.m_score);

	std::sort(scores.begin(), scores.end());

	size_t middle = scores.size() / 2;

	if (scores.size() % 2 == 0)
		return (scores[middle - 1] + scores[middle]) / 2.0;

	return scores[middle];
}

vector<int> Analytics::calculateModes() const
{
	vector<int> modes;

	if (m_records.empty())
		return modes;

	map<int, int> frequencies;

	for (const GradeRecord& record : m_records)
		frequencies[record.m_score]++;

	int highestFrequency = 0;

	for (const auto& entry : frequencies)
	{
		if (entry.second > highestFrequency)
			highestFrequency = entry.second;
	}

	if (highestFrequency <= 1)
		return modes;

	for (const auto& entry : frequencies)
	{
		if (entry.second == highestFrequency)
			modes.push_back(entry.first);
	}

	return modes;
}

double Analytics::calculatePercentile(int score) const
{
	if (m_records.empty())
		return 0.0;

	int countBelowOrEqual = 0;

	for (const GradeRecord& record : m_records)
	{
		if (record.m_score <= score)
			countBelowOrEqual++;
	}

	return static_cast<double>(countBelowOrEqual)
		/ m_records.size() * 100.0;
}

int Analytics::getHighestScore() const
{
	if (m_records.empty())
		return 0;

	int highest = m_records[0].m_score;

	for (const GradeRecord& record : m_records)
	{
		if (record.m_score > highest)
			highest = record.m_score;
	}

	return highest;
}

int Analytics::getLowestScore() const
{
	if (m_records.empty())
		return 0;

	int lowest = m_records[0].m_score;

	for (const GradeRecord& record : m_records)
	{
		if (record.m_score < lowest)
			lowest = record.m_score;
	}

	return lowest;
}

double Analytics::calculatePassRate() const
{
	if (m_records.empty())
		return 0.0;

	int passed = 0;

	for (const GradeRecord& record : m_records)
	{
		if (record.m_score >= 60)
			passed++;
	}

	return static_cast<double>(passed)
		/ m_records.size() * 100.0;
}

int Analytics::getRecordCount() const
{
	return static_cast<int>(m_records.size());
}

void Analytics::printSummary() const
{
	cout << "Mean Score: "
		<< calculateMean() << endl;

	cout << "Median Score: "
		<< calculateMedian() << endl;

	cout << "Modes: ";

	vector<int> modes = calculateModes();

	if (modes.empty())
	{
		cout << "None";
	}
	else
	{
		for (int mode : modes)
			cout << mode << " ";
	}

	cout << endl;

	cout << "Highest Score: "
		<< getHighestScore() << endl;

	cout << "Lowest Score: "
		<< getLowestScore() << endl;

	cout << "Pass Rate: "
		<< calculatePassRate() << "%" << endl;

	cout << "Records: "
		<< getRecordCount() << endl;

	cout << "Grade Distribution:" << endl;

	map<Grade, int> gradeCounts = countGrades();

	for (const auto& entry : gradeCounts)
	{
		cout << gradeToString(entry.first)
			<< ": " << entry.second << endl;
	}
}