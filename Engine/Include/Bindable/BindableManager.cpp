#include "BindableManager.h"
#include "Texture.h"
#include "TransformBuffer.h"

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

	template <typename T, typename ...Args>
	ENGINE_DLL std::shared_ptr<T> StaticCreateBindable(const std::string& strTag, Args... args)
	{
		return BindableManager<T>::GetInst()->BindableManager<T>::CreateBindable<Args...>(strTag, args...);
	}
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
		const std::shared_ptr<RasterizerState>& pRasterizer = CreateBindable("Basic", true, D3D11_CULL_BACK, D3D11_FILL_SOLID, 1.2f, 2.5f);
		//const std::shared_ptr<RasterizerState>& pRasterizer = CreateBindable("Basic", true, D3D11_CULL_BACK, D3D11_FILL_SOLID);

		if (pRasterizer)
		{
			pRasterizer->Bind();
		}

		CreateBindable("CullFront", true, D3D11_CULL_FRONT, D3D11_FILL_SOLID);
		CreateBindable("NoDepth", false, D3D11_CULL_NONE, D3D11_FILL_SOLID);

#ifdef _DEBUG
		CreateBindable("WireFrame", false, D3D11_CULL_NONE, D3D11_FILL_WIREFRAME);
		CreateBindable("CullNone", true, D3D11_CULL_NONE, D3D11_FILL_SOLID);
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

		CreateBindable("DecalBlend", false, true, vecRenderTargetBlend);
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
		CreateBindable("anisotropic_microfacet VSSkin", TEXT("anisotropic_microfacet.hlsl"), "VS_Skin");
		CreateBindable("anisotropic_microfacet VSNoSkin", TEXT("anisotropic_microfacet.hlsl"), "VS_NoSkin");
		CreateBindable("anisotropic_microfacet VS_Terrain", TEXT("anisotropic_microfacet.hlsl"), "VS_Terrain");

		CreateBindable("anisotropic_microfacet VSInst", TEXT("anisotropic_microfacet.hlsl"), "VSInst");
		CreateBindable("anisotropic_microfacet VSSkinInst", TEXT("anisotropic_microfacet.hlsl"), "VS_SkinInst");
		CreateBindable("anisotropic_microfacet VSNoSkinInst", TEXT("anisotropic_microfacet.hlsl"), "VS_NoSkinInst");

		CreateBindable("ShadowVS", TEXT("Shadow.hlsl"), "ShadowVS");
		CreateBindable("ShadowAnimVS", TEXT("Shadow.hlsl"), "ShadowAnimVS");
		CreateBindable("anisotropic_microfacet VSInstShadow", TEXT("Shadow.hlsl"), "VSInstShadow");
		CreateBindable("anisotropic_microfacet VSSkinInstShadow", TEXT("Shadow.hlsl"), "VS_SkinInstShadow");

		CreateBindable("ParticleVS", TEXT("Particle.fx"), "VS_PARTICLE");
		CreateBindable("DecalVS", TEXT("Decal.fx"), "VS_DECAL");
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
		CreateBindable("DebugPSInst", TEXT("Debug.hlsl"), "DebugPS");
#endif

		CreateBindable("MultiPS", TEXT("anisotropic_microfacet.hlsl"), "PS_Multi");
		CreateBindable("ShadowPS", TEXT("Shadow.hlsl"), "ShadowPS");

		CreateBindable("anisotropic_microfacet PS_NoTexture", TEXT("anisotropic_microfacet.hlsl"), "PS_NoTexture");
		CreateBindable("anisotropic_microfacet PS_Terrain", TEXT("anisotropic_microfacet.hlsl"), "PS_Terrain");

		CreateBindable("anisotropic_microfacet PSInst", TEXT("anisotropic_microfacet.hlsl"), "PSInst");
		CreateBindable("anisotropic_microfacet PS_NoSpecInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecInst");
		CreateBindable("anisotropic_microfacet PS_NoSpecNoNormalInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecNoNormalInst");
		CreateBindable("anisotropic_microfacet PS_NoNormalInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoNormalInst");
		CreateBindable("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormalInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoDiffuseNoSpecNoNormalInst");
		CreateBindable("anisotropic_microfacet PS_NoTextureInst", TEXT("anisotropic_microfacet.hlsl"), "PS_NoTextureInst");

		CreateBindable("ParticlePS", TEXT("Particle.fx"), "PS_PARTICLE");
		CreateBindable("DecalPS", TEXT("Decal.fx"), "PS_DECAL");

		CreateBindable("PaperBurnPS", TEXT("PixelShader.hlsl"), "PS_PaperBurn");
	}

	template <>
	BindableManager<class ComputeShader>::BindableManager()
	{
		CreateBindable("Sequence", TEXT("ComputeShader.hlsl"), "Sequence");
		CreateBindable("SequenceInst", TEXT("ComputeShader.hlsl"), "SequenceInst");
		CreateBindable("PostProcess", TEXT("ComputeShader.hlsl"), "PostProcess");

		CreateBindable("ParticleCS", TEXT("Particle.fx"), "CS_PARTICLE");
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
	inline BindableManager<class ConstantBuffer<PAPERBURNCBUFFER>>::BindableManager()
	{
		CreateBindable("PaperBurn", 10);
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
		D3D11_INPUT_ELEMENT_DESC desc[] = {
			{"Tangent", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"Texcoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
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

		const std::shared_ptr<VertexShader>& pVSSkinInst = StaticFindBindable<VertexShader>("anisotropic_microfacet VSSkinInst");

		CreateBindable("Standard_Inst", pVSSkinInst, descSkinInst, static_cast<int>(sizeof(descSkinInst) / sizeof(D3D11_INPUT_ELEMENT_DESC)), 312);
	}

	template <>
	inline BindableManager<class VertexBuffer>::BindableManager()
	{
#ifdef _DEBUG
		float fNear = Graphics::GetInst()->GetNear();

		float fAngle = Graphics::GetInst()->GetAngle();

		float fX = tanf(fAngle) * fNear;
		float fY = fX / Graphics::GetInst()->GetRatio();

		float fFarX = tanf(fAngle) * 5000.f;
		float fFarY = fFarX / Graphics::GetInst()->GetRatio();

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
	inline BindableManager<class Mesh>::BindableManager()
	{
#ifdef _DEBUG
		CreateBindable("Line", std::vector<VertexTexture>	{ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}, { 0.f,0.f,0.f,0.f,0.f,0.f,100.f,0.f,0.f,0.f,0.f,0.f }	}, std::vector<unsigned int>{ 0, 1 });
#endif
	}

	template <>
	inline BindableManager<Texture>::BindableManager()
	{
		std::shared_ptr<Texture> pNoiseTexture = CreateBindable("Noise", TEXT("noise_01.png"), TEXTURE_PATH, 17);

		assert(pNoiseTexture);

		pNoiseTexture->Bind();

		CreateBindable("PaperBurn", TEXT("DefaultBurn.png"), TEXTURE_PATH, 4);
	}
}