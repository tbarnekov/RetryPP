#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Backoff/Modifier/Cap.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Modifiers
	{
		TEST_CLASS(CapTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction) noexcept
			{
				Cap modifier{ 100ms };
			}

			TEST_METHOD(CapApply)
			{
				Cap modifier{ 100ms };
				auto delay = 200ms;
				modifier.apply(delay);
				Assert::AreEqual<count_t>(100, delay.count());

				delay = 50ms;
				modifier.apply(delay);
				Assert::AreEqual<count_t>(50, delay.count());
			}
		};
	}
}