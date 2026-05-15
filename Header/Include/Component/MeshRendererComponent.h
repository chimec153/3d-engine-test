#pragma once
#include "Component.h"
#include "../Types.h"

namespace Engine
{
	// Phase E2 — MeshRendererComponent extracts Drawable's rendering
	// responsibility into a Component. A GameObject with this Component
	// (plus a TransformComponent for placement) is a renderable entity
	// without inheriting from Drawable.
	//
	// E2 scope: data + Bind/Draw/PostBind API that mirrors Drawable's
	// existing render orchestration. Render-pipeline integration (RenderManager
	// registration, sort-by-state, instancing) lands in E4 when game-class
	// migrations actually create GameObjects with MeshRenderers.
	class ENGINE_DLL MeshRendererComponent : public Component
	{
	public:
		MeshRendererComponent();
		MeshRendererComponent(const MeshRendererComponent& other);
		virtual ~MeshRendererComponent() override = default;

	private:
		std::shared_ptr<class Mesh>                  m_pMesh;
		std::shared_ptr<class VertexShader>          m_pVertexShader;
		std::shared_ptr<class PixelShader>           m_pPixelShader;
		std::shared_ptr<class Material>              m_pMaterial;
		std::vector<std::shared_ptr<class Texture>>  m_vecTexture;
		std::shared_ptr<class Animation>             m_pAnimation;

		// Per-(container, sub) material overrides. Indexed as
		// m_OverrideMaterials[containerIdx][subIdx]; a nullptr cell (or out of
		// bounds) falls through to the mesh's own slot material. m_pMaterial
		// above is the legacy "single material applies to every slot"
		// override and stays as the lowest-priority override layer for
		// backwards compatibility.
		std::vector<std::vector<std::shared_ptr<class Material>>> m_OverrideMaterials;

		// Other Bindables a render needs (InputLayout, Topology,
		// RasterizerState, DepthStencilState, etc.). Separate list — direct
		// fields above are the "common" ones with named slots.
		std::list<std::shared_ptr<class Bindable>>   m_OtherBindables;

		RENDER_LAYER m_eRenderLayer;
		size_t       m_iInstanceKey;

	public:
		// Direct accessors / mutators (mirror Drawable's existing API).
		void SetMesh(const std::shared_ptr<class Mesh>& p);
		const std::shared_ptr<class Mesh>& GetMesh() const;

		void SetVertexShader(const std::shared_ptr<class VertexShader>& p);
		const std::shared_ptr<class VertexShader>& GetVertexShader() const;

		void SetPixelShader(const std::shared_ptr<class PixelShader>& p);
		const std::shared_ptr<class PixelShader>& GetPixelShader() const;

		void SetMaterial(const std::shared_ptr<class Material>& p);
		const std::shared_ptr<class Material>& GetMaterial() const;

		// Per-slot override API. (containerIdx, subIdx) addresses the same
		// slot model as Mesh's vecMaterial. SetOverrideMaterial(... nullptr)
		// clears the override and falls back to the mesh's slot material.
		void SetOverrideMaterial(int iContainerIdx, int iSubIdx, const std::shared_ptr<class Material>& p);
		std::shared_ptr<class Material> GetOverrideMaterial(int iContainerIdx, int iSubIdx) const;
		// Resolve final material for a slot: override → m_pMaterial (legacy
		// blanket override) → mesh slot material. Mirrors the resolver
		// callback passed to Mesh::Draw.
		std::shared_ptr<class Material> GetEffectiveMaterial(int iContainerIdx, int iSubIdx) const;
		// Build a MaterialResolver bound to this MeshRenderer's overrides.
		// Returns nullptr if no overrides are active so callers can skip
		// the per-draw indirection in the common case.
		std::function<std::shared_ptr<class Material>(int, int)> MakeMaterialResolver() const;

		void AddTexture(const std::shared_ptr<class Texture>& p);
		const std::vector<std::shared_ptr<class Texture>>& GetTextures() const;

		void SetAnimation(const std::shared_ptr<class Animation>& p);
		const std::shared_ptr<class Animation>& GetAnimation() const;

		// Generic Bindable child (IL, Topology, RS, DSS, ConstantBuffer, ...).
		// Routed by BINDABLE_TYPE: known kinds populate the named fields,
		// others land in m_OtherBindables.
		void AddBindable(const std::shared_ptr<class Bindable>& p);
		std::shared_ptr<class Bindable> FindBindable(BINDABLE_TYPE eType) const;
		const std::list<std::shared_ptr<class Bindable>>& GetOtherBindables() const;

		void SetRenderLayer(RENDER_LAYER eLayer);
		RENDER_LAYER GetRenderLayer() const;

		size_t GetInstanceKey() const;
		void   UpdateInstanceKey();

		// Phase E5 — per-instance data writer used by RenderManager's
		// MeshRenderer instancing path. Writes the host GameObject's
		// TRANSFORMBUFFER (matWorld / matWorldView / matWorldViewProject
		// = 192 bytes) followed by material data (diffuse / specular /
		// roughness / fraction = 44 bytes). Layout mirrors the legacy
		// Drawable::GetInstData so the same "_Inst" input-layout convention
		// continues to work.
		void GetInstData(char* pData, int iSize) const;

	public:
		// Render orchestration (mirrors Drawable's Bind / DrawShadow / etc.).
		// Call sites: RenderManager render passes (post-E4 integration).
		void Bind();
		void BindExceptShader();
		void PostBind();
		void PostBindExceptShader();
		void DrawShadow();

	public:
		virtual void PreDraw(float fDeltaTime) override;
		virtual std::shared_ptr<Component> Clone() override;
	};
}
