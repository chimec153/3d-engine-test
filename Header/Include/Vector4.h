#pragma once

#include "Vector3.h"

namespace Engine
{
	ENGINE_DLL typedef struct _tagVector4
	{
		float x, y, z, w;

		_tagVector4() :
			x(0.f)
			, y(0.f)
			, z(0.f)
			, w(0.f)
		{
		}

		_tagVector4(float x, float y, float z, float w) :
			x(x)
			, y(y)
			, z(z)
			, w(w)
		{
		}

		_tagVector4(const Vector3& v, float f) :
			x(v.x)
			, y(v.y)
			, z(v.z)
			, w(f)
		{
		}

		operator _tagVector3()	const
		{
			return { x,y,z };
		}

		float DotPoint(const _tagVector3& v)	const
		{
			return x * v.x + y * v.y + z * v.z + w;
		}

		float DotVector(const _tagVector3& v)	const
		{
			return x * v.x + y * v.y + z * v.z;
		}

		float Dot(const _tagVector4& v)	const
		{
			return x * v.x + y * v.y + z * v.z + w * v.w;
		}

		_tagVector4& operator=(const _tagVector3& v)
		{
			x = v.x;
			y = v.y;
			z = v.z;

			return *this;
		}

		bool operator==(const _tagVector4& v)	const
		{
			return x == v.x && y == v.y && z == v.z && w == v.w;
		}

		float& operator[](int index)
		{
			assert(index >= 0 && index < 4);
			return *((&x) + index);
		}

		const float operator[](int index)	const
		{
			assert(index >= 0 && index < 4);
			return *((&x) + index);
		}

		_tagVector4 operator*(float f)	const
		{
			return _tagVector4(x * f, y * f, z * f, w * f);
		}
		_tagVector4& operator-=(float f)
		{
			x -= f;
			y -= f;
			z -= f;
			w -= f;

			return *this;
		}
		_tagVector4& operator-=(const _tagVector4& _v)
		{
			x -= _v.x;
			y -= _v.y;
			z -= _v.z;
			w -= _v.w;

			return *this;
		}
		_tagVector4& operator/=(float f)
		{
			x /= f;
			y /= f;
			z /= f;
			w /= f;

			return *this;
		}

		static _tagVector4 ToQuaternion(float x, float y, float z) // roll (x), pitch (Y), yaw (z)
		{
			// Abbreviations for the various angular functions

			float cr = cosf(x * 0.5f);
			float sr = sinf(x * 0.5f);
			float cp = cosf(y * 0.5f);
			float sp = sinf(y * 0.5f);
			float cy = cosf(z * 0.5f);
			float sy = sinf(z * 0.5f);

			Vector4 q;
			q.w = cr * cp * cy + sr * sp * sy;
			q.x = sr * cp * cy - cr * sp * sy;
			q.y = cr * sp * cy + sr * cp * sy;
			q.z = cr * cp * sy - sr * sp * cy;

			return q;
		}

		Vector3 ToEuler()
		{
			float fsinr_cosp = 2.f * (w * x + y * z);
			float fcosr_cosp = 1.f - 2.f * (x * x + y * y);

			float fsinp = sqrtf(1.f + 2.f * (w * y - x * z));
			float fcosp = sqrtf(1.f - 2.f * (w * y - x * z));

			float fsiny_cosp = 2.f * (w * z + x* y);
			float fcosy_cosp = 1.f - 2.f * (y*y+z*z);

			return Vector3(atan2(fcosr_cosp, fsinr_cosp), 2.f * atan2(fsinp, fcosp) - PI / 2.f, atan2(fcosy_cosp, fsiny_cosp));
		}
	}Vector4, * PVector4;

	Vector4 operator*(float f, const Vector4& v)
	{
		return { f * v.x, f * v.y, f * v.z, f * v.w };
	}

	static Vector4 White = Vector4(1.f, 1.f, 1.f, 1.f);
	static Vector4 Cyan = Vector4(0.f, 1.f, 1.f, 1.f);
	static Vector4 Magenta = Vector4(1.f, 0.f, 1.f, 1.f);
	static Vector4 Yellow = Vector4(1.f, 1.f, 0.f, 1.f);
	static Vector4 Red = Vector4(1.f, 0.f, 0.f, 1.f);
	static Vector4 Green = Vector4(0.f, 1.f, 0.f, 1.f);
	static Vector4 Blue = Vector4(0.f, 0.f, 1.f, 1.f);
	static Vector4 Black = Vector4(0.f, 0.f, 0.f, 1.f);
}