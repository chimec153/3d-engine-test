#include "Sphere.h"
#include "Collider.h"
#include "Transform.h"
#include "../Input/Input.h"

namespace Engine
{
	Sphere::Sphere() :
		Component()
		, m_fSpeed(8.f)
		, m_vDir(static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()))
	{
		m_vDir.Normalize();
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Sphere::Sphere(int /*iRings*/, int /*iSector*/) :
		Component()
		, m_fSpeed(8.f)
		, m_vDir(static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()))
	{
		m_vDir.Normalize();

		// Phase E5 — Drawable-era ctor wired VS/PS/IL/Topology + built a
		// per-(rings,sector) sphere Mesh + Material via Drawable's child
		// API. Stripped for the Component shell; reintroduce under
		// MeshRendererComponent on a GameObject.
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Sphere::Sphere(const Sphere& sphere) :
		Component(sphere)
		, m_fSpeed(3.f)
		, m_vDir(static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()))
	{
		m_vDir.Normalize();
	}

	Sphere::~Sphere()
	{
	}

	float Sphere::GetSpeed() const           { return m_fSpeed; }
	const Vector3& Sphere::GetDir() const    { return m_vDir; }
	void Sphere::SetSpeed(float fSpeed)      { m_fSpeed = fSpeed; }
	void Sphere::SetDir(const Vector3& vDir) { m_vDir = vDir; }

	bool Sphere::Init()
	{
		CInput::GetInst()->AddKey(DIK_LEFTARROW);
		CInput::GetInst()->AddKey(DIK_RIGHTARROW);
		CInput::GetInst()->AddKey(DIK_UPARROW);
		CInput::GetInst()->AddKey(DIK_DOWNARROW);

		return __super::Init();
	}

	void Sphere::Input(float fDeltaTime)
	{
		// Phase E5 — input drove the owning Drawable's Transform via the
		// Drawable-side GetTransform(). For the Component shell we read
		// the owner GameObject's Transform if present.
		std::shared_ptr<Transform> pTransform;
		if (auto* pOwner = GetGameObjectOwner())
			pTransform = pOwner->GetComponent<Transform>();

		if (!pTransform) return;

		if (CInput::GetInst()->IsKey(CInput::KEY_STATE::PRESS, DIK_LEFTARROW))
			pTransform->AddX(fDeltaTime * m_fSpeed);

		if (CInput::GetInst()->IsKey(CInput::KEY_STATE::PRESS, DIK_RIGHTARROW))
			pTransform->AddX(-fDeltaTime * m_fSpeed);

		if (CInput::GetInst()->IsKey(CInput::KEY_STATE::PRESS, DIK_UPARROW))
			pTransform->AddZ(fDeltaTime * m_fSpeed);

		if (CInput::GetInst()->IsKey(CInput::KEY_STATE::PRESS, DIK_DOWNARROW))
			pTransform->AddZ(-fDeltaTime * m_fSpeed);
	}

	void Sphere::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	std::shared_ptr<Component> Sphere::Clone()
	{
		return std::make_shared<Sphere>(*this);
	}

	void Sphere::CollisionEnter(Collider* pSrc, Collider* pDest, float /*fDeltaTime*/)
	{
		switch (pDest->GetColliderType())
		{
		case COLLIDER_TYPE::SPHERE:
		{
			// Phase E5 — Sphere is dead Engine primitive. Use the
			// Component's host-agnostic Transform helper.
			std::shared_ptr<Transform> pSrcTr = pSrc->GetHostTransform();
			if (pSrcTr)
			{
				const Vector3& vNormal = (pSrcTr->GetPosition() - pSrc->GetCross()).Normalize();
				m_vDir = m_vDir - m_vDir.Dot(vNormal) * 2.f * vNormal;
			}
		}
		break;
		}
	}

	void Sphere::CreateSphereIndex(int iRings, int iSectors, std::vector<unsigned int>& vecIndex)
	{
		for (int i = 0; i < iSectors; ++i)
		{
			vecIndex.push_back(0);
			vecIndex.push_back((i + 1) % iSectors + 1);
			vecIndex.push_back(i + 1);
		}

		for (int j = 0; j < iRings - 1; ++j)
		{
			for (int i = 0; i < iSectors; ++i)
			{
				vecIndex.push_back(j * iSectors + i + 1);
				vecIndex.push_back((j + 1) * iSectors + (i + 1) % iSectors + 1);
				vecIndex.push_back((j + 1) * iSectors + i + 1);

				vecIndex.push_back(j * iSectors + i + 1);
				vecIndex.push_back(j * iSectors + (i + 1) % iSectors + 1);
				vecIndex.push_back((j + 1) * iSectors + (i + 1) % iSectors + 1);
			}
		}

		for (int i = 0; i < iSectors; ++i)
		{
			vecIndex.push_back((iRings - 1) * iSectors + i + 1);
			vecIndex.push_back((iRings - 1) * iSectors + (i + 1) % iSectors + 1);
			vecIndex.push_back(iRings * iSectors + 1);
		}
	}
}
