#include "Box.h"

namespace Engine
{
	std::vector<VertexTexture> Box::vertex =
	{
		{0.f, 0.f, 0.f, 0.f, 0.5f, 0.5f, 0.5f,		0.f, 0.f, 0.f, 0.f, 0.f},
		{0.f, 0.f, 0.f, 0.f, -0.5f, 0.5f, 0.5f,		0.f, 0.f, 0.f, 1.f, 0.f},
		{0.f, 0.f, 0.f, 0.f, 0.5f, -0.5f, 0.5f,		0.f, 0.f, 0.f, 0.f, 1.f},
		{0.f, 0.f, 0.f, 0.f, -0.5f, -0.5f, 0.5f,	0.f, 0.f, 0.f, 1.f, 1.f},
		{0.f, 0.f, 0.f, 0.f, 0.5f, 0.5f, -0.5f,		0.f, 0.f, 0.f, 1.f, 0.f},
		{0.f, 0.f, 0.f, 0.f, -0.5f, 0.5f, -0.5f,	0.f, 0.f, 0.f, 0.f, 0.f},
		{0.f, 0.f, 0.f, 0.f, 0.5f, -0.5f, -0.5f,	0.f, 0.f, 0.f, 1.f, 1.f},
		{0.f, 0.f, 0.f, 0.f, -0.5f, -0.5f, -0.5f,	0.f, 0.f, 0.f, 0.f, 1.f},
	};

	std::vector<unsigned int> Box::index =
	{
		0,1,2,
		1,3,2,
		0,2,6,
		0,6,4,
		4,6,7,
		4,7,5,
		1,5,3,
		5,7,3,
		0,5,1,
		0,4,5,
		2,3,6,
		3,7,6,
	};

	Box::Box() :
		Component()
	{
		// Phase E5 — Drawable-era ctor attached a Topology to the bindable
		// child list. Component shells skip GPU-resource setup; the cube
		// vertex/index data plus the static helpers below are still here
		// for future GameObject + MeshRenderer use.
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Box::Box(const Box& box) :
		Component(box)
	{
	}

	Box::~Box() noexcept
	{
	}

	void Box::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	std::shared_ptr<Component> Box::Clone()
	{
		return std::make_shared<Box>(*this);
	}

	void Box::SetDefaultVertexAndIndex()
	{
		// Phase E5 — Drawable's child machinery is gone. Future use should
		// build a Mesh via Engine::StaticCreateBindable<Mesh>(...) and
		// install it in the GameObject's MeshRendererComponent slot.
	}

	void Box::SetTextureVertexAndIndex()
	{
		// Phase E5 — used to call Drawable::Load(...) + FindAndAddBind<Sampler>;
		// stripped for the Component shell. Reintroduce under MeshRenderer.
	}

	std::vector<unsigned int> Box::GetTextureIndex()
	{
		return
		{
			0,1,3,
			0,3,2,
			5,4,7,
			7,4,6,
			4 + 8,0 + 8,2 + 8,
			4 + 8,2 + 8,6 + 8,
			1 + 8,5 + 8,7 + 8,
			1 + 8,7 + 8,3 + 8,
			7 + 16,6 + 16,3 + 16,
			3 + 16,6 + 16,2 + 16,
			1 + 16,0 + 16,5 + 16,
			5 + 16,0 + 16,4 + 16,
		};
	}
}
