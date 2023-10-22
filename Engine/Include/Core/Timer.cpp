#include "Timer.h"

namespace Engine
{
	Timer::Timer() :
		m_tSecond()
		, m_tFreq()
		, m_fElapsedTime(0.f)
		, m_fFPS(0.f)
		, fDeltaTime(0.f)
		, iFrame(0)
		, fFrameTime(0.f)
		, m_fScale(1.f)
	{
	}

	Timer::~Timer()
	{
	}

	float Timer::GetElapsedTime() const noexcept
	{
		return m_fElapsedTime;
	}

	const float Timer::GetDeltTime() const noexcept
	{
		return fDeltaTime * m_fScale;
	}

	float Timer::GetFPS() const
	{
		return m_fFPS;
	}

	void Timer::SetScale(float fScale) noexcept
	{
		m_fScale = fScale;
	}

	const float Timer::GetScale() const noexcept
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

		fDeltaTime = (tTime.QuadPart - m_tSecond.QuadPart) / static_cast<float>(m_tFreq.QuadPart);

		m_tSecond = tTime;

		m_fElapsedTime += fDeltaTime * m_fScale;

		fFrameTime += fDeltaTime;

		++iFrame;

		if (iFrame >= 60)
		{
			m_fFPS = iFrame / fFrameTime;

			fFrameTime = 0.f;
			iFrame = 0;
		}
	}
}