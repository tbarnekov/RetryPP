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
		throw std::logic_error("Permanent error");
	if (x % 5 == 0)
		throw std::runtime_error("Transient error");
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

class CustomResult
{
public:
	constexpr CustomResult(int v) noexcept
		: value{ v }
	{
	}

	constexpr int getValue() const noexcept { return value; }

private:
	int value = 0;
};

// std::less specialization for CustomResult
template<>
struct std::less<CustomResult>
{
	constexpr bool operator()(const CustomResult& a, const CustomResult& b) const noexcept
	{
		return a.getValue() < b.getValue();
	}
};

// hand-crafted comparator for CustomResult
struct CustomResultComparator
{
	constexpr bool operator()(const CustomResult& a, const CustomResult& b) const noexcept
	{
		return a.getValue() < b.getValue();
	}
};

template<class T>
void printResult(const T& code, RetryPP::Classification classification)
{
	std::cout << std::format("Classified code {}  as {}\n", code, toString(classification));
}

template<class T>
void printResult(const RetryPP::RetryResult<T>& result)
{
	printResult(result.code, result.classification);
}

int main()
{
	using namespace std::chrono_literals;
	using namespace RetryPP;

	Policy policy = PolicyBuilder()
		.withStrategy<Exponential>(200ms)
		.withModifier<Jitter<Algorithm::Full>>()
		.withModifier<Cap>(10s)
		.withLimit<RetryLimit>(5)
		.build();

	auto customClassifier = ClassifierBuilder<CustomResult, CustomResultComparator>()
		.withSuccessRange({ 299 }, { 200 })
		.withTransientCodes({ { 251 }, { 252 } })
		.withUndefinedCodeClassification(RetryPP::Classification::Permanent)
		.build();

	for (const auto& code : std::initializer_list<CustomResult>{ { 250 }, { 251 }, { 401} })
	{
		auto result = customClassifier.classify(code);
		printResult(code.getValue(), result);
	}

	const auto onRetry = [](const std::variant<HttpResponseCode, std::exception_ptr>& result, std::chrono::milliseconds delay)
		{
			if (std::holds_alternative<HttpResponseCode>(result))
				std::cout << std::format("Got code {}. Retrying in {} milliseconds\n", std::get<HttpResponseCode>(result), delay.count()).c_str();
			else if (std::holds_alternative<std::exception_ptr>(result))
			{
				try
				{
					std::rethrow_exception(std::get<std::exception_ptr>(result));
				}
				catch (const std::runtime_error& e)
				{
					std::cout << std::format("Runtime error: {}\n", e.what());
				}
				catch (const std::logic_error& e)
				{
					std::cout << std::format("Logic error: {}\n", e.what());
				}
				catch (const std::exception& e)
				{
					std::cout << std::format("Exception: {}\n", e.what());
				}
				catch (...)
				{
					std::cout << std::format("Unknown exception\n");
				}
			}
		};

	try
	{
		Classifier classifier = ClassifierBuilder<HttpResponseCode>()
			.withSuccessRange(100, 399)
			.withTransientCodes({ 408, 429, 500, 502, 503, 504 })
			.withUndefinedCodeClassification(Classification::Permanent)
			.withExceptionClassifier(&exceptionClassifier)
			.withRetryCallback(onRetry)
			.build();

		auto op1 = []() -> HttpResponseCode { return operation1(); };
		auto op2 = [](HttpResponseCode responseCode, const std::chrono::steady_clock::time_point& start) -> HttpResponseCode { return operation2(responseCode, start); };

		const auto start = std::chrono::steady_clock::now();

		printResult(withRetry(policy, classifier, &operation2, 1, start));
		printResult(withRetry(policy, classifier, op1));
		printResult(withRetry(policy, classifier, op2, 1, start));
		printResult(withRetry(policy, classifier, []() -> HttpResponseCode { return operation1(); }));
		printResult(withRetry(policy, classifier, [](HttpResponseCode responseCode, const std::chrono::steady_clock::time_point& start) -> HttpResponseCode { return operation2(responseCode, start); }, 1, start));

		printResult(withRetry(policy, classifier, &exceptionalOperation));

		std::stop_source token_source;
		Policy policy2 = PolicyBuilder(policy)
			.withStrategy<Linear>(3s)
			.resetModifiers()
			.build();

		std::jthread t([&] {
			std::this_thread::sleep_for(5s);
			token_source.request_stop();
		});

		const auto start2 = std::chrono::steady_clock::now();
		printResult(withRetry(policy2, classifier, token_source.get_token(), []() -> HttpResponseCode { return 408; }));
		std::cout << std::format("Finished in {} milliseconds\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start2));
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
