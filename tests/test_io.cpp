// CSV reading, error handling and JSON output.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "CSVReader.h"
#include "JSONReportGenerator.h"
#include "StreamingAnalyzer.h"

#include <sstream>

namespace {

const char* kCourses =
	"courseName,courseCode,creditHours\nAlgorithms,CS202,5\nDiscrete Math,CS404,3\n";

std::string slurp(const std::filesystem::path& path)
{
	std::ifstream in(path);
	std::stringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

} // namespace

TEST(split_line_handles_quotes_escapes_and_empty_fields)
{
	auto f = CSVReader::splitLine("a,\"b,c\",\"say \"\"hi\"\"\",,d");
	CHECK_EQ(f.size(), size_t(5));
	CHECK_EQ(f[1], std::string("b,c"));
	CHECK_EQ(f[2], std::string("say \"hi\""));
	CHECK_EQ(f[3], std::string(""));
	CHECK_EQ(f[4], std::string("d"));
	CHECK_EQ(CSVReader::splitLine("a,b,").size(), size_t(3));
}

TEST(missing_file_raises_file_not_found_error)
{
	CSVReader reader;
	std::vector<Course> courses;
	CHECK_THROWS(reader.loadCourses("definitely_missing_file.csv", courses),
		FileNotFoundError);
}

TEST(directory_instead_of_file_is_rejected)
{
	CSVReader reader;
	std::vector<Course> courses;
	CHECK_THROWS(
		reader.loadCourses(std::filesystem::temp_directory_path(), courses),
		std::runtime_error);
}

TEST(empty_file_and_missing_column_raise_format_error)
{
	CSVReader reader;
	std::vector<Course> courses;

	TempFile empty("gat_empty.csv", "");
	CHECK_THROWS(reader.loadCourses(empty.path(), courses), CsvFormatError);

	TempFile noColumn("gat_nocol.csv", "courseName,courseCode\nA,B\n");
	CHECK_THROWS(reader.loadCourses(noColumn.path(), courses), CsvFormatError);
}

TEST(columns_are_found_by_name_not_position)
{
	TempFile file("gat_reordered.csv",
		"creditHours,courseCode,courseName\n5,CS202,Algorithms\n");

	std::vector<Course> courses;
	LoadResult result = CSVReader().loadCourses(file.path(), courses);

	CHECK_EQ(result.rowsLoaded, size_t(1));
	CHECK_EQ(courses[0].m_courseCode, std::string("CS202"));
	CHECK_EQ(courses[0].m_courseHours, 5);
}

TEST(windows_line_endings_and_bom_do_not_leak_into_fields)
{
	TempFile courseFile("gat_crlf_courses.csv",
		std::string("\xEF\xBB\xBF") + "courseName,courseCode,creditHours\r\n"
		"Algorithms,CS202,5\r\n");
	TempFile studentFile("gat_crlf_students.csv",
		"name,id,major,year\r\nAnn,1,CS,2\r\n");
	TempFile gradeFile("gat_crlf_grades.csv",
		"studentId,courseCode,score,semester\r\n1,CS202,88,2026S1\r\n");

	std::vector<Course> courses;
	StudentManager manager;
	CSVReader reader(true);

	reader.loadCourses(courseFile.path(), courses);
	reader.loadStudents(studentFile.path(), manager);
	reader.loadGrades(gradeFile.path(), manager, courses);

	CHECK_EQ(manager.getSemesters().size(), size_t(1));
	CHECK_EQ(manager.getSemesters()[0], std::string("2026S1")); // no '\r'
	CHECK_EQ(manager.findStudentById("1")->getYear(), 2);
}

TEST(quoted_names_with_commas_load_correctly)
{
	TempFile file("gat_quoted.csv",
		"name,id,major,year\n\"Smith, Jane\",7,CS,1\n");

	StudentManager manager;
	CSVReader().loadStudents(file.path(), manager);

	CHECK_EQ(manager.findStudentById("7")->getName(),
		std::string("Smith, Jane"));
}

TEST(lenient_mode_skips_bad_rows_and_reports_them)
{
	TempFile courseFile("gat_len_courses.csv", kCourses);
	TempFile studentFile("gat_len_students.csv",
		"name,id,major,year\nAnn,1,CS,1\nAnn again,1,CS,1\nBad,2,CS,abc\n"
		"Short,3\n");

	std::vector<Course> courses;
	StudentManager manager;
	CSVReader reader;

	reader.loadCourses(courseFile.path(), courses);
	LoadResult students = reader.loadStudents(studentFile.path(), manager);

	CHECK_EQ(students.rowsRead, size_t(4));
	CHECK_EQ(students.rowsLoaded, size_t(1));
	CHECK_EQ(students.rowsSkipped, size_t(3));
	CHECK_EQ(students.errors.size(), size_t(3));

	TempFile gradeFile("gat_len_grades.csv",
		"studentId,courseCode,score,semester\n"
		"1,CS202,90,2026S1\n"
		"1,NOPE,90,2026S1\n"
		"99,CS202,90,2026S1\n"
		"1,CS404,abc,2026S1\n"
		"1,CS404,150,2026S1\n"
		"1,CS404,85.4,2026S2\n");

	LoadResult grades = reader.loadGrades(gradeFile.path(), manager, courses);

	CHECK_EQ(grades.rowsLoaded, size_t(2));
	CHECK_EQ(grades.rowsSkipped, size_t(4));
	CHECK_EQ(manager.findStudentById("1")->getGrades().size(), size_t(2));
}

TEST(strict_mode_throws_on_first_bad_row_with_line_number)
{
	TempFile file("gat_strict.csv", "name,id,major,year\nAnn,1,CS,1\nBad,2,CS,x\n");
	StudentManager manager;

	bool threw = false;
	try { CSVReader(true).loadStudents(file.path(), manager); }
	catch (const std::runtime_error& e)
	{
		threw = std::string(e.what()).find("line 3") != std::string::npos;
	}
	CHECK(threw);
}

TEST(streaming_analysis_matches_in_memory_analysis)
{
	TempFile courseFile("gat_st_courses.csv", kCourses);
	TempFile gradeFile("gat_st_grades.csv",
		"studentId,courseCode,score,semester\n"
		"1,CS202,90,2025S1\n2,CS202,70,2025S1\n3,CS404,55,2025S2\n"
		"1,CS404,70,2025S2\n2,UNKNOWN,70,2025S2\n");

	std::vector<Course> courses;
	CSVReader().loadCourses(courseFile.path(), courses);

	StreamingResult streamed = analyzeGradesStreaming(gradeFile.path(), courses);

	CHECK_EQ(streamed.load.rowsLoaded, size_t(4));
	CHECK_EQ(streamed.load.rowsSkipped, size_t(1));

	ScoreStats expected = computeStats(recordsFromScores({ 90, 70, 55, 70 }));
	ScoreStats actual = streamed.overall.toStats();

	CHECK_NEAR(actual.mean, expected.mean, 1e-9);
	CHECK_NEAR(actual.median, expected.median, 1e-9);
	CHECK(actual.modes == expected.modes);
	CHECK_EQ(streamed.byCourse.size(), size_t(2));
	CHECK_EQ(streamed.bySemester.size(), size_t(2));
	CHECK_EQ(streamed.bySemester[0].first, std::string("2025S1"));
}

TEST(json_report_is_written_parsed_and_ordered)
{
	StudentManager manager = makeSmallManager();
	auto path = std::filesystem::temp_directory_path()
		/ "gat_report_dir" / "nested" / "report.json";

	JSONReportGenerator().saveReport(path, manager); // creates directories

	ordered_json json = ordered_json::parse(slurp(path));

	CHECK_EQ(json["overall"]["records"].get<int>(), 4);
	CHECK_EQ(json["topPerformers"][0]["name"].get<std::string>(),
		std::string("Ann"));
	CHECK_EQ(json["topPerformers"][0]["rank"].get<int>(), 1);
	CHECK_EQ(json["students"].size(), size_t(3));
	CHECK_EQ(json["students"][0]["id"].get<std::string>(), std::string("1"));
	CHECK_EQ(json["majors"]["CS"]["students"].get<int>(), 2);
	CHECK_EQ(json["years"]["2"]["students"].get<int>(), 2);
	CHECK(json["courses"].contains("CS202"));
	CHECK(json["courses"].contains("CS404"));
	CHECK_EQ(json["studentTrends"]["declining"].get<int>(), 1);     // Ann: 90 -> 80
	CHECK_EQ(json["studentTrends"]["insufficient_data"].get<int>(), 2); // Bob, Cy: one semester

	std::filesystem::remove_all(path.parent_path().parent_path());
}

TEST(json_report_can_omit_student_details)
{
	StudentManager manager = makeSmallManager();
	auto path = std::filesystem::temp_directory_path() / "gat_nodetails.json";

	ReportOptions options;
	options.includeStudentDetails = false;
	JSONReportGenerator().saveReport(path, manager, options);

	ordered_json json = ordered_json::parse(slurp(path));
	CHECK(!json.contains("students"));
	CHECK(!json["courses"]["CS202"].contains("students"));

	std::filesystem::remove(path);
}

TEST(json_report_to_unwritable_path_raises_permission_error)
{
	StudentManager manager = makeSmallManager();

	// A path whose "parent" is an existing regular file cannot be created.
	TempFile blocker("gat_blocker", "x");
	auto bad = blocker.path() / "report.json";

	CHECK_THROWS(JSONReportGenerator().saveReport(bad, manager),
		FilePermissionError);
}
