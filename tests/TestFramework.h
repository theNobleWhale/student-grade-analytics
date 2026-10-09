#pragma once

// Minimal dependency-free test framework (a stand-in for pytest so the
// project builds anywhere with no downloads).
//
//   TEST(my_test_name) { CHECK(1 + 1 == 2); }
//
// CHECK / CHECK_EQ / CHECK_NEAR / CHECK_THROWS record a failure and let the
// test continue; the runner prints every failure and returns non-zero.

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct TestCase {
	std::string name;
	std::function<void()> body;
};

inline std::vector<TestCase>& testRegistry()
{
	static std::vector<TestCase> registry;
	return registry;
}

inline int& failureCount()
{
	static int failures = 0;
	return failures;
}

struct TestRegistrar {
	TestRegistrar(const std::string& name, std::function<void()> body)
	{
		testRegistry().push_back({ name, std::move(body) });
	}
};

inline void reportFailure(const char* file, int line, const std::string& message)
{
	++failureCount();
	std::cerr << "    FAIL " << file << ":" << line << "  " << message << '\n';
}

#define TEST(name)                                                          \
	static void name();                                                     \
	static TestRegistrar registrar_##name(#name, name);                     \
	static void name()

#define CHECK(condition)                                                    \
	do { if (!(condition))                                                  \
		reportFailure(__FILE__, __LINE__, "CHECK(" #condition ")"); } while (0)

#define CHECK_EQ(actual, expected)                                          \
	do {                                                                    \
		auto a_ = (actual); auto e_ = (expected);                       \
		if (!(a_ == e_)) {                                                  \
			std::ostringstream os_;                                         \
			os_ << #actual " == " #expected "  (got " << a_                 \
				<< ", expected " << e_ << ")";                              \
			reportFailure(__FILE__, __LINE__, os_.str());                   \
		}                                                                   \
	} while (0)

#define CHECK_NEAR(actual, expected, tolerance)                             \
	do {                                                                    \
		double a_ = (actual); double e_ = (expected);                       \
		if (std::fabs(a_ - e_) > (tolerance)) {                             \
			std::ostringstream os_;                                         \
			os_ << #actual " ~= " #expected "  (got " << a_                 \
				<< ", expected " << e_ << ")";                              \
			reportFailure(__FILE__, __LINE__, os_.str());                   \
		}                                                                   \
	} while (0)

#define CHECK_THROWS(expression, ExceptionType)                             \
	do {                                                                    \
		bool threw_ = false;                                                \
		try { (void)(expression); }                                         \
		catch (const ExceptionType&) { threw_ = true; }                     \
		catch (...) {}                                                      \
		if (!threw_)                                                        \
			reportFailure(__FILE__, __LINE__,                               \
				"expected " #ExceptionType " from " #expression);           \
	} while (0)
