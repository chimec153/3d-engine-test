#include "Sound.h"
#include "../Core/PathManager.h"

namespace Engine
{
	Sound::Sound(FMOD::System* pSystem, const std::string& strName, const char* pFilePath, const std::string& strPath, float fMin, float fMax, FMOD_MODE b3D, bool bLoop) :
		m_fMin(fMin)
		, m_fMax(fMax)
		, m_b3D(b3D)
		, m_bLoop(bLoop)
		, m_strTag(strName)
		, m_pChannel(nullptr)
		, m_pSystem(pSystem)
	{
		char strFullPath[MAX_PATH] = {};

		CPathManager::GetInst()->ResolveMB(pFilePath, strPath, strFullPath);

		pSystem->createSound(strFullPath, b3D, nullptr, &m_pSound);

		m_pSound->set3DMinMaxDistance(fMin, fMax);

		if (bLoop)
		{
			m_pSound->setMode(FMOD_LOOP_NORMAL);
		}
	}

	Sound::~Sound()
	{
		if(m_pSound)
		{
			m_pSound->release();
			m_pSound = nullptr;
		}
	}

	void Sound::Play()
	{
		m_pSystem->playSound(m_pSound, nullptr, m_bLoop, &m_pChannel);

		if (m_bLoop)
		{
			m_pChannel->setPaused(false);
		}
	}

	void Sound::Stop()
	{
		m_pChannel->setPaused(true);
	}

	void Sound::Resume()
	{
		m_pChannel->setPaused(false);
	}

	void Sound::Toggle()
	{
		bool bPaused = false;

		m_pChannel->getPaused(&bPaused);
		m_pChannel->setPaused(!bPaused);
	}

	void Sound::SetPosition(const Vector3& vPos, const Vector3& vVel)
	{
		FMOD_VECTOR _vPos = {vPos.x,vPos.y, vPos.z };
		FMOD_VECTOR _vVel = { vVel.x, vVel.y, vVel.z};

		m_pChannel->set3DAttributes(&_vPos, &_vVel);
	}

}