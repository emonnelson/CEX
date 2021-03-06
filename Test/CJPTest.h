#ifndef CEXTEST_CJPTEST_H
#define CEXTEST_CJPTEST_H

#include "ITest.h"
#include "../CEX/IProvider.h"

namespace Test
{
	using Provider::IProvider;

	/// <summary>
	/// Tests the cpu jitter entropy provider output with random sampling analysis, and stress tests
	/// </summary>
	class CJPTest final : public ITest
	{
	private:

		static const std::string CLASSNAME;
		static const std::string DESCRIPTION;
		static const std::string SUCCESS;
		static const size_t MAXM_ALLOC = 10240;
		static const size_t MINM_ALLOC = 1024;
		// 10KB sample, should be 100MB or more for accuracy
		// Note: the sample size must be evenly divisible by 8.
		static const size_t SAMPLE_SIZE = 10240;
		static const size_t TEST_CYCLES = 2;

		TestEventHandler m_progressEvent;

	public:

		/// <summary>
		/// Compares known answer CMAC vectors for equality
		/// </summary>
		CJPTest();

		/// <summary>
		/// Destructor
		/// </summary>
		~CJPTest();

		/// <summary>
		/// Get: The test description
		/// </summary>
		const std::string Description() override;

		/// <summary>
		/// Progress return event callback
		/// </summary>
		TestEventHandler &Progress() override;

		/// <summary>
		/// Start the tests
		/// </summary>
		std::string Run() override;

		/// <summary>
		///  Test drbg output using chisquare, mean value, and ordered runs tests
		/// </summary>
		void Evaluate(IProvider* Rng);

		/// <summary>
		/// Test exception handlers for correct execution
		/// </summary>
		void Exception();

		/// <summary>
		/// Test behavior parallel and sequential processing in a looping [TEST_CYCLES] stress-test using randomly sized input and data
		/// </summary>
		void Stress();

	private:

		void OnProgress(const std::string &Data);
	};
}

#endif
