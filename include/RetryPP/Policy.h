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
#include <functional>
#include <thread>

namespace RetryPP
{

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

	} // namespace internal


	class Policy final : public internal::PolicyData
	{
	public:
		Policy(const Policy&) noexcept = default;
		Policy(Policy&&) noexcept = default;
		Policy& operator=(const Policy&) noexcept = default;
		Policy& operator=(Policy&&) noexcept = default;
		~Policy() = default;

		static Policy null() noexcept
		{
			return {};
		}

		bool valid() const noexcept
		{
			return m_backoff_factory && m_limit_factory;
		}

		std::unique_ptr<Strategy> createBackoffStrategy() const
		{
			return m_backoff_factory();
		}

		std::unique_ptr<Limit> createLimitPolicy() const
		{
			return m_limit_factory();
		}

		std::vector<std::unique_ptr<Modifier>> createBackoffModifiers() const
		{
			std::vector<std::unique_ptr<Modifier>> modifiers;
			for (const auto& modifier_factory : m_backoff_modifier_factories)
				modifiers.emplace_back(std::move(modifier_factory()));
			return modifiers;
		}

	private:
		Policy() noexcept = default;

		using internal::PolicyData::m_backoff_factory;
		using internal::PolicyData::m_backoff_modifier_factories;
		using internal::PolicyData::m_limit_factory;
	};


	class PolicyBuilder final : public internal::PolicyData
	{
	public:
		template<RetryStrategy T, class... Args>
		PolicyBuilder& withStrategy(Args&&... args)
		{
			m_backoff_factory = createFactory<T>(std::forward<Args>(args)...);
			return *this;
		}

		template<RetryBackoffModifier T, class... Args>
		PolicyBuilder& withModifier(Args&&... args)
		{
			m_backoff_modifier_factories.emplace_back(std::move(createFactory<T>(std::forward<Args>(args)...)));
			return *this;
		}

		template<RetryLimitPolicy T, class... Args>
		PolicyBuilder& withLimit(Args&&... args)
		{
			m_limit_factory = createFactory<T>(std::forward<Args>(args)...);
			return *this;
		}

		Policy build() const
		{
			if (!m_backoff_factory)
				throw InvalidPolicy("Missing backoff strategy");
			if (!m_limit_factory)
				throw InvalidPolicy("Missing limit policy");

			return { *reinterpret_cast<const Policy*>(this) };
		}

	private:
		using internal::PolicyData::m_backoff_factory;
		using internal::PolicyData::m_backoff_modifier_factories;
		using internal::PolicyData::m_limit_factory;

		template<class T, class... Args>
		constexpr static inline Factory<T> createFactory(Args&&... args)
		{
			return [=] { return std::make_unique<T>(args...); };
		}
	};

	template<class T>
	struct RetryResult
	{
		T code;
		Classification classification;
	};

	template<class T, class F, class... Args>
	RetryResult<T> withRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args)
	{
		if (!policy.valid())
			throw InvalidPolicy();

		auto backoff = policy.createBackoffStrategy();
		auto backoff_modifiers = policy.createBackoffModifiers();
		auto limiter = policy.createLimitPolicy();

		while (true)
		{
			try
			{
				T code = f(std::forward<Args>(args)...);

				auto classification = classifier.classify(code);

				// If code is a success code, return it
				if (classification == Classification::Success)
					return { code, classification };

				// If code is a permanent error, return it
				if (classification == Classification::Permanent)
					return { code, classification };

				// Otherwise it must be a transient code...

				// If retries were exhausted, return the code
				if (limiter->exhausted() || limiter->time_remaining().count() == 0)
					return { code, classification };
			}
			catch (...)
			{
				// If the exception is classified as a permanent error, just rethrow it
				if (classifier.classify(std::current_exception()) == Classification::Permanent)
					throw;

				// If retries were exhausted, rethrow the exception
				if (limiter->exhausted() || limiter->time_remaining().count() == 0)
					throw;
			}

			auto delay = backoff->next();
			for (const auto& modifier : backoff_modifiers)
				modifier->apply(delay);

			std::this_thread::sleep_for(std::chrono::milliseconds{ std::min(limiter->time_remaining().count(), delay.count()) });
		}
	}

} // namespace RetryPP
