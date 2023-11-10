#include "Box.h"
#include <Windows.h>
#include "TransformBuffer.h"
#include <DirectXMath.h>
#include "TransformBuffer.h"
#include "InputLayout.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Topology.h"
#include "ConstantBuffer.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "../Core/Window.h"
#include "Texture.h"
#include "Sampler.h"
#include "Material.h"
#include "FbxLoader.h"
#include "BindableManager.h"
#include "Mesh.h"

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
	//std::vector<VertexTexture> Box::vertex =
	//{
	//	{0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f,		0.f, 0.f, 0.f, 0.f, 0.f},
	//	{0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f,		0.f, 0.f, 0.f, 1.f, 0.f},
	//	{0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f,		0.f, 0.f, 0.f, 0.f, 1.f},
	//	{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f,	0.f, 0.f, 0.f, 1.f, 1.f},
	//	{0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,		0.f, 0.f, 0.f, 1.f, 0.f},
	//	{0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f,	0.f, 0.f, 0.f, 0.f, 0.f},
	//	{0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,	0.f, 0.f, 0.f, 1.f, 1.f},
	//	{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,	0.f, 0.f, 0.f, 0.f, 1.f},
	//};

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
		Drawable()
	{
		FindAndAddBind<Topology>("TriangleList");
	}

	Box::Box(const Box& box) :
		Drawable(box)
	{
	}

	Box::~Box() noexcept
	{
	}

	void Box::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	void Box::Bind()
	{
		__super::Bind();
	}

	std::shared_ptr<Bindable> Box::Clone()
	{
		return std::make_shared<Box>(*this);
	}

	void Box::SetDefaultVertexAndIndex()
	{
		std::shared_ptr<Mesh> pMesh = StaticFindBindable<Mesh>("box");

		if (pMesh == nullptr)
		{
			std::vector<VertexTexture> vecVertex = vertex;

			SetNormals<VertexTexture>(vertex, index);

			SetTangent(vertex, index);

			pMesh = StaticCreateBindable<Mesh>("box", vertex, index);
		}

		AddChild(pMesh);
	}

	void Box::SetTextureVertexAndIndex()
	{
		Load(TEXT("nano_textured\\nanosuit.obj"));

		FindAndAddBind<Sampler>("Anisotropic");
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