#include "Matrix.h"

namespace Engine
{
	Matrix Matrix::matIdentity =
	{
		1.f, 0.f , 0.f , 0.f ,
		0.f, 1.f, 0.f , 0.f ,
		0.f , 0.f, 1.f, 0.f ,
		0.f , 0.f , 0.f, 1.f
	};
}