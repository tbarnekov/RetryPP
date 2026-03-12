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
#include <algorithm>
#include <chrono>
#include <exception>
#include <functional>
#include <span>
#include <set>
#include <type_traits>
#include <variant>
#include <vector>

namespace RetryPP
{
	
	template<class T, class Comp = std::less<T>>
	class Range
	{
	public:
		using Code = std::decay_t<T>;

		inline constexpr explicit Range(const Code& start, const Code& end) noexcept;

		constexpr Range(const Range&) noexcept = default;
		constexpr Range(Range&&) noexcept = default;
		constexpr Range& operator=(const Range&) noexcept = default;
		constexpr Range& operator=(Range&&) noexcept = default;
		~Range() noexcept = default;

		constexpr const Code& start() const noexcept;
		constexpr const Code& end() const noexcept;

		constexpr bool in_range(const Code& code) const noexcept;

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

		template<class T, class Comp>
		class ClassifierData
		{
		public:
			using Range = Range<T, Comp>;
			using Code = Range::Code;

		protected:
			Classification m_undefined_code_classification = Classification::Permanent;

			std::set<Code, Comp> m_success_codes;
			std::set<Code, Comp> m_transient_codes;
			std::set<Code, Comp> m_permanent_codes;

			std::vector<Range> m_success_ranges;
			std::vector<Range> m_transient_ranges;
			std::vector<Range> m_permanent_ranges;

			std::function<Classification(std::exception_ptr)> m_exception_classifier;
			std::function<void(const std::variant<Code, std::exception_ptr>&, std::chrono::milliseconds)> m_retry_callback;
		};

	} // namespace internal


	template<class T, class Comp>
	class ClassifierBuilder;

	template<class T, class Comp = std::less<T>>
	class Classifier final : public internal::ClassifierData<T, Comp>
	{
	public:
		using Code = internal::ClassifierData<T, Comp>::Code;
		using Range = internal::ClassifierData<T, Comp>::Range;

		Classifier(const Classifier&) noexcept = default;
		Classifier(Classifier&&) noexcept = default;
		Classifier& operator=(const Classifier&) noexcept = default;
		Classifier& operator=(Classifier&&) noexcept = default;
		~Classifier() = default;

		static Classifier null();
		bool valid() const;

		const std::span<const Code> successCodes() const noexcept;
		const std::span<const Range> successRanges() const noexcept;
		const std::span<const Code> transientCodes() const noexcept;
		const std::span<const Range> transientRanges() const noexcept;
		const std::span<const Code> permanentCodes() const noexcept;
		const std::span<const Range> permanentRanges() const noexcept;
		const std::function<Classification(std::exception_ptr)> exceptionClassifier() const noexcept;

		bool isSuccessCode(const Code& code) const;
		bool isTransientCode(const Code& code) const;
		bool isPermanentCode(const Code& code) const;

		Classification classify(const Code& code) const;
		Classification classify(std::exception_ptr e) const;

		void onRetry(const std::variant<Code, std::exception_ptr>& result, std::chrono::milliseconds sleep) const;

	private:
		friend class ClassifierBuilder<T, Comp>;

		struct InRange
		{
			InRange(const Code& code) noexcept;
			constexpr bool operator()(const Range& range) const noexcept;

			const Code& m_code;
		};

		struct Equals
		{
			Equals(const Code& code) noexcept;
			constexpr bool operator()(const Code& code) const noexcept;

			const Code& m_code;
		};

		using internal::ClassifierData<T, Comp>::m_undefined_code_classification;
		using internal::ClassifierData<T, Comp>::m_success_codes;
		using internal::ClassifierData<T, Comp>::m_transient_codes;
		using internal::ClassifierData<T, Comp>::m_permanent_codes;
		using internal::ClassifierData<T, Comp>::m_success_ranges;
		using internal::ClassifierData<T, Comp>::m_transient_ranges;
		using internal::ClassifierData<T, Comp>::m_permanent_ranges;
		using internal::ClassifierData<T, Comp>::m_exception_classifier;
		using internal::ClassifierData<T, Comp>::m_retry_callback;

