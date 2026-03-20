#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Backoff/Exponential.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Strategies
	{
		TEST_CLASS(ExponentialTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				Exponential strategy{ 100ms };
				Assert::AreEqual<count_t>(100, strategy.initial_delay().count());
				Assert::AreEqual<float>(2.0f, strategy.multiplier());

				Exponential strategy2{ 120ms, 1.5f };
				Assert::AreEqual<count_t>(120, strategy2.initial_delay().count());
				Assert::AreEqual<float>(1.5f, strategy2.multiplier());
			}

			TEST_METHOD(Progress)
			{
				Exponential strategy{ 200ms };
				Assert::AreEqual<count_t>(200, strategy.next().count());
				Assert::AreEqual<count_t>(400, strategy.next().count());
				Assert::AreEqual<count_t>(800, strategy.next().count());
			}

			TEST_METHOD(ProgressWithMultiplier)
			{
				Exponential strategy{ 100ms, 1.5f };
				Assert::AreEqual<count_t>(100, strategy.next().count());
				Assert::AreEqual<count_t>(150, strategy.next().count());
				Assert::AreEqual<count_t>(225, strategy.next().count());
			}
		};
	}
}
