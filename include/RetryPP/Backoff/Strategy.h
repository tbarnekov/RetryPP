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
#include <chrono>
#include <concepts>

namespace RetryPP
{

	class Strategy
	{
	public:
		explicit Strategy(std::chrono::milliseconds initial_delay)
			: m_initial_delay{ initial_delay }
		{
			if (m_initial_delay.count() <= 0)
				throw InvalidParameter("Initial delay must be > 0");
		}

		Strategy(const Strategy&) = delete;
		Strategy(Strategy&&) = delete;
		Strategy& operator=(const Strategy&) = delete;
		Strategy& operator=(Strategy&&) = delete;
		virtual ~Strategy() noexcept = default;

		std::chrono::milliseconds initial_delay() const noexcept
		{
			return m_initial_delay;
		}

		virtual std::chrono::milliseconds next() noexcept = 0;

	private:
		const std::chrono::milliseconds m_initial_delay;
	};


	template<class T>
	concept RetryStrategy = std::derived_from<T, RetryPP::Strategy>;

} // namespace RetryPP
