#pragma once

#include "EMath.h"
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
		// Spot-only cone falloff exponent. Used in shared.hlsl::SPOT branch as
		// pow(dot(lightDir, surface→light), fSpotConeExponent). Higher value
		// → narrower / sharper cone. fIntensity now stays a pure brightness
		// multiplier for every light type (matches POINT/DIRECTIONAL semantics).
		float fSpotConeExponent;
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

	// UE 모델 매핑: ShadingModelID(GBuffer 패킹)·MID 스칼라(HitFlash).
	// 80바이트 이후 필드는 런타임 전용 — Material::Save/Load는 80바이트
	// 까지만 직렬화하므로 .mesh 파일 호환 유지.
	enum ENGINE_SHADING_MODEL : int
	{
		SHADING_MODEL_DEFAULT_LIT = 0,
		SHADING_MODEL_TOON        = 1,
		SHADING_MODEL_UNLIT       = 2,
	};

	ENGINE_DLL typedef struct alignas(16) _tagMaterial
	{
		Vector4 diffuseColor;       // offset   0
		Vector4 ambientColor;       // offset  16
		Vector4 specularColor;      // offset  32
		Vector4 emissiveColor;      // offset  48
		float fSpecPower;           // offset  64
		float fFraction;            // offset  68
		DirectX::XMFLOAT2 vRoughness; // offset 72 (8B)
		int bUsePaperBurn;          // offset  80 (HLSL bool == 4B)
		int iShadingModel;          // offset  84
		int _padShading[2];         // offset  88 (8B pad → next float4 aligned to 96)
		Vector4 vHitFlash;          // offset  96 (xyz=color, w=intensity 0..1)

		_tagMaterial() :
			diffuseColor()
			, ambientColor()
			, specularColor()
			, emissiveColor()
			, fSpecPower()
			, fFraction()
			, vRoughness()
			, bUsePaperBurn()
			, iShadingModel(SHADING_MODEL_DEFAULT_LIT)
			, _padShading{0, 0}
			, vHitFlash(0.f, 0.f, 0.f, 0.f)
		{
		}
	}MATERIAL, * PMATERIAL;

	ENGINE_DLL typedef struct _tagPerspectiveBuffer
	{
		Vector4 vPerspective;
		Matrix matCameraViewToLightClip;
		Matrix matInvView;
	}PERSPECTIVEBUFFER, * PPERSPECTIVEBUFFER;

	// UE의 outline post-process material 파라미터에 대응. RenderManager가
	// RenderOutline()에서 매 프레임 갱신해 b14에 바인드.
	ENGINE_DLL typedef struct alignas(16) _tagOutlineBuffer
	{
		Vector4 vOutlineColor    = Vector4(0.f, 0.f, 0.f, 1.f);     // stencil == 1
		Vector4 vOutlineColorAlt = Vector4(1.f, 0.7f, 0.f, 1.f);    // stencil >= 2
		DirectX::XMFLOAT2 vTexelSize = {0.f, 0.f};
		int iThickness = 2;
		int _pad = 0;
	}OUTLINECBUFFER, * POUTLINECBUFFER;

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

	ENGINE_DLL typedef struct alignas(16) _tagBoneInfo
	{
		float fTime;
		float fMaxTime;
		int iMaxFrame;
		int iMaxJoint;
		Vector3 vRootPos;
		int iFrame;
		int iNextFrame;
		float fBlendMaxTime;
		float fSequenceTime;

		_tagBoneInfo() :
			fTime(0.f)
			, fMaxTime(0.f)
			, iMaxFrame(0)
			, iMaxJoint(0)
			, vRootPos()
			, iFrame(0)
			, iNextFrame(0)
			, fBlendMaxTime(0.f)
			, fSequenceTime(0.f)
		{
		}
	}BONEINFO, *PBONEINFO;

	ENGINE_DLL typedef struct alignas(16) _tagBoneCBuffer
	{
		BONEINFO pInfo[2];
		int iSequenceCount;
		float pBlendPallete[256];
		_tagBoneCBuffer() :
			pInfo()
			, iSequenceCount()
			, pBlendPallete()
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

	ENGINE_DLL typedef struct _tagJointFrame
	{
		std::string strJoint;
		int iParentIndex;
		double dTime;
		Vector3 vPos;
		Vector4 vQueternion;
		Vector3 vScale;
	}JOINTFRAME, * PJOINTFRAME;

	ENGINE_DLL typedef struct _tagJoint
	{
		std::vector<JOINTFRAME> vecFrame;
	}JOINT, * PJOINT;

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
		Vector3 vMaxVelocity;
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
			, vMaxVelocity()
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

	typedef struct ENGINE_DLL alignas(16) _tagDownScaleCBuffer
	{
		unsigned int iResX;
		unsigned int iResY;
		unsigned int iDomain;
		unsigned int iGroupSize;
		float fAdaptation;
		float fBloomThreshold;
	}DOWNSCALECBUFFER, *PDOWNSCALECBUFFER;

	typedef struct ENGINE_DLL alignas(16) _tagHDRCBuffer
	{
		float fMiddleGray;
		float fLumWhiteSqr;
		float fBloomScale;
		Vector2 vDOFFarValues;
	}HDRCBUFFER, *PHDRCBUFFER;

	// Radial screen-shockwave parameters consumed by HDR.fx's FinalPassPS
	// (the full-screen tonemap resolve). RenderManager animates active
	// shockwaves CPU-side and uploads up to 4 per frame to b13; the PS
	// warps its scene-colour sample UV along each expanding ring. iCount==0
	// is the common case and the shader early-outs (identity resolve).
	typedef struct ENGINE_DLL alignas(16) _tagShockwaveCBuffer
	{
		Vector4 vShockwaves[4] = {};   // xy = centre UV, z = radius (UV), w = amplitude
		int     iCount      = 0;
		float   fThickness  = 0.12f;   // ring half-width in UV
		float   fAspect     = 1.f;     // width/height — keeps the ring circular
		float   _pad        = 0.f;
		// Player damage-feedback overlays (HDR.fx FinalPassPS). Reuse the b13
		// cbuffer so no extra pass / register is needed. All in [0,1] except
		// fFxTime (a free-running clock driving the low-HP vignette pulse).
		float   fDamageFlash = 0.f;    // sharp full-screen red flash (single hits)
		float   fChipRed     = 0.f;    // subtle persistent red edge (contact/DoT)
		float   fLowHp       = 0.f;    // low-HP vignette + desaturation strength
		float   fFxTime      = 0.f;    // seconds, for the pulse
		// Heal feedback — green vignette flash on the player when a heal pulse
		// lands. Mirrors fDamageFlash (max-merged, decays CPU-side). Own float4
		// row so the b13 layout stays 16-byte aligned.
		float   fHealFlash   = 0.f;    // green vignette flash (heal received)
		float   _padHeal[3]  = {};
	}SHOCKWAVECBUFFER, *PSHOCKWAVECBUFFER;

	typedef struct ENGINE_DLL alignas(16) _tagUICBuffer
	{
		Vector2 vStartUV;
		Vector2 vEndUV;
		Vector2 vStartPos;
		Vector2 vSize;
	}UICBUFFER, *PUICBUFFER;

	// PS_UITint cbuffer — alpha-from-atlas + RGBA tint. Used by the
	// floating combat-text renderer (and any future glyph-atlas UI).
	typedef struct ENGINE_DLL alignas(16) _tagUITintBuffer
	{
		Vector4 vTint;   // (r, g, b, alpha_master). Default white opaque.
		_tagUITintBuffer() : vTint(1.f, 1.f, 1.f, 1.f) {}
	}UITINTBUFFER, *PUITINTBUFFER;

	typedef struct alignas(16) ENGINE_DLL _tagFogCBuffer
	{
		Vector3 vFogColor;
		float fFogStartDepth;
		Vector3 vFogHighlightColor;
		float fFogGlobalDensity;
		float fFogHeightFallOff;       // offset 32
		// HLSL b12에서 float4 g_vAmbient는 16바이트 정렬 규칙상 offset 48에 놓인다.
		// Vector4(정렬 4)는 그냥 두면 offset 36에 와서 12바이트 어긋난다(=셰이더가
		// 엉뚱한 메모리를 앰비언트로 읽어 모델이 시안색 등으로 깨짐). 명시 패딩으로
		// 48에 맞춘다.
		float _padAmbient[3];          // offset 36 -> 48
		// 전역 씬 앰비언트: xyz=색, w=세기. HLSL g_vAmbient와 매칭 (offset 48).
		Vector4 vAmbient;
	}FOGCBUFFER, *PFOGCBUFFER;
}