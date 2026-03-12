#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Policy.h>
#include <RetryPP/Classifier.h>
#include <RetryPP/Backoff/Fixed.h>
#include <RetryPP/Limit/RetryLimit.h>
#include <type_traits>
#include <stdexcept>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;


struct CustomWithCompare
{
	int code = 0;
	bool operator<(const CustomWithCompare& rhs) const { return code < rhs.code; }
};

struct CustomerWithSpaceship
{
	int code = 0;
	auto operator<=>(const CustomerWithSpaceship& rhs) const { return code <=> rhs.code; }
};

struct CustomWithoutCompare
{
	int code = 0;
};

template<>
struct std::less<CustomWithoutCompare>
{
	constexpr bool operator()(const CustomWithoutCompare& lhs, const CustomWithoutCompare& rhs) const noexcept
	{
		return lhs.code < rhs.code;
	}
};

namespace Tests
{
	namespace Classifiers
	{
		TEST_CLASS(ClassifierBuilderTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Construction)
			{
				auto intBuilder = std::make_unique<ClassifierBuilder<int>>();
				Assert::IsNotNull(intBuilder.get());

				auto boolBuilder = std::make_unique<ClassifierBuilder<bool>>();
				Assert::IsNotNull(boolBuilder.get());
			}

			TEST_METHOD(Copy)
			{
				auto builder = ClassifierBuilder<int>()
					.withSuccessRange(200, 210)
					.withTransientCode(201)
					.withPermanentCode(202);

				auto builder2 = builder;

				auto classifier = builder.build();
				auto classifier2 = builder2.build();

				Assert::IsTrue(classifier.classify(200) == Classification::Success);
				Assert::IsTrue(classifier.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier.classify(202) == Classification::Permanent);

				Assert::IsTrue(classifier2.classify(200) == Classification::Success);
				Assert::IsTrue(classifier2.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier2.classify(202) == Classification::Permanent);
			}

			TEST_METHOD(Move)
			{
				auto builder = ClassifierBuilder<int>()
					.withSuccessRange(200, 210)
					.withTransientCode(201)
					.withPermanentCode(202);

				auto builder2 = std::move(builder);

				Assert::ExpectException<RetryPP::InvalidClassifier>([&] { builder.build(); });
				auto classifier = builder2.build();

				Assert::IsTrue(classifier.classify(200) == Classification::Success);
				Assert::IsTrue(classifier.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier.classify(202) == Classification::Permanent);
			}

			TEST_METHOD(BuilderFromClassifier)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 210)
					.withTransientCode(201)
					.withPermanentCode(202)
					.build();

				ClassifierBuilder<int> builder{ classifier };
				auto classifier2 = builder.build();

