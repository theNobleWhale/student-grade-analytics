#include "JSONReportGenerator.h"

#include "CSVReader.h"

#include <algorithm>
#include <fstream>
#include <system_error>
#include <unordered_map>

namespace fs = std::filesystem;

using std::string;
using std::vector;

AnalyticsReport JSONReportGenerator::buildReport(
	const StudentManager& studentManager,
	const ReportOptions& options) const
{
	AnalyticsReport report;

	const vector<GradeRecord> allGrades =
		studentManager.getAllGradeRecords();

	// OVERALL
	report.overall = computeStats(allGrades);

	// SEMESTERS (chronological)
	for (const string& semester : studentManager.getSemesters())
	{
		report.semesters.emplace_back(
			semester,
			computeStats(studentManager.getGradesForSemester(semester)));
	}

	// TRENDS
	report.semesterTrend = computeTrend(allGrades, options.trendWindow);

	auto trendCounts = countStudentTrends(
		studentManager, options.trendWindow, options.trendTolerance);

	for (TrendDirection direction : {
		TrendDirection::Improving,
		TrendDirection::Declining,
		TrendDirection::Stable,
		TrendDirection::Insufficient })
	{
		auto it = trendCounts.find(direction);

		report.studentTrends.emplace_back(
			trendToString(direction),
			it == trendCounts.end() ? 0 : it->second);
	}

	// COURSES (discovered from the data, not hardcoded)
	for (const string& courseCode : studentManager.getCourseCodes())
	{
		vector<GradeRecord> courseGrades =
			studentManager.getGradesForCourse(courseCode);

		if (courseGrades.empty())
			continue;

		CourseReport course;
		course.courseName = courseGrades[0].m_course.m_courseName;
		course.courseCode = courseCode;
		course.creditHours = courseGrades[0].m_course.m_courseHours;
		course.stats = computeStats(courseGrades);

		if (options.includeStudentDetails)
		{
			vector<StudentGrade> entries =
				studentManager.getStudentGradesForCourse(courseCode);

			// unordered_map iteration order is unspecified; sort for a
			// reproducible report.
			std::stable_sort(entries.begin(), entries.end(),
				[](const StudentGrade& a, const StudentGrade& b)
				{
					return a.studentId < b.studentId;
				});

			course.students.reserve(entries.size());

			for (const StudentGrade& entry : entries)
			{
				course.students.push_back({
					entry.studentName,
					entry.studentId,
					entry.gradeRecord.m_score,
					gradeToString(scoreToGrade(entry.gradeRecord.m_score)) });
			}
		}

		report.courses.push_back(std::move(course));
	}

	// MAJORS (defaultdict-style grouping)
	auto byMajor = studentManager.groupByMajor();

	for (const string& major : studentManager.getMajors())
	{
		report.majors.push_back({
			major,
			static_cast<int>(byMajor[major].size()),
			computeStats(studentManager.getGradesForMajor(major)) });
	}

	// YEARS
	auto byYear = studentManager.groupByYear();

	for (int year : studentManager.getYears())
	{
		report.years.push_back({
			std::to_string(year),
			static_cast<int>(byYear[year].size()),
			computeStats(studentManager.getGradesForYear(year)) });
	}

	// TOP PERFORMERS
	vector<RankedStudent> ranked =
		studentManager.rankStudentsWithPercentile();

	for (std::size_t i = 0; i < ranked.size() && i < options.topN; ++i)
	{
		const RankedStudent& entry = ranked[i];

		report.topPerformers.push_back({
			entry.rank,
			entry.student->getName(),
			entry.student->getId(),
			entry.student->getMajor(),
			entry.student->getYear(),
			entry.average,
			entry.gpa,
			entry.percentile });
	}

	return report;
}

void JSONReportGenerator::saveReport(
	const fs::path& filename,
	const StudentManager& studentManager,
	const ReportOptions& options) const
{
	ordered_json json = buildReport(studentManager, options);

	if (options.includeStudentDetails)
	{
		// rank / percentile lookup by student ID
		vector<RankedStudent> ranked =
			studentManager.rankStudentsWithPercentile();

		std::unordered_map<string, const RankedStudent*> rankById;

		for (const RankedStudent& entry : ranked)
			rankById.emplace(entry.student->getId(), &entry);

		// Sorted by ID for a reproducible file.
		vector<const Student*> students;

		for (const auto& [studentId, student] : studentManager.getStudents())
			students.push_back(&student);

		std::sort(students.begin(), students.end(),
			[](const Student* a, const Student* b)
			{
				return a->getId() < b->getId();
			});

		json["students"] = ordered_json::array();

		for (const Student* student : students)
		{
			ordered_json data = ordered_json::object();

			data["name"] = student->getName();
			data["id"] = student->getId();
			data["major"] = student->getMajor();
			data["year"] = student->getYear();
			data["average"] = student->calculateAverage();
			data["weightedAverage"] = student->calculateWeightedAverage();
			data["gpa"] = student->calculateGPA();

			auto rankIt = rankById.find(student->getId());

			if (rankIt != rankById.end())
			{
				data["rank"] = rankIt->second->rank;
				data["percentile"] = rankIt->second->percentile;
			}

			data["grades"] = ordered_json::array();

			for (const GradeRecord& record : student->getGrades())
			{
				ordered_json grade = ordered_json::object();

				grade["courseName"] = record.m_course.m_courseName;
				grade["courseCode"] = record.m_course.m_courseCode;
				grade["creditHours"] = record.m_course.m_courseHours;
				grade["score"] = record.m_score;
				grade["grade"] =
					gradeToString(scoreToGrade(record.m_score));
				grade["semester"] = record.m_semester;

				data["grades"].push_back(std::move(grade));
			}

			json["students"].push_back(std::move(data));
		}
	}

	std::error_code ec;

	if (filename.has_parent_path())
		fs::create_directories(filename.parent_path(), ec);

	std::ofstream file(filename);

	if (!file)
		throw FilePermissionError(
			"Could not create JSON report file: " + filename.string());

	file << json.dump(4);

	if (!file)
		throw std::runtime_error(
			"Failed while writing JSON report: " + filename.string());
}
