#pragma once

#include "Analytics.h"
#include "Grade.h"
#include "GradeRecord.h"
#include "StudentManager.h"
#include "Trends.h"
#include "json.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

/// Preserves insertion order of keys (collections.OrderedDict equivalent).
/// Plain nlohmann::json sorts keys alphabetically.
using ordered_json = nlohmann::ordered_json;

/// Typed description of the JSON report (TypedDict equivalent). Building a
/// typed struct first and converting once means the report layout is
/// declared in one place and checked by the compiler.
struct ScoreStats {
	int records = 0;
	double mean = 0.0;
	double median = 0.0;
	int highest = 0;
	int lowest = 0;
	double passRate = 0.0;
	std::vector<int> modes;
	std::map<Grade, int> gradeCounts;
};

struct CourseStudentEntry {
	std::string name;
	std::string id;
	int score = 0;
	std::string grade;
};

struct CourseReport {
	std::string courseName;
	std::string courseCode;
	int creditHours = 0;
	ScoreStats stats;
	std::vector<CourseStudentEntry> students; // empty when details are off
};

struct GroupReport {
	std::string key;   // a major name or a year
	int students = 0;
	ScoreStats stats;
};

struct RankedEntry {
	int rank = 0;
	std::string name;
	std::string id;
	std::string major;
	int year = 0;
	double average = 0.0;
	double gpa = 0.0;
	double percentile = 0.0;
};

struct AnalyticsReport {
	ScoreStats overall;
	// vectors of pairs keep insertion order like an OrderedDict
	std::vector<std::pair<std::string, ScoreStats>> semesters;
	std::vector<TrendPoint> semesterTrend;
	std::vector<std::pair<std::string, int>> studentTrends;
	std::vector<CourseReport> courses;
	std::vector<GroupReport> majors;
	std::vector<GroupReport> years;
	std::vector<RankedEntry> topPerformers;
};

/// Computes every statistic in ScoreStats from a set of records.
ScoreStats computeStats(const std::vector<GradeRecord>& records);

void to_json(ordered_json& j, const ScoreStats& stats);
void to_json(ordered_json& j, const TrendPoint& point);
void to_json(ordered_json& j, const CourseStudentEntry& entry);
void to_json(ordered_json& j, const CourseReport& course);
void to_json(ordered_json& j, const GroupReport& group);
void to_json(ordered_json& j, const RankedEntry& entry);
void to_json(ordered_json& j, const AnalyticsReport& report);
