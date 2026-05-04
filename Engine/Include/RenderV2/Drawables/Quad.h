#pragma once

#include "../Drawable.h"
#include "../Component.h"
#include "../RenderProxy.h"
#include <DirectXMath.h>
#include <memory>

struct ID3D11Device;

namespace Engine::RenderV2::Drawables
{
	// Textured world-space quad with alpha blending. Reuses TexturedMesh.hlsl
	// — same vertex format (pos+UV) and CB layout as Box. Distinct from Box
	// only in geometry (4 verts) and a non-null BlendState in its proxy.
	class RENDERV2_API Quad : public Drawable
	{
	public:
		Quad();
		~Quad() override;

		bool Init(ID3D11Device* device);

		// Provide the world position so the demo can place the quad next to
		// the cube. Defaults centered on +X axis.
		void SetPosition(const DirectX::XMFLOAT3& pos);

		void Update(float dt) override;
		void Submit(RenderQueue& queue, const FrameInfo& frame) override;

	private:
		struct TransformComp : public Component
		{
			DirectX::XMFLOAT3 position{1.5f, 0.0f, 0.0f};
			DirectX::XMFLOAT3 rotation{0, 0, 0};
			DirectX::XMFLOAT3 scale{1, 1, 1};
			DirectX::XMMATRIX World() const;
		};

		struct PerObjectCB
		{
			DirectX::XMFLOAT4X4 mvp;
		};

		std::shared_ptr<TransformComp> m_transform;
		RenderProxy                    m_proxy;
		PerObjectCB                    m_cbData{};
		bool                           m_ready = false;
	};
}
