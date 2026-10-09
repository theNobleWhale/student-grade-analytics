#pragma once

#include "Course.h"
#include "StudentManager.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/// Thrown when a file does not exist (Python's FileNotFoundError).
class FileNotFoundError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/// Thrown when a file exists but cannot be opened or created
/// (Python's PermissionError).
class FilePermissionError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/// Thrown for structural CSV problems: empty file, missing header columns.
class CsvFormatError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

using ColumnIndex = std::unordered_map<std::string, std::size_t>;

/// One data row addressed by column name (csv.DictReader equivalent).
/// Only valid inside the row callback.
class CsvRow {
private:
	const ColumnIndex& m_columns;
	const std::vector<std::string>& m_fields;

public:
	CsvRow(const ColumnIndex& columns, const std::vector<std::string>& fields)
		: m_columns(columns), m_fields(fields)
	{
	}

	/// Returns the trimmed field for `column`; throws std::runtime_error
	/// if the column does not exist.
	const std::string& operator[](const std::string& column) const;
};

/// Outcome of loading a file: how many rows were read, loaded and skipped,
/// plus the first few skip reasons (kept small so a bad 1M-row file
/// cannot exhaust memory with error strings).
struct LoadResult {
	static constexpr std::size_t kMaxStoredErrors = 20;

	std::size_t rowsRead = 0;
	std::size_t rowsLoaded = 0;
	std::size_t rowsSkipped = 0;
	std::vector<std::string> errors;

	void recordError(const std::string& message)
	{
		++rowsSkipped;

		if (errors.size() < kMaxStoredErrors)
			errors.push_back(message);
	}
};

class CSVReader {
public:
	using RowHandler =
		std::function<void(const CsvRow& row, std::size_t lineNumber)>;
	using MalformedHandler =
		std::function<void(std::size_t lineNumber, const std::string& message)>;

private:
	bool m_strict;

	MalformedHandler makeMalformedHandler(LoadResult& result) const;

public:
	/// strict = false (default): bad rows are skipped and reported in the
	///   LoadResult. strict = true: the first bad row throws.
	explicit CSVReader(bool strict = false) : m_strict(strict) {}

	/// Quote-aware split: handles "a,b" and "" escapes. Does not support
	/// quoted fields that span multiple lines.
	static std::vector<std::string> splitLine(const std::string& line);

	/// Opens a file for reading, throwing FileNotFoundError or
	/// FilePermissionError instead of failing silently. The returned stream
	/// closes itself when it goes out of scope (RAII = a `with` block).
	static std::ifstream openForReading(const std::filesystem::path& path);

	/// Streams the file one row at a time, never holding more than a single
	/// row in memory (the C++ analogue of a Python generator). The header
	/// row maps column names to positions, so column order does not matter.
	void forEachRow(
		const std::filesystem::path& path,
		const std::vector<std::string>& requiredColumns,
		const RowHandler& onRow,
		const MalformedHandler& onMalformed = nullptr) const;

	LoadResult loadCourses(
		const std::filesystem::path& path,
		std::vector<Course>& courses) const;

	LoadResult loadStudents(
		const std::filesystem::path& path,
		StudentManager& studentManager) const;

	LoadResult loadGrades(
		const std::filesystem::path& path,
		StudentManager& studentManager,
		const std::vector<Course>& courses) const;
};
