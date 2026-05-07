#include "Cone.h"

namespace Engine
{
	Cone::Cone() :
		Component()
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Cone::Cone(int /*iBaseCount*/) :
		Component()
	{
		// Phase E5 — Drawable-era ctor created a Mesh / VertexShader /
		// PixelShader / InputLayout (CPN) / Topology / Material and wired
		// them via Drawable's child machinery. Stripped for the Component
		// shell; reintroduce under MeshRendererComponent on a GameObject.
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Cone::Cone(const Cone& cone) :
		Component(cone)
	{
	}

	void Cone::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	std::shared_ptr<Component> Cone::Clone()
	{
		return std::make_shared<Cone>(*this);
	}

	void Cone::CreateConeIndex(int iBaseCount, std::vector<unsigned int>& vecIndex)
	{
		for (int i = 0; i < iBaseCount; ++i)
		{
			vecIndex.push_back(0);
			vecIndex.push_back((i + 1) % (iBaseCount)+1);
			vecIndex.push_back((i + 1) % (iBaseCount + 1));
		}

		for (int i = 0; i < iBaseCount - 2; ++i)
		{
			if (i % 2 == 0)
			{
				vecIndex.push_back(i / 2 + 1);
				vecIndex.push_back(i / 2 + 1 + 1);
				vecIndex.push_back(iBaseCount - 1 - i / 2 + 1);
			}
			else
			{
				vecIndex.push_back(iBaseCount - 1 - i / 2 + 1);
				vecIndex.push_back((i + 1) / 2 + 1);
				vecIndex.push_back(iBaseCount - 1 - (i + 1) / 2 + 1);
			}
		}
	}
}
