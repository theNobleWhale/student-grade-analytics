cmake_minimum_required(VERSION 3.16)
project(StudentGradeAnalytics CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Everything except main.cpp, shared by the app and the tests.
set(CORE_SOURCES
  student-grade-analytics/Analytics.cpp
  student-grade-analytics/CSVReader.cpp
  student-grade-analytics/JSONReportGenerator.cpp
  student-grade-analytics/MemoryBenchmark.cpp
  student-grade-analytics/ReportGenerator.cpp
  student-grade-analytics/ReportTypes.cpp
  student-grade-analytics/StreamingAnalyzer.cpp
  student-grade-analytics/Student.cpp
  student-grade-analytics/StudentManager.cpp
  student-grade-analytics/Trends.cpp)

add_library(analytics_core STATIC ${CORE_SOURCES})
target_include_directories(analytics_core PUBLIC student-grade-analytics)

if(MSVC)
  target_compile_options(analytics_core PUBLIC /W4 /utf-8)
else()
  target_compile_options(analytics_core PUBLIC -Wall -Wextra)
endif()

add_executable(student-grade-analytics student-grade-analytics/main.cpp)
target_link_libraries(student-grade-analytics PRIVATE analytics_core)

enable_testing()

add_executable(run_tests
  tests/test_main.cpp
  tests/test_models.cpp
  tests/test_collections.cpp
  tests/test_analytics.cpp
  tests/test_io.cpp
  tests/test_memory.cpp)
target_link_libraries(run_tests PRIVATE analytics_core)
add_test(NAME all_tests COMMAND run_tests)
