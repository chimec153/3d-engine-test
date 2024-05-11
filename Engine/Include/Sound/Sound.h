#pragma once

#include "../Types.h"

namespace Engine
{
	class ENGINE_DLL Sound
	{
	public:
		Sound(FMOD::System* pSystem, const std::string& strName, const char* pFilePath, const std::string& strPath = SOUND_PATH, float fMin = 0.5f, float fMax = 5000.f, FMOD_MODE b3D = FMOD_3D, bool bLoop = false);
		~Sound();

	private:
		float m_fMin;
		float m_fMax;
		FMOD_MODE m_b3D;
		bool m_bLoop;
		std::string m_strTag;
		FMOD::Sound* m_pSound;
		FMOD::Channel* m_pChannel;
		FMOD::System* m_pSystem;

	public:
		void Play();
		void Stop();
		void Resume();
		void Toggle();
		void SetPosition(const Vector3& vPos, const Vector3& vVel);
	};

}