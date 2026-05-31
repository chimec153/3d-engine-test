#include "Timer.h"
#include "Graphics.h"

namespace Engine
{
	Timer::Timer() :
		m_tSecond()
		, m_tFreq()
		, m_fElapsedTime(0.f)
		, m_fFPS(0.f)
		, iFrame(0)
		, fFrameTime(0.f)
		, m_fScale(1.f)
		, m_fDeltaTime(0.f)
		, m_fHitStopScale(1.f)
		, m_fHitStopRemain(0.f)
		, m_bStop(false)
	{
	}

	Timer::~Timer()
	{
	}

	constexpr float Timer::GetElapsedTime() const noexcept
	{
		return m_fElapsedTime;
	}

	constexpr float Timer::GetDeltTime() const noexcept
	{
		// Single source of truth for the per-frame game delta. When the
		// game is paused, return zero so every consumer (Input, Scene,
		// per-object Update) naturally no-ops without each having to
		// branch on a "paused" flag.
		return m_bStop ? 0.f : (m_fDeltaTime * m_fScale * m_fHitStopScale);
	}

	constexpr float Timer::GetFPS() const noexcept
	{
		return m_fFPS;
	}

	void Timer::SetScale(float fScale) noexcept
	{
		m_fScale = fScale;
	}

	void Timer::RequestHitStop(float fDuration, float fScale) noexcept
	{
		// Max-merge so a weaker request can't shorten an active stronger one.
		if (fDuration > m_fHitStopRemain) m_fHitStopRemain = fDuration;
		if (fScale    < m_fHitStopScale)  m_fHitStopScale  = fScale;
	}

	constexpr float Timer::GetScale() const noexcept
	{
		return m_fScale;
	}

	bool Timer::Init()
	{
		QueryPerformanceFrequency(&m_tFreq);
		QueryPerformanceCounter(&m_tSecond);

		return true;
	}

	void Timer::Update()
	{
		LARGE_INTEGER tTime;

		QueryPerformanceCounter(&tTime);

		m_fDeltaTime = (tTime.QuadPart - m_tSecond.QuadPart) / static_cast<float>(m_tFreq.QuadPart);

		m_tSecond = tTime;

		// Count the hit-stop down on RAW delta (unaffected by m_fScale /
		// m_fHitStopScale) so a freeze to scale 0 always recovers.
		if (m_fHitStopRemain > 0.f)
		{
			m_fHitStopRemain -= m_fDeltaTime;
			if (m_fHitStopRemain <= 0.f)
			{
				m_fHitStopRemain = 0.f;
				m_fHitStopScale  = 1.f;
			}
		}

		m_fElapsedTime += m_fDeltaTime * m_fScale;

		fFrameTime += m_fDeltaTime;

		++iFrame;

		if (iFrame >= 60)
		{
			m_fFPS = iFrame / fFrameTime;

			fFrameTime = 0.f;
			iFrame = 0;
		}
	}
}