#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Backoff/Modifier/Jitter.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Modifiers
	{
		TEST_CLASS(FullJitterTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto modifier = std::make_unique<Jitter<Algorithm::Full>>();
				Assert::IsNotNull(modifier.get());
			}

			TEST_METHOD(Apply)
			{
				auto modifier = std::make_unique<Jitter<Algorithm::Full>>();

				for (size_t i = 1; i < 100; ++i)
				{
					count_t time = i * 100;
					auto delay = std::chrono::milliseconds{ time };
					modifier->apply(delay);
					Assert::IsTrue(delay.count() >= static_cast<count_t>(0) && delay.count() <= time);
				}
			}
		};

		TEST_CLASS(EqualJitterTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto modifier = std::make_unique<Jitter<Algorithm::Equal>>();
				Assert::IsNotNull(modifier.get());
			}

			TEST_METHOD(Apply)
			{
				auto modifier = std::make_unique<Jitter<Algorithm::Equal>>();

				for (size_t i = 1; i < 100; ++i)
				{
					count_t time = i * 100;
					auto delay = std::chrono::milliseconds{ time };
					modifier->apply(delay);
					Assert::IsTrue(delay.count() >= static_cast<count_t>(time / 2) && delay.count() <= time);
				}
			}
		};

		TEST_CLASS(DecorrelatedJitterTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto modifier = std::make_unique<Jitter<Algorithm::Decorrelated>>();
				Assert::IsNotNull(modifier.get());
			}

			TEST_METHOD(Apply)
			{
				auto modifier = std::make_unique<Jitter<Algorithm::Decorrelated>>();

				auto delay = std::chrono::milliseconds{ 100 };
				for (size_t i = 0; i < 100; ++i)
				{
					auto last_delay = delay * 3;
					modifier->apply(delay);
					Assert::IsTrue(delay.count() >= static_cast<count_t>(0) && delay.count() <= last_delay.count());
				}
			}
		};
	}
}