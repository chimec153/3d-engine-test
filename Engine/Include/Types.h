#pragma once

#include "Math.h"
#include "Matrix.h"
#include "Vector2.h"

namespace Engine
{
	ENGINE_DLL typedef struct _tagVertex
	{
		Vector3 pos;
	}VERTEX, * PVERTEX;

	ENGINE_DLL typedef struct _tagVertexBasic
	{
		Vector3 pos;
		Vector3 normal;
	}VERTEXBASIC, * PVERTEXBASIC;

	ENGINE_DLL typedef struct _tagVertexNormal
	{
		Vector3 pos;
		Vector3 normal;
		Vector3 tangent;

		_tagVertexNormal() :
			pos(0.f, 0.f, 0.f)
			, normal(0.f, 0.f, 0.f)
			, tangent(0.f, 0.f, 0.f)
		{
		}

		_tagVertexNormal(float x, float y, float z, float nx, float ny, float nz) :
			pos(x, y, z)
			, normal(nx, ny, nz)
			, tangent(0.f, 0.f, 0.f)
		{
		}
	}VertexNormal, * PVertexNormal;

	ENGINE_DLL typedef struct _tagVertexColor
	{
		float r, g, b, a;
		Vector3 pos;
		Vector3 normal;
		Vector3 tangent;
		_tagVertexColor() :
			r(0.f)
			, g(0.f)
			, b(0.f)
			, a(0.f)
			, pos(0.f, 0.f, 0.f)
			, normal(0.f, 0.f, 0.f)
			, tangent(0.f, 0.f, 0.f)
		{
		}
	}VertexColor, * PVertexColor;

	ENGINE_DLL typedef struct _tagVertexTexture
	{
		Vector4 tangent;
		Vector3 pos;
		Vector3 normal;
		Vector3 blendWeight;
		DirectX::XMFLOAT2 uv;
		char blendIndecies[4];
		_tagVertexTexture() :
			tangent(0.f, 0.f, 0.f, 0.f)
			, pos(0.f, 0.f, 0.f)
			, normal(0.f, 0.f, 0.f)
			, blendWeight()
			, uv(0.f, 0.f)
			, blendIndecies()
		{
		}
		_tagVertexTexture(float tx, float ty, float tz, float tw, float px, float py, float pz, float nx, float ny, float nz, float ux, float uy) :
			tangent(tx, ty, tz, tw)
			, pos(px, py, pz)
			, normal(nx, ny, nz)
			, blendWeight()
			, uv(ux, uy)
			, blendIndecies()
		{
		}
	}VertexTexture, * PVertexTexture;

	ENGINE_DLL typedef struct _tagVertexStandard
	{
		Vector4 tangent;
		Vector4 blendIndecies;
		Vector3 pos;
		Vector3 normal;
		Vector3 blendWeight;
		DirectX::XMFLOAT2 uv;
		_tagVertexStandard() :
			tangent(0.f, 0.f, 0.f, 0.f)
			, blendIndecies()
			, pos(0.f, 0.f, 0.f)
			, normal(0.f, 0.f, 0.f)
			, blendWeight()
			, uv(0.f, 0.f)
		{
		}
		_tagVertexStandard(float tx, float ty, float tz, float tw, float px, float py, float pz, float nx, float ny, float nz, float ux, float uy) :
			tangent(tx, ty, tz, tw)
			, blendIndecies()
			, pos(px, py, pz)
			, normal(nx, ny, nz)
			, blendWeight()
			, uv(ux, uy)
		{
		}
	}VertexStandard, * PVertexStandard;

	ENGINE_DLL typedef struct _tagColor
	{
		DirectX::XMVECTOR color[6];
	}COLOR, * PCOLOR;

	ENGINE_DLL typedef struct alignas(16) _tagPointLight
	{
		Vector3 pos;
		float fConstantAttenuation;
		Vector4 color;
		Vector4 ambientColor;
		Vector3 dir;
		float fLinearAttenuation;
		float fQuadraticAttenuation;
		LIGHT_TYPE eLightType;
		float fIntensity;
	}POINTLIGHT, * PPOINTLIGHT;


