#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Limit/NoLimit.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Limits
	{
		TEST_CLASS(NoLimitTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction) noexcept
			{
				NoLimit limit;
			}

			TEST_METHOD(NoLimitProgress)
			{
				NoLimit limit;
				for (size_t i = 0; i < 100; ++i)
					Assert::AreEqual(false, limit.exhausted());
			}
		};
	}
}