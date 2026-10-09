#pragma once

#include "GradeRecord.h"

#include <string>
#include <vector>

using std::string, std::vector;

class Student {
private:
	string m_studentName;
	string m_studentId;
	string m_studentMajor;
	int m_studentYear;
	vector<GradeRecord> m_studentGrades;

public:
	Student(
		const string& studentName,
		const string& studentId,
		const string& studentMajor,
		int studentYear);

	const string& getName() const;
	const string& getId() const;
	const string& getMajor() const;
	int getYear() const;
	const vector<GradeRecord>& getGrades() const;

	void addGrade(const GradeRecord& grade);

	double calculateAverage() const;
	double calculateWeightedAverage() const;
	double calculateGPA() const;

	void print() const;
};