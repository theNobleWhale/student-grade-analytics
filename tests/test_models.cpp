// Data models, Optional/Union equivalents, validation.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "Grade.h"
#include "ScoreInput.h"

TEST(grade_boundaries_map_to_letters)
{
	CHECK(scoreToGrade(90) == Grade::A);
	CHECK(scoreToGrade(89) == Grade::B);
	CHECK(scoreToGrade(80) == Grade::B);
	CHECK(scoreToGrade(70) == Grade::C);
	CHECK(scoreToGrade(60) == Grade::D);
	CHECK(scoreToGrade(59) == Grade::F);
}

TEST(optional_grade_rejects_out_of_range_scores)
{
	CHECK(!tryScoreToGrade(-1).has_value());
	CHECK(!tryScoreToGrade(101).has_value());
	CHECK(tryScoreToGrade(95).value() == Grade::A);
}

TEST(optional_parse_grade_is_case_insensitive)
{
	CHECK(parseGrade("a").value() == Grade::A);
	CHECK(parseGrade("F").value() == Grade::F);
	CHECK(!parseGrade("E").has_value());
	CHECK(!parseGrade("AA").has_value());
	CHECK(!parseGrade("").has_value());
}

TEST(variant_score_input_accepts_int_double_and_string)
{
	CHECK_EQ(toScore(ScoreInput{ 85 }).value(), 85);
	CHECK_EQ(toScore(ScoreInput{ 84.6 }).value(), 85);
	CHECK_EQ(toScore(ScoreInput{ std::string("  72 ") }).value(), 72);
	CHECK_EQ(toScore(ScoreInput{ std::string("91.5") }).value(), 92);
}

TEST(variant_score_input_rejects_bad_values)
{
	CHECK(!toScore(ScoreInput{ 101 }).has_value());
	CHECK(!toScore(ScoreInput{ -5 }).has_value());
	CHECK(!toScore(ScoreInput{ std::string("abc") }).has_value());
	CHECK(!toScore(ScoreInput{ std::string("") }).has_value());
	CHECK(!toScore(ScoreInput{ std::string("90x") }).has_value());
	CHECK(!toScore(ScoreInput{ std::nan("") }).has_value());
}

TEST(course_validates_fields)
{
	CHECK_THROWS(Course("", "CS1", 3), std::runtime_error);
	CHECK_THROWS(Course("Algo", "", 3), std::runtime_error);
	CHECK_THROWS(Course("Algo", "CS1", 0), std::runtime_error);
}

TEST(grade_record_validates_score_and_semester)
{
	Course course("Algo", "CS1", 3);
	CHECK_THROWS(GradeRecord(course, -1, "2026S1"), std::runtime_error);
	CHECK_THROWS(GradeRecord(course, 101, "2026S1"), std::runtime_error);
	CHECK_THROWS(GradeRecord(course, 50, ""), std::runtime_error);
}

TEST(student_validates_fields)
{
	CHECK_THROWS(Student("", "1", "CS", 1), std::runtime_error);
	CHECK_THROWS(Student("A", "", "CS", 1), std::runtime_error);
	CHECK_THROWS(Student("A", "1", "", 1), std::runtime_error);
	CHECK_THROWS(Student("A", "1", "CS", 0), std::runtime_error);
	CHECK_THROWS(Student("A", "1", "CS", 7), std::runtime_error);
}

TEST(student_average_weighted_average_and_gpa)
{
	Course big("Big", "B1", 4);
	Course small("Small", "S1", 1);

	Student student("A", "1", "CS", 1);
	student.addGrade(GradeRecord(big, 90, "2026S1"));   // A, 4 credits
	student.addGrade(GradeRecord(small, 60, "2026S1")); // D, 1 credit

	CHECK_NEAR(student.calculateAverage(), 75.0, 1e-9);
	CHECK_NEAR(student.calculateWeightedAverage(), 84.0, 1e-9);
	CHECK_NEAR(student.calculateGPA(), (4.0 * 4 + 1.0 * 1) / 5, 1e-9);
}

TEST(student_without_grades_has_zero_statistics)
{
	Student student("A", "1", "CS", 1);
	CHECK_NEAR(student.calculateAverage(), 0.0, 1e-9);
	CHECK_NEAR(student.calculateGPA(), 0.0, 1e-9);
}
