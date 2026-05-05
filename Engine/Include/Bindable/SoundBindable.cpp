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
			// Phase B.4 — Component's owner is the attached Drawable.
			// Set by Drawable::AddChild(shared_ptr<Component>) when this
			// sound was added to the entity.
			if (Drawable* pOwner = GetOwner())
			{
				std::shared_ptr<Transform> pTransform = pOwner->GetTransform();
				if (pTransform)
				{
					m_pSound->SetPosition(pTransform->GetPosition(), pTransform->GetVelocity());
				}
			}
		}
	}
	std::shared_ptr<Component> SoundBindable::Clone()
	{
		return std::make_shared<SoundBindable>(*this);
	}
}