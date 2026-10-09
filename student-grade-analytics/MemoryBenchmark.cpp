#include "MemoryBenchmark.h"

#include "Course.h"
#include "GradeRecord.h"
#include "StreamingAnalyzer.h"
#include "TrackingAllocator.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <list>
#include <map>
#include <ostream>
#include <random>
#include <set>
#include <unordered_map>

using std::size_t;
using std::string;
using std::vector;

namespace {

template <typename T>
using Tracked = TrackingAllocator<T>;

vector<int> makeScores(size_t n)
{
	std::mt19937 rng(42); // fixed seed: repeatable runs
	std::uniform_int_distribution<int> dist(30, 100);

	vector<int> scores(n);

	for (int& score : scores)
		score = dist(rng);

	return scores;
}

// Runs `build` (which must create and fill its container), recording the
// memory it requested and how long it took.
template <typename Build>
BenchmarkRow measure(const string& label, size_t n, Build&& build)
{
	AllocationCounters& counters = allocationCounters();
	counters.reset();

	auto start = std::chrono::steady_clock::now();
	build();
	auto stop = std::chrono::steady_clock::now();

	BenchmarkRow row;
	row.label = label;
	row.peakBytes = counters.peak;
	row.allocations = counters.allocations;
	row.bytesPerItem =
		n == 0 ? 0.0 : static_cast<double>(counters.peak) / n;
	row.buildMillis =
		std::chrono::duration<double, std::milli>(stop - start).count();

	return row;
}

// Flyweight: shares one Course instead of copying it into every record.
struct FlyweightRecord {
	const Course* course;
	string semester;
	int score;
};

// Packed: indices into shared tables of courses / semesters.
struct PackedRecord {
	std::uint16_t courseIndex;
	std::uint16_t semesterIndex;
	std::uint8_t score;
};

} // namespace

vector<BenchmarkRow> benchmarkCountingApproaches(size_t n)
{
	const vector<int> scores = makeScores(n);
	vector<BenchmarkRow> rows;

	rows.push_back(measure("vector<int> (push_back)", n, [&]
		{
			vector<int, Tracked<int>> c;
			for (int s : scores) c.push_back(s);
		}));

	rows.push_back(measure("vector<int> (reserve first)", n, [&]
		{
			vector<int, Tracked<int>> c;
			c.reserve(n);
			for (int s : scores) c.push_back(s);
		}));

	rows.push_back(measure("deque<int>", n, [&]
		{
			std::deque<int, Tracked<int>> c;
			for (int s : scores) c.push_back(s);
		}));

	rows.push_back(measure("list<int>", n, [&]
		{
			std::list<int, Tracked<int>> c;
			for (int s : scores) c.push_back(s);
		}));

	rows.push_back(measure("multiset<int> (sorted)", n, [&]
		{
			std::multiset<int, std::less<int>, Tracked<int>> c;
			for (int s : scores) c.insert(s);
		}));

	rows.push_back(measure("map<int,int> counter", n, [&]
		{
			std::map<int, int, std::less<int>,
				Tracked<std::pair<const int, int>>> c;
			for (int s : scores) c[s]++;
		}));

	rows.push_back(measure("unordered_map<int,int> counter", n, [&]
		{
			std::unordered_map<int, int, std::hash<int>, std::equal_to<int>,
				Tracked<std::pair<const int, int>>> c;
			for (int s : scores) c[s]++;
		}));

	// The histogram never allocates, so report its fixed object size.
	BenchmarkRow histogramRow = measure("ScoreHistogram (101 counters)", n, [&]
		{
			ScoreHistogram h;
			for (int s : scores) h.add(s);
		});
	histogramRow.peakBytes = sizeof(ScoreHistogram);
	histogramRow.bytesPerItem =
		n == 0 ? 0.0 : static_cast<double>(sizeof(ScoreHistogram)) / n;
	rows.push_back(histogramRow);

	return rows;
}

vector<BenchmarkRow> benchmarkRecordLayouts(size_t n)
{
	const vector<int> scores = makeScores(n);
	const Course course("Algorithms", "CS202", 5);
	vector<BenchmarkRow> rows;

	rows.push_back(measure(
		"GradeRecord (embedded Course) sizeof="
		+ std::to_string(sizeof(GradeRecord)), n, [&]
		{
			vector<GradeRecord, Tracked<GradeRecord>> c;
			c.reserve(n);
			for (int s : scores) c.emplace_back(course, s, "2026S1");
		}));

	rows.push_back(measure(
		"Flyweight (Course*) sizeof="
		+ std::to_string(sizeof(FlyweightRecord)), n, [&]
		{
			vector<FlyweightRecord, Tracked<FlyweightRecord>> c;
			c.reserve(n);
			for (int s : scores) c.push_back({ &course, "2026S1", s });
		}));

	rows.push_back(measure(
		"PackedRecord (indices) sizeof="
		+ std::to_string(sizeof(PackedRecord)), n, [&]
		{
			vector<PackedRecord, Tracked<PackedRecord>> c;
			c.reserve(n);
			for (int s : scores)
				c.push_back({ 0, 0, static_cast<std::uint8_t>(s) });
		}));

	return rows;
}

void printBenchmarkTable(
	const string& title,
	const vector<BenchmarkRow>& rows,
	std::ostream& out)
{
	out << "\n===== " << title << " =====\n";

	out << std::left << std::setw(52) << "Approach"
		<< std::right << std::setw(14) << "Peak bytes"
		<< std::setw(12) << "Allocs"
		<< std::setw(12) << "Bytes/item"
		<< std::setw(12) << "Build ms" << '\n';

	for (const BenchmarkRow& row : rows)
	{
		out << std::left << std::setw(52) << row.label
			<< std::right << std::setw(14) << row.peakBytes
			<< std::setw(12) << row.allocations
			<< std::setw(12) << std::fixed << std::setprecision(2)
			<< row.bytesPerItem
			<< std::setw(12) << std::setprecision(1) << row.buildMillis
			<< '\n';
	}
}

void runMemoryBenchmark(size_t n, std::ostream& out)
{
	out << "\nMemory benchmark with " << n << " items\n"
		<< "(requested bytes via TrackingAllocator; excludes heap overhead "
		<< "and strings beyond the small-string buffer)\n";

	printBenchmarkTable(
		"HOLDING / COUNTING SCORES",
		benchmarkCountingApproaches(n), out);

	printBenchmarkTable(
		"GRADE RECORD LAYOUTS",
		benchmarkRecordLayouts(n), out);
}
