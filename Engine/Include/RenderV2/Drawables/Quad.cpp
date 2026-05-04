#include "Quad.h"
#include "../GpuResources.h"
#include "../RenderQueue.h"
#include "../../Core/PathManager.h"
#include <array>
#include <string>

using namespace DirectX;

namespace Engine::RenderV2::Drawables
{
	// Named sub-namespace to avoid unity-build collisions with sibling
	// drawables that also define a private Vertex/kVertices/etc.
	namespace quad_detail
	{
		struct Vertex
		{
			XMFLOAT3 pos;
			XMFLOAT2 uv;
		};

		// Quad in XY plane, facing +Z. CCW-when-viewed-from-+Z (matches Box's
		// front-face winding given the back-face cull default).
		const Vertex kVertices[4] = {
			{{-0.5f, -0.5f, 0.0f}, {0, 1}},
			{{ 0.5f, -0.5f, 0.0f}, {1, 1}},
			{{-0.5f,  0.5f, 0.0f}, {0, 0}},
			{{ 0.5f,  0.5f, 0.0f}, {1, 0}},
		};

		const uint32_t kIndices[6] = { 0, 2, 1,  1, 2, 3 };

		// Translucent gradient texture: alpha varies horizontally so the cube
		// behind shows through. 8x1 RGBA8 strip is enough to demonstrate.
		std::array<uint8_t, 8 * 1 * 4> MakeAlphaGradient()
		{
			std::array<uint8_t, 8 * 1 * 4> px{};
			for (int x = 0; x < 8; ++x)
			{
				int i = x * 4;
				px[i + 0] = 255;
				px[i + 1] = 80;
				px[i + 2] = 80;
				px[i + 3] = static_cast<uint8_t>((x * 255) / 7);
			}
			return px;
		}
	}

	XMMATRIX Quad::TransformComp::World() const
	{
		XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX r = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
		return s * r * t;
	}

	Quad::Quad() = default;
	Quad::~Quad() = default;

	void Quad::SetPosition(const XMFLOAT3& pos)
	{
		if (m_transform) m_transform->position = pos;
	}

	bool Quad::Init(ID3D11Device* device)
	{
		using namespace quad_detail;

		m_transform = std::make_shared<TransformComp>();

		const TCHAR* dir = CPathManager::GetInst()->FindPath(SHADER_PATH);
		std::wstring shaderPath = (dir ? dir : L"") + std::wstring(L"TexturedMesh.hlsl");

		auto vs = std::make_shared<VertexShaderRes>();
		if (!vs->LoadFromFile(device, shaderPath.c_str(), "VSMain")) return false;

		auto ps = std::make_shared<PixelShaderRes>();
		if (!ps->LoadFromFile(device, shaderPath.c_str(), "PSMain")) return false;

		const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		auto layout = std::make_shared<InputLayoutRes>();
		if (!layout->Create(device, layoutDesc, _countof(layoutDesc),
		                    vs->Bytecode().data(), vs->Bytecode().size())) return false;

		auto vb = std::make_shared<VertexBufferRes>();
		if (!vb->Create(device, kVertices, sizeof(kVertices))) return false;

		auto ib = std::make_shared<IndexBufferRes>();
		if (!ib->Create(device, kIndices, sizeof(kIndices))) return false;

		auto cb = std::make_shared<ConstantBufferRes>();
		if (!cb->Create(device, sizeof(PerObjectCB))) return false;

		auto pixels = MakeAlphaGradient();
		auto tex = std::make_shared<TextureRes>();
		if (!tex->CreateFromMemory(device, 8, 1, pixels.data())) return false;

		auto smp = std::make_shared<SamplerRes>();
		if (!smp->CreateLinearWrap(device)) return false;

		auto blend = std::make_shared<BlendStateRes>();
		if (!blend->CreateAlphaBlend(device)) return false;

		m_proxy.vs            = vs;
		m_proxy.ps            = ps;
		m_proxy.inputLayout   = layout;
		m_proxy.vertexBuffer  = vb;
		m_proxy.indexBuffer   = ib;
		m_proxy.perObjectCB   = cb;
		m_proxy.texturePS0    = tex;
		m_proxy.samplerPS0    = smp;
		m_proxy.blendState    = blend;
		m_proxy.vertexStride  = sizeof(Vertex);
		m_proxy.indexFormat   = IndexFormat::UInt32;
		m_proxy.topology      = Topology::TriangleList;
		m_proxy.indexCount    = _countof(kIndices);
		m_proxy.vsId          = 1;   // shares VS/PS with Box (same shader file)
		m_proxy.psId          = 1;
		m_proxy.layer         = 1;   // alpha layer renders after opaque (Box layer 0)

		m_ready = true;
		return true;
	}

	void Quad::Update(float dt)
	{
		if (!m_transform) return;
		m_transform->rotation.y += dt * 0.5f;
	}

	void Quad::Submit(RenderQueue& queue, const FrameInfo& frame)
	{
		if (!m_ready) return;

		XMMATRIX world = m_transform->World();
		XMMATRIX mvp   = world * frame.viewProj;
		XMStoreFloat4x4(&m_cbData.mvp, XMMatrixTranspose(mvp));

		DrawCommand cmd = m_proxy.BuildCommand(&m_cbData, sizeof(m_cbData));
		queue.Submit(cmd);
	}
}
