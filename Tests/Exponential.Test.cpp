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
				auto strategy = std::make_unique<Exponential>(100ms);
				Assert::IsNotNull(strategy.get());
				Assert::AreEqual(static_cast<count_t>(100), strategy->initial_delay().count());
				Assert::AreEqual(2.0f, strategy->multiplier());

				strategy = std::make_unique<Exponential>(120ms, 1.5f);
				Assert::IsNotNull(strategy.get());
				Assert::AreEqual(static_cast<count_t>(120), strategy->initial_delay().count());
				Assert::AreEqual(1.5f, strategy->multiplier());
			}

			TEST_METHOD(Progress)
			{
				auto strategy = std::make_unique<Exponential>(200ms, 2.0f);
				Assert::AreEqual(static_cast<count_t>(200), strategy->next().count());
				Assert::AreEqual(static_cast<count_t>(400), strategy->next().count());
				Assert::AreEqual(static_cast<count_t>(800), strategy->next().count());
			}

			TEST_METHOD(ProgressWithMultiplier)
			{
				auto strategy = std::make_unique<Exponential>(100ms, 1.5f);
				Assert::AreEqual(static_cast<count_t>(100), strategy->next().count());
				Assert::AreEqual(static_cast<count_t>(150), strategy->next().count());
				Assert::AreEqual(static_cast<count_t>(225), strategy->next().count());
			}
		};
	}
}
