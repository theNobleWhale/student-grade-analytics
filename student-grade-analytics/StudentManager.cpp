#include "StudentManager.h"

#include <algorithm>
#include <set>
#include <stdexcept>

using std::invalid_argument;
using std::runtime_error;

void StudentManager::addStudent(const Student& student)
{
	auto result = m_students.emplace(student.getId(), student);

	if (!result.second)
		throw invalid_argument("Student ID already exists");
}

const unordered_map<string, Student>& StudentManager::getStudents() const
{
	return m_students;
}

vector<Student> StudentManager::getStudentList() const
{
	vector<Student> students;

	for (const auto& [studentId, student] : m_students)
		students.push_back(student);

	return students;
}

vector<GradeRecord> StudentManager::getAllGradeRecords() const
{
	vector<GradeRecord> allGradeRecords;

	for (const auto& [studentId, student] : m_students)
	{
		const auto& records = student.getGrades();

		allGradeRecords.insert(
			allGradeRecords.end(),
			records.begin(),
			records.end());
	}

	return allGradeRecords;
}

const Student* StudentManager::findStudentById(
	const string& studentId) const
{
	auto it = m_students.find(studentId);

	if (it != m_students.end())
		return &it->second;

	return nullptr;
}

Student* StudentManager::findStudentById(
	const string& studentId)
{
	auto it = m_students.find(studentId);

	if (it != m_students.end())
		return &it->second;

	return nullptr;
}

const Student* StudentManager::findStudentByName(
	const string& studentName) const
{
	for (const auto& student : m_students)
	{
		if (student.second.getName() == studentName)
			return &student.second;
	}

	return nullptr;
}

vector<Student> StudentManager::rankStudents() const
{
	vector<Student> rankedStudents = getStudentList();

	std::sort(rankedStudents.begin(), rankedStudents.end(),
		[](const Student& firstStudent, const Student& secondStudent)
		{
			return firstStudent.calculateAverage()
				> secondStudent.calculateAverage();
		});

	return rankedStudents;
}

unordered_map<string, vector<string>> StudentManager::groupByMajor() const
{
	unordered_map<string, vector<string>> majorToNames;

	for (const auto& [studentId, student] : m_students)
		majorToNames[student.getMajor()].push_back(
			student.getName());

	return majorToNames;
}

vector<string> StudentManager::getMajors() const
{
	vector<string> majors;

	for (const auto& [studentId, student] : m_students)
	{
		const string& major = student.getMajor();

		if (std::find(
			majors.begin(),
			majors.end(),
			major) == majors.end())
		{
			majors.push_back(major);
		}
	}

	std::sort(majors.begin(), majors.end());

	return majors;
}

vector<string> StudentManager::getSemesters() const
{
	vector<string> semesters;

	for (const auto& [studentId, student] : m_students)
	{
		for (const GradeRecord& record : student.getGrades())
		{
			const string& semester = record.m_semester;

			if (std::find(
				semesters.begin(),
				semesters.end(),
				semester) == semesters.end())
			{
				semesters.push_back(semester);
			}
		}
	}

	std::sort(semesters.begin(), semesters.end());

	return semesters;
}

vector<GradeRecord> StudentManager::getGradesForMajor(
	const string& majorName) const
{
	vector<GradeRecord> majorGrades;

	for (const auto& [studentId, student] : m_students)
	{
		if (student.getMajor() != majorName)
			continue;

		for (const GradeRecord& record : student.getGrades())
			majorGrades.push_back(record);
	}

	return majorGrades;
}

vector<GradeRecord> StudentManager::getGradesForSemester(
	const string& semester) const
{
	vector<GradeRecord> semesterGrades;

	for (const auto& [studentId, student] : m_students)
	{
		for (const GradeRecord& record : student.getGrades())
		{
			if (record.m_semester == semester)
				semesterGrades.push_back(record);
		}
	}

	return semesterGrades;
}

vector<GradeRecord> StudentManager::getGradesForCourse(
	const string& courseCode) const
{
	vector<GradeRecord> courseGrades;

	for (const auto& [studentId, student] : m_students)
	{
		for (const auto& gradeRecord : student.getGrades())
		{
			if (gradeRecord.m_course.m_courseCode == courseCode)
				courseGrades.push_back(gradeRecord);
		}
	}

	return courseGrades;
}

vector<GradeRecord> StudentManager::getGradesForStudent(
	const string& studentId) const
{
	vector<GradeRecord> studentGrades;

	const Student* student = findStudentById(studentId);

	if (student == nullptr)
		return studentGrades;

	for (const GradeRecord& record : student->getGrades())
		studentGrades.push_back(record);

	return studentGrades;
}