	ENGINE_DLL typedef struct alignas(16) _tagTransformBuffer
	{
		Matrix matWorldViewProject;
		Matrix matWorldView;
		Matrix matLightWVP;
		Matrix matJoint;
		Matrix matWorld;
		Matrix matView;
		Matrix matProj;
		int	iJointSocket;

		_tagTransformBuffer() :
			matWorldViewProject()
			, matWorldView()
			, matLightWVP()
			, matJoint()
			, matWorld()
			, matView()
			, matProj()
			, iJointSocket(-1)
		{
		}
	}TRANSFORMBUFFER, * PTRANSFORMBUFFER;

	ENGINE_DLL typedef struct alignas(16) _tagMaterial
	{
		Vector4 diffuseColor;
		Vector4 ambientColor;
		Vector4 specularColor;
		Vector4 emissiveColor;
		float fSpecPower;
		float fFraction;
		DirectX::XMFLOAT2 vRoughness;
		int iContainerIndex;
	}MATERIAL, * PMATERIAL;

	ENGINE_DLL typedef struct _tagPerspectiveBuffer
	{
		Vector4 vPerspective;
		Matrix matCameraViewToLightClip;
	}PERSPECTIVEBUFFER, * PPERSPECTIVEBUFFER;

	ENGINE_DLL typedef struct _tagLineColliderInfo
	{
		Vector3 vStart;
		Vector3 vDir;
	}LINECOLLIDERINFO;

	ENGINE_DLL typedef struct _tagSphereColliderInfo
	{
		Vector3 vCenter;
		float fRadius;
	}SPHERECOLLIDERINFO, * PSPHERECOLLIDERINFO;

	ENGINE_DLL typedef struct _tagMeshColliderInfo
	{
		std::vector<float> vecPoint;
		std::vector<int> vecIndex;
		_tagMeshColliderInfo(const std::vector<float>& vecPoint, const std::vector<int>& vecIndex) :
			vecPoint(vecPoint)
			, vecIndex(vecIndex)
		{
		}

		_tagMeshColliderInfo()
		{
		}
	}MESHCOLLIDERINFO, *PMESHCOLLIDERINFO;

	ENGINE_DLL typedef struct _tagOBBInfo
	{
		Vector3 vAxis[3];
		Vector3 vCenter;
	}OBBINFO, * POBBINFO;

	ENGINE_DLL typedef struct _tagElipsoidInfo
	{
		Vector3 vRST[3];
		Vector3 vCenter;
	}ELIPSOIDINFO, * PELIPSOIDINFO;

	ENGINE_DLL typedef struct _tagCylinderInfo
	{
		float fRadius;
		Vector3 vStart;
		Vector3 vEnd;
	}CYLINDERINFO, * PCYLINDERINFO;

	ENGINE_DLL typedef struct alignas(16) _tagBoneCBuffer
	{
		float fTime;
		float fMaxTime;
		int iMaxFrame;
		int iMaxJoint;
		Vector3 vRootPos;
		int iFrame;
		int iNextFrame;
		int iInfoCount;

		_tagBoneCBuffer()	:
			fTime(0.f)
			, fMaxTime(0.f)
			, iMaxFrame(0)
			, iMaxJoint(0)
			, vRootPos()
			, iFrame(0)
			, iNextFrame(0)
			, iInfoCount(0)
		{
		}
	}BONECBUFFER, * PBONECBUFFER;

	ENGINE_DLL typedef struct alignas(16) _tagTerrainCBuffer
	{
		int m_iWidth;
		int m_iHeight;
		int m_iBlendCount;
	}TERRAINCBUFFER, * PTERRAINCBUFFER;

	ENGINE_DLL typedef struct _tagBone
	{
		std::string strName;
		int iParent;
		Matrix matInvBindPose;
		Matrix matBone;
		_tagBone() :
			strName()
			, iParent(-1)
		{

		}
	}BONE, * PBONE;

	ENGINE_DLL typedef struct _tagJoint
	{
		std::string strJoint;
		int iParentIndex;
		double dTime;
		Vector3 vPos;
		Vector4 vQueternion;
		Vector3 vScale;
	}JOINT, * PJOINT;

	ENGINE_DLL typedef struct _tagPose
	{
		std::vector<JOINT> vecJoint;
	}POSE, * PPOSE;

