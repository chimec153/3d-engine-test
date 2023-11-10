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

	public:
		constexpr float GetElapsedTime() const noexcept;
		constexpr  float GetDeltTime() const noexcept;
		constexpr float GetFPS()	const noexcept;
		void SetScale(float fScale) noexcept;
		constexpr float GetScale()	const noexcept;

	public:
		bool Init();
		void Update();

	};

}