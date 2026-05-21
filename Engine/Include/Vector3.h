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

		const float operator[](int index)	const
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
			float fLengthSq = x * x + y * y + z * z;

			if (fLengthSq > 1e-6f)
			{
				long i;
				float x2, y_val;
				const float threehalfs = 1.5F;

				x2 = fLengthSq * 0.5F;
				y_val = fLengthSq;
				i = *(long*)&y_val;           // float bits as integer
				i = 0x5f3759df - (i >> 1);    // Quake III fast inverse sqrt magic
				y_val = *(float*)&i;          // back to float
				y_val = y_val * (threehalfs - (x2 * y_val * y_val));   // Newton-Raphson

				x *= y_val;
				y *= y_val;
				z *= y_val;
			}
			else
			{
				x = 0.0f; y = 0.0f; z = 0.0f;
			}

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