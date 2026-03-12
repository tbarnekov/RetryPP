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
#include "Strategy.h"
#include "../Exceptions.h"
#include <chrono>
#include <cmath>


namespace RetryPP
{

	class Exponential : public Strategy
	{
	public:
		using Strategy::Strategy;

		inline explicit Exponential(std::chrono::milliseconds initial_delay, float multiplier);

		inline float multiplier() const noexcept;

		inline std::chrono::milliseconds next() noexcept override;

	private:
		const float m_multiplier = 2.0f;
		size_t m_attempt = 0;
	};

} // namespace RetryPP


//////////////////////////////////////////////////////////////////////////
// Exponential implementation

RetryPP::Exponential::Exponential(std::chrono::milliseconds initial_delay, float multiplier)
	: Strategy{ initial_delay }, m_multiplier{ multiplier }
{
	if (m_multiplier < 1.0f)
		throw OutOfRange("Exponential multiplier value must be >= 1");
}

float RetryPP::Exponential::multiplier() const noexcept
{
	return m_multiplier;
}

std::chrono::milliseconds RetryPP::Exponential::next() noexcept
{
	auto exponent = std::pow(m_multiplier, static_cast<float>(m_attempt++));
	return std::chrono::milliseconds{ static_cast<count_t>(static_cast<float>(initial_delay().count()) * exponent) };
}
