// One group of tests per collection type named in the assignment:
// Counter -> map<Grade,int>/ScoreHistogram, defaultdict -> grouping,
// OrderedDict -> ordered_json, deque -> RollingAverage.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "Analytics.h"
#include "ReportTypes.h"
#include "RollingAverage.h"
#include "JSONReportGenerator.h"
#include "StreamingAnalyzer.h"
#include "Trends.h"

#include <random>

// ---------- Counter ----------

TEST(counter_counts_grade_distribution)
{
	auto records = recordsFromScores({ 95, 91, 85, 72, 65, 10, 10 });
	Analytics analytics(records);
	auto counts = analytics.countGrades();

	CHECK_EQ(counts[Grade::A], 2);
	CHECK_EQ(counts[Grade::B], 1);
	CHECK_EQ(counts[Grade::C], 1);
	CHECK_EQ(counts[Grade::D], 1);
	CHECK_EQ(counts[Grade::F], 2);
}

TEST(histogram_counter_matches_vector_based_analytics)
{
	std::mt19937 rng(1);
	std::uniform_int_distribution<int> dist(0, 100);

	std::vector<int> scores;
	ScoreHistogram histogram;

	for (int i = 0; i < 5001; ++i) // odd count -> single middle value
	{
		scores.push_back(dist(rng));
		histogram.add(scores.back());
	}

	for (size_t size : { size_t(5001), size_t(5000) }) // odd and even
	{
		ScoreHistogram h;
		std::vector<int> subset(scores.begin(), scores.begin() + size);
		for (int s : subset) h.add(s);

		auto records = recordsFromScores(subset);
		ScoreStats expected = computeStats(records);
		ScoreStats actual = h.toStats();

		CHECK_EQ(actual.records, expected.records);
		CHECK_NEAR(actual.mean, expected.mean, 1e-9);
		CHECK_NEAR(actual.median, expected.median, 1e-9);
		CHECK_EQ(actual.highest, expected.highest);
		CHECK_EQ(actual.lowest, expected.lowest);
		CHECK_NEAR(actual.passRate, expected.passRate, 1e-9);
		CHECK(actual.modes == expected.modes);
		CHECK(actual.gradeCounts == expected.gradeCounts);
	}
}

TEST(histogram_rejects_out_of_range_and_handles_empty)
{
	ScoreHistogram h;
	CHECK_THROWS(h.add(101), std::out_of_range);
	CHECK_THROWS(h.add(-1), std::out_of_range);
	CHECK_EQ(h.toStats().records, 0);
	CHECK_NEAR(h.percentile(50), 0.0, 1e-9);
}

// ---------- defaultdict ----------

TEST(defaultdict_groups_students_by_major_and_year)
{
	StudentManager manager = makeSmallManager();

	auto byMajor = manager.groupByMajor();
	CHECK_EQ(byMajor["CS"].size(), size_t(2));
	CHECK_EQ(byMajor["IT"].size(), size_t(1));
	CHECK_EQ(byMajor.size(), size_t(2));

	auto byYear = manager.groupByYear();
	CHECK_EQ(byYear[1].size(), size_t(1));
	CHECK_EQ(byYear[2].size(), size_t(2));
	CHECK(manager.getYears() == (std::vector<int>{ 1, 2 }));
}

TEST(defaultdict_missing_key_creates_empty_group)
{
	StudentManager manager = makeSmallManager();
	auto byYear = manager.groupByYear();
	CHECK(byYear[5].empty()); // like defaultdict(list)[5] -> []
}

// ---------- OrderedDict ----------

TEST(ordered_json_preserves_insertion_order)
{
	ordered_json object = ordered_json::object();
	object["zebra"] = 1;
	object["apple"] = 2;
	object["mango"] = 3;

	std::vector<std::string> keys;
	for (auto it = object.begin(); it != object.end(); ++it)
		keys.push_back(it.key());

	CHECK(keys == (std::vector<std::string>{ "zebra", "apple", "mango" }));
}

TEST(report_sections_appear_in_declared_order)
{
	StudentManager manager = makeSmallManager();
	AnalyticsReport report = JSONReportGenerator().buildReport(manager);

	ordered_json json = report;
	std::vector<std::string> keys;
	for (auto it = json.begin(); it != json.end(); ++it)
		keys.push_back(it.key());

	CHECK(keys == (std::vector<std::string>{
		"overall", "semesters", "semesterTrend", "studentTrends",
		"courses", "majors", "years", "topPerformers" }));
}

// ---------- deque ----------

TEST(deque_rolling_average_slides_over_window)
{
	RollingAverage rolling(3);

	CHECK_NEAR(rolling.add(1), 1.0, 1e-9);
	CHECK_NEAR(rolling.add(2), 1.5, 1e-9);
	CHECK_NEAR(rolling.add(3), 2.0, 1e-9);
	CHECK(rolling.full());
	CHECK_NEAR(rolling.add(4), 3.0, 1e-9); // 1 evicted -> (2+3+4)/3

	CHECK_EQ(rolling.size(), size_t(3));
	CHECK_EQ(rolling.values().front(), 2.0);
	CHECK_EQ(rolling.values().back(), 4.0);
}

TEST(deque_rolling_average_edge_cases)
{
	CHECK_THROWS(RollingAverage(0), std::invalid_argument);

	RollingAverage one(1);
	one.add(10);
	CHECK_NEAR(one.add(20), 20.0, 1e-9);

	RollingAverage empty(2);
	CHECK_NEAR(empty.average(), 0.0, 1e-9);

	empty.add(5);
	empty.clear();
	CHECK_EQ(empty.size(), size_t(0));
}

TEST(trend_uses_rolling_average_per_semester)
{
	std::vector<GradeRecord> records;
	for (auto [semester, score] : { std::pair<const char*, int>{"2025S1", 80},
									{"2025S2", 90}, {"2026S1", 100} })
	{
		auto r = recordsFromScores({ score }, semester);
		records.insert(records.end(), r.begin(), r.end());
	}

	auto points = computeTrend(records, 2);

	CHECK_EQ(points.size(), size_t(3));
	CHECK_NEAR(points[0].rollingMean, 80.0, 1e-9);
	CHECK_NEAR(points[1].rollingMean, 85.0, 1e-9);
	CHECK_NEAR(points[2].rollingMean, 95.0, 1e-9);
	CHECK(classifyTrend(points) == TrendDirection::Improving);
}

TEST(trend_classification_declining_stable_insufficient)
{
	auto make = [](int a, int b)
		{
			auto r1 = recordsFromScores({ a }, "2025S1");
			auto r2 = recordsFromScores({ b }, "2025S2");
			r1.insert(r1.end(), r2.begin(), r2.end());
			return computeTrend(r1);
		};

	CHECK(classifyTrend(make(90, 70)) == TrendDirection::Declining);
	CHECK(classifyTrend(make(80, 80)) == TrendDirection::Stable);
	CHECK(classifyTrend(computeTrend(recordsFromScores({ 80 }))) ==
		TrendDirection::Insufficient);
}
