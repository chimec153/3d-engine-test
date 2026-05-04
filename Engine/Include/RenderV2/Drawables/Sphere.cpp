#include "Sphere.h"
#include "../GpuResources.h"
#include "../RenderQueue.h"
#include "../../Core/PathManager.h"

// Reuse the existing engine sphere geometry generators. They're static
// templates on Engine::Sphere — we only need the math, not the Drawable
// base class.
#include "../../Bindable/Sphere.h"

#include <array>
#include <string>
#include <vector>
#include <cmath>

using namespace DirectX;

namespace Engine::RenderV2::Drawables
{
	namespace sphere_detail
	{
		struct Vertex
		{
			XMFLOAT3 pos;
			XMFLOAT2 uv;
		};

		// Spherical → equirectangular UV. Pos is in unit-sphere-ish range
		// produced by Engine::Sphere::CreateSphereVertex (radius 0.5).
		void ComputeUV(Vertex& v)
		{
			float x = v.pos.x, y = v.pos.y, z = v.pos.z;
			float len = std::sqrt(x*x + y*y + z*z);
			if (len > 0.0f) { x /= len; y /= len; z /= len; }
			float u = 0.5f + std::atan2(z, x) / (2.0f * 3.14159265f);
			float vCoord = 0.5f - std::asin(y) / 3.14159265f;
			v.uv = { u, vCoord };
		}

		// Procedural 16x8 RGBA8 — vertical color bands so rotation is
		// visible on a sphere. UV-mapped equirectangularly.
		std::array<uint8_t, 16 * 8 * 4> MakeBandedTexture()
		{
			std::array<uint8_t, 16 * 8 * 4> px{};
			for (int y = 0; y < 8; ++y)
				for (int x = 0; x < 16; ++x)
				{
					int i = (y * 16 + x) * 4;
					// Hue cycles across longitude.
					float t = static_cast<float>(x) / 16.0f;
					px[i + 0] = static_cast<uint8_t>(180 + 60 * std::sin(t * 6.28f));
					px[i + 1] = static_cast<uint8_t>(120 + 80 * std::sin(t * 6.28f + 2.0f));
					px[i + 2] = static_cast<uint8_t>(140 + 80 * std::sin(t * 6.28f + 4.0f));
					px[i + 3] = 255;
				}
			return px;
		}
	}

	XMMATRIX Sphere::TransformComp::World() const
	{
		XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX r = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
		return s * r * t;
	}

	Sphere::Sphere(int rings, int sectors)
		: m_rings(rings), m_sectors(sectors)
	{
	}

	Sphere::~Sphere() = default;

	void Sphere::SetPosition(const XMFLOAT3& pos)
	{
		if (m_transform) m_transform->position = pos;
	}

	bool Sphere::Init(ID3D11Device* device)
	{
		using namespace sphere_detail;

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

		// Reuse Engine::Sphere's static template — fills pos only; we add UVs.
		std::vector<Vertex> vertices;
		Engine::Sphere::CreateSphereVertex<Vertex>(m_rings, m_sectors, vertices);
		for (auto& v : vertices) ComputeUV(v);

		std::vector<unsigned int> indices;
		Engine::Sphere::CreateSphereIndex(m_rings, m_sectors, indices);

		auto vb = std::make_shared<VertexBufferRes>();
		if (!vb->Create(device, vertices.data(), vertices.size() * sizeof(Vertex))) return false;

		auto ib = std::make_shared<IndexBufferRes>();
		if (!ib->Create(device, indices.data(), indices.size() * sizeof(unsigned int))) return false;

		auto cb = std::make_shared<ConstantBufferRes>();
		if (!cb->Create(device, sizeof(PerObjectCB))) return false;

		auto pixels = MakeBandedTexture();
		auto tex = std::make_shared<TextureRes>();
		if (!tex->CreateFromMemory(device, 16, 8, pixels.data())) return false;

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
		m_proxy.indexCount    = static_cast<uint32_t>(indices.size());
		m_proxy.vsId          = 1;
		m_proxy.psId          = 1;
		m_proxy.layer         = 0;

		m_ready = true;
		return true;
	}

	void Sphere::Update(float dt)
	{
		if (!m_transform) return;
		m_transform->rotation.y += dt * 0.7f;
	}

	void Sphere::Submit(RenderQueue& queue, const FrameInfo& frame)
	{
		if (!m_ready) return;

		XMMATRIX world = m_transform->World();
		XMMATRIX mvp   = world * frame.viewProj;
		XMStoreFloat4x4(&m_cbData.mvp, XMMatrixTranspose(mvp));

		DrawCommand cmd = m_proxy.BuildCommand(&m_cbData, sizeof(m_cbData));
		queue.Submit(cmd);
	}
}
