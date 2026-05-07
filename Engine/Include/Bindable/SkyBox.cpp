#include "SkyBox.h"
#include "BindableManager.h"
#include "Mesh.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Topology.h"
#include "InputLayout.h"
#include "Texture.h"
#include "Transform.h"
#include "Camera.h"
#include "../Core/Graphics.h"
#include "../GameObject/GameObject.h"
#include "../Render/RenderManager.h"

namespace Engine
{
	SkyBox::SkyBox()
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	SkyBox::SkyBox(const TCHAR* pTexturePath, const std::string& strKey)
	{
		SetComponentType(COMPONENT_TYPE::NONE);

		m_pMesh        = StaticFindBindable<Mesh>("Box");
		m_pVS          = StaticFindBindable<VertexShader>("EnvironmentVS");
		m_pPS          = StaticFindBindable<PixelShader>("EnvironmentPS");
		m_pTopology    = StaticFindBindable<Topology>(STANDARD_TOPOLOGY);
		m_pInputLayout = StaticFindBindable<InputLayout>(STANDARD_INPUT_LAYOUT);

		// Texture is created here (not just looked up) because each SkyBox
		// instance may have a different cubemap. Mirrors the previous
		// CreateBindable<Texture> call in the Drawable-era ctor.
		m_pTexture = StaticCreateBindable<Texture>("SkyBoxTexture", pTexturePath, strKey, 5);
	}

	SkyBox::SkyBox(const SkyBox& other) :
		Component(other)
		, m_pMesh(other.m_pMesh)
		, m_pVS(other.m_pVS)
		, m_pPS(other.m_pPS)
		, m_pTopology(other.m_pTopology)
		, m_pInputLayout(other.m_pInputLayout)
		, m_pTexture(other.m_pTexture)
	{
	}

	void SkyBox::SetMesh(const std::shared_ptr<Mesh>& p) { m_pMesh = p; }
	void SkyBox::SetVertexShader(const std::shared_ptr<VertexShader>& p) { m_pVS = p; }
	void SkyBox::SetPixelShader(const std::shared_ptr<PixelShader>& p) { m_pPS = p; }
	void SkyBox::SetTopology(const std::shared_ptr<Topology>& p) { m_pTopology = p; }
	void SkyBox::SetInputLayout(const std::shared_ptr<InputLayout>& p) { m_pInputLayout = p; }
	void SkyBox::SetTexture(const std::shared_ptr<Texture>& p) { m_pTexture = p; }

	bool SkyBox::Init()
	{
		if (!__super::Init())
			return false;

		// Owner GameObject is expected to provide a Transform sibling.
		// Set initial scale to a large box so the skybox encloses the scene.
		GameObject* pOwner = GetGameObjectOwner();
		if (pOwner)
		{
			std::shared_ptr<Transform> pTr = pOwner->GetComponent<Transform>();
			if (pTr) pTr->SetScale(5000.f, 5000.f, 5000.f);
		}

		RenderManager::GetInst()->SetSkyBox(std::static_pointer_cast<SkyBox>(shared_from_this()));

		return true;
	}

	void SkyBox::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();
		if (!pCamera) return;

		GameObject* pOwner = GetGameObjectOwner();
		if (!pOwner) return;

		std::shared_ptr<Transform> pTr = pOwner->GetComponent<Transform>();
		if (pTr) pTr->SetPosition(pCamera->GetTransform()->GetPosition());
	}

	void SkyBox::Bind()
	{
		if (m_pVS)          m_pVS->Bind();
		if (m_pPS)          m_pPS->Bind();
		if (m_pTopology)    m_pTopology->Bind();
		if (m_pInputLayout) m_pInputLayout->Bind();
		if (m_pTexture)     m_pTexture->Bind();

		// Transform constant-buffer binding is RenderManager's responsibility
		// when iterating the skybox pass (mirrors the new MeshRenderer path).

		if (m_pMesh) m_pMesh->Draw();
	}

	std::shared_ptr<Component> SkyBox::Clone()
	{
		return std::make_shared<SkyBox>(*this);
	}

	void SkyBox::Save(FILE* pFile)
	{
		__super::Save(pFile);
	}

	void SkyBox::Load(FILE* pFile)
	{
		__super::Load(pFile);

		RenderManager::GetInst()->SetSkyBox(std::static_pointer_cast<SkyBox>(shared_from_this()));
	}
}
