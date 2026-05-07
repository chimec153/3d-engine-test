#include "Trail.h"
#include "Bindable/Topology.h"
#include "Bindable/RasterizerState.h"
#include "Bindable/InputLayout.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/Transform.h"
#include "Bindable/Drawable.h"  // Phase E5 — for Drawable::SetNormals/SetTangent
                                 // static template utilities only.
#include "Bindable/BindableManager.h"
#include "Render/RenderManager.h"

namespace Client
{
	Trail::Trail() :
		m_eRenderLayer(Engine::RENDER_LAYER::BLUR)
	{
		SetComponentType(Engine::COMPONENT_TYPE::NONE);
	}

	Trail::Trail(int iCount) :
		m_eRenderLayer(Engine::RENDER_LAYER::BLUR)
	{
		SetComponentType(Engine::COMPONENT_TYPE::NONE);

		assert(iCount % 2 == 0);

		m_vecVertex.resize(iCount);

		for (int i = 0; i < m_vecVertex.size() / 2; ++i)
		{
			m_vecVertex[i * 2].uv.x = (i * 2) / static_cast<float>((m_vecVertex.size() / 2) - 1.f);
			m_vecVertex[i * 2].uv.y = 0;

			m_vecVertex[i * 2 + 1].uv.x = (i * 2) / static_cast<float>((m_vecVertex.size() / 2) - 1.f);
			m_vecVertex[i * 2 + 1].uv.y = 1;
		}

		for (int i = 0; i < (iCount - 2) / 2; ++i)
		{
			m_vecIndex.push_back(2 * i + 2);
			m_vecIndex.push_back(2 * i);
			m_vecIndex.push_back(2 * i + 1);

			m_vecIndex.push_back(2 * i + 2);
			m_vecIndex.push_back(2 * i + 1);
			m_vecIndex.push_back(2 * i + 3);
		}

		// Phase E5 — was CreateBindable<Mesh> (Drawable child). Now register
		// the dynamic Mesh in BindableManager directly.
		m_pMesh = Engine::StaticCreateBindable<Engine::Mesh>("TrailMesh", m_vecVertex, m_vecIndex, D3D11_USAGE_DYNAMIC);
	}

	Trail::Trail(const Trail& other) :
		Engine::Component(other)
		, m_vecVertex(other.m_vecVertex)
		, m_vecIndex(other.m_vecIndex)
		, m_pMesh(other.m_pMesh)
		, m_pVS(other.m_pVS)
		, m_pPS(other.m_pPS)
		, m_pTopology(other.m_pTopology)
		, m_pInputLayout(other.m_pInputLayout)
		, m_pRasterizerState(other.m_pRasterizerState)
		, m_pMaterial(other.m_pMaterial)
		, m_pTransform(other.m_pTransform)
		, m_eRenderLayer(other.m_eRenderLayer)
	{
	}

	void Trail::SetAllPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom)
	{
		for (int i = 0; i < m_vecVertex.size() / 2; ++i)
		{
			m_vecVertex[i * 2].pos = vTop;
			m_vecVertex[i * 2 + 1].pos = vBottom;
		}
	}

	void Trail::SetPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom)
	{
		for (int i = static_cast<int>(m_vecVertex.size()) - 1; i >= 2 ; --i)
		{
			m_vecVertex[i].pos = m_vecVertex[i - 2].pos;
		}

		m_vecVertex[0].pos = vTop;
		m_vecVertex[1].pos = vBottom;

		Engine::Drawable::SetNormals(m_vecVertex, m_vecIndex);
		Engine::Drawable::SetTangent(m_vecVertex, m_vecIndex);

		if (m_pMesh)
			m_pMesh->SetVertexBuffer(0, &m_vecVertex[0], static_cast<int>(sizeof(Engine::VertexStandard) * m_vecVertex.size()));
	}

	bool Trail::Init()
	{
		if (!__super::Init())
			return false;

		// Per-instance Material (clone of the canonical "Material").
		std::shared_ptr<Engine::Material> pSrcMaterial =
			Engine::StaticFindBindable<Engine::Material>("Material");
		if (pSrcMaterial)
		{
			m_pMaterial = std::static_pointer_cast<Engine::Material>(pSrcMaterial->Clone());
			m_pMaterial->SetDiffuseColor(0.8f, 0.02f, 0.76f, 0.5f);
			m_pMaterial->SetEmissiveColor({ 0.8f, 0.02f, 0.76f, 0.2f });
			m_pMaterial->SetReflectivity(1.f);
		}

		// Resolve shared GPU pipeline resources from BindableManager.
		m_pVS              = Engine::StaticFindBindable<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
		m_pPS              = Engine::StaticFindBindable<Engine::PixelShader>("AlphaNoUVPS");
		m_pTopology        = Engine::StaticFindBindable<Engine::Topology>("TriangleList");
		m_pRasterizerState = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);
		m_pInputLayout     = Engine::StaticFindBindable<Engine::InputLayout>("Standard");

		// Per-instance Transform.
		if (!m_pTransform) m_pTransform = std::make_shared<Engine::Transform>();

		m_eRenderLayer = Engine::RENDER_LAYER::BLUR;

		return true;
	}

	void Trail::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		// Phase E5 — register a generic render callback with RenderManager
		// so the blur pass invokes Trail::Bind without RenderManager needing
		// to know the Trail type. Capture as weak ref to avoid keeping the
		// component alive past its owning GameObject's lifetime.
		std::weak_ptr<Trail> wpSelf = std::dynamic_pointer_cast<Trail>(shared_from_this());
		Engine::RenderManager::GetInst()->AddCustomRender(m_eRenderLayer,
			[wpSelf]()
			{
				if (auto pSelf = wpSelf.lock())
					pSelf->Bind();
			});
	}

	void Trail::Bind()
	{
		if (m_pTransform)        m_pTransform->Bind();
		if (m_pVS)               m_pVS->Bind();
		if (m_pPS)               m_pPS->Bind();
		if (m_pTopology)         m_pTopology->Bind();
		if (m_pInputLayout)      m_pInputLayout->Bind();
		if (m_pRasterizerState)  m_pRasterizerState->Bind();
		if (m_pMaterial)         m_pMaterial->Bind();

		if (m_pMesh) m_pMesh->Draw();

		if (m_pVS)               m_pVS->PostBind();
		if (m_pPS)               m_pPS->PostBind();
		if (m_pRasterizerState)  m_pRasterizerState->PostBind();
		if (m_pMaterial)         m_pMaterial->PostBind();
		if (m_pTransform)        m_pTransform->PostBind();
	}

	std::shared_ptr<Engine::Component> Trail::Clone()
	{
		return std::make_shared<Trail>(*this);
	}
}
