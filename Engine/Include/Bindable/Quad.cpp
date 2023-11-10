#include "Quad.h"
#include "Texture.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Topology.h"
#include "InputLayout.h"
#include "Sampler.h"
#include "TransformBuffer.h"
#include "Material.h"
#include "BindableManager.h"
#include "Mesh.h"

namespace Engine
{
	Quad::Quad(const TCHAR* pFileName) :
		Drawable()
	{
		std::string name = "Quad";

		std::vector<unsigned int> vecIndex =
		{
			0, 1, 2,
			1, 3, 2
		};

		std::shared_ptr<Mesh> pMesh = StaticFindBindable<Mesh>(name);

		if (pMesh == nullptr)
		{
			std::vector<VertexTexture> vecVertex = CreateQuadVertex<VertexTexture>();

			SetNormals(vecVertex, vecIndex);

			pMesh = StaticCreateBindable<Mesh>(name, vecVertex, vecIndex);
		}

		AddChild(pMesh);

		FindAndAddBind<Topology>("TriangleList");

		std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>("QuadTexture");

		if (pTexture == nullptr)
		{
			pTexture = StaticCreateBindable<Texture>("QuadTexture", pFileName, TEXTURE_PATH);
		}

		AddChild(pTexture);

		FindAndAddBind<Sampler>("Anisotropic");

		std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

		SetMaterial(pMaterial);

		AddChild(pMaterial);
	}

	Quad::Quad(const Quad& quad) :
		Drawable(quad)
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

	void Quad::Update(float fDeltaTime)
	{
		CheckRangeAndMove();

		__super::Update(fDeltaTime);
	}

	void Quad::Bind()
	{
		__super::Bind();
	}

	std::shared_ptr<Bindable> Quad::Clone()
	{
		return std::make_shared<Quad>(*this);
	}

}