#include "pch.h"
#include "CppUnitTest.h"
#include <RetryPP/Classifier.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace RetryPP;
using namespace std::chrono_literals;

namespace Tests
{
	namespace Classifiers
	{
		TEST_CLASS(ClassifierTest)
		{
		public:
			using count_t = std::chrono::milliseconds::rep;

			TEST_METHOD(Copy)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 210)
					.withTransientCode(201)
					.withPermanentCode(202)
					.build();

				Assert::IsTrue(classifier.valid());
				Assert::IsTrue(classifier.classify(200) == Classification::Success);
				Assert::IsTrue(classifier.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier.classify(202) == Classification::Permanent);

				auto classifier2 = classifier;

				Assert::IsTrue(classifier.valid());
				Assert::IsTrue(classifier.classify(200) == Classification::Success);
				Assert::IsTrue(classifier.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier.classify(202) == Classification::Permanent);

				Assert::IsTrue(classifier2.valid());
				Assert::IsTrue(classifier2.classify(200) == Classification::Success);
				Assert::IsTrue(classifier2.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier2.classify(202) == Classification::Permanent);
			}

			TEST_METHOD(Move)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 210)
					.withTransientCode(201)
					.withPermanentCode(202)
					.build();

				auto classifier2 = std::move(classifier);

				Assert::IsFalse(classifier.valid());
				Assert::IsTrue(classifier.classify(200) == Classification::Permanent);
				Assert::IsTrue(classifier.classify(201) == Classification::Permanent);
				Assert::IsTrue(classifier.classify(202) == Classification::Permanent);

				Assert::IsTrue(classifier2.valid());
				Assert::IsTrue(classifier2.classify(200) == Classification::Success);
				Assert::IsTrue(classifier2.classify(201) == Classification::Transient);
				Assert::IsTrue(classifier2.classify(202) == Classification::Permanent);
			}

			TEST_METHOD(Valid)
			{
				auto classifier = Classifier<int>::null();
				Assert::IsFalse(classifier.valid());

				classifier = ClassifierBuilder<int>()
					.withSuccessCode(200)
					.build();
				Assert::IsTrue(classifier.valid());

				classifier = ClassifierBuilder<int>()
					.withSuccessCodes({ 200, 201 })
					.build();
				Assert::IsTrue(classifier.valid());

				classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 210)
					.build();
				Assert::IsTrue(classifier.valid());

				classifier = ClassifierBuilder<int>()
					.withSuccessCode(1)
					.withSuccessRange(200, 210)
					.build();
				Assert::IsTrue(classifier.valid());
			}

			TEST_METHOD(SuccessClassifications)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(1)
					.withSuccessCodes({ 5, 6, 7 })
					.withSuccessRange(10, 15)
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify(1));
				Assert::IsTrue(Classification::Success == classifier.classify(5));
				Assert::IsTrue(Classification::Success == classifier.classify(12));
				Assert::IsFalse(Classification::Success == classifier.classify(100));
			}

			TEST_METHOD(TransientClassifications)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withTransientCode(1)
					.withTransientCodes({ 5, 6, 7 })
					.withTransientRange(10, 15)
					.build();

				Assert::IsTrue(Classification::Transient == classifier.classify(1));
				Assert::IsTrue(Classification::Transient == classifier.classify(5));
				Assert::IsTrue(Classification::Transient == classifier.classify(12));
				Assert::IsFalse(Classification::Transient == classifier.classify(100));
			}

			TEST_METHOD(PermanentClassifications)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessCode(0)
					.withPermanentCode(1)
					.withPermanentCodes({ 5, 6, 7 })
					.withPermanentRange(10, 15)
					.withUndefinedCodeClassification(Classification::Transient)
					.build();

				Assert::IsTrue(Classification::Permanent == classifier.classify(1));
				Assert::IsTrue(Classification::Permanent == classifier.classify(5));
				Assert::IsTrue(Classification::Permanent == classifier.classify(12));
				Assert::IsFalse(Classification::Permanent == classifier.classify(100));
			}

			TEST_METHOD(RangeExceptions)
			{
				auto classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 399)
					.withTransientCode(304)
					.withPermanentCode(305)
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify(200));
				Assert::IsTrue(Classification::Success == classifier.classify(201));
				Assert::IsTrue(Classification::Transient == classifier.classify(304));
				Assert::IsTrue(Classification::Permanent == classifier.classify(305));
				Assert::IsTrue(Classification::Success == classifier.classify(398));
				Assert::IsTrue(Classification::Success == classifier.classify(399));
			}

			TEST_METHOD(OverlappingRanges)
			{
				// Check prioritized ranges
				// Priority 1: Success
				// Priority 2: Transient
				// Priority 3: Permanent
				auto classifier = ClassifierBuilder<int>()
					.withSuccessRange(200, 399)
					.withTransientRange(375, 499)
					.withPermanentRange(475, 599)
					.build();

				Assert::IsTrue(Classification::Success == classifier.classify(399));
				Assert::IsTrue(Classification::Transient == classifier.classify(400));
				Assert::IsTrue(Classification::Transient == classifier.classify(480));
				Assert::IsTrue(Classification::Permanent== classifier.classify(500));
			}
		};
	}
}