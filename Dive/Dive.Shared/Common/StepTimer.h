#pragma once

#include <wrl.h>

namespace DX
{
	class StepTimer
	{
	public:
		StepTimer() :
			m_elapsedTicks(0),
			m_totalTicks(0),
			m_leftOverTicks(0),
			m_frameCount(0),
			m_framesPerSecond(0),
			m_framesThisSecond(0),
			m_qpcSecondCounter(0),
			m_isFixedTimeStep(false),
			m_targetElapsedTicks(TicksPerSecond / 60)
		{
			if (!QueryPerformanceFrequency(&m_qpcFrenquency))
				throw ref new Platform::FailureException();

			if (!QueryPerformanceCounter(&m_qpcLastTime))
				throw ref new Platform::FailureException();

			m_qpcMaxDelta = m_qpcFrenquency.QuadPart / 10;
		}

		uint64	GetElapsedTicks() const							{ return m_elapsedTicks; }
		double	GetElapsedSeconds() const						{ return TicksToSeconds(m_elapsedTicks); }

		uint64	GetTotalTicks() const							{ return m_totalTicks; }
		double	GetTotalSeconds() const							{ return TicksToSeconds(m_totalTicks); }

		uint32	GetFrameCount() const							{ return m_frameCount; }

		uint32	GetFramePerSecond() const						{ return m_framesPerSecond; }

		void	SetFixedTimeStep(bool isFixedTimestep)			{ m_isFixedTimeStep = isFixedTimestep; }

		void	SetTargetElapsedTicks(uint64 targetElapsed)		{ m_targetElapsedTicks = targetElapsed; }
		void	SetTargetElapsedSeconds(double targetElapsed)	{ m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

		static const uint64	TicksPerSecond = 10000000;

		static double	TicksToSeconds(uint64 ticks)			{ return static_cast<double>(ticks) / TicksPerSecond; }
		static uint64	SecondsToTicks(double seconds)			{ return static_cast<uint64>(seconds * TicksPerSecond); }

		void	ResetElapsedTime()
		{
			if (!QueryPerformanceCounter(&m_qpcLastTime))
				throw ref new Platform::FailureException();

			m_leftOverTicks = 0;
			m_framesPerSecond = 0;
			m_framesThisSecond = 0;
			m_qpcSecondCounter = 0;
		}

		template<typename TUpdate>
		void	Tick(TUpdate const& update)
		{
			LARGE_INTEGER	currentTime;

			if (!QueryPerformanceCounter(&currentTime))
				throw ref new Platform::FailureException();

			uint64	timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

			m_qpcLastTime = currentTime;
			m_qpcSecondCounter += timeDelta;

			if (timeDelta > m_qpcMaxDelta)
				timeDelta = m_qpcMaxDelta;

			timeDelta *= TicksPerSecond;
			timeDelta /= m_qpcFrenquency.QuadPart;

			uint32	lastFrameCount = m_frameCount;

			if (m_isFixedTimeStep)
			{
				if (abs(static_cast<int64>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
					timeDelta = m_targetElapsedTicks;

				m_leftOverTicks += timeDelta;

				while (m_leftOverTicks >= m_targetElapsedTicks)
				{
					m_elapsedTicks = m_targetElapsedTicks;
					m_totalTicks += m_targetElapsedTicks;
					m_leftOverTicks -= m_targetElapsedTicks;
					m_frameCount++;

					update();
				}
			}
			else
			{
				m_elapsedTicks = timeDelta;
				m_totalTicks += timeDelta;
				m_leftOverTicks = 0;
				m_frameCount++;

				update();
			}

			if (m_frameCount != lastFrameCount)
				m_framesThisSecond++;

			if (m_qpcSecondCounter >= static_cast<uint64>(m_qpcFrenquency.QuadPart))
			{
				m_framesPerSecond = m_framesThisSecond;
				m_framesThisSecond = 0;
				m_qpcSecondCounter %= m_qpcFrenquency.QuadPart;
			}
		}

	private:
		LARGE_INTEGER	m_qpcFrenquency;
		LARGE_INTEGER	m_qpcLastTime;
		uint64			m_qpcMaxDelta;

		uint64	m_elapsedTicks;
		uint64	m_totalTicks;
		uint64	m_leftOverTicks;

		uint32	m_frameCount;
		uint32	m_framesPerSecond;
		uint32	m_framesThisSecond;
		uint64	m_qpcSecondCounter;

		bool	m_isFixedTimeStep;
		uint64	m_targetElapsedTicks;
	};
}