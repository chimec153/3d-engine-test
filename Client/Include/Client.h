#pragma once

#include <cmath>

#include "Core/Window.h"

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "Engine_Debug.lib")
		#pragma comment(lib, "Game_Debug.lib")
	#else
		#pragma comment(lib, "Engine.lib")
		#pragma comment(lib, "Game.lib")
	#endif
#elif
	#ifdef _DEBUG
		#pragma comment(lib, "Engine32_Debug.lib")
		#pragma comment(lib, "Game32_Debug.lib")
	#else
		#pragma comment(lib, "Engine32.lib")
		#pragma comment(lib, "Game32.lib")
	#endif
#endif