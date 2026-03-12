#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Policy.h>
#include <RetryPP/Classifier.h>
#include <RetryPP/Backoff/Fixed.h>
#include <RetryPP/Limit/NoLimit.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Policies
	{
		TEST_CLASS(PolicyTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(InvalidPolicy)
			{
				auto policy = Policy::null();
				Assert::IsFalse(policy.valid());

				Assert::ExpectException<RetryPP::InvalidPolicy>([&] { withRetry(policy, ClassifierBuilder<bool>().withSuccessCode(true).build(), [] { return true; }); });
			}

			TEST_METHOD(Copy)
			{
				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<NoLimit>()
					.build();

				auto policy2 = policy;

				Assert::IsTrue(policy.valid());
				Assert::IsTrue(policy2.valid());
				Assert::IsNotNull(dynamic_cast<Fixed*>(policy.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<Fixed*>(policy2.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy.createLimitPolicy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy2.createLimitPolicy().get()));
			}

			TEST_METHOD(Move)
			{
				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<NoLimit>()
					.build();

				Assert::IsTrue(policy.valid());
				Assert::IsNotNull(dynamic_cast<Fixed*>(policy.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy.createLimitPolicy().get()));

				auto policy2{ std::move(policy) };

				Assert::IsFalse(policy.valid());
				Assert::IsTrue(policy2.valid());
				Assert::IsNotNull(dynamic_cast<Fixed*>(policy2.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy2.createLimitPolicy().get()));
			}

			TEST_METHOD(Valid)
			{
				auto policy = Policy::null();
				Assert::IsFalse(policy.valid());

				policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<NoLimit>()
					.build();

				Assert::IsTrue(policy.valid());
			}
		};
	}
}