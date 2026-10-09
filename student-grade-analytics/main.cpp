#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "Course.h"
#include "Grade.h"
#include "GradeRecord.h"
#include "Student.h"

#include "CSVReader.h"
#include "StudentManager.h"

#include "Analytics.h"
#include "MemoryBenchmark.h"
#include "StreamingAnalyzer.h"
#include "Trends.h"

#include "JSONReportGenerator.h"
#include "ReportGenerator.h"

namespace fs = std::filesystem;

using std::cout, std::cerr, std::string, std::vector;

namespace {

struct Options {
	fs::path studentsFile = "students.csv";
	fs::path gradesFile = "grades.csv";
	fs::path coursesFile = "courses.csv";
	fs::path reportFile = "analytics_report.json";
	string lookupId = "9032104";
	bool benchmark = false;
	bool streaming = false;
	bool strict = false;
	bool noStudentDetails = false;
	bool help = false;
};

void printUsage()
{
	cout <<
		"Usage: student-grade-analytics [students.csv] [grades.csv] "
		"[courses.csv] [report.json] [options]\n\n"
		"Options:\n"
		"  --student <id>       student to look up (default 9032104)\n"
		"  --strict             stop at the first bad CSV row instead of skipping it\n"
		"  --no-student-details leave per-student grades out of the JSON report\n"
		"  --stream             also analyse grades.csv row-by-row (constant memory)\n"
		"  --benchmark          print the memory / performance comparison\n"
		"  --help               show this message\n";
}

Options parseArguments(int argc, char* argv[])
{
	Options options;
	vector<string> positional;

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];

		if (arg == "--help") options.help = true;
		else if (arg == "--benchmark") options.benchmark = true;
		else if (arg == "--stream") options.streaming = true;
		else if (arg == "--strict") options.strict = true;
		else if (arg == "--no-student-details") options.noStudentDetails = true;
		else if (arg == "--student")
		{
			if (i + 1 >= argc)
				throw std::runtime_error("--student needs an ID");

			options.lookupId = argv[++i];
		}
		else if (arg.rfind("--", 0) == 0)
			throw std::runtime_error("Unknown option: " + arg);
		else
			positional.push_back(arg);
	}

	if (positional.size() > 0) options.studentsFile = positional[0];
	if (positional.size() > 1) options.gradesFile = positional[1];
	if (positional.size() > 2) options.coursesFile = positional[2];
	if (positional.size() > 3) options.reportFile = positional[3];

	return options;
}

void printLoadResult(const string& label, const LoadResult& result)
{
	cout << label << ": " << result.rowsLoaded << " loaded";

	if (result.rowsSkipped > 0)
		cout << ", " << result.rowsSkipped << " skipped";

	cout << '\n';

	for (const string& error : result.errors)
		cout << "  warning: " << error << '\n';

	if (result.rowsSkipped > result.errors.size())
		cout << "  ... and "
		<< (result.rowsSkipped - result.errors.size())
		<< " more skipped rows\n";
}

double secondsSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double>(
		std::chrono::steady_clock::now() - start).count();
}

