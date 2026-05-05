#include "RenderQueue.h"
#include "RenderContext.h"
#include <algorithm>

namespace Engine::RenderV2
{
	void RenderQueue::Submit(const DrawCommand& cmd)
	{
		m_cmds.push_back(cmd);
	}

	void RenderQueue::Clear()
	{
		m_cmds.clear();
	}

	void RenderQueue::Flush(RenderContext& ctx)
	{
		std::sort(m_cmds.begin(), m_cmds.end(),
		          [](const DrawCommand& a, const DrawCommand& b) { return a.sortKey < b.sortKey; });

		// Track last-bound state — only re-bind on change. This is the core
		// performance win over the old per-drawable Bind/PostBind pattern.
		GpuResource* curVS     = nullptr;
		GpuResource* curPS     = nullptr;
		GpuResource* curLayout = nullptr;
		GpuResource* curVB     = nullptr;
		GpuResource* curIB     = nullptr;
		GpuResource* curTexPS0 = nullptr;
		GpuResource* curTexPS1 = nullptr;
		GpuResource* curTexPS2 = nullptr;
		GpuResource* curTexPS3 = nullptr;
		GpuResource* curSmpPS0 = nullptr;
		GpuResource* curMatCB  = nullptr;
		GpuResource* curBoneVS = nullptr;
		GpuResource* curBlend  = nullptr;
		bool         blendSet  = false;
		Topology     curTopo   = Topology::TriangleList;
		bool         topoSet   = false;

		for (const DrawCommand& cmd : m_cmds)
		{
			if (cmd.vs != curVS)            { ctx.SetVertexShader(cmd.vs);                      curVS     = cmd.vs; }
			if (cmd.ps != curPS)            { ctx.SetPixelShader(cmd.ps);                       curPS     = cmd.ps; }
			if (cmd.inputLayout != curLayout) { ctx.SetInputLayout(cmd.inputLayout);            curLayout = cmd.inputLayout; }
			if (cmd.vertexBuffer != curVB)  { ctx.SetVertexBuffer(cmd.vertexBuffer, cmd.vertexStride, 0); curVB = cmd.vertexBuffer; }
			if (cmd.indexBuffer != curIB)   { ctx.SetIndexBuffer(cmd.indexBuffer, cmd.indexFormat); curIB = cmd.indexBuffer; }
			if (!topoSet || cmd.topology != curTopo) { ctx.SetTopology(cmd.topology); curTopo = cmd.topology; topoSet = true; }
			if (cmd.texturePS0 != curTexPS0) { ctx.SetTexture(ShaderStage::Pixel, 0, cmd.texturePS0); curTexPS0 = cmd.texturePS0; }
			if (cmd.texturePS1 != curTexPS1) { ctx.SetTexture(ShaderStage::Pixel, 1, cmd.texturePS1); curTexPS1 = cmd.texturePS1; }
			if (cmd.texturePS2 != curTexPS2) { ctx.SetTexture(ShaderStage::Pixel, 2, cmd.texturePS2); curTexPS2 = cmd.texturePS2; }
			if (cmd.texturePS3 != curTexPS3) { ctx.SetTexture(ShaderStage::Pixel, 3, cmd.texturePS3); curTexPS3 = cmd.texturePS3; }
			if (cmd.samplerPS0 != curSmpPS0) { ctx.SetSampler(ShaderStage::Pixel, 0, cmd.samplerPS0); curSmpPS0 = cmd.samplerPS0; }
			if (cmd.boneBufferVS != curBoneVS) { ctx.SetStructuredBufferVS(30, cmd.boneBufferVS); curBoneVS = cmd.boneBufferVS; }
			if (!blendSet || cmd.blendState != curBlend) { ctx.SetBlendState(cmd.blendState); curBlend = cmd.blendState; blendSet = true; }

			// Per-object CB is updated every draw — these vary per instance
			// so caching makes no sense; only the buffer object itself stays.
			// Bound to BOTH stages so engine shaders that read transform CB
			// from PS (e.g., for view-space derived calculations) also see it.
			if (cmd.perObjectCB && cmd.perObjectData && cmd.perObjectSize)
			{
				ctx.UpdateConstantBuffer(cmd.perObjectCB, cmd.perObjectData, cmd.perObjectSize);
				ctx.SetConstantBuffer(ShaderStage::Vertex, 0, cmd.perObjectCB);
				ctx.SetConstantBuffer(ShaderStage::Pixel,  0, cmd.perObjectCB);
			}

			// Material CB at b2 (PS) — engine shader convention. Updated only
			// when buffer object changes; data is per-mesh, not per-frame.
			if (cmd.materialCB != curMatCB)
			{
				if (cmd.materialCB && cmd.materialData && cmd.materialSize)
					ctx.UpdateConstantBuffer(cmd.materialCB, cmd.materialData, cmd.materialSize);
				ctx.SetConstantBuffer(ShaderStage::Pixel, 2, cmd.materialCB);
				curMatCB = cmd.materialCB;
			}

			if (cmd.preDraw) cmd.preDraw();

			ctx.DrawIndexed(cmd.indexCount, cmd.startIndex, cmd.baseVertex);
		}
	}
}