		Classifier() noexcept = default;

		explicit Classifier(const internal::ClassifierData<T, Comp>& data) noexcept;
		explicit Classifier(internal::ClassifierData<T, Comp>&& data) noexcept;
	};


	template<class T, class Comp = std::less<T>>
	class ClassifierBuilder final : public internal::ClassifierData<T, Comp>
	{
	public:
		using Code = internal::ClassifierData<T, Comp>::Code;
		using Range = internal::ClassifierData<T, Comp>::Range;

		ClassifierBuilder() noexcept = default;
		ClassifierBuilder(const ClassifierBuilder&) noexcept = default;
		ClassifierBuilder(ClassifierBuilder&&) noexcept = default;
		ClassifierBuilder& operator=(const ClassifierBuilder&) noexcept = default;
		ClassifierBuilder& operator=(ClassifierBuilder&&) noexcept = default;
		~ClassifierBuilder() noexcept = default;

		explicit ClassifierBuilder(const Classifier<T, Comp>& classifier) noexcept;

		ClassifierBuilder& withSuccessCode(const Code& code);
		ClassifierBuilder& withSuccessCodes(const std::set<Code, Comp>& codes);
		ClassifierBuilder& withSuccessRange(const Code& start, const Code& end);

		ClassifierBuilder& withTransientCode(const Code& code);
		ClassifierBuilder& withTransientCodes(const std::set<Code, Comp>& codes);
		ClassifierBuilder& withTransientRange(const Code& start, const Code& end);

		ClassifierBuilder& withPermanentCode(const Code& code);
		ClassifierBuilder& withPermanentCodes(const std::set<Code, Comp>& codes);
		ClassifierBuilder& withPermanentRange(const Code& start, const Code& end);

		ClassifierBuilder& withUndefinedCodeClassification(Classification classification) noexcept;

		ClassifierBuilder& withExceptionClassifier(const std::function<Classification(std::exception_ptr)>& f);

		ClassifierBuilder& withRetryCallback(const std::function<void(const std::variant<Code, std::exception_ptr>&, std::chrono::milliseconds)>& f);

		const Classifier<Code, Comp> build() const;

	private:
		using internal::ClassifierData<T, Comp>::m_undefined_code_classification;
		using internal::ClassifierData<T, Comp>::m_success_codes;
		using internal::ClassifierData<T, Comp>::m_transient_codes;
		using internal::ClassifierData<T, Comp>::m_permanent_codes;
		using internal::ClassifierData<T, Comp>::m_success_ranges;
		using internal::ClassifierData<T, Comp>::m_transient_ranges;
		using internal::ClassifierData<T, Comp>::m_permanent_ranges;
		using internal::ClassifierData<T, Comp>::m_exception_classifier;
		using internal::ClassifierData<T, Comp>::m_retry_callback;
	};

} // namespace RetryPP


//////////////////////////////////////////////////////////////////////////
// Range implementation

template<class T, class Comp>
constexpr RetryPP::Range<T, Comp>::Range(const Code& start, const Code& end) noexcept
	: m_start{ std::min(start, end, Comp{}) }, m_end{ std::max(start, end, Comp{}) }
{
}

template<class T, class Comp>
constexpr const RetryPP::Range<T, Comp>::Code& RetryPP::Range<T, Comp>::start() const noexcept
{
	return m_start;
}

template<class T, class Comp>
constexpr const RetryPP::Range<T, Comp>::Code& RetryPP::Range<T, Comp>::end() const noexcept
{
	return m_end;
}

template<class T, class Comp>
constexpr bool RetryPP::Range<T, Comp>::in_range(const Code& code) const noexcept
{
	Comp comp;
	return !comp(code, m_start) && !comp(m_end, code); // Equivalent to code >= m_start && code <= m_end
}


