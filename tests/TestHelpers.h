#pragma once

#include "Course.h"
#include "GradeRecord.h"
#include "Student.h"
#include "StudentManager.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Writes `content` to a file in the system temp directory and returns its
// path. Files are removed when the TempFile goes out of scope.
class TempFile {
private:
	std::filesystem::path m_path;

public:
	TempFile(const std::string& name, const std::string& content)
		: m_path(std::filesystem::temp_directory_path() / name)
	{
		std::ofstream out(m_path, std::ios::binary);
		out << content;
	}

	~TempFile()
	{
		std::error_code ec;
		std::filesystem::remove(m_path, ec);
	}

	TempFile(const TempFile&) = delete;
	TempFile& operator=(const TempFile&) = delete;

	const std::filesystem::path& path() const { return m_path; }
};

inline std::vector<GradeRecord> recordsFromScores(
	const std::vector<int>& scores, const std::string& semester = "2026S1")
{
	Course course("Algorithms", "CS202", 5);
	std::vector<GradeRecord> records;

	for (int score : scores)
		records.emplace_back(course, score, semester);

	return records;
}

// Three students: Ann (CS, y1, 90/80), Bob (CS, y2, 70), Cy (IT, y2, 50).
inline StudentManager makeSmallManager()
{
	Course algorithms("Algorithms", "CS202", 5);
	Course discrete("Discrete Math", "CS404", 3);

	StudentManager manager;

	Student ann("Ann", "1", "CS", 1);
	ann.addGrade(GradeRecord(algorithms, 90, "2025S1"));
	ann.addGrade(GradeRecord(discrete, 80, "2025S2"));

	Student bob("Bob", "2", "CS", 2);
	bob.addGrade(GradeRecord(algorithms, 70, "2025S1"));

	Student cy("Cy", "3", "IT", 2);
	cy.addGrade(GradeRecord(algorithms, 50, "2025S1"));

	manager.addStudent(ann);
	manager.addStudent(bob);
	manager.addStudent(cy);

	return manager;
}
