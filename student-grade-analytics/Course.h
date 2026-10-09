#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

using std::runtime_error, std::cout, std::string;

struct Course {
	string m_courseName;
	string m_courseCode;
	int m_courseHours;

	Course(const string& courseName, const string& courseCode, int courseHours)
		: m_courseName(courseName), m_courseCode(courseCode), m_courseHours(courseHours)
	{
		if (m_courseName.empty())
			throw runtime_error("Course name cannot be empty");

		if (m_courseCode.empty())
			throw runtime_error("Course code cannot be empty");

		if (m_courseHours <= 0)
			throw runtime_error("Course hours must be greater than 0");
	}

	void print() const
	{
		cout << "Course: " << m_courseName
			<< " (" << m_courseCode << "), "
			<< m_courseHours << " credit hours\n";
	}
};