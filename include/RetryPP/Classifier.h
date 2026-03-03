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
#include "Exceptions.h"
#include <concepts>
#include <chrono>
#include <exception>
#include <functional>
#include <optional>
#include <ranges>
#include <set>
#include <vector>

namespace RetryPP
{
	
	template<std::totally_ordered T>
	class Range
	{
	public:
		using Code = std::decay_t<T>;

		constexpr explicit Range(const Code& start, const Code& end) noexcept
			: m_start{ std::min(start, end) }, m_end{ std::max(start, end) }
		{
		}

		constexpr Range(const Range&) noexcept = default;
		constexpr Range(Range&&) noexcept = default;
		constexpr Range& operator=(const Range&) noexcept = default;
		constexpr Range& operator=(Range&&) noexcept = default;
		~Range() noexcept = default;

		constexpr const Code& start() const noexcept { return m_start; }
		constexpr const Code& end() const noexcept { return m_end; }

		constexpr bool in_range(const Code& code) const noexcept
		{
			return code >= m_start && code <= m_end;
		}

	private:
		const Code m_start;
		const Code m_end;
	};


	enum class Classification
	{
		Success,
		Transient,
		Permanent,
	};


	namespace internal
	{

		template<class T>
		class ClassifierData
		{
		public:
			using Range = Range<T>;
			using Code = Range::Code;

		protected:
			Classification m_undefined_code_classification = Classification::Transient;

			std::set<Code> m_success_codes;
			std::set<Code> m_transient_codes;
			std::set<Code> m_permanent_codes;

			std::vector<Range> m_success_ranges;
			std::vector<Range> m_transient_ranges;
			std::vector<Range> m_permanent_ranges;

			std::function<Classification(std::exception_ptr)> m_exception_classifier;
			std::function<void(const std::optional<Code>&, std::chrono::milliseconds)> m_retry_callback;
		};

	} // namespace internal


	template<class T>
	class Classifier final : public internal::ClassifierData<T>
	{
	public:
		using internal::ClassifierData<T>::Code;
		using internal::ClassifierData<T>::Range;

		Classifier(const Classifier&) noexcept = default;
		Classifier(Classifier&&) noexcept = default;
		Classifier& operator=(const Classifier&) noexcept = default;
		Classifier& operator=(Classifier&&) noexcept = default;
		~Classifier() = default;

		static Classifier null() { return {}; }

		bool valid() const
		{
			return !m_success_codes.empty() && !m_success_ranges.empty();
		}

		const std::span<const Code> successCodes() const noexcept { return m_success_codes; }
		const std::span<const Range> successRanges() const noexcept { return m_success_ranges; }
		const std::span<const Code> transientCodes() const noexcept { return m_transient_codes; }
		const std::span<const Range> transientRanges() const noexcept { return m_transient_ranges; }
		const std::span<const Code> permanentCodes() const noexcept { return m_permanent_codes; }
		const std::span<const Range> permanentRanges() const noexcept { return m_permanent_ranges; }

		bool isSuccessCode(const Code& code) const
		{
			const auto in_range = [&code](const Range& range) { return range.in_range(code); };
			return (
				std::ranges::find(m_success_codes, code) != m_success_codes.cend() ||
				std::ranges::find_if(m_success_ranges, in_range) != m_success_ranges.cend()
				) &&
				std::ranges::find(m_transient_codes, code) == m_transient_codes.cend() &&
				std::ranges::find(m_permanent_codes, code) == m_permanent_codes.cend();
		}

		bool isTransientCode(const Code& code) const
		{
			const auto in_range = [&code](const Range& range) { return range.in_range(code); };
			return (
				std::ranges::find(m_transient_codes, code) != m_transient_codes.cend() ||
				std::ranges::find_if(m_transient_ranges, in_range) != m_transient_ranges.cend()
				) &&
				std::ranges::find(m_success_codes, code) == m_success_codes.cend() &&
				std::ranges::find(m_permanent_codes, code) == m_permanent_codes.cend();
		}

		bool isPermanentCode(const Code& code) const
		{
			const auto in_range = [&code](const Range& range) { return range.in_range(code); };
			return (
				std::ranges::find(m_permanent_codes, code) != m_permanent_codes.cend() ||
				std::ranges::find_if(m_permanent_ranges, in_range) != m_permanent_ranges.cend()
				) &&
				std::ranges::find(m_success_codes, code) == m_success_codes.cend() &&
				std::ranges::find(m_transient_codes, code) == m_transient_codes.cend();
		}

