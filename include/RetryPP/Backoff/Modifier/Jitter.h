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
#include "Modifier.h"
#include <optional>
#include <random>


namespace RetryPP
{

	namespace Algorithm
	{

		class JitterAlgorithm
		{
		public:
			JitterAlgorithm() noexcept = default;
			JitterAlgorithm(const JitterAlgorithm&) = delete;
			JitterAlgorithm(JitterAlgorithm&&) = delete;
			JitterAlgorithm& operator=(const JitterAlgorithm&) = delete;
			JitterAlgorithm& operator=(JitterAlgorithm&&) = delete;
			virtual ~JitterAlgorithm() = default;

			virtual void apply(std::chrono::milliseconds& delay) noexcept = 0;
		};


		class Full final : public JitterAlgorithm
		{
		public:
			void apply(std::chrono::milliseconds& delay) noexcept override
			{
				std::uniform_int_distribution<long long> distribution{ 0, delay.count() };
				delay = std::chrono::milliseconds{ distribution(m_random_engine) };
			}

		private:
			mutable std::mt19937 m_random_engine;
		};


		class Equal final : public JitterAlgorithm
		{
		public:
			void apply(std::chrono::milliseconds& delay) noexcept override
			{
				std::uniform_int_distribution<long long> distribution{ 0, delay.count() / 2 };
				delay = std::chrono::milliseconds{ (delay.count() / 2) + distribution(m_random_engine) };
			}

		private:
			mutable std::mt19937 m_random_engine;
		};


		class Decorrelated final : public JitterAlgorithm
		{
		public:
			void apply(std::chrono::milliseconds& delay) noexcept override
			{
				if (!m_initial_delay.has_value())
					m_initial_delay = delay.count();

				if (!m_last_delay.has_value())
					m_last_delay = std::uniform_int_distribution<long long>{ 0, delay.count() }(m_random_engine) * 3;

				std::uniform_int_distribution<long long> distribution{ std::min(m_initial_delay.value(), m_last_delay.value()), std::max(m_initial_delay.value(), m_last_delay.value()) };
				delay = std::chrono::milliseconds{ distribution(m_random_engine) };
				m_last_delay = delay.count() * 3;
			}

		private:
			std::optional<long long> m_initial_delay;
			std::optional<long long> m_last_delay;
			mutable std::mt19937 m_random_engine;
		};

	} // namespace Algorithm


	template<class T>
	concept RetryJitterAlgorithm = std::derived_from<T, Algorithm::JitterAlgorithm>;


	template<RetryJitterAlgorithm S>
	class Jitter : public Modifier
	{
	public:
		void apply(std::chrono::milliseconds& delay) noexcept override
		{
			m_algorithm.apply(delay);
		}

	private:
		S m_algorithm;
	};

} // namespace RetryPP
