#pragma once

#include <DirectXMath.h>

// RenderV2 export macro. Self-contained (does not include Core/Macro.h
// because that drags zlib/fbx/libxml2 auto-links into external consumers).
#ifndef RENDERV2_API
#ifdef EXPORT_ENGINE
#define RENDERV2_API __declspec(dllexport)
#else
#define RENDERV2_API __declspec(dllimport)
#endif
#endif

namespace Engine::RenderV2
{
	class RenderQueue;

	// Per-frame camera info handed to every Drawable::Submit. Expanded from
	// just viewProj because lit shaders need view and proj separately
	// (matWorldView, etc.). Future fields (camera world pos, time, etc.)
	// can be added without breaking signatures.
	struct FrameInfo
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX viewProj;
	};

	// Minimal renderable interface for the V2 path. A Drawable owns whatever
	// CPU-side state it needs (Components, RenderProxy) and emits one or more
	// DrawCommands when asked. Replaces the old Bindable + Drawable hierarchy
	// where the same base class was reused for GPU resources, components, and
	// renderable actors.
	class Drawable
	{
	public:
		virtual ~Drawable() = default;

		virtual void Update(float /*dt*/) {}

		virtual void Submit(RenderQueue& queue, const FrameInfo& frame) = 0;
	};
}
