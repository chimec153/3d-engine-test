#pragma once
#include <Windows.h>
#include "Ref.h"

namespace Engine
{
	template <typename T>
	class ConstantBuffer;

	class ENGINE_DLL Timer :
		public CRef
	{
	public:
		Timer();
		~Timer();

	private:
		LARGE_INTEGER m_tSecond;
		LARGE_INTEGER m_tFreq;
		float m_fElapsedTime;
		float m_fFPS;
		int iFrame;
		float fFrameTime;
		float m_fScale;
		float m_fDeltaTime;
		// Hit-stop overlay — a short gameplay time-freeze that multiplies on
		// top of m_fScale (so it composes with the editor's time scrub).
		// m_fHitStopRemain counts down on RAW (unscaled) delta in Update() so
		// a freeze to scale 0 still recovers.
		float m_fHitStopScale;
		float m_fHitStopRemain;
		// Frame-time gate for game-time pause. Set via Stop()/Resume();
		// while set, GetDeltTime() returns 0 so all callers (Input,
		// Scene, etc.) see "no time passed" without each having to know
		// the gate. Update() still runs every frame so the QPC delta
		// keeps tracking real wall time — only the *exposed* delta is
		// gated.
		bool m_bStop;

	public:
		constexpr float GetElapsedTime() const noexcept;
		constexpr  float GetDeltTime() const noexcept;
		constexpr float GetFPS()	const noexcept;
		void SetScale(float fScale) noexcept;
		constexpr float GetScale()	const noexcept;
		// Request a hit-stop: freeze game time to fScale for fDuration seconds.
		// Stronger/longer requests win (max-merge) so chained hits never cut a
		// freeze short. Counted down on raw delta in Update().
		void RequestHitStop(float fDuration, float fScale = 0.f) noexcept;
		// Pause / resume game time. Previously lived on Window (bStop)
		// where every Update path had to multiply by !bStop manually;
		// folding the gate into Timer means GetDeltTime is the single
		// source of truth — no caller-side branching needed.
		void Stop() noexcept   { m_bStop = true;  }
		void Resume() noexcept { m_bStop = false; }
		bool IsStop() const noexcept { return m_bStop; }

	public:
		bool Init();
		void Update();

	};

}