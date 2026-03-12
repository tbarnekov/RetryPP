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

			TEST_METHOD(Construction)
			{
				auto modifier = std::make_unique<Cap>(100ms);
				Assert::IsNotNull(modifier.get());
			}
			TEST_METHOD(CapApply)
			{
				auto modifier = std::make_unique<Cap>(100ms);
				auto delay = 200ms;
				modifier->apply(delay);
				Assert::AreEqual(static_cast<count_t>(100), delay.count());

				delay = 50ms;
				modifier->apply(delay);
				Assert::AreEqual(static_cast<count_t>(50), delay.count());
			}
		};
	}
}