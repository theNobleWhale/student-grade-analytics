# Requirements checklist: assessment PDF vs this C++ project

Status: **Original** = already in your code; **Added** = added in this pass;
**N/A** = a Python-only deliverable (see the note at the end).

## 1. Data structures and collections
| Requirement | C++ equivalent | Status |
|---|---|---|
| namedtuple/dataclass for Student, Course | `Student`, `Course`, `GradeRecord`, `Grade` | Original |
| Counter for grade distribution | `Analytics::countGrades()` (`map<Grade,int>`) | Original |
| Counter-style constant-memory counting | `ScoreHistogram` | Added |
| defaultdict grouping by major | `groupByMajor()` | Original (never used in output) |
| defaultdict grouping by **year** | `groupByYear()`, `getGradesForYear()` | Added |
| Groupings used in reports | `majors` and `years` sections in JSON, console year summary | Added |
| OrderedDict for report order | `nlohmann::ordered_json` | Added |
| deque rolling-average calculator | `RollingAverage` | Added |

## 2. Type hints and data modeling
| Requirement | C++ equivalent | Status |
|---|---|---|
| Static typing | enforced by the compiler | Original |
| `Optional[Grade]` | `tryScoreToGrade`, `parseGrade` return `std::optional<Grade>` | Added |
| TypedDict for report | `AnalyticsReport` and related structs | Added |
| Union types for flexible input | `ScoreInput = variant<int,double,string>`, `toScore()` | Added |

## 3. File I/O
| Requirement | C++ equivalent | Status |
|---|---|---|
| Read CSV with context manager | RAII `ifstream` | Original |
| `csv.DictReader` | header-based `CsvRow` (by column name) | Added (was positional) |
| Write JSON | `json.dump` | Original |
| `pathlib` | `std::filesystem::path` | Added |
| Handle file not found / permission errors | `FileNotFoundError`, `FilePermissionError`, `CsvFormatError`, try/catch in `main` with exit codes | Added (before, errors were uncaught) |
| Bad-row handling | skipped and reported with line numbers, `--strict` option | Added |

## 4. Statistical processing
| Requirement | C++ equivalent | Status |
|---|---|---|
| Mean, median, mode | `Analytics` | Original |
| Percentile rankings | `calculatePercentile` existed but was unused; per-student `rank` and `percentile` now in console and JSON | Added |
| Ranking algorithm | `rankStudents` (original); `rankStudentsWithPercentile` handles ties, skips gradeless students, computes averages once | Added |
| Grade trends over semesters with deque | `computeTrend`, `classifyTrend`, `countStudentTrends` | Added |
| Top performers in report | `topPerformers` section | Added |

## 5. Memory efficiency
| Requirement | C++ equivalent | Status |
|---|---|---|
| Compare memory of collection approaches | `MemoryBenchmark` (`--benchmark`), `TrackingAllocator` | Added |
| Generators for large files (bonus) | `forEachRow`, `analyzeGradesStreaming` (`--stream`) | Added |
| Document performance considerations | README, "Collection performance notes" | Added |

## 6. Objectives, testing, documentation, submission
| Requirement | Status |
|---|---|
| Statistical reports and **visualizations** | ASCII bar charts for grade distribution and semester trend (Added) |
| Tests for each collection type and calculations | 48 tests in `tests/` (Added) |
| Docstrings / usage examples | `///` comments on new code; README usage (Added) |
| Sample data | `sample_data/` (Added) |
| README (setup, sample I/O, performance notes) | `README.md` (Added) |
| JSON report examples | `sample_output/sample_report.json` (Added) |
| GitHub-ready repo | `.gitignore` (Added) |
| Screenshot of execution | **You need to take this**: run the tool and capture the console. `sample_output/console_output.txt` has the text. |
| Python 3.11+ codebase, `requirements.txt` (pytest, mypy) | **N/A** for a C++ project (see below) |

## Bugs fixed along the way
- **Duplicate semester keys on Linux/macOS.** CSV lines were split without
  removing `\r`, so `2025S1` and `2025S1\r` were counted as different
  semesters. This was invisible on Windows.
- **Hardcoded course codes** (`CS202`, `CS404`) in `main` and the JSON
  generator; courses are now loaded from `courses.csv` and discovered
  from the data.
- **Uncaught exceptions** from a missing file or a non-numeric field
  crashed the program.
- **Broken project reference** to `..\..\..\..\Downloads\json.hpp` in the
  `.vcxproj` now points at the `json.hpp` in the project folder.

## Note on language
The PDF specifies a **Python 3.11+** codebase with `requirements.txt`
(pytest, mypy). This project is C++, so those literal items cannot be
satisfied by it. If your course requires Python specifically, confirm
with your instructor that C++ is acceptable; the design here maps
one-to-one onto the Python version if a port is needed.
