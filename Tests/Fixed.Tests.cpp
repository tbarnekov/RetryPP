#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Backoff/Fixed.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Strategies
	{
		TEST_CLASS(FixedTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				Fixed strategy{ 100ms };
				Assert::AreEqual<count_t>(100, strategy.initial_delay().count());
			}

			TEST_METHOD(FixedProgress)
			{
				Fixed strategy{ 200ms };
				Assert::AreEqual<count_t>(200, strategy.next().count());
				Assert::AreEqual<count_t>(200, strategy.next().count());
				Assert::AreEqual<count_t>(200, strategy.next().count());
			}
		};
	}
}
