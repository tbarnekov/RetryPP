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
#include <format>
#include <stdexcept>

namespace RetryPP
{

	class InvalidPolicy : public std::invalid_argument
	{
	public:
		inline InvalidPolicy();
		inline InvalidPolicy(const char* message);
	};


	class InvalidClassifier : public std::invalid_argument
	{
	public:
		inline InvalidClassifier();
		inline InvalidClassifier(const char* message);
	};


	class OutOfRange : public std::out_of_range
	{
	public:
		using std::out_of_range::out_of_range;
	};

} // namespace RetryPP


//////////////////////////////////////////////////////////////////////////
// InvalidPolicy implementation

RetryPP::InvalidPolicy::InvalidPolicy()
	: std::invalid_argument{ "Invalid policy" }
{
}

RetryPP::InvalidPolicy::InvalidPolicy(const char* message)
	: std::invalid_argument{ std::format("Invalid policy: {}", message).c_str() }
{
}


//////////////////////////////////////////////////////////////////////////
// InvalidClassifier implementation

RetryPP::InvalidClassifier::InvalidClassifier()
	: std::invalid_argument{ "Invalid classifier" }
{
}

RetryPP::InvalidClassifier::InvalidClassifier(const char* message)
	: std::invalid_argument{ std::format("Invalid classifier: {}", message).c_str() }
{
}
