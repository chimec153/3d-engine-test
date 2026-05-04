#pragma once

namespace Engine::RenderV2
{
	// Base for game-side components. Distinct from GpuResource: this is CPU
	// logic (transform, collider, animation controller, AI agent) that does
	// not bind GPU state. Old Bindable conflated both concerns; here they are
	// split so a Component can be tested without a graphics device.
	class Component
	{
	public:
		virtual ~Component() = default;

		virtual void Update(float /*dt*/) {}
		virtual void FixedUpdate(float /*dt*/) {}
	};
}
