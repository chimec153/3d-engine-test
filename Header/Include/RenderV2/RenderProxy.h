#pragma once

#include "GpuResource.h"
#include "DrawCommand.h"
#include <memory>

namespace Engine::RenderV2
{
	// Container that bundles all the GpuResources needed to render an object.
	// Replaces the old pattern where a Drawable's m_ChildList held a mix of
	// renderables, components, and physics bindables — here we only keep
	// rendering data.
	struct RenderProxy
	{
		std::shared_ptr<GpuResource> vs;
		std::shared_ptr<GpuResource> ps;
		std::shared_ptr<GpuResource> inputLayout;
		std::shared_ptr<GpuResource> vertexBuffer;
		std::shared_ptr<GpuResource> indexBuffer;
		std::shared_ptr<GpuResource> perObjectCB;
		std::shared_ptr<GpuResource> materialCB;          // PS b2
		std::shared_ptr<GpuResource> texturePS0;          // diffuse  t0
		std::shared_ptr<GpuResource> texturePS1;          // normal   t1
		std::shared_ptr<GpuResource> texturePS2;          // specular t2
		std::shared_ptr<GpuResource> texturePS3;          // emissive t3
		std::shared_ptr<GpuResource> samplerPS0;
		std::shared_ptr<GpuResource> boneBufferVS;        // VS t30 (skinning)
		std::shared_ptr<GpuResource> blendState;          // nullptr = opaque

		uint32_t    vertexStride = 0;
		IndexFormat indexFormat  = IndexFormat::UInt32;
		Topology    topology     = Topology::TriangleList;
		uint32_t    indexCount   = 0;

		uint16_t vsId = 0;   // assigned by caller for sort key
		uint16_t psId = 0;
		uint8_t  layer = 0;

		// Builds a DrawCommand referencing this proxy's resources. `cbData`
		// is the per-object constants (e.g., MVP matrix).
		DrawCommand BuildCommand(const void* cbData, uint32_t cbSize, uint32_t depth24 = 0) const
		{
			DrawCommand cmd;
			cmd.sortKey       = MakeSortKey(layer, vsId, psId, depth24);
			cmd.vs            = vs.get();
			cmd.ps            = ps.get();
			cmd.inputLayout   = inputLayout.get();
			cmd.vertexBuffer  = vertexBuffer.get();
			cmd.indexBuffer   = indexBuffer.get();
			cmd.perObjectCB   = perObjectCB.get();
			cmd.perObjectData = cbData;
			cmd.perObjectSize = cbSize;
			cmd.materialCB    = materialCB.get();
			cmd.texturePS0    = texturePS0.get();
			cmd.texturePS1    = texturePS1.get();
			cmd.texturePS2    = texturePS2.get();
			cmd.texturePS3    = texturePS3.get();
			cmd.samplerPS0    = samplerPS0.get();
			cmd.boneBufferVS  = boneBufferVS.get();
			cmd.blendState    = blendState.get();
			cmd.vertexStride  = vertexStride;
			cmd.indexFormat   = indexFormat;
			cmd.topology      = topology;
			cmd.indexCount    = indexCount;
			return cmd;
		}
	};
}
