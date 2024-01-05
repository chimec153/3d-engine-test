#include "Cylinder.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Transform.h"
#include "InputLayout.h"
#include "Topology.h"
#include "Material.h"
#include "ConstantBuffer.h"
#include "BindableManager.h"

namespace Engine
{
	Cylinder::Cylinder(int iCount) :
		Drawable()
	{
		std::string name = "Cylinder";

		name += std::to_string(iCount);

		const std::vector<unsigned int>& vecIndex = CreateCylinderIndex(iCount);

		std::shared_ptr<Mesh> pMesh = StaticFindBindable<Mesh>(name);

		if (pMesh == nullptr)
		{
			std::vector<VertexNormal> vecVertex = CreateCylinderVertex<VertexNormal>(iCount);

			SetNormals<VertexNormal>(vecVertex, vecIndex);

			pMesh = StaticCreateBindable<Mesh>(name, vecVertex, vecIndex);
		}

		AddChild(pMesh);

		std::shared_ptr<VertexShader> pVertexShader = StaticFindBindable<VertexShader>("VertexShader main");

		if (pVertexShader == nullptr)
		{
			pVertexShader = StaticCreateBindable<VertexShader>("VertexShader main", TEXT("VertexShader.hlsl"), "main");
		}

		AddChild(pVertexShader);

		std::shared_ptr<PixelShader> pPixelShader = StaticFindBindable<PixelShader>("PixelShader PS_Sphere");

		if (pPixelShader == nullptr)
		{
			pPixelShader = StaticCreateBindable<PixelShader>("PixelShader PS_Sphere", TEXT("PixelShader.hlsl"), "PS_Sphere");
		}

		AddChild(pPixelShader);

		AddChild(StaticFindBindable<InputLayout>("PN"));

		AddChild(StaticFindBindable<Topology>("TriangleList"));

		SetMaterial(std::make_shared<Material>());

		COLOR color[6] =
		{
			{1.f, 0.f, 0.f, 1.f},
			{0.f, 1.f, 0.f, 1.f},
			{0.f, 0.f, 1.f, 1.f},
			{0.f, 1.f, 1.f, 1.f},
			{1.f, 0.f, 1.f, 1.f},
			{1.f, 1.f, 0.f, 1.f},
		};

		std::shared_ptr<ConstantBuffer<COLOR>> pColor = StaticFindBindable<ConstantBuffer<COLOR>>("COLOR");

		if (pColor == nullptr)
		{
			pColor = StaticCreateBindable<ConstantBuffer<COLOR>>("COLOR", color, 0);
		}

		AddChild(pColor);
	}

	Cylinder::Cylinder(const Cylinder& cylinder) :
		Drawable(cylinder)
	{
		const std::shared_ptr<Transform>& pTransform = GetTransform();

		if (pTransform != nullptr)
		{
			pTransform->SetRandomPosAndRotation();
		}

		const std::shared_ptr<Material>& pMaterial = GetMaterial();

		if (pMaterial != nullptr)
		{
			pMaterial->SetRandomColor();
		}
	}

	void Cylinder::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		CheckRangeAndMove();
	}

	void Cylinder::Bind()
	{
		__super::Bind();
	}

	std::shared_ptr<Bindable> Cylinder::Clone()
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
		// 0 5 1 4 2 3

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