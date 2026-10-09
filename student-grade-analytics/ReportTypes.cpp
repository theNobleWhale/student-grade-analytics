#include "ReportTypes.h"

ScoreStats computeStats(const std::vector<GradeRecord>& records)
{
	Analytics analytics(records);

	ScoreStats stats;
	stats.records = analytics.getRecordCount();
	stats.mean = analytics.calculateMean();
	stats.median = analytics.calculateMedian();
	stats.highest = analytics.getHighestScore();
	stats.lowest = analytics.getLowestScore();
	stats.passRate = analytics.calculatePassRate();
	stats.modes = analytics.calculateModes();
	stats.gradeCounts = analytics.countGrades();

	return stats;
}

void to_json(ordered_json& j, const ScoreStats& stats)
{
	j = ordered_json::object();
	j["records"] = stats.records;
	j["mean"] = stats.mean;
	j["median"] = stats.median;
	j["highest"] = stats.highest;
	j["lowest"] = stats.lowest;
	j["passRate"] = stats.passRate;
	j["modes"] = stats.modes;

	ordered_json counts = ordered_json::object();

	for (const auto& [grade, count] : stats.gradeCounts)
		counts[gradeToString(grade)] = count;

	j["gradeCounts"] = counts;
}

void to_json(ordered_json& j, const TrendPoint& point)
{
	j = ordered_json::object();
	j["semester"] = point.semester;
	j["records"] = point.records;
	j["mean"] = point.mean;
	j["rollingMean"] = point.rollingMean;
}

void to_json(ordered_json& j, const CourseStudentEntry& entry)
{
	j = ordered_json::object();
	j["name"] = entry.name;
	j["id"] = entry.id;
	j["score"] = entry.score;
	j["grade"] = entry.grade;
}

void to_json(ordered_json& j, const CourseReport& course)
{
	j = ordered_json::object();
	j["courseName"] = course.courseName;
	j["courseCode"] = course.courseCode;
	j["creditHours"] = course.creditHours;
	j.update(ordered_json(course.stats));

	if (!course.students.empty())
		j["students"] = course.students;
}

void to_json(ordered_json& j, const GroupReport& group)
{
	j = ordered_json::object();
	j["students"] = group.students;
	j.update(ordered_json(group.stats));
}

void to_json(ordered_json& j, const RankedEntry& entry)
{
	j = ordered_json::object();
	j["rank"] = entry.rank;
	j["name"] = entry.name;
	j["id"] = entry.id;
	j["major"] = entry.major;
	j["year"] = entry.year;
	j["average"] = entry.average;
	j["gpa"] = entry.gpa;
	j["percentile"] = entry.percentile;
}

void to_json(ordered_json& j, const AnalyticsReport& report)
{
	j = ordered_json::object();
	j["overall"] = report.overall;

	j["semesters"] = ordered_json::object();
	for (const auto& [semester, stats] : report.semesters)
		j["semesters"][semester] = stats;

	j["semesterTrend"] = report.semesterTrend;

	j["studentTrends"] = ordered_json::object();
	for (const auto& [label, count] : report.studentTrends)
		j["studentTrends"][label] = count;

	j["courses"] = ordered_json::object();
	for (const CourseReport& course : report.courses)
		j["courses"][course.courseCode] = course;

	j["majors"] = ordered_json::object();
	for (const GroupReport& group : report.majors)
		j["majors"][group.key] = group;

	j["years"] = ordered_json::object();
	for (const GroupReport& group : report.years)
		j["years"][group.key] = group;

	j["topPerformers"] = report.topPerformers;
}
