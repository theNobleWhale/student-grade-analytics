#include "TestFramework.h"

int main()
{
	int failedTests = 0;

	for (const TestCase& test : testRegistry())
	{
		int before = failureCount();

		try
		{
			test.body();
		}
		catch (const std::exception& error)
		{
			reportFailure(__FILE__, __LINE__,
				std::string("unexpected exception: ") + error.what());
		}

		bool passed = failureCount() == before;
		if (!passed) ++failedTests;

		std::cout << (passed ? "[ PASS ] " : "[ FAIL ] ") << test.name << '\n';
	}

	std::cout << '\n' << (testRegistry().size() - failedTests) << " / "
		<< testRegistry().size() << " tests passed\n";

	return failedTests == 0 ? 0 : 1;
}
