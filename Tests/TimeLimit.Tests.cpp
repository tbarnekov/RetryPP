#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Limit/TimeLimit.h>
#include <random>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Limits
	{
		TEST_CLASS(TimeLimitTest)
		{
		public:

			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto limit = std::make_unique<TimeLimit>(std::chrono::seconds{ 3 });
				Assert::IsNotNull(limit.get());
				Assert::AreEqual(static_cast<count_t>(3000), limit->timeout().count());
			}

			TEST_METHOD(TimeLimitProgress)
			{
				std::random_device random_device;
				std::uniform_int_distribution<count_t> distribution(500, 5'000);
				const std::chrono::milliseconds time_limit = std::chrono::milliseconds{ distribution(random_device) };

				auto limit = std::make_unique<TimeLimit>(time_limit);
				Assert::AreEqual(time_limit.count(), limit->timeout().count());

				auto start = std::chrono::steady_clock::now();
				while (!limit->exhausted())
				{
					std::this_thread::sleep_for(100ms);
				}

				std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
				Assert::IsTrue(elapsed.count() >= time_limit.count());
				Assert::IsTrue(elapsed.count() - 100 < time_limit.count());
			}
		};
	}
}