int run(int argc, char* argv[])
{
	Options options = parseArguments(argc, argv);

	if (options.help)
	{
		printUsage();
		return 0;
	}

	auto programStart = std::chrono::steady_clock::now();

	// ---------- LOAD ----------
	CSVReader csvReader(options.strict);
	StudentManager studentManager;
	vector<Course> courses;

	cout << "===== LOADING DATA =====\n";

	printLoadResult("Courses",
		csvReader.loadCourses(options.coursesFile, courses));

	printLoadResult("Students",
		csvReader.loadStudents(options.studentsFile, studentManager));

	printLoadResult("Grades",
		csvReader.loadGrades(options.gradesFile, studentManager, courses));

	cout << "Load time: " << secondsSince(programStart) << " s\n";

	vector<GradeRecord> gradeRecords = studentManager.getAllGradeRecords();

	cout << "\n===== DATA LOADED =====\n";
	cout << "Students: " << studentManager.getStudents().size() << '\n';
	cout << "Grade records: " << gradeRecords.size() << '\n';

	// ---------- OVERALL ----------
	Analytics analytics(gradeRecords);
	ReportGenerator reportGenerator;

	reportGenerator.printAnalytics(analytics, "OVERALL ANALYTICS");
	reportGenerator.printGradeChart(
		analytics.countGrades(), "Overall grade distribution");

	// ---------- STUDENT LOOKUP ----------
	vector<RankedStudent> ranked =
		studentManager.rankStudentsWithPercentile();

	const Student* foundStudent =
		studentManager.findStudentById(options.lookupId);

	cout << "\n===== STUDENT LOOKUP =====\n";

	if (foundStudent == nullptr)
	{
		cout << "No student with ID " << options.lookupId << '\n';
	}
	else
	{
		cout << "Found: " << foundStudent->getName() << '\n';
		cout << "ID: " << foundStudent->getId() << '\n';
		cout << "Major: " << foundStudent->getMajor() << '\n';
		cout << "Average: " << foundStudent->calculateAverage() << '\n';
		cout << "GPA: " << foundStudent->calculateGPA() << '\n';

		for (const RankedStudent& entry : ranked)
		{
			if (entry.student == foundStudent)
			{
				cout << "Rank: " << entry.rank << " of " << ranked.size()
					<< " (percentile " << entry.percentile << ")\n";
				break;
			}
		}

		auto studentTrend = computeTrend(foundStudent->getGrades());
		cout << "Trend: "
			<< trendToString(classifyTrend(studentTrend)) << '\n';
	}

	// ---------- COURSES ----------
	for (const string& courseCode : studentManager.getCourseCodes())
	{
		vector<GradeRecord> courseGrades =
			studentManager.getGradesForCourse(courseCode);

		if (courseGrades.empty()) continue;

		Analytics courseAnalytics(courseGrades);

		string courseTitle =
			"COURSE ANALYTICS: "
			+ courseGrades[0].m_course.m_courseName
			+ " (" + courseCode + ")";

		reportGenerator.printAnalytics(courseAnalytics, courseTitle);
		reportGenerator.printGradeChart(
			courseAnalytics.countGrades(), courseCode + " grade distribution");

		auto courseRanking = studentManager.rankStudentsInCourse(courseCode);

		cout << "\n----- Top " << courseCode << " scores -----\n";

		for (std::size_t i = 0; i < courseRanking.size() && i < 10; ++i)
		{
			cout << (i + 1) << ". "
				<< courseRanking[i].studentName << " - "
				<< courseRanking[i].gradeRecord.m_score << " ("
				<< gradeToString(scoreToGrade(
					courseRanking[i].gradeRecord.m_score)) << ")\n";
		}
	}

	// ---------- RANKINGS ----------
	reportGenerator.printRankings(ranked, 10, "TOP STUDENTS");

	// ---------- GROUPING ----------
	cout << "\n===== STUDENTS BY YEAR =====\n";

	auto byYear = studentManager.groupByYear();

	for (int year : studentManager.getYears())
		cout << "Year " << year << ": " << byYear[year].size()
		<< " students\n";

	// ---------- MAJORS ----------
	for (const string& majorName : studentManager.getMajors())
	{
		vector<GradeRecord> majorGrades =
			studentManager.getGradesForMajor(majorName);

		if (majorGrades.empty()) continue;

		Analytics majorAnalytics(majorGrades);
		reportGenerator.printAnalytics(majorAnalytics, majorName + " ANALYTICS");
	}

	// ---------- SEMESTERS ----------
	for (const string& semester : studentManager.getSemesters())
	{
		vector<GradeRecord> semesterGrades =
			studentManager.getGradesForSemester(semester);

		if (semesterGrades.empty()) continue;

		Analytics semesterAnalytics(semesterGrades);
		reportGenerator.printAnalytics(
			semesterAnalytics, semester + " ANALYTICS");
	}

	// ---------- TRENDS ----------
	reportGenerator.printTrend(
		computeTrend(gradeRecords), "Semester trend (rolling window = 2)");

	cout << "\n----- Student trend summary -----\n";

	for (const auto& [direction, count] : countStudentTrends(studentManager))
		cout << "  " << trendToString(direction) << ": " << count << '\n';

	// ---------- STREAMING (optional) ----------
	if (options.streaming)
	{
		auto start = std::chrono::steady_clock::now();

		StreamingResult streamed =
			analyzeGradesStreaming(options.gradesFile, courses, csvReader);

		ScoreStats stats = streamed.overall.toStats();

		cout << "\n===== STREAMING ANALYSIS (constant memory) =====\n";
		cout << "Rows processed: " << streamed.load.rowsLoaded << '\n';
		cout << "Mean: " << stats.mean << "  Median: " << stats.median
			<< "  Pass rate: " << stats.passRate << "%\n";
		cout << "Time: " << secondsSince(start) << " s\n";
	}

	// ---------- JSON ----------
	ReportOptions reportOptions;
	reportOptions.includeStudentDetails = !options.noStudentDetails;

	JSONReportGenerator jsonReportGenerator;
	jsonReportGenerator.saveReport(
		options.reportFile, studentManager, reportOptions);

	cout << "\n===== REPORT =====\n";
	cout << "JSON report written to " << options.reportFile.string() << '\n';

	// ---------- MEMORY BENCHMARK (optional) ----------
	if (options.benchmark)
		runMemoryBenchmark(gradeRecords.size(), cout);

	cout << "\nProgram completed successfully in "
		<< secondsSince(programStart) << " s.\n";

	return 0;
}

} // namespace

int main(int argc, char* argv[])
{
	try
	{
		return run(argc, argv);
	}
	catch (const FileNotFoundError& error)
	{
		cerr << "Error: " << error.what() << '\n';
		return 2;
	}
	catch (const FilePermissionError& error)
	{
		cerr << "Error: " << error.what() << '\n';
		return 3;
	}
	catch (const std::exception& error)
	{
		cerr << "Error: " << error.what() << '\n';
		return 1;
	}
}
