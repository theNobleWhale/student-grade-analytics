#include "CSVReader.h"

#include "ScoreInput.h"

#include <charconv>
#include <system_error>

namespace fs = std::filesystem;

using std::runtime_error;
using std::size_t;
using std::string;
using std::vector;

namespace {

string trim(const string& text)
{
	size_t begin = text.find_first_not_of(" \t");

	if (begin == string::npos)
		return "";

	size_t end = text.find_last_not_of(" \t");

	return text.substr(begin, end - begin + 1);
}

// Files written on Windows end lines with \r\n. Reading them on Linux/macOS
// would otherwise leave a stray '\r' glued to the last field.
void stripLineEnding(string& line)
{
	while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
		line.pop_back();
}

int parseInt(const string& text, const char* fieldName)
{
	int value = 0;
	const char* first = text.data();
	const char* last = first + text.size();

	auto [ptr, ec] = std::from_chars(first, last, value);

	if (text.empty() || ec != std::errc{} || ptr != last)
		throw runtime_error(
			string("Invalid integer for '") + fieldName + "': '" + text + "'");

	return value;
}

// Runs one row's work; on failure either rethrows (strict) or records the
// problem and carries on (lenient).
template <typename Work>
void processRow(
	LoadResult& result, bool strict, size_t lineNumber, Work&& work)
{
	++result.rowsRead;

	try
	{
		work();
		++result.rowsLoaded;
	}
	catch (const std::exception& error)
	{
		string message =
			"line " + std::to_string(lineNumber) + ": " + error.what();

		if (strict)
			throw runtime_error(message);

		result.recordError(message);
	}
}

} // namespace

const string& CsvRow::operator[](const string& column) const
{
	auto it = m_columns.find(column);

	if (it == m_columns.end())
		throw runtime_error("Unknown column: " + column);

	return m_fields[it->second];
}

vector<string> CSVReader::splitLine(const string& line)
{
	vector<string> fields;
	string field;
	bool inQuotes = false;

	for (size_t i = 0; i < line.size(); ++i)
	{
		char c = line[i];

		if (inQuotes)
		{
			if (c == '"')
			{
				if (i + 1 < line.size() && line[i + 1] == '"')
				{
					field += '"'; // escaped quote
					++i;
				}
				else
				{
					inQuotes = false;
				}
			}
			else
			{
				field += c;
			}
		}
		else if (c == '"' && field.empty())
		{
			inQuotes = true;
		}
		else if (c == ',')
		{
			fields.push_back(field);
			field.clear();
		}
		else
		{
			field += c;
		}
	}

	fields.push_back(field);

	return fields;
}

std::ifstream CSVReader::openForReading(const fs::path& path)
{
	std::error_code ec;

	if (!fs::exists(path, ec))
		throw FileNotFoundError("File not found: " + path.string());

	if (!fs::is_regular_file(path, ec))
		throw runtime_error("Not a regular file: " + path.string());

	std::ifstream file(path);

	if (!file)
		throw FilePermissionError(
			"Cannot open file (permission denied or file in use): "
			+ path.string());

	return file;
}

CSVReader::MalformedHandler CSVReader::makeMalformedHandler(
	LoadResult& result) const
{
	return [this, &result](size_t lineNumber, const string& message)
		{
			++result.rowsRead;

			string full =
				"line " + std::to_string(lineNumber) + ": " + message;

			if (m_strict)
				throw runtime_error(full);

			result.recordError(full);
		};
}

