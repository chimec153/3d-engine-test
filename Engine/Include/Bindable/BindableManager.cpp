#include "BindableManager.h"
#include "Texture.h"
#include "Transform.h"
#include "Box.h"
// VertexShader's full type is required so the post-IL attachment code in
// BindableManager<InputLayout>::BindableManager() can call SetInstInputLayout
// through a shared_ptr<VertexShader>. The existing CreateBindable<VertexShader>
// calls earlier in this file only need a forward declaration (they pass
// shared_ptrs through without dereferencing), so the include wasn't pulled
// in before.
#include "VertexShader.h"
// Full shader types + Graphics for the engine-side shader hot-reload entry
// point (RecompileAllShaders) at the bottom of this file.
#include "PixelShader.h"
#include "GeometryShader.h"
#include "ComputeShader.h"
#include "../Core/Graphics.h"
#include <string>
#ifdef _DEBUG
#include "Camera.h"
#endif

namespace Engine
{
	class HullShader;
	class InputLayout;
	class Topology;
	class DepthStencilState;
	class DomainShader;
	template <typename T>
	class ConstantBuffer;
	class GeometryShader;
	
	Engine::BindableManager<class Engine::VertexBuffer>* Engine::BindableManager<class Engine::VertexBuffer>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::IndexBuffer>* Engine::BindableManager<class Engine::IndexBuffer>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::RasterizerState>* Engine::BindableManager<class Engine::RasterizerState>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::DepthStencilState>* Engine::BindableManager<class Engine::DepthStencilState>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::BlendState>* Engine::BindableManager<class Engine::BlendState>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::VertexShader>* Engine::BindableManager<class Engine::VertexShader>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::HullShader>* Engine::BindableManager<class Engine::HullShader>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::DomainShader>* Engine::BindableManager<class Engine::DomainShader>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::GeometryShader>* Engine::BindableManager<class Engine::GeometryShader>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::PixelShader>* Engine::BindableManager<class Engine::PixelShader>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ComputeShader>* Engine::BindableManager<class Engine::ComputeShader>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::Sampler>* Engine::BindableManager<class Engine::Sampler>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::Topology>* Engine::BindableManager<class Engine::Topology>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::InputLayout>* Engine::BindableManager<class Engine::InputLayout>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::Material>* Engine::BindableManager<class Engine::Material>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::Mesh>* Engine::BindableManager<class Engine::Mesh>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::Transform>* Engine::BindableManager<class Engine::Transform>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::Texture>* Engine::BindableManager<class Engine::Texture>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagTransformBuffer>>* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagTransformBuffer>>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagBoneCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagBoneCBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagTerrainCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagTerrainCBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPointLight> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPointLight> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagMaterial> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagMaterial> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagParticleCBuffer>>* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagParticleCBuffer>>::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPerspectiveBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPerspectiveBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagColor> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagColor> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagIKCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagIKCBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagGlobalCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagGlobalCBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagDecalCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagDecalCBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagUICBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagUICBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagUITintBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagUITintBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPaperBurnCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPaperBurnCBuffer> >::m_pInst = nullptr;
	Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagFluidCBuffer> >* Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagFluidCBuffer> >::m_pInst = nullptr;

	template <typename T>
	ENGINE_DLL std::shared_ptr<T> StaticFindBindable(const std::string& strTag)
	{
		return BindableManager<T>::GetInst()->BindableManager<T>::FindBindable(strTag);
	}

	template ENGINE_DLL std::shared_ptr<Transform> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<VertexShader> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<HullShader> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<InputLayout> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<Material> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<Mesh> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<PixelShader> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<Texture> StaticFindBindable(const std::string& strTag);
	template ENGINE_DLL std::shared_ptr<Topology> StaticFindBindable(const std::string& strTag);
	// Game-side floating combat text reaches in for the UITint cbuffer.
	// Other ConstantBuffer<T> get exported implicitly because Engine
	// code itself calls StaticFindBindable on them; UITint has no
	// in-engine user, so spell the export out explicitly here.
	template ENGINE_DLL std::shared_ptr<ConstantBuffer<UITINTBUFFER>> StaticFindBindable(const std::string& strTag);

	template<typename T>
	inline BindableManager<T>::BindableManager()
	{
	}

	template<typename T>
	inline BindableManager<T>::~BindableManager()
	{
	}

