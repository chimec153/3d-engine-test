#include "Decal.h"
#include "InputLayout.h"
#include "Material.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "../Core/Graphics.h"
#include "Camera.h"
#include "Transform.h"
#include "Mesh.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Topology.h"
#include "Texture.h"
#include "../Render/RenderManager.h"

namespace Engine
{
	Decal::Decal() :
		m_tCBuffer{}
		, m_pCBuffer(StaticFindBindable<ConstantBuffer<DECALCBUFFER>>("Decal"))
		, m_bFadeStart(false)
		, m_eRenderLayer(RENDER_LAYER::DECAL)
	{
		m_tCBuffer.fMaxFadeTime = 1.f;

		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Decal::Decal(const Decal& decal) :
		Component(decal)
		, m_tCBuffer(decal.m_tCBuffer)
		, m_pCBuffer(decal.m_pCBuffer)
		, m_bFadeStart(decal.m_bFadeStart)
		, m_pTransform(decal.m_pTransform)
		, m_pMesh(decal.m_pMesh)
		, m_pVS(decal.m_pVS)
		, m_pPS(decal.m_pPS)
		, m_pTopology(decal.m_pTopology)
		, m_pInputLayout(decal.m_pInputLayout)
		, m_pTexture(decal.m_pTexture)
		, m_pMaterial(decal.m_pMaterial)
		, m_eRenderLayer(decal.m_eRenderLayer)
	{
	}

	void Decal::SetMaxFadeTime(float fMax) { m_tCBuffer.fMaxFadeTime = fMax; }
	void Decal::SetFadeStartTime(float fStart) { m_tCBuffer.fFadeStartTime = fStart; }
	void Decal::StartFade() { m_bFadeStart = true; }

	void Decal::SetFadeTime(float fTime)
	{
		m_tCBuffer.fFadeTime = fTime;
	}

	void Decal::SetMesh(const std::shared_ptr<Mesh>& p) { m_pMesh = p; }
	void Decal::SetVertexShader(const std::shared_ptr<VertexShader>& p) { m_pVS = p; }
	void Decal::SetPixelShader(const std::shared_ptr<PixelShader>& p) { m_pPS = p; }
	void Decal::SetTopology(const std::shared_ptr<Topology>& p) { m_pTopology = p; }
	void Decal::SetInputLayout(const std::shared_ptr<InputLayout>& p) { m_pInputLayout = p; }
	void Decal::SetTexture(const std::shared_ptr<Texture>& p) { m_pTexture = p; }
	void Decal::SetMaterial(const std::shared_ptr<Material>& p) { m_pMaterial = p; }

	bool Decal::Init()
	{
		if (!__super::Init()) return false;

		// Resolve well-known shared resources via BindableManager.
		// Pixel shader is intentionally NOT defaulted — different decal
		// instances (terrain brush vs. blood decal) use different PSs.
		if (!m_pVS)          m_pVS          = StaticFindBindable<VertexShader>(DECAL_VS);
		if (!m_pInputLayout) m_pInputLayout = StaticFindBindable<InputLayout>(STANDARD_INPUT_LAYOUT);

		// Per-instance Material clone (matches the old Drawable-era ctor).
		if (!m_pMaterial)
		{
			std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");
			if (pMaterial) m_pMaterial = std::static_pointer_cast<Material>(pMaterial->Clone());
		}

		// Per-instance Transform owned by this decal Component.
		if (!m_pTransform) m_pTransform = std::make_shared<Transform>();

		return true;
	}

	void Decal::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (m_pTransform) m_pTransform->Update(fDeltaTime);
	}

	void Decal::PostUpdate(float fDeltaTime)
	{
		__super::PostUpdate(fDeltaTime);

		if (m_bFadeStart)
		{
			m_tCBuffer.fFadeTime += fDeltaTime;

			if (m_tCBuffer.fMaxFadeTime < m_tCBuffer.fFadeTime)
			{
				m_tCBuffer.fFadeTime = m_tCBuffer.fMaxFadeTime;
			}
		}

		if (!m_pTransform) return;

		m_pTransform->PostUpdate(fDeltaTime);

		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();
		if (!pCamera) return;

		m_tCBuffer.matInvWorldView =
			pCamera->GetInvView()
			* Matrix::TranslateFromVector(-m_pTransform->GetPosition())
			* m_pTransform->GetRotationMatrix().Transpose()
			* Matrix::Scaling(1.f / m_pTransform->GetScale());

		m_tCBuffer.matInvWorldView.Transpose();
	}

	void Decal::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		// Phase E5 — self-register with RenderManager so the decal pass
		// renders this instance. Mirrors MeshRendererComponent's pattern.
		auto pSelf = std::dynamic_pointer_cast<Decal>(shared_from_this());
		if (pSelf) RenderManager::GetInst()->AddDecalComponent(pSelf);
	}

	void Decal::Bind()
	{
		if (!m_pCBuffer) return;

		m_pCBuffer->UpdateBuffer(m_tCBuffer);
		m_pCBuffer->Bind();

		if (m_pTransform)    m_pTransform->Bind();
		if (m_pVS)           m_pVS->Bind();
		if (m_pPS)           m_pPS->Bind();
		if (m_pTopology)     m_pTopology->Bind();
		if (m_pInputLayout)  m_pInputLayout->Bind();
		if (m_pMaterial)     m_pMaterial->Bind();
		if (m_pTexture)      m_pTexture->Bind();

		if (m_pMesh) m_pMesh->Draw();

		if (m_pVS)        m_pVS->PostBind();
		if (m_pPS)        m_pPS->PostBind();
		if (m_pMaterial)  m_pMaterial->PostBind();
		if (m_pTransform) m_pTransform->PostBind();
	}

	std::shared_ptr<Component> Decal::Clone()
	{
		return std::make_shared<Decal>(*this);
	}

	void Decal::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_tCBuffer, sizeof(DECALCBUFFER), 1, pFile);
		fwrite(&m_bFadeStart, 1, 1, pFile);
	}

	void Decal::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_tCBuffer, sizeof(DECALCBUFFER), 1, pFile);
		fread(&m_bFadeStart, 1, 1, pFile);

		m_pCBuffer = StaticFindBindable<ConstantBuffer<DECALCBUFFER>>("Decal");
	}
}