void CSVReader::forEachRow(
	const fs::path& path,
	const vector<string>& requiredColumns,
	const RowHandler& onRow,
	const MalformedHandler& onMalformed) const
{
	std::ifstream file = openForReading(path);

	string line;

	if (!std::getline(file, line))
		throw CsvFormatError("CSV file is empty: " + path.string());

	// Excel's "CSV UTF-8" adds a byte-order mark that would corrupt the
	// first column name.
	if (line.size() >= 3
		&& static_cast<unsigned char>(line[0]) == 0xEF
		&& static_cast<unsigned char>(line[1]) == 0xBB
		&& static_cast<unsigned char>(line[2]) == 0xBF)
	{
		line.erase(0, 3);
	}

	stripLineEnding(line);

	ColumnIndex columns;
	vector<string> header = splitLine(line);

	for (size_t i = 0; i < header.size(); ++i)
		columns.emplace(trim(header[i]), i);

	for (const string& required : requiredColumns)
	{
		if (columns.find(required) == columns.end())
			throw CsvFormatError(
				"Missing required column '" + required
				+ "' in " + path.string());
	}

	size_t lineNumber = 1;

	while (std::getline(file, line))
	{
		++lineNumber;
		stripLineEnding(line);

		if (line.empty())
			continue;

		vector<string> fields = splitLine(line);

		if (fields.size() != header.size())
		{
			string message =
				"expected " + std::to_string(header.size())
				+ " fields but found " + std::to_string(fields.size());

			if (!onMalformed)
				throw CsvFormatError(
					"line " + std::to_string(lineNumber) + ": " + message);

			onMalformed(lineNumber, message);
			continue;
		}

		for (string& field : fields)
			field = trim(field);

		onRow(CsvRow(columns, fields), lineNumber);
	}
}

LoadResult CSVReader::loadCourses(
	const fs::path& path, vector<Course>& courses) const
{
	LoadResult result;

	forEachRow(
		path,
		{ "courseName", "courseCode", "creditHours" },
		[&](const CsvRow& row, size_t lineNumber)
		{
			processRow(result, m_strict, lineNumber, [&]
				{
					Course course(
						row["courseName"],
						row["courseCode"],
						parseInt(row["creditHours"], "creditHours"));

					for (const Course& existing : courses)
						if (existing.m_courseCode == course.m_courseCode)
							throw runtime_error(
								"Duplicate course code: " + course.m_courseCode);

					courses.push_back(course);
				});
		},
		makeMalformedHandler(result));

	return result;
}

LoadResult CSVReader::loadStudents(
	const fs::path& path, StudentManager& studentManager) const
{
	LoadResult result;

	forEachRow(
		path,
		{ "name", "id", "major", "year" },
		[&](const CsvRow& row, size_t lineNumber)
		{
			processRow(result, m_strict, lineNumber, [&]
				{
					Student student(
						row["name"],
						row["id"],
						row["major"],
						parseInt(row["year"], "year"));

					studentManager.addStudent(student);
				});
		},
		makeMalformedHandler(result));

	return result;
}

LoadResult CSVReader::loadGrades(
	const fs::path& path,
	StudentManager& studentManager,
	const vector<Course>& courses) const
{
	// Build the lookup once instead of scanning the course list per row.
	std::unordered_map<string, const Course*> courseByCode;

	for (const Course& course : courses)
		courseByCode.emplace(course.m_courseCode, &course);

	LoadResult result;

	forEachRow(
		path,
		{ "studentId", "courseCode", "score", "semester" },
		[&](const CsvRow& row, size_t lineNumber)
		{
			processRow(result, m_strict, lineNumber, [&]
				{
					Student* student =
						studentManager.findStudentById(row["studentId"]);

					if (student == nullptr)
						throw runtime_error(
							"Student ID not found: " + row["studentId"]);

					auto courseIt = courseByCode.find(row["courseCode"]);

					if (courseIt == courseByCode.end())
						throw runtime_error(
							"Course not found: " + row["courseCode"]);

					std::optional<int> score =
						toScore(ScoreInput{ row["score"] });

					if (!score)
						throw runtime_error(
							"Invalid score: '" + row["score"] + "'");

					student->addGrade(GradeRecord(
						*courseIt->second, *score, row["semester"]));
				});
		},
		makeMalformedHandler(result));

	return result;
}