				Assert::IsTrue(classifier.classify(200) == Classification::Success);
				Assert::IsTrue(classifier.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier.classify(202) == Classification::Permanent);
			}

			TEST_METHOD(InvalidClassifier)
			{
				auto builder = ClassifierBuilder<int>();
				Assert::ExpectException<RetryPP::InvalidClassifier>([&] { builder.build(); });

				builder.withTransientCode(200);
				Assert::ExpectException<RetryPP::InvalidClassifier>([&] { builder.build(); });

				builder.withPermanentCode(200);
				Assert::ExpectException<RetryPP::InvalidClassifier>([&] { builder.build(); });
			}

			TEST_METHOD(UndefinedCodeClassification)
			{
				auto builder = ClassifierBuilder<int>()
					.withSuccessCode(200);

				auto classifier = builder.build();
				Assert::IsTrue(Classification::Success == classifier.classify(200));

				// Undefined code defaults to Permanent classification
				Assert::IsTrue(Classification::Permanent == classifier.classify(300));

				classifier = builder
					.withUndefinedCodeClassification(Classification::Transient)
					.build();

				// Undefined code defaults to Permanent classification
				Assert::IsTrue(Classification::Transient == classifier.classify(300));
			}

			TEST_METHOD(CustomWithCompareOperator)
			{
				auto classifier = ClassifierBuilder<CustomWithCompare>()
					.withSuccessCode({ 200 })
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify({ 200 }));
				Assert::IsFalse(Classification::Success == classifier.classify({ 201 }));
			}

			TEST_METHOD(CustomWithSpaceshipOperator)
			{
				auto classifier = ClassifierBuilder<CustomerWithSpaceship>()
					.withSuccessCode({ 200 })
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify({ 200 }));
				Assert::IsFalse(Classification::Success == classifier.classify({ 201 }));
			}

			TEST_METHOD(CustomWithoutCompareOperator)
			{
				auto classifier = ClassifierBuilder<CustomWithoutCompare>()
					.withSuccessCode({ 200 })
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify({ 200 }));
				Assert::IsFalse(Classification::Success == classifier.classify({ 201 }));
			}

			TEST_METHOD(WithSuccessCode)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(200)
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify({ 200 }));
				Assert::IsFalse(Classification::Success == classifier.classify({ 201 }));
			}

			TEST_METHOD(WithSuccessCodes)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCodes({ 200, 201 })
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify({ 200 }));
				Assert::IsTrue(Classification::Success == classifier.classify({ 201 }));
				Assert::IsFalse(Classification::Success == classifier.classify({ 202 }));
			}

			TEST_METHOD(WithSuccessRange)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 299)
					.build();

				Assert::IsFalse(Classification::Success == classifier.classify({ 199 }));
				Assert::IsTrue(Classification::Success == classifier.classify({ 200 }));
				Assert::IsTrue(Classification::Success == classifier.classify({ 299 }));
				Assert::IsFalse(Classification::Success == classifier.classify({ 300 }));
			}

			TEST_METHOD(WithTransientCode)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientCode(200)
					.build();

				Assert::IsTrue(Classification::Transient == classifier.classify({ 200 }));
				Assert::IsFalse(Classification::Transient == classifier.classify({ 201 }));
			}

			TEST_METHOD(WithTransientCodes)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientCodes({ 200, 201 })
					.build();

				Assert::IsTrue(Classification::Transient == classifier.classify({ 200 }));
				Assert::IsTrue(Classification::Transient == classifier.classify({ 201 }));
				Assert::IsFalse(Classification::Transient == classifier.classify({ 202 }));
			}

			TEST_METHOD(WithTransientRange)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientRange(200, 299)
					.build();

				Assert::IsFalse(Classification::Transient == classifier.classify({ 199 }));
				Assert::IsTrue(Classification::Transient == classifier.classify({ 200 }));
				Assert::IsTrue(Classification::Transient == classifier.classify({ 299 }));
				Assert::IsFalse(Classification::Transient == classifier.classify({ 300 }));
			}

			TEST_METHOD(WithPermanentCode)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withPermanentCode(200)
					.withUndefinedCodeClassification(Classification::Transient)
					.build();

				Assert::IsTrue(Classification::Permanent == classifier.classify({ 200 }));
				Assert::IsFalse(Classification::Permanent == classifier.classify({ 201 }));
			}

			TEST_METHOD(WithPermanentCodes)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withPermanentCodes({ 200, 201 })
					.withUndefinedCodeClassification(Classification::Transient)
					.build();

				Assert::IsTrue(Classification::Permanent == classifier.classify({ 200 }));
				Assert::IsTrue(Classification::Permanent == classifier.classify({ 201 }));
				Assert::IsFalse(Classification::Permanent == classifier.classify({ 202 }));
			}

			TEST_METHOD(WithPermanentRange)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withPermanentRange(200, 299)
					.withUndefinedCodeClassification(Classification::Transient)
					.build();

				Assert::IsFalse(Classification::Permanent == classifier.classify({ 199 }));
				Assert::IsTrue(Classification::Permanent == classifier.classify({ 200 }));
				Assert::IsTrue(Classification::Permanent == classifier.classify({ 299 }));
				Assert::IsFalse(Classification::Permanent == classifier.classify({ 300 }));
			}

			TEST_METHOD(WithExceptionClassifier)
			{
				int runCount = 0;
				const auto exceptionClassifier = [&](std::exception_ptr e)
					{
						try
						{
							++runCount;
							std::rethrow_exception(e);
						}
						catch (const std::runtime_error&)
						{
							return Classification::Transient;
						}
						catch (...)
						{
							return Classification::Permanent;
						}
					};

				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withExceptionClassifier(exceptionClassifier)
					.build();

				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(200ms)
					.withLimit<RetryLimit>(3)
					.build();


				auto function = []() -> int { throw std::runtime_error("Runtime error"); };

				Assert::ExpectException<std::runtime_error>([&] { withRetry(policy, classifier, function); });
				Assert::AreEqual(3, runCount);
			}

			TEST_METHOD(WithRetryCallback)
			{
				int retryCallbackCount = 0;

				const auto retryCallback = [&](const std::variant<int, std::exception_ptr>& code, std::chrono::milliseconds delay)
					{
						Assert::IsTrue(std::holds_alternative<int>(code));
						Assert::AreEqual(1, std::get<int>(code));
						++retryCallbackCount;
					};

				const auto retryExcept = [&](const std::variant<int, std::exception_ptr>& code, std::chrono::milliseconds delay)
					{
						Assert::IsTrue(std::holds_alternative<std::exception_ptr>(code));
						Assert::ExpectException<std::runtime_error>([&] { std::rethrow_exception(std::get<std::exception_ptr>(code)); });
						++retryCallbackCount;
					};

				auto policy = PolicyBuilder()
					.withStrategy<Fixed>(10ms)
					.withLimit<RetryLimit>(3)
					.build();

				auto builder = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientCode(1)
					.withExceptionClassifier([](std::exception_ptr) { return Classification::Transient; })
					.withRetryCallback(retryCallback);

				auto classifier = builder.build();

				auto result = withRetry(policy, classifier, [] { return 1; });
				Assert::IsTrue(Classification::Transient == result.classification);
				Assert::AreEqual(1, result.code);
				Assert::AreEqual(2, retryCallbackCount);

				classifier = builder.withRetryCallback(retryExcept).build();

				retryCallbackCount = 0;
				Assert::ExpectException<std::runtime_error>([&] { withRetry(policy, classifier, []() -> int { throw std::runtime_error("runtime error"); }); });
				Assert::AreEqual(2, retryCallbackCount);
			}
		};
	}
}