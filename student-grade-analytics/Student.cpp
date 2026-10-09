#include "Student.h"

#include <iostream>
#include <stdexcept>

using std::cout, std::runtime_error;

Student::Student(const string& studentName, const string& studentId,
	const string& studentMajor, int studentYear)
	: m_studentName(studentName), m_studentId(studentId),
	m_studentMajor(studentMajor), m_studentYear(studentYear)
{
	if (m_studentName.empty())
		throw runtime_error("Student name cannot be empty");

	if (m_studentId.empty())
		throw runtime_error("Student ID cannot be empty");

	if (m_studentMajor.empty())
		throw runtime_error("Student major cannot be empty");

	if (m_studentYear < 1 || m_studentYear > 6)
		throw runtime_error("Student year must be between 1 and 6");
}

const string& Student::getName() const
{
	return m_studentName;
}

const string& Student::getId() const
{
	return m_studentId;
}

const string& Student::getMajor() const
{
	return m_studentMajor;
}

int Student::getYear() const
{
	return m_studentYear;
}

const vector<GradeRecord>& Student::getGrades() const
{
	return m_studentGrades;
}

void Student::addGrade(const GradeRecord& grade)
{
	m_studentGrades.push_back(grade);
}

double Student::calculateAverage() const
{
	if (m_studentGrades.empty()) return 0.0;

	double totalScore = 0.0;

	for (const GradeRecord& grade : m_studentGrades)
		totalScore += grade.m_score;

	return totalScore / m_studentGrades.size();
}

double Student::calculateWeightedAverage() const
{
	if (m_studentGrades.empty()) return 0.0;

	double weightedTotal = 0.0;
	int totalCreditHours = 0;

	for (const GradeRecord& grade : m_studentGrades)
	{
		weightedTotal +=
			grade.m_score * grade.m_course.m_courseHours;

		totalCreditHours += grade.m_course.m_courseHours;
	}

	if (totalCreditHours == 0) return 0.0;

	return weightedTotal / totalCreditHours;
}

double Student::calculateGPA() const
{
	if (m_studentGrades.empty()) return 0.0;

	double totalGradePoints = 0.0;
	int totalCreditHours = 0;

	for (const GradeRecord& grade : m_studentGrades)
	{
		Grade studentGrade = scoreToGrade(grade.m_score);

		totalGradePoints +=
			gradeToPoint(studentGrade) *
			grade.m_course.m_courseHours;

		totalCreditHours += grade.m_course.m_courseHours;
	}

	if (totalCreditHours == 0)  return 0.0;

	return totalGradePoints / totalCreditHours;
}

void Student::print() const
{
	cout << "Name: " << m_studentName << "\n";
	cout << "ID: " << m_studentId << "\n";
	cout << "Major: " << m_studentMajor << "\n";
	cout << "Year: " << m_studentYear << "\n";

	for (const GradeRecord& grade : m_studentGrades)
	{
		cout << "\n";
		grade.print();
	}
}