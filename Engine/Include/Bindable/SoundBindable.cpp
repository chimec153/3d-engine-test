#include "SoundBindable.h"
#include "Drawable.h"
#include "Transform.h"
#include "../Sound/Sound.h"
#include "../Resource/ResourceManager.h"

namespace Engine
{
	SoundBindable::SoundBindable(const std::string& strSound) :
		Bindable()
		, m_pSound(ResourceManager::GetInst()->FindSound(strSound))
	{
		SetBindableType(BINDABLE_TYPE::SOUND);
	}

	SoundBindable::SoundBindable(const SoundBindable& tBindable)	:
		Bindable(tBindable)
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
			Bindable* pParent = GetParent();

			if (pParent)
			{
				if (OBJECT_TYPE::DRAW == pParent->GetObjectType())
				{
					std::shared_ptr<Transform> pTransform = static_cast<Drawable*>(pParent)->GetTransform();

					if (pTransform)
					{
						m_pSound->SetPosition(pTransform->GetPosition(), pTransform->GetVelocity());
					}
				}
			}
		}
	}
	std::shared_ptr<Bindable> SoundBindable::Clone()
	{
		return std::make_shared<SoundBindable>(*this);
	}
}