	template<>
	inline BindableManager<class RasterizerState>::BindableManager()
	{
		// Opaque-pass raster — NO depth bias. The previous 0.1f/1.5f values
		// (DepthBiasClamp / SlopeScaledDepthBias) were left over from a 2023
		// "postprocessing" commit and were shadow-map-magnitude biases applied
		// to ALL deferred geometry. With SlopeScaledBias=1.5 clamped at 0.1
		// NDC, tilted faces (e.g. a tall tower's front from an isometric
		// camera) got pushed back enough that enemies a short distance behind
		// passed the LESS depth test and drew in front of the tower.
		const std::shared_ptr<RasterizerState>& pRasterizer = CreateBindable("Basic", true, D3D11_CULL_BACK, D3D11_FILL_SOLID, 1.f, 1.8f);

		if (pRasterizer)
		{
			pRasterizer->Bind();
		}

		CreateBindable("CullFront", true, D3D11_CULL_FRONT, D3D11_FILL_SOLID);
		CreateBindable("NoDepth", false, D3D11_CULL_NONE, D3D11_FILL_SOLID);
		CreateBindable(CULL_NONE, true, D3D11_CULL_NONE, D3D11_FILL_SOLID);

#ifdef _DEBUG
		CreateBindable(WIREFRAME, false, D3D11_CULL_NONE, D3D11_FILL_WIREFRAME);
#endif
	}


	template<>
	inline BindableManager<class DepthStencilState>::BindableManager()
	{
		CreateBindable("Basic", true, D3D11_DEPTH_WRITE_MASK_ALL);
		CreateBindable("NoDepth", false);
		CreateBindable("NoDepthWrite", true, D3D11_DEPTH_WRITE_MASK_ZERO);
		CreateBindable("DepthAlways", true, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_ALWAYS);
		CreateBindable("Greater", false, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_GREATER_EQUAL);
		CreateBindable("OutLineMask", true, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS,
			true, 0, 0xff, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_INCR);
		CreateBindable("OutLine", true, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_ALWAYS,
			true, 0xff, 0, D3D11_COMPARISON_EQUAL);
	}

	template<>
	inline BindableManager<class BlendState>::BindableManager()
	{
		CreateBindable("AlphaBlend");

		CreateBindable("AccBlend", D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD);

		std::vector<D3D11_RENDER_TARGET_BLEND_DESC> vecRenderTargetBlend = 
		{
			{true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL},
			{true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL},
			{true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL},
			{true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL},
		};

		// RT4 (emissive): write-enable with the same blend as the colour targets
		// so decals can composite into the emissive G-buffer (e.g. glowing ring).
		vecRenderTargetBlend.push_back(vecRenderTargetBlend[0]);

		CreateBindable("DecalBlend", false, true, vecRenderTargetBlend);


		CreateBindable("DestAlpha", D3D11_BLEND_ONE, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_ADD);
	}

	template<>
	inline BindableManager<class VertexShader>::BindableManager()
	{

#ifdef _DEBUG
		CreateBindable("NullVS", TEXT("Debug.hlsl"), "NullVS");
#endif

		CreateBindable("MultiVS", TEXT("anisotropic_microfacet.hlsl"), "VS_Multi");
		CreateBindable("PointLightVS", TEXT("anisotropic_microfacet.hlsl"), "VS_PointLight");

		CreateBindable("anisotropic_microfacet VS", TEXT("anisotropic_microfacet.hlsl"), "VS");
		CreateBindable(STANDARD_ANIM_VS, TEXT("anisotropic_microfacet.hlsl"), "VS_Skin");
		CreateBindable(STANDARD_VS, TEXT("anisotropic_microfacet.hlsl"), "VS_NoSkin");
		CreateBindable("anisotropic_microfacet VS_Terrain", TEXT("anisotropic_microfacet.hlsl"), "VS_Terrain");

		CreateBindable("anisotropic_microfacet VSInst", TEXT("anisotropic_microfacet.hlsl"), "VSInst");
		CreateBindable("anisotropic_microfacet VSSkinInst", TEXT("anisotropic_microfacet.hlsl"), "VS_SkinInst");
		CreateBindable("anisotropic_microfacet VSNoSkinInst", TEXT("anisotropic_microfacet.hlsl"), "VS_NoSkinInst");

		CreateBindable("ShadowVS", TEXT("Shadow.hlsl"), "ShadowVS");
		CreateBindable("ShadowAnimVS", TEXT("Shadow.hlsl"), "ShadowAnimVS");
		CreateBindable("anisotropic_microfacet VSInstShadow", TEXT("Shadow.hlsl"), "VSInstShadow");
		CreateBindable("anisotropic_microfacet VSSkinInstShadow", TEXT("Shadow.hlsl"), "VS_SkinInstShadow");

		CreateBindable("ParticleVS", TEXT("Particle.fx"), "VS_PARTICLE");
		CreateBindable("DecalVSInst", TEXT("Decal.fx"), "VS_DECAL_INST");
		CreateBindable("FluidVS", TEXT("VertexShader.hlsl"), "VS_FLUID");
		CreateBindable("EnvironmentVS", TEXT("VertexShader.hlsl"), "VS_ENV");
		CreateBindable("UIVS", TEXT("UI.fx"), "VS_UI");
		CreateBindable("UIVSInst", TEXT("UI.fx"), "VS_UIInst");
		// Laser beam billboard — consumes world-space corners (pos+uv+colour)
		// built CPU-side by BeamRenderManager; pairs with the "BeamVtx" IL.
		CreateBindable("BeamVS", TEXT("Beam.fx"), "VS_Beam");

#ifdef _DEBUG
		// Position-only VS for the collider wireframe debug pass. Pre-
		// transformed world-space verts; the VS just multiplies by
		// g_matTransform (set to view*proj by RenderManager's debug flush).
		CreateBindable("DebugLineVS", TEXT("VertexShader.hlsl"), "VS");
#endif
	}

