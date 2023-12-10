#include "EMath.h"

namespace Engine
{
	ENGINE_DLL float RadToDeg(float fRad)
	{
		return fRad / PI * 180.f;
	}

	ENGINE_DLL float DegToRad(float fDeg)
	{
		return fDeg / 180.f * PI;
	}
}