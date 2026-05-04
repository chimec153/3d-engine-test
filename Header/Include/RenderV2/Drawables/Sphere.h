#pragma once

#include "../Drawable.h"
#include "../Component.h"
#include "../RenderProxy.h"
#include <DirectXMath.h>
#include <memory>

struct ID3D11Device;

namespace Engine::RenderV2::Drawables
{
	// Parametric textured sphere. Reuses TexturedMesh.hlsl (pos+UV vertex
	// format) and the geometry generation code from Engine::Sphere's static
	// template methods (CreateSphereVertex / CreateSphereIndex).
	class RENDERV2_API Sphere : public Drawable
	{
	public:
		Sphere(int rings = 16, int sectors = 24);
		~Sphere() override;

		bool Init(ID3D11Device* device);

		void SetPosition(const DirectX::XMFLOAT3& pos);

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

		int                            m_rings;
		int                            m_sectors;
		std::shared_ptr<TransformComp> m_transform;
		RenderProxy                    m_proxy;
		PerObjectCB                    m_cbData{};
		bool                           m_ready = false;
	};
}
