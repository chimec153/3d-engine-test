#include "Trail.h"
#include "Bindable/Topology.h"
#include "Bindable/RasterizerState.h"
#include "Bindable/InputLayout.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Mesh.h"

Client::Trail::Trail(int iCount)	:
	Drawable()
{
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

	m_pMesh = CreateBindable<Engine::Mesh>("TrailMesh", m_vecVertex, m_vecIndex, D3D11_USAGE_DYNAMIC);
}

void Client::Trail::SetAllPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom)
{
	for (int i = 0; i < m_vecVertex.size() / 2; ++i)
	{
		m_vecVertex[i * 2].pos = vTop;
		m_vecVertex[i * 2 + 1].pos = vBottom;
	}
}

void Client::Trail::SetPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom)
{
	for (int i = static_cast<int>(m_vecVertex.size()) - 1; i >= 2 ; --i)
	{
		m_vecVertex[i].pos = m_vecVertex[i - 2].pos;
	}

	m_vecVertex[0].pos = vTop;
	m_vecVertex[1].pos = vBottom;

	SetNormals(m_vecVertex, m_vecIndex);
	SetTangent(m_vecVertex, m_vecIndex);

	m_pMesh->SetVertexBuffer(0, &m_vecVertex[0], static_cast<int>(sizeof(Engine::VertexStandard) * m_vecVertex.size()));
}

bool Client::Trail::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	std::shared_ptr<Engine::Material> pMaterial = Engine::StaticFindBindable<Engine::Material>("Material");

	pMaterial = std::static_pointer_cast<Engine::Material>(pMaterial->Clone());

	pMaterial->SetDiffuseColor(0.8f, 0.02f, 0.76f, 0.5f);
	pMaterial->SetEmissiveColor({ 0.8f, 0.02f, 0.76f, 0.2f });
	pMaterial->SetReflectivity(1.f);

	AddChild(pMaterial);
	SetRenderLayer(Engine::RENDER_LAYER::BLUR);
	FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
	FindAndAddBind<Engine::PixelShader>("AlphaNoUVPS");
	FindAndAddBind<Engine::Topology>("TriangleList");
	FindAndAddBind<Engine::RasterizerState>(CULL_NONE);
	FindAndAddBind<Engine::InputLayout>("Standard");

	return true;
}
