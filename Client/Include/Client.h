#pragma once

#define ENGINE_DLL __declspec(dllimport)

#include <cmath>

#include "Core/Window.h"

#define	INVENTORY_WIDTH		8
#define INVENTORY_HEIGHT	4

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "Engine_Debug.lib")
	#else
		#pragma comment(lib, "Engine.lib")
	#endif
#elif
	#ifdef _DEBUG
		#pragma comment(lib, "Engine32_Debug.lib")
	#else
		#pragma comment(lib, "Engine32.lib")
	#endif
#endif
