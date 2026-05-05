#pragma once

#include "RenderContext.h"
#include <cstdint>
#include <functional>

namespace Engine::RenderV2
{
	class GpuResource;

	// Sort key layout (high → low priority for state-change minimization):
	//   bits 56..63 : layer        (8)
	//   bits 40..55 : vertex shader id (16)
	//   bits 24..39 : pixel shader id  (16)
	//   bits  0..23 : depth/material   (24)
	//
	// Drawables sharing VS+PS sort adjacent → minimize SetShader calls.
	using SortKey = uint64_t;

	inline SortKey MakeSortKey(uint8_t layer, uint16_t vsId, uint16_t psId, uint32_t depth24)
	{
		return (SortKey(layer) << 56)
		     | (SortKey(vsId)  << 40)
		     | (SortKey(psId)  << 24)
		     | (SortKey(depth24) & 0xFFFFFFu);
	}

	// One unit of rendering work. Self-contained — references GpuResources by
	// pointer (lifetime managed by owning RenderProxy / resource cache).
	struct DrawCommand
	{
		SortKey       sortKey = 0;

		GpuResource*  vs            = nullptr;
		GpuResource*  ps            = nullptr;
		GpuResource*  inputLayout   = nullptr;
		GpuResource*  vertexBuffer  = nullptr;
		GpuResource*  indexBuffer   = nullptr;
		GpuResource*  perObjectCB   = nullptr;   // optional, slot 0 by convention
		const void*   perObjectData = nullptr;   // mapped each frame
		uint32_t      perObjectSize = 0;

		// PS material CB at slot b2 (engine shader convention). Per-mesh
		// constant; updated once at init by drawables that use lit shaders.
		GpuResource*  materialCB    = nullptr;
		const void*   materialData  = nullptr;
		uint32_t      materialSize  = 0;

		// PS textures t0..t3 (diffuse / normal / specular / emissive — engine
		// convention). Drawables using simple shaders only fill t0.
		GpuResource*  texturePS0    = nullptr;
		GpuResource*  texturePS1    = nullptr;
		GpuResource*  texturePS2    = nullptr;
		GpuResource*  texturePS3    = nullptr;
		GpuResource*  samplerPS0    = nullptr;

		// VS skinning: StructuredBuffer<float4x4> bound at t30 (engine
		// convention — `g_vecBones` in shared.hlsl). nullptr = unskinned.
		GpuResource*  boneBufferVS  = nullptr;

		// nullptr = opaque (default). Set to a BlendStateRes for transparency.
		GpuResource*  blendState    = nullptr;

		uint32_t      vertexStride  = 0;
		IndexFormat   indexFormat   = IndexFormat::UInt32;
		Topology      topology      = Topology::TriangleList;

		uint32_t      indexCount    = 0;
		uint32_t      startIndex    = 0;
		int32_t       baseVertex    = 0;

		// Optional callback executed at flush time, right before this draw's
		// DrawIndexed. Used to bridge V2 to legacy bind paths that have to
		// run per-frame (e.g., engine Animation's compute shader pass).
		std::function<void()> preDraw;
	};
}
