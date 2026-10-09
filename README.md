# Student Grade Analytics Tool (C++20)

Processes student and grade CSV files, computes statistics (mean, median,
mode, percentile rankings, grade distributions, semester trends), prints
console reports with ASCII charts, and writes a structured JSON report.

This is the C++ implementation of the _Student Grade Analytics Tool with
Advanced Collections_ assessment. See
[`REQUIREMENTS_CHECKLIST.md`](REQUIREMENTS_CHECKLIST.md) for how every
Python requirement maps to a C++ equivalent.

## Setup

Requirements: a C++20 compiler (MSVC 2022, GCC 11+, or Clang 14+).
No third-party installs: `json.hpp` (nlohmann/json) is included.

**Visual Studio:** open `student-grade-analytics.slnx`, build, and run. The
working directory must contain `students.csv`, `grades.csv` and
`courses.csv` (the project folder does by default).

**CMake (any platform, or VS "Open Folder"):**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd student-grade-analytics          # folder holding the large CSV files
../build/student-grade-analytics
ctest --test-dir ../build --output-on-failure   # run the tests
```

**Without CMake (g++):**

```bash
# app
g++ -std=c++20 -O2 -o student-grade-analytics student-grade-analytics/*.cpp
# tests
g++ -std=c++20 -O1 -Istudent-grade-analytics -o run_tests tests/*.cpp \
    $(ls student-grade-analytics/*.cpp | grep -v main.cpp) && ./run_tests
```

## Usage

```
student-grade-analytics [students.csv] [grades.csv] [courses.csv] [report.json] [options]

  --student <id>         student to look up (default 9032104)
  --strict               stop at the first bad CSV row instead of skipping it
  --no-student-details   omit per-student grades from the JSON report
  --stream               also analyse grades.csv row by row (constant memory)
  --benchmark            print the memory / performance comparison
  --help
```

Try it on the small bundled data set:

```bash
student-grade-analytics sample_data/students.csv sample_data/grades.csv \
                sample_data/courses.csv report.json --stream --benchmark
```

> **Report size:** with the full 100,000-student data set the JSON report
> with per-student details is about 230 MB. Use `--no-student-details`
> (about 15 KB) for a summary-only report. `analytics_report.json` is in
> `.gitignore` because GitHub rejects files over 100 MB.

## Input files

| File           | Columns                               |
| -------------- | ------------------------------------- |
| `courses.csv`  | `courseName,courseCode,creditHours`   |
| `students.csv` | `name,id,major,year`                  |
| `grades.csv`   | `studentId,courseCode,score,semester` |

Columns are matched **by header name**, so their order does not matter.
Quoted fields (`"Smith, Jane"`), Windows line endings and a UTF-8 BOM are
handled. Scores may be integers or decimals (`91.5` rounds to `92`).

Bad rows (unknown student or course, score outside 0-100, wrong field
count, duplicate student ID) are **skipped and reported** with their line
number, or abort the run with `--strict`.

## Sample output

Console (from `sample_output/console_output.txt`):

```
===== OVERALL ANALYTICS =====
Records: 120
Mean: 73.05
Median: 74
Pass Rate: 85.8333%

----- Overall grade distribution -----
  A | ##############                                 12 (10.0%)
  B | ###############################                27 (22.5%)
  C | ########################################       34 (28.3%)
  D | ###################################            30 (25.0%)
  F | ####################                           17 (14.2%)

----- Semester trend (rolling window = 2) -----
  2025S1 | ############################             mean  71.88  rolling  71.88  (40 records)
  2025S2 | #############################            mean  73.22  rolling  72.55  (40 records)
  2026S1 | #############################            mean  74.05  rolling  73.64  (40 records)
```

JSON (from `sample_output/sample_report.json`; keys keep this order):

```json
{
  "overall":       { "records": 120, "mean": 73.05, "median": 74.0, "highest": 100,
                     "lowest": 36, "passRate": 85.83, "modes": [65, 72],
                     "gradeCounts": { "A": 12, "B": 27, "C": 34, "D": 30, "F": 17 } },
  "semesters":     { "2025S1": { ... }, "2025S2": { ... }, "2026S1": { ... } },
  "semesterTrend": [ { "semester": "2025S1", "records": 40, "mean": 71.875, "rollingMean": 71.875 }, ... ],
  "studentTrends": { "improving": 12, "declining": 7, "stable": 1, "insufficient_data": 0 },
  "courses":       { "CS202": { ... }, "CS404": { ... } },
  "majors":        { "Biology": { "students": 4, ... }, ... },
  "years":         { "1": { ... }, ... },
  "topPerformers": [ { "rank": 1, "name": "Daniel Garcia", "average": 92.5, "percentile": 100.0, ... } ],
  "students":      [ { "id": "...", "rank": 16, "percentile": 25.0, "grades": [ ... ] } ]
}
```

## Design: Python collections to C++

| Python (assignment)                     | C++ used here                                          | Where                                                     |
| --------------------------------------- | ------------------------------------------------------ | --------------------------------------------------------- |
| `dataclass` / `namedtuple`              | `class`/`struct` with validating constructors          | `Student.h`, `Course.h`, `GradeRecord.h`, `ReportTypes.h` |
| `Counter`                               | `std::map<Grade,int>`; `ScoreHistogram` (101 counters) | `Analytics.cpp`, `StreamingAnalyzer.h`                    |
| `defaultdict(list)`                     | `unordered_map<K, vector<V>>` with `operator[]`        | `StudentManager::groupByMajor/groupByYear`                |
| `OrderedDict`                           | `nlohmann::ordered_json`; vectors of pairs             | `ReportTypes.h`                                           |
| `deque` rolling average                 | `std::deque<double>` sliding window                    | `RollingAverage.h`, `Trends.cpp`                          |
| `Optional[Grade]`                       | `std::optional<Grade>`                                 | `Grade.h`                                                 |
| `Union[int, float, str]`                | `std::variant<int,double,string>`                      | `ScoreInput.h`                                            |
| `TypedDict` report                      | `AnalyticsReport` struct plus `to_json`                | `ReportTypes.h`                                           |
| `with open(...)`                        | RAII `std::ifstream` (closes at end of scope)          | `CSVReader.cpp`                                           |
| `csv.DictReader`                        | header-based `CsvRow`                                  | `CSVReader.h`                                             |
| `pathlib`                               | `std::filesystem::path`                                | `CSVReader`, `JSONReportGenerator`                        |
| `FileNotFoundError` / `PermissionError` | `FileNotFoundError` / `FilePermissionError`            | `CSVReader.h`                                             |
| generators                              | row-by-row callback streaming                          | `CSVReader::forEachRow`, `analyzeGradesStreaming`         |
| pytest                                  | dependency-free test runner in `tests/`                | `tests/`                                                  |

C++ is statically typed, so the "type hints" requirement is met by the
compiler itself; `optional`, `variant` and the report structs are the
explicit equivalents of `Optional`, `Union` and `TypedDict`.

## Collection performance notes

Measured with g++ 13, `-O2`, Linux, on the full data set (100,000 students,
444,384 grade records). `--benchmark` reproduces the tables for your own
machine; absolute numbers vary by compiler and standard library (MSVC
Release will differ, Debug will be much slower and larger).
"Bytes" are bytes _requested_ via a counting allocator; real heap use is
higher for node-based containers because each allocation has bookkeeping
overhead.

**Holding or counting 444,384 scores**

| Approach                         | Peak bytes | Allocations | Notes                                              |
| -------------------------------- | ---------: | ----------: | -------------------------------------------------- |
| `vector<int>` push_back          |  3,145,728 |          20 | growth leaves slack and copies during reallocation |
| `vector<int>` with `reserve`     |  1,777,536 |           1 | 4 B/item, the minimum possible                     |
| `deque<int>`                     |  1,870,304 |       3,483 | chunked; cheap at both ends, ~5% overhead          |
| `list<int>`                      | 10,665,216 |     444,384 | 24 B/item, one allocation per element              |
| `multiset<int>`                  | 17,775,360 |     444,384 | 40 B/item and ~100x slower to build                |
| `map<int,int>` counter           |      2,840 |          71 | memory depends on _distinct_ values, not rows      |
| `unordered_map<int,int>` counter |      2,448 |          75 | same, and faster than `map`                        |
| `ScoreHistogram`                 |        824 |           0 | fixed size for bounded 0-100 scores                |

Takeaways: prefer `vector` (with `reserve` when the size is known); avoid
`list` and ordered sets unless you need their specific guarantees; use a
counter (map or fixed array) when you only need frequencies, since memory
then stays constant however many records you process.

**Grade record layout (444,384 records)**

| Layout                                      | `sizeof` |   Total |
| ------------------------------------------- | -------: | ------: |
| `GradeRecord` (embeds a full `Course` copy) |    112 B | 49.8 MB |
| Flyweight (pointer to shared `Course`)      |     48 B | 21.3 MB |
| Packed (small indices into shared tables)   |      6 B |  2.7 MB |

The project's `GradeRecord` copies the `Course` into every record, which
is simple and safe but is the largest single memory cost. A shared
`Course*` cuts it by more than half; interning courses and semesters into
small indices cuts it by ~19x. (Course names fit in the small-string
buffer, so they cost no extra heap here; longer names would.)

**In-memory vs streaming (grades.csv only)**

| Approach                                     | Peak RSS |    Time |
| -------------------------------------------- | -------: | ------: |
| Load everything (`--no-student-details` run) |  ~272 MB |  ~2.1 s |
| Stream with `ScoreHistogram` (`--stream`)    |   ~11 MB | ~0.11 s |

Streaming gives exact mean, median, mode, percentile and grade counts
(verified equal to the in-memory results by a test) because scores are
bounded integers. It cannot answer per-student questions such as rankings,
which need the students in memory.

Other performance choices:

- Student ranking computes each average once before sorting, rather than
  inside the comparator.
- Grade lookups build a course-code hash map once instead of scanning the
  course list per row.
- Report output is sorted by ID, so reports are reproducible even though
  `unordered_map` iteration order is unspecified.

## Testing

`tests/` has 48 tests, grouped by what they cover:

| File                   | Covers                                                           |
| ---------------------- | ---------------------------------------------------------------- |
| `test_models.cpp`      | models and validation, `optional` grades, `variant` score input  |
| `test_collections.cpp` | Counter, defaultdict, OrderedDict, deque, and trend tests        |
| `test_analytics.cpp`   | mean/median/mode, percentile, ranking with ties                  |
| `test_io.cpp`          | CSV parsing, error handling, streaming vs in-memory, JSON output |
| `test_memory.cpp`      | counting allocator and benchmark properties                      |

## Limitations

- Quoted CSV fields cannot span multiple lines.
- Student IDs are not cross-checked in `--stream` mode (it keeps no
  students in memory).
- `Analytics` stores a _reference_ to the record vector it is given, so
  that vector must outlive the `Analytics` object.

## License

This project is intended as an educational/software-engineering project.

The bundled `json.hpp` file is provided by the nlohmann/json project and remains subject to its original license.
