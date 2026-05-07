#include "SoundBindable.h"
#include "Drawable.h"
#include "Transform.h"
#include "../Sound/Sound.h"
#include "../Resource/ResourceManager.h"

namespace Engine
{
	SoundBindable::SoundBindable(const std::string& strSound) :
		Component()
		, m_pSound(ResourceManager::GetInst()->FindSound(strSound))
	{
		SetComponentType(COMPONENT_TYPE::SOUND);
	}

	SoundBindable::SoundBindable(const SoundBindable& tBindable)	:
		Component(tBindable)
		, m_pSound(tBindable.m_pSound)
	{
	}

	void SoundBindable::Play()
	{
		if (m_pSound)
		{
			m_pSound->Play();
		}
	}

	void SoundBindable::Stop()
	{
		if (m_pSound)
		{
			m_pSound->Stop();
		}
	}

	void SoundBindable::Resume()
	{
		if (m_pSound)
		{
			m_pSound->Resume();
		}
	}

	void SoundBindable::Toggle()
	{
		if (m_pSound)
		{
			m_pSound->Toggle();
		}
	}

	void SoundBindable::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (m_pSound)
		{
			// Phase E5 — host transform via the host-agnostic helper.
			std::shared_ptr<Transform> pTransform = GetHostTransform();
			if (pTransform)
			{
				m_pSound->SetPosition(pTransform->GetPosition(), pTransform->GetVelocity());
			}
		}
	}
	std::shared_ptr<Component> SoundBindable::Clone()
	{
		return std::make_shared<SoundBindable>(*this);
	}
}