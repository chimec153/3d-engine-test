#pragma once
#include <Windows.h>
#include "Ref.h"

namespace Engine
{
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
		float fDeltaTime;
		int iFrame;
		float fFrameTime;
		float m_fScale;

	public:
		float GetElapsedTime() const noexcept;
		const float GetDeltTime() const noexcept;
		float GetFPS()	const;
		void SetScale(float fScale) noexcept;
		const float GetScale()	const noexcept;

	public:
		bool Init();
		void Update();

	};

}