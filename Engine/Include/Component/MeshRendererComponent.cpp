#include "MeshRendererComponent.h"
#include "../Bindable/Mesh.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Material.h"
#include "../Bindable/Texture.h"
#include "../Bindable/Animation.h"

namespace Engine
{
	MeshRendererComponent::MeshRendererComponent() :
		m_eRenderLayer(RENDER_LAYER::OPACUE)
		, m_iInstanceKey(0)
	{
	}

	MeshRendererComponent::MeshRendererComponent(const MeshRendererComponent& other) :
		Component(other)
		, m_pMesh(other.m_pMesh)
		, m_pVertexShader(other.m_pVertexShader)
		, m_pPixelShader(other.m_pPixelShader)
		, m_pMaterial(other.m_pMaterial)
		, m_vecTexture(other.m_vecTexture)
		, m_pAnimation(other.m_pAnimation)
		, m_OtherBindables(other.m_OtherBindables)
		, m_eRenderLayer(other.m_eRenderLayer)
		, m_iInstanceKey(other.m_iInstanceKey)
	{
	}

	void MeshRendererComponent::SetMesh(const std::shared_ptr<Mesh>& p) { m_pMesh = p; UpdateInstanceKey(); }
	const std::shared_ptr<Mesh>& MeshRendererComponent::GetMesh() const { return m_pMesh; }

	void MeshRendererComponent::SetVertexShader(const std::shared_ptr<VertexShader>& p) { m_pVertexShader = p; UpdateInstanceKey(); }
	const std::shared_ptr<VertexShader>& MeshRendererComponent::GetVertexShader() const { return m_pVertexShader; }

	void MeshRendererComponent::SetPixelShader(const std::shared_ptr<PixelShader>& p) { m_pPixelShader = p; UpdateInstanceKey(); }
	const std::shared_ptr<PixelShader>& MeshRendererComponent::GetPixelShader() const { return m_pPixelShader; }

	void MeshRendererComponent::SetMaterial(const std::shared_ptr<Material>& p) { m_pMaterial = p; UpdateInstanceKey(); }
	const std::shared_ptr<Material>& MeshRendererComponent::GetMaterial() const { return m_pMaterial; }

	void MeshRendererComponent::AddTexture(const std::shared_ptr<Texture>& p)
	{
		m_vecTexture.push_back(p);
		UpdateInstanceKey();
	}
	const std::vector<std::shared_ptr<Texture>>& MeshRendererComponent::GetTextures() const { return m_vecTexture; }

	void MeshRendererComponent::SetAnimation(const std::shared_ptr<Animation>& p) { m_pAnimation = p; UpdateInstanceKey(); }
	const std::shared_ptr<Animation>& MeshRendererComponent::GetAnimation() const { return m_pAnimation; }

	void MeshRendererComponent::AddBindable(const std::shared_ptr<Bindable>& p)
	{
		assert(p != nullptr);

		// Route known types into named slots; everything else goes into the
		// generic side-list. Mirrors Drawable::AddChild's switch dispatch.
		switch (p->GetBindableType())
		{
		case BINDABLE_TYPE::MESH:
			SetMesh(std::static_pointer_cast<Mesh>(p));
			return;
		case BINDABLE_TYPE::VERTEX_SHADER:
			SetVertexShader(std::static_pointer_cast<VertexShader>(p));
			return;
		case BINDABLE_TYPE::PIXEL_SHADER:
			SetPixelShader(std::static_pointer_cast<PixelShader>(p));
			return;
		case BINDABLE_TYPE::MATERIAL:
			SetMaterial(std::static_pointer_cast<Material>(p));
			return;
		case BINDABLE_TYPE::TEXTURE:
			AddTexture(std::static_pointer_cast<Texture>(p));
			return;
		// Phase E3 — Animation no longer routes through AddBindable
		// (it's a Component now). Use SetAnimation(std::shared_ptr<Animation>)
		// directly via the Component API.
		default:
			break;
		}

		m_OtherBindables.push_back(p);
	}

	std::shared_ptr<Bindable> MeshRendererComponent::FindBindable(BINDABLE_TYPE eType) const
	{
		switch (eType)
		{
		case BINDABLE_TYPE::MESH:          return m_pMesh;
		case BINDABLE_TYPE::VERTEX_SHADER: return m_pVertexShader;
		case BINDABLE_TYPE::PIXEL_SHADER:  return m_pPixelShader;
		case BINDABLE_TYPE::MATERIAL:      return m_pMaterial;
		// Phase E3 — Animation migrated to Component; no longer surfaced
		// via the Bindable lookup path. Use GetAnimation() directly.
		case BINDABLE_TYPE::TEXTURE:
			return m_vecTexture.empty() ? nullptr : m_vecTexture.front();
		default:
			break;
		}

		for (const auto& p : m_OtherBindables)
		{
			if (p->GetBindableType() == eType) return p;
		}
		return nullptr;
	}