	ENGINE_DLL typedef struct _tagTransform
	{
		Vector3 vPos;
		Vector4 vQueternion;
		Vector3 vScale;
		_tagTransform(const Vector3& vPos, const Vector4& vQueternion, const Vector3& vScale) :
			vPos(vPos)
			, vQueternion(vQueternion)
			, vScale(vScale)
		{
		}
		_tagTransform() :
			vPos()
			, vQueternion(0.f, 0.f, 0.f, 1.f)
			, vScale(1.f, 1.f, 1.f)
		{
		}
	}TRANSFORM, *PTRANSFORM;

	ENGINE_DLL typedef struct _tagBoneInstData
	{
		float fTime;
		int iMaxFrame;
		int iFrame;
		int iNextFrame;
		int iAnimationID;
		int iRootPos;

		_tagBoneInstData() :
			fTime(0.f)
			, iFrame(0)
			, iMaxFrame(0)
			, iNextFrame(0)
			, iAnimationID(0)
			, iRootPos(0)
		{
		}
	}BONEINSTDATA, *PBONEINSTDATA;

	ENGINE_DLL typedef struct _tagIKInfo
	{
		int iJointIndex;
		Vector3 vPosition;
		int iRootIndex;
		_tagIKInfo() :
			iJointIndex(-1)
			, vPosition()
			, iRootIndex(-1)
		{
		}
	}IKINFO, *PIKINFO;

	ENGINE_DLL typedef struct alignas(16) _tagIKCBuffer
	{
		_tagIKInfo tInfo[256];
	}IKCBUFFER, *PIKCBUFFER;

	typedef ENGINE_DLL struct alignas(16) _tagParticleCBuffer
	{
		Vector4 vStartColor;
		Vector4 vEndColor;
		Vector3 vVelocity;
		float   fMaxLifeTime;
		Vector3 vAccelation;
		int     iMaxParticleCount;
		Vector3 vMinimumPosition;
		int		iCreateCount;
		Vector2 vStartSize;
		Vector2 vEndSize;
		Vector3 vMaximumPosition;
		int		iMaxFrame;
		int		iFrameWidth;
		int		iFrameHeight;

		_tagParticleCBuffer(int iCount) :
			vStartColor()
			, vEndColor()
			, vVelocity()
			, fMaxLifeTime()
			, vAccelation()
			, iMaxParticleCount(iCount)
			, vMinimumPosition()
			, iCreateCount()
			, vStartSize()
			, vEndSize()
			, vMaximumPosition()
			, iMaxFrame(1)
			, iFrameWidth(1)
			, iFrameHeight(1)
		{
		}
	}PARTICLECBUFFER, * PPARTICLECBUFFER;

	ENGINE_DLL typedef struct _tagParticle
	{
		Vector3 pos;
		bool alive;
		Vector4 color;
		Vector3 speed;
		float age;
		Vector2 size;
		float maxage;
		int frame;
	}PARTICLE, *PPARTICLE;

	ENGINE_DLL typedef struct alignas(16) _tagGlobalCBuffer
	{
		float	fAccTime;
		float	fDeltaTime;
		int		iNoiseTextureWidth;
		int		iNoiseTextureHeight;
	}GLOBALCBUFFER, *PGLOBALCBUFFER;

	ENGINE_DLL typedef struct alignas(16) _tagDecalCBuffer
	{
		Matrix matInvWorldView;
		float fFadeTime;
		float fMaxFadeTime;
		float fFadeStartTime;

		_tagDecalCBuffer() :
			fFadeTime(0.f)
			, fMaxFadeTime(1.f)
			, fFadeStartTime(0.f)
		{
		}
	}DECALCBUFFER, *PDECALCBUFFER;

	typedef struct ENGINE_DLL alignas(16) _tagPaperBurnCBuffer
	{
		Vector4 vStartColor;
		Vector4 vMidColor;
		Vector4 vFinalColor;
		float fStartRate;
		float fMidRate;
		float fFinalRate;
		float fEndRate;
		float fTime;
		float fMaxTime;

		_tagPaperBurnCBuffer() :
			vStartColor()
			, vMidColor()
			, vFinalColor()
			, fStartRate()
			, fMidRate()
			, fFinalRate()
			, fEndRate()
			, fTime()
			, fMaxTime()
		{
		}
	}PAPERBURNCBUFFER, *PPAPERBURNCBUFFER;

	typedef struct ENGINE_DLL alignas(16)_tagFluidCBuffer
	{
		float c1;
		float c2;
		float c3;
		int iWidth;
		float dist;
	}FLUIDCBUFFER, *PFLUIDCBUFFER;
}