vector<StudentGrade> StudentManager::getStudentGradesForCourse(
	const string& courseCode) const
{
	vector<StudentGrade> studentGrades;

	for (const auto& [studentId, student] : m_students)
	{
		for (const GradeRecord& record : student.getGrades())
		{
			if (record.m_course.m_courseCode == courseCode)
			{
				StudentGrade studentGrade{
					student.getName(), student.getId(), record
				};

				studentGrades.push_back(studentGrade);
			}
		}
	}

	return studentGrades;
}

StudentGrade StudentManager::getHighestCoursePerformer(
	const string& courseCode) const
{
	auto studentGrades =
		getStudentGradesForCourse(courseCode);

	if (studentGrades.empty())
		throw runtime_error(
			"No grades found for course: " + courseCode);

	StudentGrade highestPerformer = studentGrades[0];

	for (const StudentGrade& studentGrade : studentGrades)
	{
		if (studentGrade.gradeRecord.m_score
			> highestPerformer.gradeRecord.m_score)
		{
			highestPerformer = studentGrade;
		}
	}

	return highestPerformer;
}

StudentGrade StudentManager::getLowestCoursePerformer(
	const string& courseCode) const
{
	auto studentGrades =
		getStudentGradesForCourse(courseCode);

	if (studentGrades.empty())
		throw runtime_error(
			"No grades found for course: " + courseCode);

	StudentGrade lowestPerformer = studentGrades[0];

	for (const StudentGrade& studentGrade : studentGrades)
	{
		if (studentGrade.gradeRecord.m_score
			< lowestPerformer.gradeRecord.m_score)
		{
			lowestPerformer = studentGrade;
		}
	}

	return lowestPerformer;
}

vector<StudentGrade> StudentManager::rankStudentsInCourse(
	const string& courseCode) const
{
	vector<StudentGrade> studentGrades =
		getStudentGradesForCourse(courseCode);

	std::sort(studentGrades.begin(), studentGrades.end(),
		[](const StudentGrade& firstStudent, const StudentGrade& secondStudent)
		{
			return firstStudent.gradeRecord.m_score
			> secondStudent.gradeRecord.m_score;
		});

	return studentGrades;
}

unordered_map<int, vector<string>> StudentManager::groupByYear() const
{
	unordered_map<int, vector<string>> yearToNames;

	for (const auto& [studentId, student] : m_students)
		yearToNames[student.getYear()].push_back(student.getName());

	return yearToNames;
}

vector<int> StudentManager::getYears() const
{
	std::set<int> years;

	for (const auto& [studentId, student] : m_students)
		years.insert(student.getYear());

	return vector<int>(years.begin(), years.end());
}

vector<string> StudentManager::getCourseCodes() const
{
	std::set<string> codes;

	for (const auto& [studentId, student] : m_students)
		for (const GradeRecord& record : student.getGrades())
			codes.insert(record.m_course.m_courseCode);

	return vector<string>(codes.begin(), codes.end());
}

vector<GradeRecord> StudentManager::getGradesForYear(int year) const
{
	vector<GradeRecord> yearGrades;

	for (const auto& [studentId, student] : m_students)
	{
		if (student.getYear() != year)
			continue;

		for (const GradeRecord& record : student.getGrades())
			yearGrades.push_back(record);
	}

	return yearGrades;
}

vector<RankedStudent> StudentManager::rankStudentsWithPercentile() const
{
	vector<RankedStudent> ranked;
	ranked.reserve(m_students.size());

	for (const auto& [studentId, student] : m_students)
	{
		// A student with no grades has no performance to rank.
		if (student.getGrades().empty())
			continue;

		ranked.push_back({
			&student,
			student.calculateAverage(),
			student.calculateGPA(),
			0,
			0.0 });
	}

	// Best average first; ties broken by ID so output is deterministic
	// (unordered_map iteration order is unspecified).
	std::sort(ranked.begin(), ranked.end(),
		[](const RankedStudent& a, const RankedStudent& b)
		{
			if (a.average != b.average)
				return a.average > b.average;

			return a.student->getId() < b.student->getId();
		});

	const size_t total = ranked.size();
	size_t groupStart = 0;

	for (size_t i = 0; i < total; ++i)
	{
		if (i > 0 && ranked[i].average != ranked[i - 1].average)
			groupStart = i;

		ranked[i].rank = static_cast<int>(groupStart) + 1;

		// Everyone from groupStart to the end scored <= this average.
		ranked[i].percentile =
			static_cast<double>(total - groupStart)
			/ static_cast<double>(total) * 100.0;
	}

	return ranked;
}
