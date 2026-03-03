#include "include/RetryPP/Policy.h"
#include "include/RetryPP/Classifier.h"
#include "include/RetryPP/Backoff/Exponential.h"
#include "include/RetryPP/Backoff/Fixed.h"
#include "include/RetryPP/Backoff/Linear.h"
#include "include/RetryPP/Backoff/Modifier/Cap.h"
#include "include/RetryPP/Backoff/Modifier/Jitter.h"
#include "include/RetryPP/Limit/RetryLimit.h"
#include "include/RetryPP/Limit/TimeLimit.h"
#include <chrono>
#include <format>
#include <iostream>

using HttpResponseCode = int;


HttpResponseCode operation1()
{
	static HttpResponseCode responseCode = 1;
	std::cout << std::format("Running operation with response code {}\n", responseCode);
	return responseCode++;
}


HttpResponseCode operation2(HttpResponseCode responseCode, const std::chrono::steady_clock::time_point& start)
{
	std::cout << std::format("[+{:04} ms] Running operation with response code {}\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count(), responseCode);
	return responseCode;
}


RetryPP::Classification exceptionClassifier(std::exception_ptr e)
{
	try
	{
		std::rethrow_exception(e);
	}
	catch (const std::runtime_error&)
	{
		return RetryPP::Classification::Transient;
	}
	catch (...)
	{
		return RetryPP::Classification::Permanent;
	}
	return RetryPP::Classification::Success;
}


HttpResponseCode exceptionalOperation()
{
	static int x = 200;

	struct X
	{
		~X()
		{
			++x;
		}
	} increment_x;

	if (x % 6 == 0)
		throw std::logic_error("Logic error");
	if (x % 5 == 0)
		throw std::runtime_error("Runtime error");
	return x;
}

std::string toString(RetryPP::Classification c)
{
	switch (c)
	{
	case RetryPP::Classification::Success: return { "Success" };
	case RetryPP::Classification::Transient: return { "Transient" };
	case RetryPP::Classification::Permanent: return { "Permanent" };
	}
	return {};
}

int main()
{
	using namespace std::chrono_literals;
	using namespace RetryPP;

	Policy policy = PolicyBuilder()
		.withStrategy<Exponential>(200ms)
		.withModifier<Jitter<Algorithm::Full>>()
		.withModifier<Cap>(10s)
		.withLimit<RetryLimit>(3)
		.build();

	try
	{
		Classifier classifier = ClassifierBuilder<HttpResponseCode>()
			.withSuccessRange(100, 399)
			.withTransientCodes({ 408, 429, 500, 502, 503, 504 })
			.withUndefinedCodeClassification(Classification::Permanent)
			.withExceptionClassifier(&exceptionClassifier)
			.build();

		auto op1 = []() -> HttpResponseCode { return operation1(); };
		auto op2 = [](HttpResponseCode responseCode, const std::chrono::steady_clock::time_point& start) -> HttpResponseCode { return operation2(responseCode, start); };

		const auto start = std::chrono::steady_clock::now();

		withRetry(policy, classifier, &operation2, 1, start);
		withRetry(policy, classifier, op1);
		withRetry(policy, classifier, op2, 1, start);
		withRetry(policy, classifier, []() -> HttpResponseCode { return operation1(); });
		withRetry(policy, classifier, [](HttpResponseCode responseCode, const std::chrono::steady_clock::time_point& start) -> HttpResponseCode { return operation2(responseCode, start); }, 1, start);

		RetryResult result = withRetry(policy, classifier, &exceptionalOperation);
		std::cout << std::format("Returned code: {} [{}]\n", result.code, toString(result.classification));
	}
	catch (const std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
	catch (...)
	{
		std::cout << "Unknown exception\n";
	}
}
