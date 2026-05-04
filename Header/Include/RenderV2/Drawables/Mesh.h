#pragma once

#include "../Drawable.h"
#include "../Component.h"
#include "../RenderProxy.h"
#include "../EngineShaderCB.h"
#include <DirectXMath.h>
#include <memory>
#include <vector>

struct ID3D11Device;

namespace Engine::RenderV2::Drawables
{
	// FBX-loaded static mesh, lit + normal-mapped via the engine's
	// NormalShader.hlsl (no skinning yet). Phase 2.4c: single submesh,
	// diffuse + normal + specular textures, real Phong lighting.
	// Uses VertexStandard layout end-to-end so we don't repack vertices.
	class RENDERV2_API Mesh : public Drawable
	{
	public:
		Mesh();
		~Mesh() override;

		// Loads FBX from MESH_PATH/`fbxFile`. Diffuse / normal / specular
		// textures are auto-discovered from the FBX's TEXTUREINFO list by
		// type; missing types fall back to small procedural defaults so the
		// shader's t0..t3 are never reading from null SRVs.
		bool Init(ID3D11Device* device, const wchar_t* fbxFile);

		void SetPosition(const DirectX::XMFLOAT3& pos);
		void SetScale(float uniform);

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

		struct BoneAnim
		{
			std::vector<float>            times;        // sorted, seconds
			std::vector<DirectX::XMMATRIX> matrices;     // mesh-space pose at each time
		};

		std::shared_ptr<TransformComp>     m_transform;
		RenderProxy                        m_proxy;
		EngineTransformCB                  m_cbData{};
		std::vector<DirectX::XMFLOAT4X4>   m_boneMatrices;       // uploaded each frame
		std::vector<DirectX::XMMATRIX>     m_invBindPose;        // per bone
		std::vector<BoneAnim>              m_anim;               // per bone, sequence 0
		int                                m_boneCount = 0;
		float                              m_animTime  = 0.0f;
		float                              m_animMaxTime = 0.0f;
		bool                               m_ready = false;
	};
}
