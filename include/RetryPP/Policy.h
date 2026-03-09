/*
MIT License

Copyright (c) 2026 Thomas Barnekov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include "Classifier.h"
#include "Exceptions.h"
#include "Backoff/Strategy.h"
#include "Backoff/Modifier/Modifier.h"
#include "Limit/Limit.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <variant>

namespace RetryPP
{
	template<class T>
	struct RetryResult
	{
		RetryResult(Classification classification, const T& result) noexcept;

		RetryResult(const RetryResult&) noexcept = default;
		RetryResult(RetryResult&&) noexcept = default;
		RetryResult& operator=(const RetryResult&) noexcept = default;
		RetryResult& operator=(RetryResult&&) noexcept = default;
		~RetryResult() = default;

		Classification classification;
		T code;
	};


	namespace internal
	{

		class PolicyData
		{
		protected:
			template<class T>
			using Factory = std::function<std::unique_ptr<T>()>;

			Factory<Strategy> m_backoff_factory;
			std::vector<Factory<Modifier>> m_backoff_modifier_factories;
			Factory<Limit> m_limit_factory;
		};

		template<class T>
		struct wrapped_task;

		template<template<class> class Task, class T>
		struct wrapped_task<Task<T>>
		{
			using type = Task<RetryResult<T>>;
		};

		template<typename R, typename... Args>
		static R function_return_type(R(*)(Args...));

		template<typename R, typename... Args>
		static R function_return_type(R(*)(Args..., ...));


	} // namespace internal


	class PolicyBuilder;


	class Policy final : public internal::PolicyData
	{
	public:
		Policy(const Policy&) noexcept = default;
		Policy(Policy&&) noexcept = default;
		Policy& operator=(const Policy&) noexcept = default;
		Policy& operator=(Policy&&) noexcept = default;
		~Policy() = default;

		static Policy null() noexcept;
		bool valid() const noexcept;

		std::unique_ptr<Strategy> createBackoffStrategy() const;
		std::unique_ptr<Limit> createLimitPolicy() const;
		std::vector<std::unique_ptr<Modifier>> createBackoffModifiers() const;

	private:
		friend class PolicyBuilder;

		Policy() noexcept = default;
		explicit Policy(const internal::PolicyData& data) noexcept;
		explicit Policy(internal::PolicyData&& data) noexcept;

		using internal::PolicyData::m_backoff_factory;
		using internal::PolicyData::m_backoff_modifier_factories;
		using internal::PolicyData::m_limit_factory;
	};


	class PolicyBuilder final : public internal::PolicyData
	{
	public:
		PolicyBuilder() noexcept = default;
		PolicyBuilder(const PolicyBuilder&) noexcept = default;
		PolicyBuilder(PolicyBuilder&&) noexcept = default;
		PolicyBuilder& operator=(const PolicyBuilder&) noexcept = default;
		PolicyBuilder& operator=(PolicyBuilder&&) noexcept = default;
		~PolicyBuilder() noexcept = default;

		explicit PolicyBuilder(const Policy& policy) noexcept;

		template<RetryStrategy T, class... Args>
		PolicyBuilder& withStrategy(Args&&... args);
		
		template<RetryBackoffModifier T, class... Args>
		PolicyBuilder& withModifier(Args&&... args);

		template<RetryLimitPolicy T, class... Args>
		PolicyBuilder& withLimit(Args&&... args);

		PolicyBuilder& resetModifiers() noexcept;

		const Policy build() const;

	private:
		using internal::PolicyData::m_backoff_factory;
		using internal::PolicyData::m_backoff_modifier_factories;
		using internal::PolicyData::m_limit_factory;

		template<class T, class... Args>
		constexpr static inline Factory<T> createFactory(Args&&... args);
	};


	template<class T, class F, class... Args>
	RetryResult<T> withRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args);

	template<class T, class F, class... Args>
	RetryResult<T> withRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args);


	template<class TaskResultType, class T, class F, class... Args>
	TaskResultType withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args);

	template<class TaskResultType, class T, class F, class... Args>
	TaskResultType withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args);

	template<class T, class F, class... Args>
	auto withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args) -> typename internal::wrapped_task<decltype(internal::function_return_type(f))>::type
	{
		co_return co_await withAsyncRetry<typename internal::wrapped_task<decltype(internal::function_return_type(f))>::type>(policy, classifier, stop_token, std::forward<F>(f), std::forward<Args>(args)...);
	}

	template<class T, class F, class... Args>
	auto withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args) -> typename internal::wrapped_task<decltype(internal::function_return_type(f))>::type
	{
		std::stop_source token_source;
		co_return co_await withAsyncRetry<typename internal::wrapped_task<decltype(internal::function_return_type(f))>::type>(policy, classifier, token_source.get_token(), std::forward<F>(f), std::forward<Args>(args)...);
	}

} // namespace RetryPP


//////////////////////////////////////////////////////////////////////////
// RetryResult implementation

template<class T>
RetryPP::RetryResult<T>::RetryResult(Classification classification, const T& code) noexcept
	: classification{ classification }, code{ code }
{
}


//////////////////////////////////////////////////////////////////////////
// Policy implementation

RetryPP::Policy::Policy(const internal::PolicyData& data) noexcept
	: internal::PolicyData{ data }
{
}

RetryPP::Policy::Policy(internal::PolicyData&& data) noexcept
	: internal::PolicyData{ std::move(data) }
{
}

RetryPP::Policy RetryPP::Policy::null() noexcept
{
	return {};
}

bool RetryPP::Policy::valid() const noexcept
{
	return m_backoff_factory && m_limit_factory;
}

std::unique_ptr<RetryPP::Strategy> RetryPP::Policy::createBackoffStrategy() const
{
	return m_backoff_factory();
}

std::unique_ptr<RetryPP::Limit> RetryPP::Policy::createLimitPolicy() const
{
	return m_limit_factory();
}

std::vector<std::unique_ptr<RetryPP::Modifier>> RetryPP::Policy::createBackoffModifiers() const
{
	std::vector<std::unique_ptr<RetryPP::Modifier>> modifiers;
	for (const auto& modifier_factory : m_backoff_modifier_factories)
		modifiers.emplace_back(modifier_factory());
	return modifiers;
}


//////////////////////////////////////////////////////////////////////////
// PolicyBuilder implementation

RetryPP::PolicyBuilder::PolicyBuilder(const Policy& policy) noexcept
	: RetryPP::internal::PolicyData{ policy }
{
}

template<RetryPP::RetryStrategy T, class... Args>
RetryPP::PolicyBuilder& RetryPP::PolicyBuilder::withStrategy(Args&&... args)
{
	m_backoff_factory = createFactory<T>(std::forward<Args>(args)...);
	return *this;
}

template<RetryPP::RetryBackoffModifier T, class... Args>
RetryPP::PolicyBuilder& RetryPP::PolicyBuilder::withModifier(Args&&... args)
{
	m_backoff_modifier_factories.emplace_back(createFactory<T>(std::forward<Args>(args)...));
	return *this;
}

template<RetryPP::RetryLimitPolicy T, class... Args>
RetryPP::PolicyBuilder& RetryPP::PolicyBuilder::withLimit(Args&&... args)
{
	m_limit_factory = createFactory<T>(std::forward<Args>(args)...);
	return *this;
}

RetryPP::PolicyBuilder& RetryPP::PolicyBuilder::resetModifiers() noexcept
{
	m_backoff_modifier_factories.clear();
	return *this;
}

const RetryPP::Policy RetryPP::PolicyBuilder::build() const
{
	if (!m_backoff_factory)
		throw InvalidPolicy("Missing backoff strategy");
	if (!m_limit_factory)
		throw InvalidPolicy("Missing limit policy");

	return Policy{ *this };
}

template<class T, class... Args>
constexpr inline RetryPP::PolicyBuilder::Factory<T> RetryPP::PolicyBuilder::createFactory(Args&&... args)
{
	return [=] { return std::make_unique<T>(args...); };
}


//////////////////////////////////////////////////////////////////////////
// withRetry implementation

template<class T, class F, class... Args>
RetryPP::RetryResult<T> RetryPP::withRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args)
{
	if (!policy.valid())
		throw InvalidPolicy();

	auto backoff = policy.createBackoffStrategy();
	auto backoff_modifiers = policy.createBackoffModifiers();
	auto limiter = policy.createLimitPolicy();

	while (true)
	{
		std::variant<T, std::exception_ptr> result;
		std::optional<Classification> classification;
		try
		{
			result = f(std::forward<Args>(args)...);

			classification = classifier.classify(std::get<T>(result));

			// If code is a success code, return it
			if (classification.value() == Classification::Success)
				return { classification.value(), std::get<T>(result) };

			// If code is a permanent error, return it
			if (classification.value() == Classification::Permanent)
				return { classification.value(), std::get<T>(result) };

			// Otherwise it must be a transient code...

			// If retries were exhausted, return the code
			if (limiter->exhausted() || limiter->time_remaining().count() == 0)
				return { classification.value(), std::get<T>(result) };
		}
		catch (...)
		{
			// If the exception is classified as a permanent error, just rethrow it
			if (classifier.classify(std::current_exception()) == Classification::Permanent)
				throw;

			// If retries were exhausted, rethrow the exception
			if (limiter->exhausted() || limiter->time_remaining().count() == 0)
				throw;

			result = std::current_exception();
		}

		auto delay = backoff->next();
		for (const auto& modifier : backoff_modifiers)
			modifier->apply(delay);

		// Clamp delay to either calculated delay or time remaining on the limiter
		delay = std::chrono::milliseconds{ std::min(limiter->time_remaining().count(), delay.count()) };

		// Notify caller that we're retrying
		classifier.onRetry(result, delay);

		std::mutex mutex;
		std::condition_variable_any condvar;

		std::unique_lock lock(mutex);
		if (condvar.wait_for(lock, stop_token, delay, [&] { return stop_token.stop_requested(); }))
		{
			// If stop is requested rethrow latest exception (if present)
			if (std::holds_alternative<std::exception_ptr>(result))
				std::rethrow_exception(std::get<std::exception_ptr>(result));

			// Otherwise return latest result
			if (std::holds_alternative<T>(result))
				return { classification.value(), std::get<T>(result) };
		}
	}
}

template<class T, class F, class... Args>
RetryPP::RetryResult<T> RetryPP::withRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args)
{
	std::stop_source token_source;
	return withRetry(policy, classifier, token_source.get_token(), std::forward<F>(f), std::forward<Args>(args)...);
}


//////////////////////////////////////////////////////////////////////////
// withAsyncRetry implementation

template<class TaskResultType, class T, class F, class... Args>
TaskResultType RetryPP::withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args)
{
	if (!policy.valid())
		throw InvalidPolicy();

	auto backoff = policy.createBackoffStrategy();
	auto backoff_modifiers = policy.createBackoffModifiers();
	auto limiter = policy.createLimitPolicy();

	while (true)
	{
		std::variant<T, std::exception_ptr> result;
		std::optional<Classification> classification;
		try
		{
			result = co_await f(std::forward<Args>(args)...);

			classification = classifier.classify(std::get<T>(result));

			// If code is a success code, return it
			if (classification.value() == Classification::Success)
				co_return { classification.value(), std::get<T>(result) };

			// If code is a permanent error, return it
			if (classification.value() == Classification::Permanent)
				co_return { classification.value(), std::get<T>(result) };

			// Otherwise it must be a transient code...

			// If retries were exhausted, return the code
			if (limiter->exhausted() || limiter->time_remaining().count() == 0)
				co_return { classification.value(), std::get<T>(result) };
		}
		catch (...)
		{
			// If the exception is classified as a permanent error, just rethrow it
			if (classifier.classify(std::current_exception()) == Classification::Permanent)
				throw;

			// If retries were exhausted, rethrow the exception
			if (limiter->exhausted() || limiter->time_remaining().count() == 0)
				throw;

			result = std::current_exception();
		}

		auto delay = backoff->next();
		for (const auto& modifier : backoff_modifiers)
			modifier->apply(delay);

		// Clamp delay to either calculated delay or time remaining on the limiter
		delay = std::chrono::milliseconds{ std::min(limiter->time_remaining().count(), delay.count()) };

		// Notify caller that we're retrying
		classifier.onRetry(result, delay);

		std::mutex mutex;
		std::condition_variable_any condvar;

		std::unique_lock lock(mutex);
		if (condvar.wait_for(lock, stop_token, delay, [&] { return stop_token.stop_requested(); }))
		{
			// If stop is requested rethrow latest exception (if present)
			if (std::holds_alternative<std::exception_ptr>(result))
				std::rethrow_exception(std::get<std::exception_ptr>(result));

			// Otherwise return latest result
			if (std::holds_alternative<T>(result))
				co_return { classification.value(), std::get<T>(result) };
		}
	}
}

template<class TaskResultType, class T, class F, class... Args>
TaskResultType RetryPP::withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args)
{
	std::stop_source token_source;
	co_return co_await withAsyncRetry<TaskResultType>(policy, classifier, token_source.get_token(), std::forward<F>(f), std::forward<Args>(args)...);
}
