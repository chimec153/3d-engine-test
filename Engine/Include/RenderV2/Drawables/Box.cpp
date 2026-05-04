#include "Box.h"
#include "../GpuResources.h"
#include "../RenderQueue.h"
#include "../../Core/PathManager.h"
#include <array>
#include <string>

using namespace DirectX;

namespace Engine::RenderV2::Drawables
{
	// Named sub-namespace (not anonymous) so unity-build merges with sibling
	// drawables don't collide on common names like Vertex/kVertices.
	namespace box_detail
	{
		struct Vertex
		{
			XMFLOAT3 pos;
			XMFLOAT2 uv;
		};

		// 24 vertices (4 per face) so each face has its own UVs.
		const Vertex kVertices[24] = {
			// -Z face
			{{-0.5f, -0.5f, -0.5f}, {0, 1}}, {{ 0.5f, -0.5f, -0.5f}, {1, 1}},
			{{-0.5f,  0.5f, -0.5f}, {0, 0}}, {{ 0.5f,  0.5f, -0.5f}, {1, 0}},
			// +Z face
			{{ 0.5f, -0.5f,  0.5f}, {0, 1}}, {{-0.5f, -0.5f,  0.5f}, {1, 1}},
			{{ 0.5f,  0.5f,  0.5f}, {0, 0}}, {{-0.5f,  0.5f,  0.5f}, {1, 0}},
			// -X face
			{{-0.5f, -0.5f,  0.5f}, {0, 1}}, {{-0.5f, -0.5f, -0.5f}, {1, 1}},
			{{-0.5f,  0.5f,  0.5f}, {0, 0}}, {{-0.5f,  0.5f, -0.5f}, {1, 0}},
			// +X face
			{{ 0.5f, -0.5f, -0.5f}, {0, 1}}, {{ 0.5f, -0.5f,  0.5f}, {1, 1}},
			{{ 0.5f,  0.5f, -0.5f}, {0, 0}}, {{ 0.5f,  0.5f,  0.5f}, {1, 0}},
			// -Y face
			{{-0.5f, -0.5f,  0.5f}, {0, 1}}, {{ 0.5f, -0.5f,  0.5f}, {1, 1}},
			{{-0.5f, -0.5f, -0.5f}, {0, 0}}, {{ 0.5f, -0.5f, -0.5f}, {1, 0}},
			// +Y face
			{{-0.5f,  0.5f, -0.5f}, {0, 1}}, {{ 0.5f,  0.5f, -0.5f}, {1, 1}},
			{{-0.5f,  0.5f,  0.5f}, {0, 0}}, {{ 0.5f,  0.5f,  0.5f}, {1, 0}},
		};

		const uint32_t kIndices[36] = {
			 0,  2,  1,   1,  2,  3,
			 4,  6,  5,   5,  6,  7,
			 8, 10,  9,   9, 10, 11,
			12, 14, 13,  13, 14, 15,
			16, 18, 17,  17, 18, 19,
			20, 22, 21,  21, 22, 23,
		};

		// 4x4 RGBA8 checkerboard. Two-color pattern, no file IO needed.
		std::array<uint8_t, 4 * 4 * 4> MakeCheckerboard()
		{
			std::array<uint8_t, 4 * 4 * 4> px{};
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x)
				{
					bool dark = ((x + y) & 1) == 0;
					int i = (y * 4 + x) * 4;
					px[i + 0] = dark ? 40  : 220;
					px[i + 1] = dark ? 40  : 200;
					px[i + 2] = dark ? 60  : 180;
					px[i + 3] = 255;
				}
			return px;
		}
	}

	XMMATRIX Box::TransformComp::World() const
	{
		XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX r = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
		return s * r * t;
	}

	Box::Box() = default;
	Box::~Box() = default;

	bool Box::Init(ID3D11Device* device)
	{
		using namespace box_detail;

		m_transform = std::make_shared<TransformComp>();

		const TCHAR* dir = CPathManager::GetInst()->FindPath(SHADER_PATH);
		std::wstring shaderPath = (dir ? dir : L"") + std::wstring(L"TexturedMesh.hlsl");

		auto vs = std::make_shared<VertexShaderRes>();
		if (!vs->LoadFromFile(device, shaderPath.c_str(), "VSMain"))
			return false;

		auto ps = std::make_shared<PixelShaderRes>();
		if (!ps->LoadFromFile(device, shaderPath.c_str(), "PSMain"))
			return false;

		const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		auto layout = std::make_shared<InputLayoutRes>();
		if (!layout->Create(device, layoutDesc, _countof(layoutDesc),
		                    vs->Bytecode().data(), vs->Bytecode().size()))
			return false;

		auto vb = std::make_shared<VertexBufferRes>();
		if (!vb->Create(device, kVertices, sizeof(kVertices))) return false;

		auto ib = std::make_shared<IndexBufferRes>();
		if (!ib->Create(device, kIndices, sizeof(kIndices))) return false;

		auto cb = std::make_shared<ConstantBufferRes>();
		if (!cb->Create(device, sizeof(PerObjectCB))) return false;

		auto checker = MakeCheckerboard();
		auto tex = std::make_shared<TextureRes>();
		if (!tex->CreateFromMemory(device, 4, 4, checker.data())) return false;

		auto smp = std::make_shared<SamplerRes>();
		if (!smp->CreateLinearWrap(device)) return false;

		m_proxy.vs            = vs;
		m_proxy.ps            = ps;
		m_proxy.inputLayout   = layout;
		m_proxy.vertexBuffer  = vb;
		m_proxy.indexBuffer   = ib;
		m_proxy.perObjectCB   = cb;
		m_proxy.texturePS0    = tex;
		m_proxy.samplerPS0    = smp;
		m_proxy.vertexStride  = sizeof(Vertex);
		m_proxy.indexFormat   = IndexFormat::UInt32;
		m_proxy.topology      = Topology::TriangleList;
		m_proxy.indexCount    = _countof(kIndices);
		m_proxy.vsId          = 1;
		m_proxy.psId          = 1;
		m_proxy.layer         = 0;

		m_ready = true;
		return true;
	}

	void Box::Update(float dt)
	{
		if (!m_transform) return;
		m_transform->rotation.y += dt;
	}

	void Box::Submit(RenderQueue& queue, const FrameInfo& frame)
	{
		if (!m_ready) return;

		XMMATRIX world = m_transform->World();
		XMMATRIX mvp   = world * frame.viewProj;
		XMStoreFloat4x4(&m_cbData.mvp, XMMatrixTranspose(mvp));

		DrawCommand cmd = m_proxy.BuildCommand(&m_cbData, sizeof(m_cbData));
		queue.Submit(cmd);
	}
}