	const std::list<std::shared_ptr<Bindable>>& MeshRendererComponent::GetOtherBindables() const
	{
		return m_OtherBindables;
	}

	void MeshRendererComponent::SetRenderLayer(RENDER_LAYER eLayer) { m_eRenderLayer = eLayer; }
	RENDER_LAYER MeshRendererComponent::GetRenderLayer() const { return m_eRenderLayer; }
	size_t MeshRendererComponent::GetInstanceKey() const { return m_iInstanceKey; }

	void MeshRendererComponent::UpdateInstanceKey()
	{
		std::hash<std::string> hs;

		m_iInstanceKey = 1;
		m_iInstanceKey *= m_pMesh         ? hs(m_pMesh->GetTag())         : 1;
		m_iInstanceKey *= m_pMaterial     ? hs(m_pMaterial->GetTag())     : 1;
		m_iInstanceKey *= m_pVertexShader ? hs(m_pVertexShader->GetTag()) : 1;
		m_iInstanceKey *= m_pPixelShader  ? hs(m_pPixelShader->GetTag())  : 1;
		m_iInstanceKey *= m_pAnimation    ? hs(m_pAnimation->GetTag())    : 1;

		for (const auto& tex : m_vecTexture)
			m_iInstanceKey *= hs(tex->GetTag());
	}

	void MeshRendererComponent::Bind()
	{
		// Bind every named slot + the side-list bindables. Excludes Mesh
		// (it doesn't have GPU bind logic — Mesh::Draw handles VB/IB
		// itself during the draw call). Ordering mirrors Drawable's
		// historical iteration.
		if (m_pVertexShader) m_pVertexShader->Bind();
		if (m_pPixelShader)  m_pPixelShader->Bind();
		if (m_pMaterial)     m_pMaterial->Bind();
		if (m_pAnimation)    m_pAnimation->Bind();
		for (const auto& tex : m_vecTexture) tex->Bind();
		for (const auto& b   : m_OtherBindables)
		{
			switch (b->GetObjectType())
			{
			case OBJECT_TYPE::BIND:
			case OBJECT_TYPE::COLLIDER:
				b->Bind();
				break;
			default:
				break;
			}
		}
	}

	void MeshRendererComponent::BindExceptShader()
	{
		// Used by shadow pass: skip VS/PS (they're set externally to a
		// shadow-specific pair). Keep VB/IB/IL/Topology/Transform-CB.
		for (const auto& b : m_OtherBindables)
		{
			switch (b->GetBindableType())
			{
			case BINDABLE_TYPE::VERTEX_BUFFER:
			case BINDABLE_TYPE::INDEX_BUFFER:
			case BINDABLE_TYPE::INPUTLAYOUT:
			case BINDABLE_TYPE::TOPOLOGY:
				b->Bind();
				break;
			default:
				break;
			}
		}
	}

	void MeshRendererComponent::PostBind()
	{
		if (m_pVertexShader) m_pVertexShader->PostBind();
		if (m_pPixelShader)  m_pPixelShader->PostBind();
		if (m_pMaterial)     m_pMaterial->PostBind();
		if (m_pAnimation)    m_pAnimation->PostBind();
		for (const auto& b : m_OtherBindables)
		{
			if (b->GetObjectType() == OBJECT_TYPE::BIND)
				b->PostBind();
		}
	}

	void MeshRendererComponent::PostBindExceptShader()
	{
		for (const auto& b : m_OtherBindables)
		{
			switch (b->GetBindableType())
			{
			case BINDABLE_TYPE::VERTEX_BUFFER:
			case BINDABLE_TYPE::INDEX_BUFFER:
			case BINDABLE_TYPE::INPUTLAYOUT:
			case BINDABLE_TYPE::TOPOLOGY:
				b->PostBind();
				break;
			default:
				break;
			}
		}
	}

	void MeshRendererComponent::DrawShadow()
	{
		BindExceptShader();
		if (m_pMesh) m_pMesh->Draw();
		// Animation final-buffer cleanup, if present, mirrors Drawable's path.
		// Skipping for E2 minimal — full parity will be checked when
		// MeshRenderer is wired into the render pipeline in E4.
	}

	std::shared_ptr<Component> MeshRendererComponent::Clone()
	{
		return std::make_shared<MeshRendererComponent>(*this);
	}
}