//////////////////////////////////////////////////////////////////////////
// Classifier implementation


template<class T, class Comp>
RetryPP::Classifier<T, Comp>::Classifier(const RetryPP::internal::ClassifierData<T, Comp>& data) noexcept
	: internal::ClassifierData<T, Comp>{ data }
{
}

template<class T, class Comp>
RetryPP::Classifier<T, Comp>::Classifier(RetryPP::internal::ClassifierData<T, Comp>&& data) noexcept
	: RetryPP::internal::ClassifierData<T, Comp>{ std::move(data) }
{
}

template<class T, class Comp>
RetryPP::Classifier<T, Comp> RetryPP::Classifier<T, Comp>::null()
{
	return {};
}

template<class T, class Comp>
bool RetryPP::Classifier<T, Comp>::valid() const
{
	return !m_success_codes.empty() && !m_success_ranges.empty();
}

template<class T, class Comp>
const std::span<const typename RetryPP::Classifier<T, Comp>::Code> RetryPP::Classifier<T, Comp>::successCodes() const noexcept
{
	return m_success_codes;
}

template<class T, class Comp>
const std::span<const typename RetryPP::Classifier<T, Comp>::Range> RetryPP::Classifier<T, Comp>::successRanges() const noexcept
{
	return m_success_ranges;
}

template<class T, class Comp>
const std::span<const typename RetryPP::Classifier<T, Comp>::Code> RetryPP::Classifier<T, Comp>::transientCodes() const noexcept
{
	return m_transient_codes;
}

template<class T, class Comp>
const std::span<const typename RetryPP::Classifier<T, Comp>::Range> RetryPP::Classifier<T, Comp>::transientRanges() const noexcept
{
	return m_transient_ranges;
}

template<class T, class Comp>
const std::span<const typename RetryPP::Classifier<T, Comp>::Code> RetryPP::Classifier<T, Comp>::permanentCodes() const noexcept
{
	return m_permanent_codes;
}

template<class T, class Comp>
const std::span<const typename RetryPP::Classifier<T, Comp>::Range> RetryPP::Classifier<T, Comp>::permanentRanges() const noexcept
{
	return m_permanent_ranges;
}

template<class T, class Comp>
const std::function<RetryPP::Classification(std::exception_ptr)> RetryPP::Classifier<T, Comp>::exceptionClassifier() const noexcept
{
	return m_exception_classifier;
}

template<class T, class Comp>
bool RetryPP::Classifier<T, Comp>::isSuccessCode(const Code& code) const
{
	return (
		std::ranges::find_if(m_success_codes, Equals{ code }) != m_success_codes.cend() ||
		std::ranges::find_if(m_success_ranges, InRange{ code }) != m_success_ranges.cend()
		) &&
		std::ranges::find_if(m_transient_codes, Equals{ code }) == m_transient_codes.cend() &&
		std::ranges::find_if(m_permanent_codes, Equals{ code }) == m_permanent_codes.cend();
}

template<class T, class Comp>
bool RetryPP::Classifier<T, Comp>::isTransientCode(const Code& code) const
{
	return (
		std::ranges::find_if(m_transient_codes, Equals{ code }) != m_transient_codes.cend() ||
		std::ranges::find_if(m_transient_ranges, InRange{ code }) != m_transient_ranges.cend()
		) &&
		std::ranges::find_if(m_success_codes, Equals{ code }) == m_success_codes.cend() &&
		std::ranges::find_if(m_permanent_codes, Equals{ code }) == m_permanent_codes.cend();
}

template<class T, class Comp>
bool RetryPP::Classifier<T, Comp>::isPermanentCode(const Code& code) const
{
	return (
		std::ranges::find_if(m_permanent_codes, Equals{ code }) != m_permanent_codes.cend() ||
		std::ranges::find_if(m_permanent_ranges, InRange{ code }) != m_permanent_ranges.cend()
		) &&
		std::ranges::find_if(m_success_codes, Equals{ code }) == m_success_codes.cend() &&
		std::ranges::find_if(m_transient_codes, Equals{ code }) == m_transient_codes.cend();
}

