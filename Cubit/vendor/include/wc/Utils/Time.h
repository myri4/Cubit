#pragma once

#include <chrono>
#include "Log.h"

namespace wc 
{
	class Clock 
	{
		std::chrono::time_point<std::chrono::steady_clock> m_Start, m_End;
	public:

		Clock() { restart(); }

		void start() { m_Start = std::chrono::high_resolution_clock::now(); }

		float restart()
		{
			m_End = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> dur = m_End - m_Start;

			start();
			return dur.count();
		}
	};

	class ScopeTimer 
	{
		std::chrono::time_point<std::chrono::steady_clock> m_Start;
		const char* op;
	public:

		ScopeTimer(const char* opn) 
		{
			op = opn;
			m_Start = std::chrono::high_resolution_clock::now();
		}

		~ScopeTimer() 
		{
			std::chrono::time_point<std::chrono::steady_clock> End = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> dur = End - m_Start;
			float duration = dur.count() * 1000.0f;

			WC_CORE_INFO("{0} took {1}ms!", op, duration);
		}
	};

	class Timer 
	{
		std::chrono::time_point<std::chrono::steady_clock> m_Start;
	public:

		void Start() { m_Start = std::chrono::high_resolution_clock::now();	}

		float GetElapsedTime() 
		{
			std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> dur = now - m_Start;
			return dur.count();
		}
	};
}