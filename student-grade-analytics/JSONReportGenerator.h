#pragma once

#include "ReportTypes.h"
#include "StudentManager.h"

#include <cstddef>
#include <filesystem>

/// Options controlling how much detail the JSON report contains.
struct ReportOptions {
	/// Include every student's grades and each course's per-student list.
	/// These dominate file size (megabytes for 100k students).
	bool includeStudentDetails = true;

	std::size_t topN = 10;          // size of "topPerformers"
	std::size_t trendWindow = 2;    // semesters in the rolling average
	double trendTolerance = 1.0;    // points of change still called "stable"
};

class JSONReportGenerator {
public:
	/// Computes every summary section as a typed struct.
	AnalyticsReport buildReport(
		const StudentManager& studentManager,
		const ReportOptions& options = ReportOptions{}) const;

	/// Builds the report and writes it as indented JSON (keys keep their
	/// insertion order). Creates missing parent directories. Throws
	/// FilePermissionError if the file cannot be written.
	void saveReport(
		const std::filesystem::path& filename,
		const StudentManager& studentManager,
		const ReportOptions& options = ReportOptions{}) const;
};
