#include "Vector3.h"

namespace Engine
{
	_tagVector3 _tagVector3::Axis[static_cast<int>(AXIS_TYPE::END)] = { {1.f, 0.f, 0.f},{0.f, 1.f, 0.f},{0.f, 0.f, 1.f} };
}