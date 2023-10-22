#include "ShaderManager.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/BindableManager.h"

namespace Engine
{
	ShaderManager* ShaderManager::m_pInst = nullptr;

	ShaderManager::ShaderManager()
	{
	}

	ShaderManager::~ShaderManager()
	{
	}

	bool ShaderManager::Init()
	{
		std::vector<std::shared_ptr<Bindable>> vecMicroSphere;

		std::shared_ptr<VertexShader> pMicroVSSphere = StaticCreateBindable<VertexShader>("anisotropic_microfacet VS_Sphere", TEXT("anisotropic_microfacet.hlsl"), "VS_Sphere");
		std::shared_ptr<Bindable> pMicroPSSphere = StaticCreateBindable<PixelShader>("anisotropic_microfacet PS_Sphere", TEXT("anisotropic_microfacet.hlsl"), "PS_Sphere");

		D3D11_INPUT_ELEMENT_DESC descPN[] =
		{
			{"Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
			{"Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
		};

		std::shared_ptr<Bindable> pInputLayoutPN = StaticCreateBindable<InputLayout>("PN", pMicroVSSphere, descPN, static_cast<int>(sizeof(descPN) / sizeof(D3D11_INPUT_ELEMENT_DESC)));

		vecMicroSphere.push_back(pMicroVSSphere);
		vecMicroSphere.push_back(pMicroPSSphere);
		vecMicroSphere.push_back(pInputLayoutPN);

		m_mapShader.insert(std::make_pair("anisotropic_microfacet_Sphere", vecMicroSphere));

		std::vector<std::shared_ptr<Bindable>> vecMicro;

		std::shared_ptr<VertexShader> pMicroVS = StaticFindBindable<VertexShader>("anisotropic_microfacet VS");
		std::shared_ptr<Bindable> pMicroPS = StaticCreateBindable<PixelShader>("anisotropic_microfacet PS", TEXT("anisotropic_microfacet.hlsl"), "PS");

		std::shared_ptr<Bindable> pInputLayoutTPNT = StaticFindBindable<InputLayout>("TPNT");

		vecMicro.push_back(pMicroVS);
		vecMicro.push_back(pMicroPS);
		vecMicro.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("anisotropic_microfacet", vecMicro));

		std::vector<std::shared_ptr<Bindable>> vecMicroNoSpec;

		std::shared_ptr<Bindable> pMicroPSNoSpec = StaticCreateBindable<PixelShader>("anisotropic_microfacet PS_NoSpec", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecMap");

		vecMicroNoSpec.push_back(pMicroVS);
		vecMicroNoSpec.push_back(pMicroPSNoSpec);
		vecMicroNoSpec.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("anisotropic_microfacet_NoSpec", vecMicroNoSpec));

		std::vector<std::shared_ptr<Bindable>> vecMicroNoNormal;

		std::shared_ptr<Bindable> pMicroPSNoNormal = StaticCreateBindable<PixelShader>("anisotropic_microfacet PS_NoNormal", TEXT("anisotropic_microfacet.hlsl"), "PS_NoNormalMap");

		vecMicroNoNormal.push_back(pMicroVS);
		vecMicroNoNormal.push_back(pMicroPSNoNormal);
		vecMicroNoNormal.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("anisotropic_microfacet_NoNormal", vecMicroNoNormal));

		std::vector<std::shared_ptr<Bindable>> vecMicroNoSpecNoNormal;

		std::shared_ptr<Bindable> pMicroNoSpecNoNormal = StaticCreateBindable<PixelShader>("anisotropic_microfacet PS_NoSpecNoNormal", TEXT("anisotropic_microfacet.hlsl"), "PS_NoSpecMapNoNormalMap");

		vecMicroNoSpecNoNormal.push_back(pMicroVS);
		vecMicroNoSpecNoNormal.push_back(pMicroNoSpecNoNormal);
		vecMicroNoSpecNoNormal.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("anisotropic_microfacet_NoSpecNoNormal", vecMicroNoSpecNoNormal));

		std::vector<std::shared_ptr<Bindable>> vecMicroNoDiffuseNoSpecNoNormal;

