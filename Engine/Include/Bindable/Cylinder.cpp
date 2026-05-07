#include "Cylinder.h"

namespace Engine
{
	Cylinder::Cylinder() :
		Component()
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Cylinder::Cylinder(int /*iCount*/) :
		Component()
	{
		// Phase E5 — Drawable-era ctor created a Mesh, "VertexShader main",
		// "PixelShader PS_Sphere", PN InputLayout, TriangleList Topology,
		// Material, and a COLOR ConstantBuffer via Drawable's child API.
		// Stripped for the Component shell; reintroduce under MeshRenderer.
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Cylinder::Cylinder(const Cylinder& cylinder) :
		Component(cylinder)
	{
	}

	void Cylinder::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	std::shared_ptr<Component> Cylinder::Clone()
	{
		return std::make_shared<Cylinder>(*this);
	}

	std::vector<unsigned int> Cylinder::CreateCylinderIndex(int iCount)
	{
		std::vector<unsigned int> vecIndex;

		for (int i = 0; i < iCount - 2; ++i)
		{
			if (i % 2 == 0)
			{
				vecIndex.push_back(i / 2);
				vecIndex.push_back(i / 2 + 1);
				vecIndex.push_back(iCount - i / 2 - 1);
			}
			else
			{
				vecIndex.push_back(iCount - i / 2 - 1);
				vecIndex.push_back(i / 2 + 1);
				vecIndex.push_back(iCount - (i + 1) / 2 - 1);
			}
		}

		for (int i = 0; i < iCount - 2; ++i)
		{
			if (i % 2 == 0)
			{
				vecIndex.push_back(i / 2 + iCount);
				vecIndex.push_back(iCount - i / 2 - 1 + iCount);
				vecIndex.push_back(i / 2 + 1 + iCount);
			}
			else
			{
				vecIndex.push_back(iCount - i / 2 - 1 + iCount);
				vecIndex.push_back(iCount - (i + 1) / 2 - 1 + iCount);
				vecIndex.push_back(i / 2 + 1 + iCount);
			}
		}

		for (int i = 0; i < iCount; ++i)
		{
			vecIndex.push_back(iCount + i);
			vecIndex.push_back((i + 1) % iCount);
			vecIndex.push_back(i);

			vecIndex.push_back(iCount + (i + 1) % iCount);
			vecIndex.push_back((i + 1) % iCount);
			vecIndex.push_back(iCount + i);
		}

		return vecIndex;
	}
}
