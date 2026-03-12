#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Policy.h>
#include <RetryPP/Classifier.h>
#include <RetryPP/Backoff/Fixed.h>
#include <RetryPP/Limit/RetryLimit.h>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace WithRetry
	{
		TEST_CLASS(withRetryTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(RetryUntilLimit)
			{
				int retryCount = 0;

				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<RetryLimit>(3)
					.build();

				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientCode(1)
					.withRetryCallback([&](auto x, auto y) { ++retryCount; })
					.build();

				auto result = withRetry(policy, classifier, []() -> int { return 1; });

				Assert::IsTrue(Classification::Transient == result.classification);
				Assert::AreEqual(1, result.code);
				Assert::AreEqual(2, retryCount);
			}

			TEST_METHOD(RetryUntilLimitWithException)
			{
				int retryCount = 0;

				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<RetryLimit>(3)
					.build();

				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientCode(1)
					.withExceptionClassifier([](auto x) { return Classification::Transient; })
					.withRetryCallback([&](auto x, auto y) { ++retryCount; })
					.build();

				Assert::ExpectException<std::runtime_error>([&] { return withRetry(policy, classifier, []() -> int { throw std::runtime_error("runtime error"); }); });
				Assert::AreEqual(2, retryCount);
			}

			TEST_METHOD(RetryUntilSuccess)
			{
				int retryCount = 0;

				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<RetryLimit>(999)
					.build();


				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(100)
					.withUndefinedCodeClassification(Classification::Transient)
					.withRetryCallback([&](auto x, auto y) { ++retryCount; })
					.build();

				int code = 95;
				auto result = withRetry(policy, classifier, [&] { return code++; });
				Assert::IsTrue(result.classification == Classification::Success);
				Assert::AreEqual(100, result.code);
				Assert::AreEqual(5, retryCount);
			}

			TEST_METHOD(RetryWithStopToken)
			{
				int retryCount = 0;

				// Should take ~200 seconds to hit the retry limit
				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(200ms)
					.withLimit<RetryLimit>(999)
					.build();

				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(100)
					.withUndefinedCodeClassification(Classification::Transient)
					.withRetryCallback([&](auto x, auto y) { ++retryCount; })
					.build();

				std::stop_source token_source;
				// Signal abort after 1 second
				std::jthread stopThread{ [&] { std::this_thread::sleep_for(1'100ms); token_source.request_stop(); } };

				auto start = std::chrono::steady_clock::now();
				auto result = withRetry(policy, classifier, token_source.get_token(), [] { return 1; });
				auto duration = std::chrono::steady_clock::now() - start;

				Assert::IsTrue(result.classification == Classification::Transient);
				Assert::AreEqual(1, result.code);
				Assert::AreEqual(6, retryCount);
				Assert::IsTrue(duration >= 1'100ms && duration <= 1'300ms);
			}

			TEST_METHOD(RetryWithParameters)
			{
				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<RetryLimit>(999)
					.build();

				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(5)
					.withUndefinedCodeClassification(Classification::Transient)
					.build();

				int param = 0;
				auto result = withRetry(policy, classifier, [](int& param) { return ++param; }, param);
				Assert::IsTrue(result.classification == Classification::Success);
				Assert::AreEqual(5, result.code);
			}
		};
	}
}