#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Policy.h>
#include <RetryPP/Backoff/Exponential.h>
#include <RetryPP/Backoff/Linear.h>
#include <RetryPP/Backoff/Modifier/Cap.h>
#include <RetryPP/Backoff/Modifier/Jitter.h>
#include <RetryPP/Limit/NoLimit.h>
#include <RetryPP/Limit/RetryLimit.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Policies
	{
		TEST_CLASS(PolicyBuilderTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto builder = std::make_unique<PolicyBuilder>();
				Assert::IsNotNull(builder.get());
			}

			TEST_METHOD(Copy)
			{
				auto builder = PolicyBuilder()
					.withStrategy<Linear>(10ms)
					.withLimit<NoLimit>();

				auto builder2 = builder;
				auto policy = builder.build();
				auto policy2 = builder2.build();

				Assert::IsTrue(policy.valid());
				Assert::IsTrue(policy2.valid());
				Assert::IsNotNull(dynamic_cast<Linear*>(policy.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<Linear*>(policy2.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy.createLimitPolicy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy2.createLimitPolicy().get()));
			}

			TEST_METHOD(Move)
			{
				auto builder = PolicyBuilder()
					.withStrategy<Linear>(10ms)
					.withLimit<NoLimit>();

				auto builder2 = std::move(builder);
				Assert::ExpectException<RetryPP::InvalidPolicy>([&] { builder.build(); });

				auto policy = builder2.build();

				Assert::IsTrue(policy.valid());
				Assert::IsNotNull(dynamic_cast<Linear*>(policy.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy.createLimitPolicy().get()));
			}

			TEST_METHOD(BuilderFromPolicy)
			{
				auto policy = PolicyBuilder()
					.withStrategy<Linear>(10ms)
					.withLimit<NoLimit>()
					.build();

				Assert::IsTrue(policy.valid());
				Assert::IsNotNull(dynamic_cast<Linear*>(policy.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy.createLimitPolicy().get()));

				PolicyBuilder builder{ policy };
				auto policy2 = builder.build();
				Assert::IsTrue(policy2.valid());
				Assert::IsNotNull(dynamic_cast<Linear*>(policy2.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy2.createLimitPolicy().get()));
			}

			TEST_METHOD(InvalidPolicy)
			{
				PolicyBuilder builder;
				const auto buildPolicy = [&] { builder.build(); };

				Assert::ExpectException<RetryPP::InvalidPolicy>(buildPolicy);

				builder = PolicyBuilder().withStrategy<Linear>(100ms);
				Assert::ExpectException<RetryPP::InvalidPolicy>(buildPolicy);

				builder = PolicyBuilder().withLimit<NoLimit>();
				Assert::ExpectException<RetryPP::InvalidPolicy>(buildPolicy);

				builder = PolicyBuilder().withStrategy<Linear>(100ms).withLimit<NoLimit>();
				auto policy = builder.build();
			}

			TEST_METHOD(StrategyFactory)
			{
				auto builder = PolicyBuilder();
				builder.withStrategy<Linear>(100ms);
				builder.withLimit<NoLimit>();
				auto policy = builder.build();

				Assert::IsTrue(policy.valid());
				Assert::IsNull(dynamic_cast<Exponential*>(policy.createBackoffStrategy().get()));
				Assert::IsNotNull(dynamic_cast<Linear*>(policy.createBackoffStrategy().get()));
			}

			TEST_METHOD(LimitFactory)
			{
				auto builder = PolicyBuilder();
				builder.withStrategy<Linear>(100ms);
				builder.withLimit<NoLimit>();
				auto policy = builder.build();

				Assert::IsTrue(policy.valid());
				Assert::IsNull(dynamic_cast<RetryLimit*>(policy.createLimitPolicy().get()));
				Assert::IsNotNull(dynamic_cast<NoLimit*>(policy.createLimitPolicy().get()));
			}

			TEST_METHOD(ModifierFactories)
			{
				auto builder = PolicyBuilder();
				builder.withStrategy<Linear>(100ms);
				builder.withModifier<Cap>(150ms);
				builder.withModifier<Jitter<Algorithm::Full>>();
				builder.withLimit<NoLimit>();
				auto policy = builder.build();

				Assert::IsTrue(policy.valid());

				auto modifiers = policy.createBackoffModifiers();
				Assert::AreEqual(size_t(2), modifiers.size());
				Assert::IsNotNull(dynamic_cast<Cap*>(modifiers.at(0).get()));
				Assert::IsNotNull(dynamic_cast<Jitter<Algorithm::Full>*>(modifiers.at(1).get()));
			}
		};
	}
}