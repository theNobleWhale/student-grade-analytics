// Statistics, ranking and percentile calculations.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "Analytics.h"

TEST(mean_median_mode_basic)
{
	auto records = recordsFromScores({ 50, 60, 70, 70, 80, 90 });
	Analytics a(records);

	CHECK_NEAR(a.calculateMean(), 70.0, 1e-9);
	CHECK_NEAR(a.calculateMedian(), 70.0, 1e-9);
	CHECK(a.calculateModes() == std::vector<int>{ 70 });
	CHECK_EQ(a.getHighestScore(), 90);
	CHECK_EQ(a.getLowestScore(), 50);
}

TEST(median_of_even_and_odd_counts)
{
	auto even = recordsFromScores({ 10, 20, 30, 40 });
	auto odd = recordsFromScores({ 10, 20, 30 });

	CHECK_NEAR(Analytics(even).calculateMedian(), 25.0, 1e-9);
	CHECK_NEAR(Analytics(odd).calculateMedian(), 20.0, 1e-9);
}

TEST(mode_is_empty_when_nothing_repeats_and_multimodal_lists_all)
{
	auto unique = recordsFromScores({ 1, 2, 3 });
	auto bimodal = recordsFromScores({ 5, 5, 9, 9, 1 });

	CHECK(Analytics(unique).calculateModes().empty());
	CHECK(Analytics(bimodal).calculateModes() == (std::vector<int>{ 5, 9 }));
}

TEST(percentile_and_pass_rate)
{
	auto records = recordsFromScores({ 50, 60, 70, 70, 80, 90 });
	Analytics a(records);

	CHECK_NEAR(a.calculatePercentile(70), 4.0 / 6.0 * 100.0, 1e-9);
	CHECK_NEAR(a.calculatePercentile(100), 100.0, 1e-9);
	CHECK_NEAR(a.calculatePercentile(0), 0.0, 1e-9);
	CHECK_NEAR(a.calculatePassRate(), 5.0 / 6.0 * 100.0, 1e-9);
}

TEST(analytics_on_empty_input_is_safe)
{
	std::vector<GradeRecord> none;
	Analytics a(none);

	CHECK_NEAR(a.calculateMean(), 0.0, 1e-9);
	CHECK_NEAR(a.calculateMedian(), 0.0, 1e-9);
	CHECK(a.calculateModes().empty());
	CHECK_EQ(a.getRecordCount(), 0);
}

TEST(ranking_with_percentiles)
{
	StudentManager manager = makeSmallManager();
	auto ranked = manager.rankStudentsWithPercentile();

	CHECK_EQ(ranked.size(), size_t(3));
	CHECK_EQ(ranked[0].student->getName(), std::string("Ann"));
	CHECK_EQ(ranked[0].rank, 1);
	CHECK_NEAR(ranked[0].percentile, 100.0, 1e-9);
	CHECK_EQ(ranked[1].student->getName(), std::string("Bob"));
	CHECK_NEAR(ranked[1].percentile, 2.0 / 3.0 * 100.0, 1e-9);
	CHECK_EQ(ranked[2].rank, 3);
	CHECK_NEAR(ranked[2].percentile, 1.0 / 3.0 * 100.0, 1e-9);
}

TEST(ranking_ties_share_a_rank_and_gradeless_students_are_skipped)
{
	Course course("Algo", "CS1", 3);
	StudentManager manager;

	for (auto [id, score] : { std::pair<const char*, int>{"1", 90},
							 {"2", 90}, {"3", 70} })
	{
		Student s(std::string("S") + id, id, "CS", 1);
		s.addGrade(GradeRecord(course, score, "2026S1"));
		manager.addStudent(s);
	}

	manager.addStudent(Student("NoGrades", "4", "CS", 1));

	auto ranked = manager.rankStudentsWithPercentile();

	CHECK_EQ(ranked.size(), size_t(3));
	CHECK_EQ(ranked[0].rank, 1);
	CHECK_EQ(ranked[1].rank, 1);
	CHECK_EQ(ranked[2].rank, 3);
	CHECK_NEAR(ranked[0].percentile, 100.0, 1e-9);
	CHECK_NEAR(ranked[2].percentile, 100.0 / 3.0, 1e-9);
}

TEST(manager_rejects_duplicate_ids_and_finds_by_id)
{
	StudentManager manager = makeSmallManager();

	CHECK_THROWS(manager.addStudent(Student("Dup", "1", "CS", 1)),
		std::invalid_argument);
	CHECK(manager.findStudentById("2") != nullptr);
	CHECK(manager.findStudentById("999") == nullptr);
}

TEST(course_queries_and_performers)
{
	StudentManager manager = makeSmallManager();

	CHECK(manager.getCourseCodes() ==
		(std::vector<std::string>{ "CS202", "CS404" }));
	CHECK_EQ(manager.getGradesForCourse("CS202").size(), size_t(3));
	CHECK_EQ(manager.getHighestCoursePerformer("CS202").studentName,
		std::string("Ann"));
	CHECK_EQ(manager.getLowestCoursePerformer("CS202").studentName,
		std::string("Cy"));
	CHECK_THROWS(manager.getHighestCoursePerformer("NOPE"),
		std::runtime_error);
}
