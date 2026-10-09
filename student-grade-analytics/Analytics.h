#pragma once

#include "GradeRecord.h"

#include <map>
#include <vector>

using std::map, std::vector;

class Analytics {
private:
	const vector<GradeRecord>& m_records;

public:
	Analytics(const vector<GradeRecord>& records);

	map<Grade, int> countGrades() const;

	double calculateMean() const;

	double calculateMedian() const;

	vector<int> calculateModes() const;

	double calculatePercentile(int score) const;

	int getHighestScore() const;

	int getLowestScore() const;

	double calculatePassRate() const;

	int getRecordCount() const;

	void printSummary() const;
};