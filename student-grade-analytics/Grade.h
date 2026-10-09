#pragma once
#include <array>
#include <cctype>
#include <optional>
#include <string>

using std::string;

enum class Grade {
	A, B, C, D, F
};

inline string gradeToString(Grade grade)
{
	switch (grade)
	{
	case Grade::A: return "A";
	case Grade::B: return "B";
	case Grade::C: return "C";
	case Grade::D: return "D";
	case Grade::F: return "F";
	default: return "Unknown";
	}
}

inline Grade scoreToGrade(int score)
{
	if (score >= 90)
		return Grade::A;
	if (score >= 80)
		return Grade::B;
	if (score >= 70)
		return Grade::C;
	if (score >= 60)
		return Grade::D;
	return Grade::F;
}

inline double gradeToPoint(Grade grade)
{
	switch (grade)
	{
	case Grade::A: return 4.0;
	case Grade::B: return 3.0;
	case Grade::C: return 2.0;
	case Grade::D: return 1.0;
	case Grade::F: return 0.0;
	default: return 0.0;
	}
}

/// Every grade, in descending order. Handy for iterating a full distribution.
inline constexpr std::array<Grade, 5> kAllGrades{
	Grade::A, Grade::B, Grade::C, Grade::D, Grade::F
};

/// Optional[Grade] equivalent: returns std::nullopt for scores outside 0..100
/// instead of silently mapping them to a grade.
inline std::optional<Grade> tryScoreToGrade(int score)
{
	if (score < 0 || score > 100)
		return std::nullopt;

	return scoreToGrade(score);
}

/// Optional[Grade] equivalent: parses "A".."F" (case-insensitive).
inline std::optional<Grade> parseGrade(const string& text)
{
	if (text.size() != 1)
		return std::nullopt;

	switch (std::toupper(static_cast<unsigned char>(text[0])))
	{
	case 'A': return Grade::A;
	case 'B': return Grade::B;
	case 'C': return Grade::C;
	case 'D': return Grade::D;
	case 'F': return Grade::F;
	default: return std::nullopt;
	}
}
