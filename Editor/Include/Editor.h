#pragma once
#define _CRTDBG_MAP_ALLOC
#include "Core/Window.h"
#include "Imgui/imgui.h"
#include "Types.h"

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "Engine_Debug.lib")
		#pragma comment(lib, "DebugUtils-d")
		#pragma comment(lib, "Recast-d")
	#else
		#pragma comment(lib, "Engine.lib")
		#pragma comment(lib, "DebugUtils")
		#pragma comment(lib, "Recast")
	#endif
#else
	#ifdef _DEBUG
		#pragma comment(lib, "Engine32_Debug.lib")
		#pragma comment(lib, "DebugUtils-d")
		#pragma comment(lib, "Recast-d")
	#else
		#pragma comment(lib, "Engine32.lib")
		#pragma comment(lib, "DebugUtils")
		#pragma comment(lib, "Recast")
	#endif
#endif