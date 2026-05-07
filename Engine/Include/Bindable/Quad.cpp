#include "Quad.h"

namespace Engine
{
	Quad::Quad()
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Quad::Quad(const TCHAR* /*pFileName*/) :
		Component()
	{
		// Phase E5 — the Drawable-era ctor created a quad Mesh, looked up
		// or registered a Texture, attached a Topology / Sampler / Material
		// via Drawable's child machinery. Stripped for the Component shell;
		// future re-introduction should pair this with a MeshRendererComponent
		// on a GameObject and feed it the quad mesh from CreateQuadVertex.
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Quad::Quad(const Quad& quad) :
		Component(quad)
	{
	}

	void Quad::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	std::shared_ptr<Component> Quad::Clone()
	{
		return std::make_shared<Quad>(*this);
	}
}
