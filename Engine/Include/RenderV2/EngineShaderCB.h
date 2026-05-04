#pragma once

#include <DirectXMath.h>
#include <cstdint>

// C++ mirrors of the HLSL cbuffer layouts in shared.hlsl. These are required
// when V2 calls into engine-side shaders (NormalShader / TextureShader /
// anisotropic_microfacet) — those shaders read these exact CB layouts via
// `#include "shared.hlsl"`. Field order, types, and padding must stay in
// lockstep with shared.hlsl. Matrix members are stored row-major in CPU
// memory and must be transposed before upload (HLSL packs column-major
// by default).

namespace Engine::RenderV2
{
	// shared.hlsl: cbuffer transform : register(b0)
	struct EngineTransformCB
	{
		DirectX::XMFLOAT4X4 matTransform;          // = world * view * proj
		DirectX::XMFLOAT4X4 matWorldView;          // = world * view
		DirectX::XMFLOAT4X4 matLightWVP;           // unused → identity
		DirectX::XMFLOAT4X4 matJoint;              // unused (skinning) → identity
		DirectX::XMFLOAT4X4 matWorld;
		DirectX::XMFLOAT4X4 matView;
		DirectX::XMFLOAT4X4 matProj;
		int32_t             iTransformJointSocket; // unused → 0
		int32_t             pad[3];                // 16-byte align
	};
	static_assert(sizeof(EngineTransformCB) == 7 * 64 + 16, "TransformCB layout");

	// shared.hlsl: cbuffer light : register(b1)
	struct EngineLightCB
	{
		DirectX::XMFLOAT3 vLightPos;
		float             fConstAttenuation;
		DirectX::XMFLOAT4 vLightColor;
		DirectX::XMFLOAT4 vLightAmbientColor;
		DirectX::XMFLOAT3 vLightDir;
		float             fLinearAttenuation;
		float             fQuadraticAttenuation;
		int32_t           iLightType;              // 0=POINT, 1=SPOT, 2=DIRECTIONAL
		float             fLightIntensity;
		int32_t           pad;
	};
	static_assert(sizeof(EngineLightCB) == 80, "LightCB layout");

	// shared.hlsl: cbuffer material : register(b2)
	struct EngineMaterialCB
	{
		DirectX::XMFLOAT4 vDiffuseColor;
		DirectX::XMFLOAT4 vAmbientColor;
		DirectX::XMFLOAT4 vSpecularColor;
		DirectX::XMFLOAT4 vEmissiveColor;
		float             fMaterialSpecPower;
		float             fMaterialFraction;
		DirectX::XMFLOAT2 vMaterialRoughness;
		int32_t           bMaterialUsePaperBurn;   // HLSL bool == int32 in CB
		int32_t           pad[3];
	};
	static_assert(sizeof(EngineMaterialCB) == 96, "MaterialCB layout");
}
