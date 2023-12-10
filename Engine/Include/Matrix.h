#pragma once

#include "Vector3.h"
#include "Vector4.h"

namespace Engine
{
	ENGINE_DLL typedef union _tagMatrix
	{
		typedef struct _tagFloat
		{
			float f0;
			float f1;
			float f2;
			float f3;
			float f4;
			float f5;
			float f6;
			float f7;
			float f8;
			float f9;
			float f10;
			float f11;
			float f12;
			float f13;
			float f14;
			float f15;
		}FLOAT, * PFLOAT;
		FLOAT p;
		float f[16];
		float ff[4][4];
		Vector4 v[4];

		_tagMatrix() :
			v()
		{
		}
		_tagMatrix(
			float f0,
			float f1,
			float f2,
			float f3,
			float f4,
			float f5,
			float f6,
			float f7,
			float f8,
			float f9,
			float f10,
			float f11,
			float f12,
			float f13,
			float f14,
			float f15
		) :
			p({ f0,f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13,f14,f15 })
		{
		}

		_tagMatrix(const DirectX::XMMATRIX& mat) :
			v()
		{
			memcpy_s(this, 64, &mat, 64);
		}

		Vector4& operator[](const int index)
		{
			assert(index >= 0 && index < 4);
			return v[index];
		}

		const Vector4& operator[](const int index)	const
		{
			assert(index >= 0 && index < 4);
			return v[index];
		}

		_tagMatrix operator* (const _tagMatrix& mat)	const
		{
			_tagMatrix _mat;

			float x = v[0].x;
			float y = v[0].y;
			float z = v[0].z;
			float w = v[0].w;

			_mat.f[0] = x * mat.ff[0][0] + y * mat.ff[1][0] + z * mat.ff[2][0] + w * mat.ff[3][0];
			_mat.f[1] = x * mat.ff[0][1] + y * mat.ff[1][1] + z * mat.ff[2][1] + w * mat.ff[3][1];
			_mat.f[2] = x * mat.ff[0][2] + y * mat.ff[1][2] + z * mat.ff[2][2] + w * mat.ff[3][2];
			_mat.f[3] = x * mat.ff[0][3] + y * mat.ff[1][3] + z * mat.ff[2][3] + w * mat.ff[3][3];

			x = v[1].x;
			y = v[1].y;
			z = v[1].z;
			w = v[1].w;

			_mat.f[4] = x * mat.ff[0][0] + y * mat.ff[1][0] + z * mat.ff[2][0] + w * mat.ff[3][0];
			_mat.f[5] = x * mat.ff[0][1] + y * mat.ff[1][1] + z * mat.ff[2][1] + w * mat.ff[3][1];
			_mat.f[6] = x * mat.ff[0][2] + y * mat.ff[1][2] + z * mat.ff[2][2] + w * mat.ff[3][2];
			_mat.f[7] = x * mat.ff[0][3] + y * mat.ff[1][3] + z * mat.ff[2][3] + w * mat.ff[3][3];

			x = v[2].x;
			y = v[2].y;
			z = v[2].z;
			w = v[2].w;

			_mat.f[8] = x * mat.ff[0][0] + y * mat.ff[1][0] + z * mat.ff[2][0] + w * mat.ff[3][0];
			_mat.f[9] = x * mat.ff[0][1] + y * mat.ff[1][1] + z * mat.ff[2][1] + w * mat.ff[3][1];
			_mat.f[10] = x * mat.ff[0][2] + y * mat.ff[1][2] + z * mat.ff[2][2] + w * mat.ff[3][2];
			_mat.f[11] = x * mat.ff[0][3] + y * mat.ff[1][3] + z * mat.ff[2][3] + w * mat.ff[3][3];

			x = v[3].x;
			y = v[3].y;
			z = v[3].z;
			w = v[3].w;

			_mat.f[12] = x * mat.ff[0][0] + y * mat.ff[1][0] + z * mat.ff[2][0] + w * mat.ff[3][0];
			_mat.f[13] = x * mat.ff[0][1] + y * mat.ff[1][1] + z * mat.ff[2][1] + w * mat.ff[3][1];
			_mat.f[14] = x * mat.ff[0][2] + y * mat.ff[1][2] + z * mat.ff[2][2] + w * mat.ff[3][2];
			_mat.f[15] = x * mat.ff[0][3] + y * mat.ff[1][3] + z * mat.ff[2][3] + w * mat.ff[3][3];

			return _mat;
		}

		_tagVector4 Transform(const _tagVector4& v)	const
		{
			return
			{
				v.x * ff[0][0] + v.y * ff[1][0] + v.z * ff[2][0] + v.w * ff[3][0],
				v.x * ff[0][1] + v.y * ff[1][1] + v.z * ff[2][1] + v.w * ff[3][1],
				v.x * ff[0][2] + v.y * ff[1][2] + v.z * ff[2][2] + v.w * ff[3][2],
				v.x * ff[0][3] + v.y * ff[1][3] + v.z * ff[2][3] + v.w * ff[3][3],
			};
		}

		_tagMatrix operator*= (const _tagMatrix& mat)
		{
			_tagMatrix _mat =
			{
				v[0].x * mat.ff[0][0] + v[0].y * mat.ff[1][0] + v[0].z * mat.ff[2][0] + v[0].w * mat.ff[3][0],
					v[0].x * mat.ff[0][1] + v[0].y * mat.ff[1][1] + v[0].z * mat.ff[2][1] + v[0].w * mat.ff[3][1],
					v[0].x * mat.ff[0][2] + v[0].y * mat.ff[1][2] + v[0].z * mat.ff[2][2] + v[0].w * mat.ff[3][2],
					v[0].x * mat.ff[0][3] + v[0].y * mat.ff[1][3] + v[0].z * mat.ff[2][3] + v[0].w * mat.ff[3][3],
					v[1].x * mat.ff[0][0] + v[1].y * mat.ff[1][0] + v[1].z * mat.ff[2][0] + v[1].w * mat.ff[3][0],
					v[1].x * mat.ff[0][1] + v[1].y * mat.ff[1][1] + v[1].z * mat.ff[2][1] + v[1].w * mat.ff[3][1],
					v[1].x * mat.ff[0][2] + v[1].y * mat.ff[1][2] + v[1].z * mat.ff[2][2] + v[1].w * mat.ff[3][2],
					v[1].x * mat.ff[0][3] + v[1].y * mat.ff[1][3] + v[1].z * mat.ff[2][3] + v[1].w * mat.ff[3][3],
					v[2].x * mat.ff[0][0] + v[2].y * mat.ff[1][0] + v[2].z * mat.ff[2][0] + v[2].w * mat.ff[3][0],
					v[2].x * mat.ff[0][1] + v[2].y * mat.ff[1][1] + v[2].z * mat.ff[2][1] + v[2].w * mat.ff[3][1],
					v[2].x * mat.ff[0][2] + v[2].y * mat.ff[1][2] + v[2].z * mat.ff[2][2] + v[2].w * mat.ff[3][2],
					v[2].x * mat.ff[0][3] + v[2].y * mat.ff[1][3] + v[2].z * mat.ff[2][3] + v[2].w * mat.ff[3][3],
					v[3].x * mat.ff[0][0] + v[3].y * mat.ff[1][0] + v[3].z * mat.ff[2][0] + v[3].w * mat.ff[3][0],
					v[3].x * mat.ff[0][1] + v[3].y * mat.ff[1][1] + v[3].z * mat.ff[2][1] + v[3].w * mat.ff[3][1],
					v[3].x * mat.ff[0][2] + v[3].y * mat.ff[1][2] + v[3].z * mat.ff[2][2] + v[3].w * mat.ff[3][2],
					v[3].x * mat.ff[0][3] + v[3].y * mat.ff[1][3] + v[3].z * mat.ff[2][3] + v[3].w * mat.ff[3][3],
			};

			return *this = _mat;
		}

		_tagMatrix operator/= (float v)
		{
			f[0] /= v;
			f[1] /= v;
			f[2] /= v;
			f[3] /= v;
			f[4] /= v;
			f[5] /= v;
			f[6] /= v;
			f[7] /= v;
			f[8] /= v;
			f[9] /= v;
			f[10] /= v;
			f[11] /= v;
			f[12] /= v;
			f[13] /= v;
			f[14] /= v;
			f[15] /= v;

			return *this;
		}

		bool operator==(const _tagMatrix& mat)	const
		{
			return v[0] == mat.v[0] && v[1] == mat.v[1] && v[2] == mat.v[2] && v[3] == mat.v[3];
		}

		_tagVector3 TransformCoord(const _tagVector3& v)	const
		{
			return
			{
				v.x * ff[0][0] + v.y * ff[1][0] + v.z * ff[2][0] + ff[3][0],
				v.x * ff[0][1] + v.y * ff[1][1] + v.z * ff[2][1] + ff[3][1],
				v.x * ff[0][2] + v.y * ff[1][2] + v.z * ff[2][2] + ff[3][2]
			};
		}

		_tagVector3 TransformNormal(const _tagVector3& v)	const
		{
			return
			{
				v.x * ff[0][0] + v.y * ff[1][0] + v.z * ff[2][0],
				v.x * ff[0][1] + v.y * ff[1][1] + v.z * ff[2][1],
				v.x * ff[0][2] + v.y * ff[1][2] + v.z * ff[2][2]
			};
		}

		_tagMatrix& Transpose()
		{
			float fTemp = ff[1][0];
			ff[1][0] = ff[0][1];
			ff[0][1] = fTemp;

			fTemp = ff[2][0];
			ff[2][0] = ff[0][2];
			ff[0][2] = fTemp;

			fTemp = ff[3][0];
			ff[3][0] = ff[0][3];
			ff[0][3] = fTemp;

			fTemp = ff[2][1];
			ff[2][1] = ff[1][2];
			ff[1][2] = fTemp;

			fTemp = ff[3][1];
			ff[3][1] = ff[1][3];
			ff[1][3] = fTemp;

			fTemp = ff[3][2];
			ff[3][2] = ff[2][3];
			ff[2][3] = fTemp;

			return *this;
		}

		_tagMatrix Transpose()	const
		{
			return _tagMatrix(
				f[0], f[4], f[8], f[12],
				f[1], f[5], f[9], f[13],
				f[2], f[6], f[10], f[14],
				f[3], f[7], f[11], f[15]);
		}

		float Determinant(int iStart = 0, int iSize = 4)
		{
			if (iSize == 1)
			{
				return f[iStart];
			}

			float fDeter = 0.f;

			for (int i = 0; i < iSize; ++i)
			{
				fDeter += f[iStart + i] * Determinant(iStart + i + 4, iSize - 1) * (((i + 1) % 2) * 2.f - 1.f);
			}

			return fDeter;
		}

		void GetSRT(Vector3& vScale, Vector3& vEuler, Vector3& vPos)
		{
			vPos.x = v[3][0];
			vPos.y = v[3][1];
			vPos.z = v[3][2];

			vScale.x = sqrtf(v[0][0] * v[0][0] + v[0][1] * v[0][1] + v[0][2] * v[0][2]);
			vScale.y = sqrtf(v[1][0] * v[1][0] + v[1][1] * v[1][1] + v[1][2] * v[1][2]);
			vScale.z = sqrtf(v[2][0] * v[2][0] + v[2][1] * v[2][1] + v[2][2] * v[2][2]);

			vEuler = ToEuler(vScale.x, vScale.y, vScale.z);
		}

		Vector3 ToEuler(float x = 1.f, float y = 1.f, float z = 1.f)	const
		{
			return {atan2(v[1][2] / y, v[2][2] / z), asin(-v[0][2] / x), atan2(v[0][1], v[0][0])};
		}

		static _tagMatrix RotationX(float x)
		{
			return
			{
				1.f, 0.f,0.f,0.f,
				0.f,cosf(x),sinf(x),0.f,
				0.f,-sinf(x),cosf(x),0.f,
				0.f,0.f,0.f,1.f
			};
		}

		static _tagMatrix RotationY(float y)
		{
			return
			{
				cosf(y), 0.f,-sinf(y),0.f,
				0.f, 1.f,0.f,0.f,
				sinf(y),0.f,cosf(y),0.f,
				0.f,0.f,0.f,1.f
			};
		}

		static _tagMatrix RotationZ(float z)
		{
			return
			{
				cosf(z), sinf(z),0.f,0.f,
				-sinf(z), cosf(z),0.f,0.f,
				0.f,0.f,1.f,0.f,
				0.f,0.f,0.f,1.f
			};
		}

		static _tagMatrix Rotation(const Vector4& v)
		{
			float apow2 = v.w * v.w;
			float bpow2 = v.x * v.x;
			float cpow2 = v.y * v.y;
			float dpow2 = v.z * v.z;
			float _2ab = 2.f * v.w * v.x;
			float _2ac = 2.f * v.w * v.y;
			float _2ad = 2.f * v.w * v.z;
			float _2bc = 2.f * v.x * v.y;
			float _2bd = 2.f * v.x * v.z;
			float _2cd = 2.f * v.y * v.z;

			return _tagMatrix(
				apow2 + bpow2 - cpow2 - dpow2, _2bc - _2ad, _2bd + _2ac, 0.f,
				_2bc + _2ad, apow2 - bpow2 + cpow2 - dpow2, _2cd - _2ab, 0.f,
				_2bd - _2ac, _2cd + _2ab, apow2 - bpow2 - cpow2 + dpow2, 0.f,
				0.f, 0.f, 0.f, 1.f);
		}

		static _tagMatrix RotationXYZ(const Vector3& v)
		{
			return RotationX(v.x) * RotationY(v.y) * RotationZ(v.z);
		}

		static _tagMatrix RotationZYX(const Vector3& v)
		{
			return RotationZ(v.z) * RotationY(v.y) * RotationX(v.x);
		}

		static _tagMatrix TranslateFromVector(const Vector3& v)
		{
			return
			{
				1.f, 0.f, 0.f, 0.f,
				0.f, 1.f, 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f,
				v.x, v.y, v.z, 1.f
			};
		}

		static _tagMatrix PerspectiveFovLH(float fAngle, float fRatio, float fNear, float fFar)
		{
			float fReciprocal = 1.f / tanf(fAngle / 2.f);

			return
			{
				fReciprocal / fRatio, 0.f, 0.f, 0.f,
				0.f, fReciprocal, 0.f, 0.f,
				0.f, 0.f, -fFar / (fNear - fFar), 1.f,
				0.f, 0.f, fFar * fNear / (fNear - fFar), 0.f
			};
		}

		static _tagMatrix PerspectiveFovLHInfinity(float fAngle, float fRatio, float fNear)
		{
			float fReciprocal = 1.f / tanf(fAngle / 2.f);

			return
			{
				fReciprocal / fRatio, 0.f, 0.f, 0.f,
				0.f, fReciprocal, 0.f, 0.f,
				0.f, 0.f, 1.f, 1.f,
				0.f, 0.f, -fNear, 0.f
			};
		}

		static _tagMatrix PerspectiveLHInfinity(float fWidth, float fHeight, float fNear)
		{
			return
			{
				2.f * fNear / fWidth, 0.f, 0.f, 0.f,
				0.f, 2.f * fNear / fHeight, 0.f, 0.f,
				0.f, 0.f, 1.f, 1.f,
				0.f, 0.f, -fNear, 0.f
			};
		}

		static _tagMatrix OthorGraphicLH(float fLeft, float fRight, float fTop, float fBottom, float fNear, float fFar)
		{
			float fReci = 1.f / (fFar - fNear);
			return
			{
				2.f / (fRight - fLeft), 0.f, 0.f, 0.f,
				0.f, 2.f / (fTop - fBottom), 0.f, 0.f,
				0.f, 0.f, fReci, 0.f,
				-(fRight + fLeft) / (fRight - fLeft), -(fTop + fBottom) / (fTop - fBottom), -fNear * fReci, 1.f
			};
		}

		static _tagMatrix Scaling(float x, float y, float z)
		{
			return Scaling({ x,y,z });
		}

		static _tagMatrix Scaling(const _tagVector3& v)
		{
			return
			{
				v.x, 0.f, 0.f, 0.f,
				0.f, v.y, 0.f, 0.f,
				0.f, 0.f, v.z, 0.f,
				0.f, 0.f, 0.f, 1.f
			};
		}

		static _tagMatrix matIdentity;
	}Matrix, * PMatrix;



	_tagVector3 operator*(const _tagVector3& _v, const Matrix& mat)
	{
		return
		{
			_v.x * mat.ff[0][0] + _v.y * mat.ff[1][0] + _v.z * mat.ff[2][0],
			_v.x * mat.ff[0][1] + _v.y * mat.ff[1][1] + _v.z * mat.ff[2][1],
			_v.x * mat.ff[0][2] + _v.y * mat.ff[1][2] + _v.z * mat.ff[2][2]
		};
	}
}