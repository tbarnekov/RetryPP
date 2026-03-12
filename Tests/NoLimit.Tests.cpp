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

			TEST_METHOD(Construction)
			{
				auto limit = std::make_unique<NoLimit>();
				Assert::IsNotNull(limit.get());
			}

			TEST_METHOD(NoLimitProgress)
			{
				auto limit = std::make_unique<NoLimit>();
				for (size_t i = 0; i < 100; ++i)
					Assert::AreEqual(false, limit->exhausted());
			}
		};
	}
}