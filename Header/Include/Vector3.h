#pragma once

#include "Flag.h"

namespace Engine
{
	ENGINE_DLL typedef struct _tagVector3
	{
		float x, y, z;

		_tagVector3() :
			x(0.f)
			, y(0.f)
			, z(0.f)
		{
		}

		_tagVector3(float x, float y, float z) :
			x(x)
			, y(y)
			, z(z)
		{
		}

		_tagVector3(const float v[3]) :
			x(v[0])
			, y(v[1])
			, z(v[2])
		{
		}

		float& operator[](int index)
		{
			assert(index >= 0 && index < 3);
			return *(&x + index);
		}

		_tagVector3& operator =(float f)
		{
			x = f;
			y = f;
			z = f;

			return *this;
		}

		bool operator==(const _tagVector3& v)	const
		{
			return x == v.x && y == v.y && z == v.z;
		}

		bool operator!=(const _tagVector3& v)	const
		{
			return x != v.x || y != v.y || z != v.z;
		}

		bool operator==(const float f)	const
		{
			return x == f && y == f && z == f;
		}

		_tagVector3 operator-()	const
		{
			return { -x,-y,-z };
		}

		_tagVector3 operator+(const _tagVector3& v)	const
		{
			return { x + v.x, y + v.y, z + v.z };
		}

		_tagVector3 operator-(const _tagVector3& v)	const
		{
			return { x - v.x, y - v.y, z - v.z };
		}

		_tagVector3 operator* (float f) const
		{
			return { x * f, y * f, z * f };
		}

		_tagVector3 operator* (const _tagVector3& v) const
		{
			return { x * v.x, y * v.y, z * v.z };
		}

		_tagVector3& operator+=(const _tagVector3& v)
		{
			x += v.x;
			y += v.y;
			z += v.z;

			return *this;
		}

		_tagVector3& operator-=(const _tagVector3& v)
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;

			return *this;
		}

		_tagVector3& operator*=(const _tagVector3& v)
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;

			return *this;
		}

		_tagVector3& operator/=(const _tagVector3& v)
		{
			x /= v.x;
			y /= v.y;
			z /= v.z;

			return *this;
		}

		_tagVector3 operator / (float f)	const
		{
			return { x / f, y / f, z / f };
		}

		_tagVector3& operator /= (float f)
		{
			assert(f != 0.f);

			x /= f;
			y /= f;
			z /= f;

			return *this;
		}

		float Dot(const _tagVector3& v) const
		{
			return x * v.x + y * v.y + z * v.z;
		}
		// x	y	z
		// v.x	v.y	v.z
		_tagVector3 Cross(const _tagVector3& v)	const
		{
			return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
		}

		_tagVector3 Cross(const float _x, const float _y, const float _z)	const
		{
			return { y * _z - z * _y, z * _x - x * _z, x * _y - y * _x };
		}

		float Length()	const
		{
			return sqrtf(x * x + y * y + z * z);
		}

		_tagVector3& Normalize()
		{
			float fLength = Length();

			assert(fLength);

			x /= fLength;
			y /= fLength;
			z /= fLength;

			return *this;
		}

		bool Close(const _tagVector3& v)	const
		{
			return abs(v.x) - abs(x) <= epsilon &&
				abs(v.y) - abs(y) <= epsilon &&
				abs(v.z) - abs(z) <= epsilon;
		}

		static _tagVector3 Axis[static_cast<int>(AXIS_TYPE::END)];

	}Vector3, * PVector3;

	Vector3 operator*(float f, const Vector3& v)
	{
		return { f * v.x ,f * v.y, f * v.z };
	}

	Vector3 operator/(float f, const Vector3& v)
	{
		return _tagVector3{ f / v.x, f / v.y, f / v.z };
	}
}