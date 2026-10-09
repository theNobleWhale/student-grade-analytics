#include "ReportGenerator.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

using std::cout;
using std::setw;

void ReportGenerator::printAnalytics(const Analytics& analytics, const string& title) const
{
	cout << "\n===== "
		<< title
		<< " =====\n";

	cout << "Records: "
		<< analytics.getRecordCount()
		<< '\n';

	cout << "Mean: "
		<< analytics.calculateMean()
		<< '\n';

	cout << "Median: "
		<< analytics.calculateMedian()
		<< '\n';

	cout << "Highest: "
		<< analytics.getHighestScore()
		<< '\n';

	cout << "Lowest: "
		<< analytics.getLowestScore()
		<< '\n';

	cout << "Pass Rate: "
		<< analytics.calculatePassRate()
		<< "%\n";

	cout << "Mode(s): ";

	auto modes = analytics.calculateModes();

	if (modes.empty())
		cout << "No mode";
	else
	{
		for (int score : modes)
			cout << score << ' ';
	}

	cout << '\n';

	cout << "Grade Counts:\n";

	auto gradeCounts = analytics.countGrades();

	for (const auto& [grade, count] : gradeCounts)
	{
		cout << gradeToString(grade)
			<< " -> "
			<< count
			<< '\n';
	}
}

void ReportGenerator::printGradeChart(
	const std::map<Grade, int>& gradeCounts,
	const string& title,
	int barWidth) const
{
	cout << "\n----- " << title << " -----\n";

	int total = 0;
	int largest = 0;

	for (const auto& [grade, count] : gradeCounts)
	{
		total += count;
		largest = (std::max)(largest, count);
	}

	if (total == 0)
	{
		cout << "(no data)\n";
		return;
	}

	for (Grade grade : kAllGrades)
	{
		auto it = gradeCounts.find(grade);
		int count = it == gradeCounts.end() ? 0 : it->second;

		int barLength = largest == 0 ? 0 : count * barWidth / largest;
		double percent = 100.0 * count / total;

		cout << "  " << gradeToString(grade) << " | "
			<< std::left << setw(barWidth) << string(barLength, '#')
			<< std::right << ' ' << setw(8) << count
			<< " (" << std::fixed << std::setprecision(1) << percent
			<< "%)\n";
	}

	cout.unsetf(std::ios::floatfield);
	cout << std::setprecision(6);
}

void ReportGenerator::printTrend(
	const std::vector<TrendPoint>& points,
	const string& title,
	int barWidth) const
{
	cout << "\n----- " << title << " -----\n";

	if (points.empty())
	{
		cout << "(no data)\n";
		return;
	}

	cout << std::fixed << std::setprecision(2);

	for (const TrendPoint& point : points)
	{
		int barLength = static_cast<int>(point.mean / 100.0 * barWidth);

		cout << "  " << point.semester << " | "
			<< std::left << setw(barWidth) << string(barLength, '#')
			<< std::right << " mean " << setw(6) << point.mean
			<< "  rolling " << setw(6) << point.rollingMean
			<< "  (" << point.records << " records)\n";
	}

	cout.unsetf(std::ios::floatfield);
	cout << std::setprecision(6);
}

void ReportGenerator::printRankings(
	const std::vector<RankedStudent>& ranked,
	std::size_t topN,
	const string& title) const
{
	cout << "\n===== " << title << " =====\n";

	cout << std::fixed << std::setprecision(2);

	for (std::size_t i = 0; i < ranked.size() && i < topN; ++i)
	{
		const RankedStudent& entry = ranked[i];

		cout << setw(3) << entry.rank << ". "
			<< std::left << setw(22) << entry.student->getName()
			<< std::right
			<< " avg " << setw(6) << entry.average
			<< "  GPA " << setw(4) << std::setprecision(1) << entry.gpa
			<< std::setprecision(2)
			<< "  percentile " << setw(6) << entry.percentile << '\n';
	}

	cout.unsetf(std::ios::floatfield);
	cout << std::setprecision(6);
}