template<class T, class Comp>
RetryPP::Classification RetryPP::Classifier<T, Comp>::classify(const Code& code) const
{
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

template<class T, class Comp>
RetryPP::Classification RetryPP::Classifier<T, Comp>::classify(std::exception_ptr e) const
{
	if (m_exception_classifier)
		return m_exception_classifier(e);

	std::rethrow_exception(e);
}

template<class T, class Comp>
void RetryPP::Classifier<T, Comp>::onRetry(const std::variant<Code, std::exception_ptr>& result, std::chrono::milliseconds sleep) const
{
	if (m_retry_callback)
		m_retry_callback(result, sleep);
}

template<class T, class Comp>
RetryPP::Classifier<T, Comp>::InRange::InRange(const Code& code) noexcept
	: m_code{ code }
{
}

template<class T, class Comp>
constexpr bool RetryPP::Classifier<T, Comp>::InRange::operator()(const Range& range) const noexcept
{
	return range.in_range(m_code);
}

template<class T, class Comp>
RetryPP::Classifier<T, Comp>::Equals::Equals(const Code& code) noexcept
	: m_code{ code }
{
}

template<class T, class Comp>
constexpr bool RetryPP::Classifier<T, Comp>::Equals::operator()(const Code& code) const noexcept
{
	Comp comp;
	return !comp(code, m_code) && !comp(m_code, code); // Equivalent to code == m_code
}


//////////////////////////////////////////////////////////////////////////
// ClassifierBuilder implementation

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>::ClassifierBuilder(const Classifier<T, Comp>& classifier) noexcept
	: RetryPP::internal::ClassifierData<T, Comp>{ classifier }
{
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withSuccessCode(const Code& code)
{
	m_success_codes.insert(code);
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withSuccessCodes(const std::set<Code, Comp>& codes)
{
	m_success_codes.insert(codes.cbegin(), codes.cend());
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withSuccessRange(const Code& start, const Code& end)
{
	m_success_ranges.emplace_back(Range{ start, end });
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withTransientCode(const Code& code)
{
	m_transient_codes.insert(code);
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withTransientCodes(const std::set<Code, Comp>& codes)
{
	m_transient_codes.insert(codes.cbegin(), codes.cend());
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withTransientRange(const Code& start, const Code& end)
{
	m_transient_ranges.emplace_back(Range{ start, end });
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withPermanentCode(const Code& code)
{
	m_permanent_codes.insert(code);
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withPermanentCodes(const std::set<Code, Comp>& codes)
{
	m_permanent_codes.insert(codes.cbegin(), codes.cend());
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withPermanentRange(const Code& start, const Code& end)
{
	m_permanent_ranges.emplace_back(Range{ start, end });
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withUndefinedCodeClassification(Classification classification) noexcept
{
	m_undefined_code_classification = classification;
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withExceptionClassifier(const std::function<Classification(std::exception_ptr)>& f)
{
	m_exception_classifier = f;
	return *this;
}

template<class T, class Comp>
RetryPP::ClassifierBuilder<T, Comp>& RetryPP::ClassifierBuilder<T, Comp>::withRetryCallback(const std::function<void(const std::variant<Code, std::exception_ptr>&, std::chrono::milliseconds)>& f)
{
	m_retry_callback = f;
	return *this;
}

template<class T, class Comp>
const RetryPP::Classifier<typename RetryPP::ClassifierBuilder<T, Comp>::Code, Comp> RetryPP::ClassifierBuilder<T, Comp>::build() const
{
	if (m_success_codes.empty() && m_success_ranges.empty())
		throw InvalidClassifier("At least one success code must be defined");

	return Classifier<Code, Comp>{ *this };
}
