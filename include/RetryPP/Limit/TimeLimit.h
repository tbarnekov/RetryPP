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
#include "Limit.h"
#include <algorithm>
#include <chrono>


namespace RetryPP
{

	class TimeLimit final : public Limit
	{
	public:
		TimeLimit() = delete;
		explicit TimeLimit(std::chrono::milliseconds timeout);

		std::chrono::milliseconds timeout() const noexcept;

		bool exhausted() noexcept override;
		std::chrono::milliseconds time_remaining() const noexcept override;

	private:
		const std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
		const std::chrono::milliseconds m_timeout;
	};
	
} // namespace RetryPP


//////////////////////////////////////////////////////////////////////////
// TimeLimit implementation

RetryPP::TimeLimit::TimeLimit(std::chrono::milliseconds timeout)
	: m_timeout{ timeout }
{
	if (m_timeout < std::chrono::milliseconds{ 0 })
		throw OutOfRange("TimeLimit must be positive");
}

std::chrono::milliseconds RetryPP::TimeLimit::timeout() const noexcept
{
	return m_timeout;
}

bool RetryPP::TimeLimit::exhausted() noexcept
{
	return std::chrono::steady_clock::now() > m_start + m_timeout;
}

std::chrono::milliseconds RetryPP::TimeLimit::time_remaining() const noexcept
{
	return std::chrono::milliseconds{ std::max(static_cast<std::chrono::steady_clock::rep>(0), ((m_start + m_timeout) - std::chrono::steady_clock::now()).count()) };
}
