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
#include "../Exceptions.h"

namespace RetryPP
{

	class RetryLimit final : public Limit
	{
	public:
		RetryLimit() = delete;
		inline explicit RetryLimit(size_t maximum_retries);

		inline size_t maximum_retries() const noexcept;

		inline bool exhausted() noexcept override;

	private:
		const size_t m_maximum_retries;
		size_t m_attempt = 0;
	};

} // namespace RetryPP


//////////////////////////////////////////////////////////////////////////
// RetryLimit implementation

RetryPP::RetryLimit::RetryLimit(size_t maximum_retries)
	: m_maximum_retries{ maximum_retries }
{
	if (m_maximum_retries == 0)
		throw OutOfRange("RetryLimit must be 1 or higher");
}

size_t RetryPP::RetryLimit::maximum_retries() const noexcept
{
	return m_maximum_retries;
}

bool RetryPP::RetryLimit::exhausted() noexcept
{
	return ++m_attempt >= m_maximum_retries;
}
