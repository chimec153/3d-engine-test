#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#define DIRECTINPUT_VERSION 0x0800
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#define NOMINMAX

#include <cmath>
#include <corecrt_math.h>
#include <list>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <crtdbg.h>
#include <vector>
#include "../Bindable/DirectXTex.h"
#include <unordered_map>
#include <Windows.h>
#include <dinput.h>
#include <string>
#include <stdio.h>
#include <tchar.h>
#include "fbxsdk/scene/geometry/fbxlayer.h"
#include <fbxsdk.h>
#include <memory>
#include <map>
#include "../Sound/inc/fmod.h"
#include "../Sound/inc/fmod.hpp"

#pragma comment(lib, "zlib-md.lib")
#pragma comment(lib, "libxml2-md.lib")
#pragma comment(lib, "libfbxsdk-md.lib")

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "fmod_vc.lib")

#ifdef _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#pragma comment(lib, "Detour-d.lib")
#pragma comment(lib, "DetourCrowd-d.lib")
#pragma comment(lib, "DetourTileCache-d.lib")
#else
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "Detour.lib")
#pragma comment(lib, "DetourCrowd.lib")
#pragma comment(lib, "DetourTileCache.lib")
#endif

#ifdef EXPORT_ENGINE
#define ENGINE_DLL __declspec(dllexport)
#else
#define ENGINE_DLL __declspec(dllimport)
#endif


#define SAFE_RELEASE(p) if(p) {p->Release(); p = nullptr;} 
#define SAFE_DELETE(p) if(p) {delete p; p = nullptr;}
#define SAFE_DELETE_ARRAY(p) if(p) {delete[] p; p = nullptr;}

#define PI	3.141592f

#ifdef _DEBUG
#define dbg_new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define dbg_new new
#endif

#define ROOT_PATH "RootPath"
#define SHADER_PATH "ShaderPath"
#define TEXTURE_PATH "TexturePath"
#define MESH_PATH "MeshPath"
#define SOUND_PATH "SoundPath"

#define TEXT_LEN	4096

#define DEFAULT_LAYER	"DefaultLayer"
#define ALPHA_LAYER		"AlphaLayer"
#define UI_LAYER		"UILayer"

#define epsilon	0.0001
#define FIXED_UPDATE_TIME	0.01666666f

#define STANDARD_VS "anisotropic_microfacet VSNoSkin"
#define STANDARD_ANIM_VS "anisotropic_microfacet VSSkin"
#define STANDARD_PS "anisotropic_microfacet PS"
#define STANDARD_SOLID_PS "anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal"
#define STANDARD_INPUT_LAYOUT "Standard"
#define STANDARD_TOPOLOGY "TriangleList"
#define WIREFRAME "WireFrame"
#define CULL_NONE "CullNone"
#define DECAL_VS	"DecalVS"
#define DECAL_PS	"DecalPS"
#define DECAL_PS_PBR	"DecalPSPBR"

template <typename T>
void Safe_Delete_VecList_Array(T& p)
{
	typename T::iterator iter = p.begin();
	typename T::iterator iterEnd = p.end();

	for (; iter != iterEnd;)
	{
		SAFE_DELETE_ARRAY(*iter);
		iter = p.erase(iter);
		iterEnd = p.end();
	}

	p.clear();
}

template <typename T>
void Safe_Delete_Array_Map(T& p)
{
	typename T::iterator iter = p.begin();
	typename T::iterator iterEnd = p.end();

	for (; iter != iterEnd;)
	{
		SAFE_DELETE_ARRAY(iter->second);
		iter = p.erase(iter);
		iterEnd = p.end();
	}

	p.clear();
}

template <typename T>
void Safe_Delete_VecList(T& p)
{
	typename T::iterator iter = p.begin();
	typename T::iterator iterEnd = p.end();

	for (; iter != iterEnd;)
	{
		SAFE_DELETE(*iter);
		iter = p.erase(iter);
		iterEnd = p.end();
	}

	p.clear();
}

template <typename T>
void Safe_Delete_Map(T& p)
{
	typename T::iterator iter = p.begin();
	typename T::iterator iterEnd = p.end();

	for (; iter != iterEnd;)
	{
		SAFE_DELETE(iter->second);
		iter = p.erase(iter);
		iterEnd = p.end();
	}

	p.clear();
}