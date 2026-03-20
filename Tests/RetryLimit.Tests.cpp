#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Limit/RetryLimit.h>
#include <random>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Limits
	{
		TEST_CLASS(RetryLimitTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				RetryLimit limit(3);
				Assert::AreEqual<size_t>(3, limit.maximum_retries());
			}

			TEST_METHOD(RetryLimitProgress)
			{
				std::random_device random_device;
				std::uniform_int_distribution<size_t> distribution(1, 100);
				const size_t max_retries = distribution(random_device);

				RetryLimit limit(max_retries);
				Assert::AreEqual(max_retries, limit.maximum_retries());

				for (size_t i = 0; i < max_retries - 1; ++i)
					Assert::IsFalse(limit.exhausted());
				Assert::IsTrue(limit.exhausted());
			}
		};
	}
}