		Classification classify(const Code& code) const
		{
			Classification classification = m_undefined_code_classification;

			// Check if code indicates success
			if (isSuccessCode(code))
				return Classification::Success;

			// Check if code is a transient error
			if (isTransientCode(code))
				return Classification::Transient;

			// Check if code is a permanent error
			if (isPermanentCode(code))
				return Classification::Permanent;

			return m_undefined_code_classification;
		}

		Classification classify(std::exception_ptr e) const
		{
			if (m_exception_classifier)
				return m_exception_classifier(e);

			std::rethrow_exception(e);
		}

		void onRetry(const std::optional<Code>& code, std::chrono::milliseconds sleep) const
		{
			if (m_retry_callback)
				m_retry_callback(code, sleep);
		}

	private:
		using internal::ClassifierData<T>::m_undefined_code_classification;
		using internal::ClassifierData<T>::m_success_codes;
		using internal::ClassifierData<T>::m_transient_codes;
		using internal::ClassifierData<T>::m_permanent_codes;
		using internal::ClassifierData<T>::m_success_ranges;
		using internal::ClassifierData<T>::m_transient_ranges;
		using internal::ClassifierData<T>::m_permanent_ranges;
		using internal::ClassifierData<T>::m_exception_classifier;
		using internal::ClassifierData<T>::m_retry_callback;

		Classifier() noexcept = default;
	};


	template<class T>
	class ClassifierBuilder final : public internal::ClassifierData<T>
	{
	public:
		using internal::ClassifierData<T>::Code;

		ClassifierBuilder& withSuccessCode(const T& code)
		{
			m_success_codes.insert(code);
			return *this;
		}

		ClassifierBuilder& withSuccessCodes(const std::set<Code>& codes)
		{
			m_success_codes.insert(codes.cbegin(), codes.cend());
			return *this;
		}

		ClassifierBuilder& withSuccessRange(const Code& start, const Code& end)
		{
			m_success_ranges.emplace_back(Range<Code>{ start, end });
			return *this;
		}

		ClassifierBuilder& withTransientCode(const T& code)
		{
			m_transient_codes.insert(code);
			return *this;
		}

		ClassifierBuilder& withTransientCodes(const std::set<Code>& codes)
		{
			m_transient_codes.insert(codes.cbegin(), codes.cend());
			return *this;
		}

		ClassifierBuilder& withTransientRange(const Code& start, const Code& end)
		{
			m_transient_ranges.emplace_back(Range<Code>{ start, end });
			return *this;
		}

		ClassifierBuilder& withPermanentCode(const T& code)
		{
			m_permanent_codes.insert(code);
			return *this;
		}

		ClassifierBuilder& withPermanentCodes(const std::set<Code>& codes)
		{
			m_permanent_codes.insert(codes.cbegin(), codes.cend());
			return *this;
		}

		ClassifierBuilder& withPermanentRange(const Code& start, const Code& end)
		{
			m_permanent_ranges.emplace_back(Range<Code>{ start, end });
			return *this;
		}

		ClassifierBuilder& withUndefinedCodeClassification(Classification classification) noexcept
		{
			m_undefined_code_classification = classification;
			return *this;
		}

		ClassifierBuilder& withExceptionClassifier(const std::function<Classification(std::exception_ptr)>& f)
		{
			m_exception_classifier = f;
			return *this;
		}

		ClassifierBuilder& withRetryCallback(const std::function<void(const std::optional<Code>& code, std::chrono::milliseconds)>& f)
		{
			m_retry_callback = f;
			return *this;
		}

		Classifier<T> build() const
		{
			if (m_success_codes.empty() && m_success_ranges.empty())
				throw InvalidClassifier("At least one success code must be defined");

			return Classifier<T>{ *reinterpret_cast<const Classifier<T>*>(this) };
		}

	private:
		using internal::ClassifierData<T>::m_undefined_code_classification;
		using internal::ClassifierData<T>::m_success_codes;
		using internal::ClassifierData<T>::m_transient_codes;
		using internal::ClassifierData<T>::m_permanent_codes;
		using internal::ClassifierData<T>::m_success_ranges;
		using internal::ClassifierData<T>::m_transient_ranges;
		using internal::ClassifierData<T>::m_permanent_ranges;
		using internal::ClassifierData<T>::m_exception_classifier;
		using internal::ClassifierData<T>::m_retry_callback;
	};

} // namespace RetryPP
