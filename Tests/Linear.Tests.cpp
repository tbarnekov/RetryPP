#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Backoff/Linear.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Strategies
	{
		TEST_CLASS(LinearTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				Linear strategy{ 100ms };
				Assert::AreEqual(static_cast<count_t>(100), strategy.initial_delay().count());
			}

			TEST_METHOD(LinearProgress)
			{
				Linear strategy{ 200ms };
				Assert::AreEqual(static_cast<count_t>(200), strategy.next().count());
				Assert::AreEqual(static_cast<count_t>(400), strategy.next().count());
				Assert::AreEqual(static_cast<count_t>(600), strategy.next().count());
			}
		};
	}
}