	template<>
	inline bool BindableManager<VertexShader>::Init()
	{
		std::shared_ptr<InputLayout> pStandardInputLayout = StaticFindBindable<InputLayout>(STANDARD_INPUT_LAYOUT);

		std::shared_ptr<InputLayout> pDecalInstInputLayout = StaticFindBindable<InputLayout>("Decal_Inst");

		if (!CreateBindable(DECAL_VS, TEXT("Decal.fx"), "VS_DECAL", pStandardInputLayout, pDecalInstInputLayout))
		{
			return false;
		}

		return true;
	}

	template<>
	inline BindableManager<class HullShader>::BindableManager()
	{
		CreateBindable("PointLightHS", TEXT("anisotropic_microfacet.hlsl"), "HS_PointLight");
	}

	template<>
	inline BindableManager<class DomainShader>::BindableManager()
	{
		CreateBindable("PointLightDS", TEXT("anisotropic_microfacet.hlsl"), "DS_PointLight");
	}

	template<>
	inline BindableManager<class GeometryShader>::BindableManager()
	{
		CreateBindable("ParticleGS", TEXT("Particle.fx"), "GS_PARTICLE");
	}

	template<>
	inline BindableManager<class PixelShader>::BindableManager()
	{
#ifdef _DEBUG
		CreateBindable("NullPS", TEXT("Debug.hlsl"), "NullPS");
		CreateBindable("CollideDebugPS", TEXT("Debug.hlsl"), "CollideDebugPS");
		CreateBindable("DebugPS", TEXT("Debug.hlsl"), "DebugPS");
		CreateBindable("DebugPSInst", TEXT("Debug.hlsl"), "DebugPSInst");
		CreateBindable("DebugAlphaPS", TEXT("Debug.hlsl"), "DebugAlphaPS");
		CreateBindable("DebugAlphaPSInst", TEXT("Debug.hlsl"), "DebugAlphaPSInst");
		// Pairs with DebugLineVS; reads g_vDiffuseColor uniform for tint.
		CreateBindable("DebugLinePS", TEXT("Debug.hlsl"), "PS_DebugLine");
#endif

		CreateBindable("MultiPS", TEXT("anisotropic_microfacet.hlsl"), "PS_Multi");
		CreateBindable("ShadowPS", TEXT("Shadow.hlsl"), "ShadowPS");
		CreateBindable("CustomDepthCompositePS", TEXT("customdepth_composite.hlsl"), "PS_CustomDepthComposite");
		// UE outline post-process material 대응 — CustomDepth+Stencil sobel.
		CreateBindable("OutlinePS", TEXT("Outline.hlsl"), "PS_Outline");

		CreateBindable("anisotropic_microfacet PS_NoTexture", TEXT("anisotropic_microfacet.hlsl"), "PS_NoTexture");
		CreateBindable("anisotropic_microfacet PS_Terrain", TEXT("anisotropic_microfacet.hlsl"), "PS_Terrain");
		CreateBindable("anisotropic_microfacet PS_NoSpecMapNoNormalMap", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecMapNoNormalMap");
		CreateBindable(STANDARD_SOLID_PS, TEXT("anisotropic_microfacet.hlsl"), "PS_NoDiffuseNoSpecMapNoNormalMap");

		CreateBindable("anisotropic_microfacet PSInst", TEXT("anisotropic_microfacet.hlsl"), "PSInst");
		CreateBindable("anisotropic_microfacet PS_NoSpecInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecInst");
		CreateBindable("anisotropic_microfacet PS_NoSpecNoNormalInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecNoNormalInst");
		CreateBindable("anisotropic_microfacet PS_NoNormalInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoNormalInst");
		CreateBindable("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormalInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoDiffuseNoSpecNoNormalInst");
		CreateBindable("anisotropic_microfacet PS_NoTextureInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoTextureInst");

		CreateBindable("AlphaPS", TEXT("anisotropic_microfacet.hlsl"), "PS_Alpha");
		CreateBindable("AlphaPSInst", TEXT("anisotropic_microfacet.hlsl"), "PS_AlphaInst");
		CreateBindable("AlphaNoUVPS", TEXT("anisotropic_microfacet.hlsl"), "PS_AlphaNoUV");
		CreateBindable("AlphaNoUVPSInst", TEXT("anisotropic_microfacet.hlsl"), "PS_AlphaNoUVInst");
		CreateBindable("AlphaNoUVNoShadowPS", TEXT("anisotropic_microfacet.hlsl"), "PS_AlphaNoUVNoShadow");

		CreateBindable("ParticlePS", TEXT("Particle.fx"), "PS_PARTICLE");
		CreateBindable(DECAL_PS, TEXT("Decal.fx"), "PS_DECAL");
		CreateBindable(DECAL_PS_PBR, TEXT("Decal.fx"), "PS_DECAL_PBR");
		CreateBindable(DECAL_PS_RING, TEXT("Decal.fx"), "PS_DECAL_RING");
		CreateBindable("DecalPSInst", TEXT("Decal.fx"), "PS_DECAL_INST");
		CreateBindable("DecalPSPBRInst", TEXT("Decal.fx"), "PS_DECAL_PBR_INST");

		CreateBindable("PaperBurnPS", TEXT("PixelShader.hlsl"), "PS_PaperBurn");
		CreateBindable("EnvironmentPS", TEXT("PixelShader.hlsl"), "PS_ENV");
		CreateBindable("SolidPS", TEXT("PixelShader.hlsl"), "PS_SOLID");
		CreateBindable("UIPS", TEXT("UI.fx"), "PS_UI");
		CreateBindable("UIPSTint", TEXT("UI.fx"), "PS_UITint");
		CreateBindable("UIPSInst", TEXT("UI.fx"), "PS_UIInst");
		// Laser beam: cross-section falloff + UV-scroll energy + soft-particle
		// depth fade. Output is additive HDR (intensity > 1 feeds bloom).
		CreateBindable("BeamPS", TEXT("Beam.fx"), "PS_Beam");
		// Tracer-tip glow: radial additive sprite for the head of a bullet
		// trail. Reuses BeamVS / BeamVtx; only the PS differs.
		CreateBindable("BeamGlowPS", TEXT("Beam.fx"), "PS_BeamGlow");
		// Stylized enemy-death billboards. Shape mask in the PS; colour pulled
		// from a 1xN lifetime ramp LUT (bound at t0). Reuse BeamVS / BeamVtx.
		CreateBindable("DeathPuffPS",    TEXT("Beam.fx"), "PS_DeathPuff");
		CreateBindable("DeathStarPS",    TEXT("Beam.fx"), "PS_DeathStar");
		CreateBindable("DeathDiamondPS", TEXT("Beam.fx"), "PS_DeathDiamond");
		CreateBindable("DeathRingPS",    TEXT("Beam.fx"), "PS_DeathRing");
		// Procedural muzzle flash: 8-spoke polar starburst, no texture. Additive
		// HDR (core blooms). Reuse BeamVS / BeamVtx.
		CreateBindable("MuzzlePS",       TEXT("Beam.fx"), "PS_Muzzle");
	}

	template <>
	BindableManager<class ComputeShader>::BindableManager()
	{
		CreateBindable("Sequence", TEXT("ComputeShader.hlsl"), "Sequence");
		CreateBindable("SequenceInst", TEXT("ComputeShader.hlsl"), "SequenceInst");
		CreateBindable("PostProcess", TEXT("ComputeShader.hlsl"), "PostProcess");

		CreateBindable("ParticleCS", TEXT("Particle.fx"), "CS_PARTICLE");
		CreateBindable("FluidCS", TEXT("ComputeShader.hlsl"), "CS_FLUID");
	}

	template <typename T>
	class ConstantBuffer;

	template <>
	inline BindableManager<class ConstantBuffer<TRANSFORMBUFFER>>::BindableManager()
	{
		CreateBindable("Transform");
	}

	template <>
	inline BindableManager<class ConstantBuffer<COLOR>>::BindableManager()
	{
		CreateBindable("COLOR", 0);
	}

	template <>
	inline BindableManager<ConstantBuffer<MATERIAL>>::BindableManager()
	{
		CreateBindable("Material", 2);
	}

	template <>
	inline BindableManager<class ConstantBuffer<PERSPECTIVEBUFFER>>::BindableManager()
	{
		CreateBindable("Perspective", 3);
	}

	template <>
	inline BindableManager<ConstantBuffer<BONECBUFFER>>::BindableManager()
	{
		CreateBindable("Bone", 4);
	}

	template <>
	inline BindableManager<ConstantBuffer<TERRAINCBUFFER>>::BindableManager()
	{
		CreateBindable("Terrain", 5);
	}

	template <>
	inline BindableManager<ConstantBuffer<IKCBUFFER>>::BindableManager()
	{
		CreateBindable("IK", 6);
	}

	template <>
	inline BindableManager<class ConstantBuffer<PARTICLECBUFFER>>::BindableManager()
	{
		CreateBindable("Particle", 7);
	}

	template <>
	inline BindableManager<ConstantBuffer<GLOBALCBUFFER>>::BindableManager()
	{
		CreateBindable("Global", 8);
	}

	template <>
	inline BindableManager<class ConstantBuffer<DECALCBUFFER>>::BindableManager()
	{
		CreateBindable("Decal", 9);
	}

	template <>
	inline BindableManager<class ConstantBuffer<UICBUFFER>>::BindableManager()
	{
		CreateBindable("UI", 5);
	}

	template <>
	inline BindableManager<class ConstantBuffer<UITINTBUFFER>>::BindableManager()
	{
		// Register slot has to match PS_UITint's `cbuffer UITint : register(b13)`.
		// b10 was the original choice but conflicts with shared.hlsl's
		// PaperBurn cbuffer (80 bytes vs UITint's 16) — D3D11 warned
		// "constant buffer too small" and reads returned 0 (or PaperBurn
		// leftovers), showing as a wrong tint colour.
		CreateBindable("UITint", 13);
	}

	template <>
	inline BindableManager<class ConstantBuffer<PAPERBURNCBUFFER>>::BindableManager()
	{
		CreateBindable("PaperBurn", 10);
	}

	template <>
	inline BindableManager<class ConstantBuffer<FLUIDCBUFFER>>::BindableManager()
	{
		CreateBindable("Fluid", 11);
	}

	template <>
	inline BindableManager<class ConstantBuffer<POINTLIGHT>>::BindableManager()
	{
		CreateBindable("PointLightCBuffer", 1);
	}

	template <>
	inline BindableManager<class Sampler>::BindableManager()
	{
		const std::shared_ptr<Sampler>& pPoint = CreateBindable("Point", D3D11_FILTER_MIN_MAG_MIP_POINT);

		if (pPoint != nullptr)
		{
			pPoint->Bind();
		}

		const std::shared_ptr<Sampler>& pLinear = CreateBindable("Linear", D3D11_FILTER_MIN_MAG_MIP_LINEAR, 1);

		if (pLinear != nullptr)
		{
			pLinear->Bind();
		}

		const std::shared_ptr<Sampler>& pAnisotropic = CreateBindable("Anisotropic", D3D11_FILTER_ANISOTROPIC, 2);

		if (pAnisotropic != nullptr)
		{
			pAnisotropic->Bind();
		}

		const std::shared_ptr<Sampler>& pShadow = CreateBindable("Shadow", D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, 3, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_COMPARISON_LESS);

		if (pShadow != nullptr)
		{
			pShadow->Bind();
		}
	}

	template <>
	inline BindableManager<class Topology>::BindableManager()
	{
#ifdef _DEBUG
		CreateBindable("LineList", D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		CreateBindable("LineStrip", D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
#endif

		CreateBindable("PointList", D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		CreateBindable("TriangleList", D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CreateBindable("TriangleStrip", D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		CreateBindable("1ControlPointPatch", D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	}

	template <>
	inline BindableManager<class InputLayout>::BindableManager()
	{
		// Per-vertex part MUST match VertexStandard byte layout (Types.h:87).
		// VertexStandard is 76B: tangent(16) + blendIndecies(16) + pos(12) +
		// normal(12) + blendWeight(12) + uv(8). The old TPNT_Inst used
		// explicit offsets 0/16/28/40 for a hypothetical compact "TPNT"(48B)
		// vertex — so Position read blendIndecies(=0) and every instanced
		// STANDARD_VS mesh (Tower / Bullet / HealTower at 2+ count) collapsed
		// to a degenerate point and disappeared. APPEND_ALIGNED + the same
		// 6 fields as "Standard"/"Standard_Inst" fixes it; VSInst doesn't
		// read BLENDINDICES/BLENDWEIGHT but having them in the IL is harmless
		// (D3D11 just doesn't wire unused semantics).
		D3D11_INPUT_ELEMENT_DESC desc[] = {
			{"Tangent", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WorldView", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WorldView", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WorldView", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"WorldView", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"LIGHTVP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 128, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"LIGHTVP", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 144, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"LIGHTVP", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 160, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"LIGHTVP", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 176, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"Diffuse", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 192, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"Specular", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 208, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"Roughness", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 224, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"MaterialFraction", 0, DXGI_FORMAT_R32_FLOAT, 1, 232, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		};

		const std::shared_ptr<VertexShader>& pVertexShader = StaticFindBindable<VertexShader>("anisotropic_microfacet VSInst");

		CreateBindable("TPNT_Inst", pVertexShader, desc, static_cast<int>(sizeof(desc) / sizeof(D3D11_INPUT_ELEMENT_DESC)), 236);

		D3D11_INPUT_ELEMENT_DESC descTPNT[] =
		{
			{"Tangent", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		};

		const std::shared_ptr<VertexShader>& pVS = StaticFindBindable<VertexShader>("anisotropic_microfacet VS");

		CreateBindable("TPNT", pVS, descTPNT, static_cast<int>(sizeof(descTPNT) / sizeof(D3D11_INPUT_ELEMENT_DESC)));

		D3D11_INPUT_ELEMENT_DESC descSkin[] =
		{
			{"Tangent", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		};

		const std::shared_ptr<VertexShader>& pVSSkin = StaticFindBindable<VertexShader>("anisotropic_microfacet VSSkin");

		CreateBindable("Standard", pVSSkin, descSkin, static_cast<int>(sizeof(descSkin) / sizeof(D3D11_INPUT_ELEMENT_DESC)));

		D3D11_INPUT_ELEMENT_DESC descSkinInst[] =
		{
			{"Tangent", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	16
			{"World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	32
			{"World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	48
			{"World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	64
			{"View", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"View", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"View", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"View", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	128
			{"LIGHTVP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"LIGHTVP", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"LIGHTVP", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"LIGHTVP", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	192
			{"Material", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	208
			{"Material", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	224
			{"Material", 2, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	232
			{"Material", 3, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	236
			{"JointSocket", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	252
			{"JointSocket", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	268
			{"JointSocket", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	284
			{"JointSocket", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	300
			{"Bone", 0, DXGI_FORMAT_R32_SINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	304
			{"Bone", 1, DXGI_FORMAT_R32_SINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	308
			{"Bone", 2, DXGI_FORMAT_R32_SINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	312
		};

		std::shared_ptr<VertexShader> pVSSkinInst = StaticFindBindable<VertexShader>("anisotropic_microfacet VSSkinInst");

		CreateBindable("Standard_Inst", pVSSkinInst, descSkinInst, static_cast<int>(sizeof(descSkinInst) / sizeof(D3D11_INPUT_ELEMENT_DESC)), 312);

		// Back-fill instanced input layouts onto their matching VS
		// bindables. VertexShader bindables are constructed BEFORE the
		// InputLayout pool exists (template specialization ordering), so
		// VS::m_pInputLayoutInst was left null at ctor time. RenderManager's
		// TryRenderInstancedBucket fast path reads
		// pInstVS->GetInstInputLayout() and bails when null, so without
		// this attachment every "Inst" VS silently falls back to per-MR
		// solo rendering.
		//
		// "Standard_Inst" was created with VSSkinInst for layout validation,
		// but VSStandardInstIn (its input struct) is shared by both
		// VSSkinInst and VSNoSkinInst — D3D11 allows reusing the IL across
		// any VS whose signature matches, so both shaders attach the same
		// IL pointer.
		//
		// Critical: we're still INSIDE BindableManager<InputLayout>'s ctor,
		// so the singleton's m_pInst is not yet assigned. Calling
		// StaticFindBindable<InputLayout>(...) here re-enters GetInst, sees
		// null, and constructs a SECOND BindableManager — infinite
		// recursion / stack overflow. Use the local FindBindable (just a
		// map lookup on m_mapBindable we just populated) instead. The
		// VertexShader manager is a different singleton fully built by
		// now, so StaticFindBindable<VertexShader> is safe.
		auto pTPNTInst     = FindBindable("TPNT_Inst");
		auto pStandardInst = FindBindable("Standard_Inst");
		if (auto pVSInst = StaticFindBindable<VertexShader>("anisotropic_microfacet VSInst"))
		{
			if (pTPNTInst) pVSInst->SetInstInputLayout(pTPNTInst);
		}
		if (pVSSkinInst && pStandardInst)
			pVSSkinInst->SetInstInputLayout(pStandardInst);
		if (auto pVSNoSkinInst = StaticFindBindable<VertexShader>("anisotropic_microfacet VSNoSkinInst"))
		{
			if (pStandardInst) pVSNoSkinInst->SetInstInputLayout(pStandardInst);
		}
		// VSInstShadow / VSSkinInstShadow share the same VSStandardInstIn
		// input struct, so they reuse Standard_Inst. Without this attach,
		// RenderShadow's instanced fast path reads GetInstInputLayout()
		// as null and falls back to per-MR solo draws.
		if (auto pVSInstShadow = StaticFindBindable<VertexShader>("anisotropic_microfacet VSInstShadow"))
		{
			if (pStandardInst) pVSInstShadow->SetInstInputLayout(pStandardInst);
		}
		if (auto pVSSkinInstShadow = StaticFindBindable<VertexShader>("anisotropic_microfacet VSSkinInstShadow"))
		{
			if (pStandardInst) pVSSkinInstShadow->SetInstInputLayout(pStandardInst);
		}

		D3D11_INPUT_ELEMENT_DESC descP = { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

		CreateBindable("P", StaticFindBindable<VertexShader>("PointLightVS"), &descP, static_cast<int>(sizeof(descP) / sizeof(D3D11_INPUT_ELEMENT_DESC)));

		// Laser beam billboard vertex: world-space pos (12) + uv (8) +
		// HDR colour (16) = 36-byte stride, matching BeamRenderManager's
		// BeamVertex. Pairs with BeamVS / BeamPS in Beam.fx.
		D3D11_INPUT_ELEMENT_DESC descBeam[] =
		{
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Color",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		CreateBindable("BeamVtx", StaticFindBindable<VertexShader>("BeamVS"), descBeam, static_cast<int>(sizeof(descBeam) / sizeof(D3D11_INPUT_ELEMENT_DESC)));

#ifdef _DEBUG
		// Position-only input layout dedicated to the collider wireframe
		// debug pass. Pairs with DebugLineVS (consumes Position semantic).
		D3D11_INPUT_ELEMENT_DESC descDebugLine = { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		CreateBindable("DebugLineIL", StaticFindBindable<VertexShader>("DebugLineVS"), &descDebugLine, 1);
#endif

		D3D11_INPUT_ELEMENT_DESC pDecalInstDesc[] =
		{
			{"Tangent", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	16
			{"World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	32
			{"World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	48
			{"World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	64
			{"InvWorldView", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"InvWorldView", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"InvWorldView", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	
			{"InvWorldView", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	128
			{"Diffuse", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	144
			{"Specular", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	160
			{"Emissive", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	176
			{"Roughness", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	184
			{"MaterialFraction", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	188
			{"DECAL", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	192
			{"DECAL", 1, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	196
			{"DECAL", 2, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA,1},	//	200
		};

		std::shared_ptr<VertexShader> pVSDecalInst = StaticFindBindable<VertexShader>("DecalVSInst");

		CreateBindable("Decal_Inst", pVSDecalInst, pDecalInstDesc, static_cast<int>(sizeof(pDecalInstDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC)), 200);
	}

	template <>
	inline BindableManager<class VertexBuffer>::BindableManager()
	{
#ifdef _DEBUG
		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();

		if (pCamera)
		{
			float fNear = pCamera->GetNear();

			float fAngle = pCamera->GetAngle();

			float fX = tanf(fAngle) * fNear;
			float fY = fX / pCamera->GetRatio();

			float fFarX = tanf(fAngle) * 5000.f;
			float fFarY = fFarX / pCamera->GetRatio();

			std::vector<VertexTexture> vecViewFrustomVertex =
			{
				{0.f,0.f,0.f,0.f,fX,-fY,fNear,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,fX,fY,fNear,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,-fX,-fY,fNear,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,-fX,fY,fNear,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,fFarX,-fFarY,5000.f,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,fFarX,fFarY,5000.f,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,-fFarX,-fFarY,5000.f,0.f,0.f,0.f,0.f,0.f},
				{0.f,0.f,0.f,0.f,-fFarX,fFarY,5000.f,0.f,0.f,0.f,0.f,0.f},
			};

			CreateBindable("ViewFrustom", vecViewFrustomVertex);
		}
#endif
	}

	template <>
	inline BindableManager<class IndexBuffer>::BindableManager()
	{
#ifdef _DEBUG

		std::vector<unsigned int> vecIndex =
		{
			0, 1, 2, 3, 0, 2, 1, 3,
			4, 5, 6, 7, 4, 6, 5, 7,
			0, 4, 1, 5, 2, 6, 3, 7
		};

		CreateBindable("ViewFrustomIndex", vecIndex);
#endif
	}

	template <>
	inline BindableManager<class Material>::BindableManager()
	{
		CreateBindable("Material");
	}

	template <>
	inline ENGINE_DLL BindableManager<class Mesh>::BindableManager()
	{
#ifdef _DEBUG
		CreateBindable("Line", std::vector<VertexTexture>	{ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}, { 0.f,0.f,0.f,0.f,0.f,0.f,100.f,0.f,0.f,0.f,0.f,0.f }	}, std::vector<unsigned int>{ 0, 1 });
#endif

		CreateBindable("Box", Box::CreateTextureVertex<VertexStandard>(), Box::GetTextureIndex());

		// 4-vertex unit quad for UIRenderer-hosted UI elements (HPBar et
		// al). The UI VS reads g_vUIPosition[SV_VertexID] so the actual
		// VB data is never consumed — we just need *something* to make
		// Mesh::Draw issue Draw(4, 0). No index buffer → triangle-strip
		// topology over 4 verts gives the two triangles of a quad.
		CreateBindable("UIQuad",
			std::vector<VertexStandard>(4),
			std::vector<unsigned int>{});
	}

	template <>
	inline BindableManager<Texture>::BindableManager()
	{
		std::shared_ptr<Texture> pNoiseTexture = CreateBindable("Noise", TEXT("noise_01.png"), TEXTURE_PATH, 17);

		assert(pNoiseTexture);

		pNoiseTexture->Bind();

		CreateBindable("PaperBurn", TEXT("DefaultBurn.png"), TEXTURE_PATH, 4);
	}

	// Force instantiation of Clear() in Engine.dll for types the editor
	// resets on project load (ProjectModule::Load). Without these, the
	// template member function never gets compiled into Engine.dll and
	// dllimport from Editor.exe ends with an unresolved external.
	template ENGINE_DLL void BindableManager<Mesh>::Clear();
	template ENGINE_DLL void BindableManager<Texture>::Clear();
	template ENGINE_DLL void BindableManager<Material>::Clear();

	int RecompileAllShaders(std::string& outLog)
	{
		int ok = 0, fail = 0;
		std::string errs;

		// GetMap is an inline member of the dllexport class template; called
		// here (engine-side) it's a plain inline, so no cross-DLL export of the
		// per-type instantiation is needed (which is why the editor can't loop
		// this itself). Recompile lives on the Shader base — a failed compile
		// keeps the old native shader, so a typo never blanks the viewport.
		auto recompileType = [&](auto* pMgr)
		{
			if (!pMgr) return;
			for (const auto& kv : pMgr->GetMap())
			{
				const auto& sp = kv.second;
				if (!sp) continue;
				std::string err;
				if (sp->Recompile(err)) ++ok;
				else if (err != "not hot-reloadable") { ++fail; errs += kv.first + ": " + err + "\n"; }
			}
		};

		recompileType(BindableManager<VertexShader>::GetInst());
		recompileType(BindableManager<PixelShader>::GetInst());
		recompileType(BindableManager<GeometryShader>::GetInst());
		recompileType(BindableManager<ComputeShader>::GetInst());

		// New native objects → drop the bound-shader cache so the next frame's
		// binds re-issue XSSet with the fresh pointers instead of skipping on a
		// stale "already bound" hit.
		Graphics::GetInst()->ResetBindCache();

		outLog = "Recompiled " + std::to_string(ok) + " shader(s), " +
		         std::to_string(fail) + " error(s).\n" + errs;
		return fail;
	}
}