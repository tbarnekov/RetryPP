#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Backoff/Immediate.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Strategies
	{
		TEST_CLASS(ImmediateTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto strategy = std::make_unique<Immediate>();
				Assert::IsNotNull(strategy.get());
				Assert::AreEqual(static_cast<count_t>(0), strategy->initial_delay().count());
			}

			TEST_METHOD(ImmediateProgress)
			{
				auto strategy = std::make_unique<Immediate>();
				Assert::AreEqual(static_cast<count_t>(0), strategy->next().count());
				Assert::AreEqual(static_cast<count_t>(0), strategy->next().count());
				Assert::AreEqual(static_cast<count_t>(0), strategy->next().count());
			}
		};
	}
}
