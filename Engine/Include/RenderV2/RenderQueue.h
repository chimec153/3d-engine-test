#pragma once

#include "DrawCommand.h"
#include <vector>
#include <cstddef>

namespace Engine::RenderV2
{
	class RenderContext;

	// Sort-by-state queue. Drawables submit DrawCommands; Flush() sorts them
	// by SortKey then dispatches, tracking last-bound resources to skip
	// redundant state changes. Replaces the per-drawable Bind/PostBind dance
	// in the old engine.
	class RenderQueue
	{
	public:
		void Submit(const DrawCommand& cmd);
		void Clear();
		std::size_t Size() const { return m_cmds.size(); }

		// Sorts m_cmds in place, then issues calls on `ctx`. Caller is
		// responsible for setting render targets / viewports / etc. before
		// calling Flush.
		void Flush(RenderContext& ctx);

	private:
		std::vector<DrawCommand> m_cmds;
	};
}
