#pragma once

// Self-contained export macro. We deliberately do NOT include the engine's
// Core/Macro.h here because it carries `#pragma comment(lib, ...)` directives
// that drag zlib/libxml2/fbx libs into any consumer of this header.
#ifdef EXPORT_ENGINE
#define RENDERV2_API __declspec(dllexport)
#else
#define RENDERV2_API __declspec(dllimport)
#endif

namespace Engine::RenderV2
{
	// One-shot validation: builds a BoxV2, submits + flushes immediately.
	// Returns true if the full pipeline ran without errors. Useful for
	// confirming scaffolding compiles and links; visual output unreliable
	// because the cube is overwritten by the next normal frame.
	RENDERV2_API bool RunBoxDemo();

	// Per-frame demo: lazy-initializes a persistent BoxV2 on first call,
	// then each frame rotates it slightly and submits a DrawCommand to
	// RenderManager's RenderV2 queue. The queue is flushed automatically
	// at end of RenderManager::Render(), drawing to the engine back buffer.
	// Call once per frame (e.g., from your scene's Update or Render).
	RENDERV2_API bool SubmitBoxThisFrame(float deltaTime);

	// Tear down any persistent state owned by the demo (e.g. the static
	// Box held by SubmitBoxThisFrame). Must run before Graphics is
	// destroyed; RenderManager's destructor calls this automatically.
	RENDERV2_API void ShutdownDemo();
}
