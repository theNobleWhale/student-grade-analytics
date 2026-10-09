#pragma once

#include "Student.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

using std::string, std::unordered_map, std::vector;

struct StudentGrade {
	string studentName;
	string studentId;
	GradeRecord gradeRecord;
};

/// One row of the overall student ranking.
/// `student` points into the StudentManager that produced it, so it is only
/// valid while that manager is alive and unmodified.
struct RankedStudent {
	const Student* student;
	double average;
	double gpa;
	int rank;          // competition ranking: ties share a rank (1, 2, 2, 4)
	double percentile; // % of ranked students scoring at or below this average
};

class StudentManager {
private:
	unordered_map<string, Student> m_students;

public:
	void addStudent(const Student& student);

	const unordered_map<string, Student>& getStudents() const;

	vector<Student> getStudentList() const;

	vector<GradeRecord> getAllGradeRecords() const;

	const Student* findStudentById(const string& studentId) const;

	Student* findStudentById(const string& studentId);

	const Student* findStudentByName(const string& studentName) const;

	vector<Student> rankStudents() const;

	unordered_map<string, vector<string>> groupByMajor() const;

	// defaultdict(list) equivalent: operator[] default-constructs the vector
	// the first time a year is seen.
	unordered_map<int, vector<string>> groupByYear() const;

	vector<int> getYears() const;

	vector<string> getCourseCodes() const;

	// Ranks every student that has at least one grade, best first, with
	// percentile rank. Averages are computed once (not inside the sort).
	vector<RankedStudent> rankStudentsWithPercentile() const;

	vector<GradeRecord> getGradesForYear(int year) const;

	vector<string> getMajors() const;

	vector<string> getSemesters() const;

	vector<GradeRecord> getGradesForMajor(
		const string& majorName) const;

	vector<GradeRecord> getGradesForSemester(
		const string& semester) const;

	vector<GradeRecord> getGradesForCourse(
		const string& courseCode) const;

	vector<GradeRecord> getGradesForStudent(
		const string& studentId) const;

	vector<StudentGrade> getStudentGradesForCourse(
		const string& courseCode) const;

	StudentGrade getHighestCoursePerformer(
		const string& courseCode) const;

	StudentGrade getLowestCoursePerformer(
		const string& courseCode) const;

	vector<StudentGrade> rankStudentsInCourse(
		const string& courseCode) const;
};