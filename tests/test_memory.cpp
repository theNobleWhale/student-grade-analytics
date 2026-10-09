// Memory-comparison tooling.
#include "TestFramework.h"

#include "MemoryBenchmark.h"
#include "TrackingAllocator.h"

#include <list>
#include <sstream>
#include <vector>

namespace {

const BenchmarkRow& find(const std::vector<BenchmarkRow>& rows,
	const char* prefix)
{
	for (const BenchmarkRow& row : rows)
		if (row.label.rfind(prefix, 0) == 0)
			return row;

	throw std::runtime_error(std::string("row not found: ") + prefix);
}

} // namespace

TEST(tracking_allocator_counts_requests_and_releases)
{
	allocationCounters().reset();

	{
		std::vector<int, TrackingAllocator<int>> v;
		v.reserve(100);
		CHECK_EQ(allocationCounters().current, size_t(100 * sizeof(int)));
		CHECK_EQ(allocationCounters().allocations, size_t(1));
	}

	CHECK_EQ(allocationCounters().current, size_t(0));
	CHECK_EQ(allocationCounters().peak, size_t(100 * sizeof(int)));
}

TEST(list_uses_more_memory_than_vector_and_reserve_avoids_growth)
{
	auto rows = benchmarkCountingApproaches(10000);

	const auto& grown = find(rows, "vector<int> (push_back)");
	const auto& reserved = find(rows, "vector<int> (reserve");
	const auto& list = find(rows, "list<int>");

	CHECK_EQ(reserved.peakBytes, size_t(10000 * sizeof(int)));
	CHECK(grown.peakBytes >= reserved.peakBytes);
	CHECK(list.peakBytes > reserved.peakBytes * 2);
	CHECK_EQ(list.allocations, size_t(10000));
}

TEST(counter_approaches_use_constant_memory)
{
	auto small = benchmarkCountingApproaches(1000);
	auto large = benchmarkCountingApproaches(20000);

	CHECK_EQ(find(small, "ScoreHistogram").peakBytes,
		find(large, "ScoreHistogram").peakBytes);
	CHECK(find(large, "map<int,int>").peakBytes < 8000);
	CHECK(find(large, "unordered_map").peakBytes < 8000);
}

TEST(record_layouts_shrink_from_embedded_to_flyweight_to_packed)
{
	auto rows = benchmarkRecordLayouts(5000);

	size_t embedded = find(rows, "GradeRecord").peakBytes;
	size_t flyweight = find(rows, "Flyweight").peakBytes;
	size_t packed = find(rows, "PackedRecord").peakBytes;

	CHECK(embedded > flyweight);
	CHECK(flyweight > packed);
}

TEST(benchmark_table_prints_every_row)
{
	std::ostringstream out;
	runMemoryBenchmark(500, out);

	CHECK(out.str().find("HOLDING / COUNTING SCORES") != std::string::npos);
	CHECK(out.str().find("GRADE RECORD LAYOUTS") != std::string::npos);
	CHECK(out.str().find("ScoreHistogram") != std::string::npos);
}
