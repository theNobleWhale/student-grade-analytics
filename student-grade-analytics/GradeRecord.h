#pragma once

#include "Course.h"
#include "Grade.h"

#include <iostream>
#include <stdexcept>
#include <string>

using std::cout, std::runtime_error, std::string;

struct GradeRecord {
	Course m_course;
	int m_score;
	string m_semester;

	GradeRecord(const Course& course, int score, const string& semester)
		: m_course(course), m_score(score), m_semester(semester)
	{
		if (m_score < 0 || m_score > 100)
			throw runtime_error("Score must be between 0 and 100");

		if (m_semester.empty())
			throw runtime_error("Semester cannot be empty");
	}

	void print() const
	{
		cout << "Course: " << m_course.m_courseName
			<< " (" << m_course.m_courseCode << "), "
			<< m_course.m_courseHours << " credit hours\n";

		cout << "Score: " << m_score
			<< "  Grade: " << gradeToString(scoreToGrade(m_score))
			<< "  Semester: " << m_semester << "\n";
	}
};