		std::shared_ptr<Bindable> pMicroNoAlbedoNoSpecNoNormal = StaticCreateBindable<PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal", TEXT("anisotropic_microfacet.hlsl"), "PS_NoDiffuseNoSpecMapNoNormalMap");

		vecMicroNoDiffuseNoSpecNoNormal.push_back(pMicroVS);
		vecMicroNoDiffuseNoSpecNoNormal.push_back(pMicroNoAlbedoNoSpecNoNormal);
		vecMicroNoDiffuseNoSpecNoNormal.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("anisotropic_microfacet_NoDiffuseNoNormalNoSpec", vecMicroNoDiffuseNoSpecNoNormal));

		std::vector<std::shared_ptr<Bindable>> vecNormal;

		std::shared_ptr<VertexShader> pNormalVS = StaticCreateBindable<VertexShader>("NormalShader VS", TEXT("NormalShader.hlsl"), "VS");
		std::shared_ptr<Bindable> pNormalPS = StaticCreateBindable<PixelShader>("NormalShader PS", TEXT("NormalShader.hlsl"), "PS");

		vecNormal.push_back(pNormalVS);
		vecNormal.push_back(pNormalPS);
		vecNormal.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("Phong", vecNormal));

		std::vector<std::shared_ptr<Bindable>> vecNormalNoNormal;

		std::shared_ptr<Bindable> pNormalPSNoNormal = StaticCreateBindable<PixelShader>("NormalShader PS_NoNormal", TEXT("NormalShader.hlsl"), "PS_NoNormal");

		vecNormalNoNormal.push_back(pNormalVS);
		vecNormalNoNormal.push_back(pNormalPSNoNormal);
		vecNormalNoNormal.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("Phong_NoNormal", vecNormalNoNormal));

		std::vector<std::shared_ptr<Bindable>> vecNormalNoNormalNoSpec;

		std::shared_ptr<Bindable> pNormalPSNoNormalNoSpec = StaticCreateBindable<PixelShader>("NormalShader PS_NoNormalNoSpec", TEXT("NormalShader.hlsl"), "PS_NoNormalNoSpec");

		vecNormalNoNormalNoSpec.push_back(pNormalVS);
		vecNormalNoNormalNoSpec.push_back(pNormalPSNoNormalNoSpec);
		vecNormalNoNormalNoSpec.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("Phong_NoNormalNoSpec", vecNormalNoNormalNoSpec));

		std::vector<std::shared_ptr<Bindable>> vecNormalNoSpec;

		std::shared_ptr<Bindable> pNormalPSNoSpec = StaticCreateBindable<PixelShader>("NormalShader PS_NoSpec", TEXT("NormalShader.hlsl"), "PS_NoSpec");

		vecNormalNoSpec.push_back(pNormalVS);
		vecNormalNoSpec.push_back(pNormalPSNoSpec);
		vecNormalNoSpec.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("Phong_NoSpec", vecNormalNoSpec));

		std::vector<std::shared_ptr<Bindable>> vecNormalNoDiffuseNoNormalNoSpec;

		std::shared_ptr<Bindable> pNormalPSNoDiffuseNoNormalNoSpec = StaticCreateBindable<PixelShader>("NormalShader PS_NoDiffuseNoNormalNoSpec", TEXT("NormalShader.hlsl"), "PS_NoDiffuseNoNormalNoSpec");

		vecNormalNoDiffuseNoNormalNoSpec.push_back(pNormalVS);
		vecNormalNoDiffuseNoNormalNoSpec.push_back(pNormalPSNoDiffuseNoNormalNoSpec);
		vecNormalNoDiffuseNoNormalNoSpec.push_back(pInputLayoutTPNT);

		m_mapShader.insert(std::make_pair("Phong_NoDiffuseNoNormalNoSpec", vecNormalNoDiffuseNoNormalNoSpec));

		return true;
	}

	const std::vector<std::shared_ptr<Bindable>>* ShaderManager::FindShader(const std::string& strShader)	const
	{
		std::unordered_map<std::string, std::vector<class std::shared_ptr<class Bindable>>>::const_iterator iter = m_mapShader.find(strShader);

		if (iter == m_mapShader.end())
		{
			return nullptr;
		}

		return &iter->second;
	}
}