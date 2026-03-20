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

			TEST_METHOD(Construction) noexcept
			{
				Jitter<Algorithm::Full> modifier;
			}

			TEST_METHOD(Apply)
			{
				Jitter<Algorithm::Full> modifier;

				for (size_t i = 1; i < 100; ++i)
				{
					count_t time = i * 100;
					auto delay = std::chrono::milliseconds{ time };
					modifier.apply(delay);
					Assert::IsTrue(delay.count() >= static_cast<count_t>(0) && delay.count() <= time);
				}
			}
		};

		TEST_CLASS(EqualJitterTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction) noexcept
			{
				Jitter<Algorithm::Equal> modifier;
			}

			TEST_METHOD(Apply)
			{
				Jitter<Algorithm::Equal> modifier;

				for (size_t i = 1; i < 100; ++i)
				{
					count_t time = i * 100;
					auto delay = std::chrono::milliseconds{ time };
					modifier.apply(delay);
					Assert::IsTrue(delay.count() >= time / count_t{ 2 } && delay.count() <= time);
				}
			}
		};

		TEST_CLASS(DecorrelatedJitterTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction) noexcept
			{
				Jitter<Algorithm::Decorrelated> modifier;
			}

			TEST_METHOD(Apply)
			{
				Jitter<Algorithm::Decorrelated> modifier;

				auto delay = std::chrono::milliseconds{ 100 };
				for (size_t i = 0; i < 100; ++i)
				{
					const auto last_delay = delay;
					modifier.apply(delay);

					Assert::IsTrue(delay.count() >= static_cast<count_t>(0));
					Assert::IsTrue(delay.count() <= last_delay.count() * 3);
				}
			}
		};
	}
}