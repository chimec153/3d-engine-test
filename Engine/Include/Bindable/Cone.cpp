#include "Cone.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "TransformBuffer.h"
#include "InputLayout.h"
#include "Topology.h"
#include "Material.h"
#include "BindableManager.h"
#include "Mesh.h"

namespace Engine
{
	Cone::Cone(int iBaseCount) :
		Drawable()
	{
		std::string name = "Cone";

		name += std::to_string(iBaseCount);

		std::vector<unsigned int> vecIndex;

		CreateConeIndex(iBaseCount, vecIndex);

		std::shared_ptr<Mesh> pMesh = StaticFindBindable<Mesh>(name);

		if (pMesh == nullptr)
		{
			std::vector<VertexColor> vecVertex;

			CreateConeVertex<VertexColor>(iBaseCount, vecVertex);

			SetNormals(vecVertex, vecIndex);

			pMesh = StaticCreateBindable<Mesh>(name, vecVertex, vecIndex);
		}

		AddChild(pMesh);

		std::shared_ptr<VertexShader> pVertexShader = StaticFindBindable<VertexShader>("VertexShader VS_Cone");

		if (pVertexShader == nullptr)
		{
			pVertexShader = StaticCreateBindable<VertexShader>("VertexShader VS_Cone", TEXT("VertexShader.hlsl"), "VS_Cone");
		}

		AddChild(pVertexShader);

		std::shared_ptr<PixelShader> pPixelShader = StaticFindBindable<PixelShader>("PixelShader PS_Cone");

		if (pPixelShader == nullptr)
		{
			pPixelShader = StaticCreateBindable<PixelShader>("PixelShader PS_Cone", TEXT("PixelShader.hlsl"), "PS_Cone");
		}

		AddChild(pPixelShader);

		D3D11_INPUT_ELEMENT_DESC desc[] = {
			{"Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		std::shared_ptr<InputLayout> pInputLayoutCPN = StaticFindBindable<InputLayout>("CPN");

		if (pInputLayoutCPN == nullptr)
		{
			pInputLayoutCPN = StaticCreateBindable<InputLayout>("CPN", pVertexShader, desc, static_cast<int>(sizeof(desc) / sizeof(D3D11_INPUT_ELEMENT_DESC)));
		}

		AddChild(pInputLayoutCPN);

		FindAndAddBind<Topology>("TriangleList");

		std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

		SetMaterial(pMaterial);

		AddChild(pMaterial);
	}

	Cone::Cone(const Cone& cone) :
		Drawable(cone)
	{
		const std::shared_ptr<TransformBuffer>& pTransform = GetTransform();

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

	void Cone::Update(float fDeltaTime)
	{
		CheckRangeAndMove();

		__super::Update(fDeltaTime);
	}

	void Cone::Bind()
	{
		__super::Bind();

	}

	std::shared_ptr<Bindable> Cone::Clone()
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