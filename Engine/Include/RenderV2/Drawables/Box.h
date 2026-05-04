#pragma once

#include "../Drawable.h"
#include "../Component.h"
#include "../RenderProxy.h"
#include <DirectXMath.h>
#include <memory>

struct ID3D11Device;

namespace Engine::RenderV2::Drawables
{
	// Textured cube. First formal V2 drawable — replaces the colored
	// pilot (Pilot/BoxV2). Uses a 4x4 procedural checkerboard texture so
	// no file-loading dependency is needed yet.
	class RENDERV2_API Box : public Drawable
	{
	public:
		Box();
		~Box() override;

		// Builds GPU resources (shaders, mesh, texture, sampler, CB). Resolves
		// the shared TexturedMesh.hlsl path internally via the engine's
		// PathManager — caller only provides the device.
		bool Init(ID3D11Device* device);

		void Update(float dt) override;
		void Submit(RenderQueue& queue, const FrameInfo& frame) override;

	private:
		struct TransformComp : public Component
		{
			DirectX::XMFLOAT3 position{0, 0, 0};
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
