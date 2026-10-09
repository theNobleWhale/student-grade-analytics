#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

struct BenchmarkRow {
	std::string label;
	std::size_t peakBytes;      // highest memory requested at once
	std::size_t allocations;    // number of separate allocations
	double bytesPerItem;
	double buildMillis;
};

/// Ways of holding / counting `n` scores: vector, deque, list, multiset,
/// map and unordered_map counters, and a fixed 101-slot histogram.
std::vector<BenchmarkRow> benchmarkCountingApproaches(std::size_t n);

/// Three layouts for `n` grade records: the project's GradeRecord (embeds a
/// full Course copy), a flyweight (pointer to a shared Course), and a packed
/// record using small integer indices into shared tables.
std::vector<BenchmarkRow> benchmarkRecordLayouts(std::size_t n);

void printBenchmarkTable(
	const std::string& title,
	const std::vector<BenchmarkRow>& rows,
	std::ostream& out);

/// Runs both benchmarks and prints the tables.
void runMemoryBenchmark(std::size_t n, std::ostream& out);
