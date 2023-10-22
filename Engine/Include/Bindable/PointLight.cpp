#include "PointLight.h"
#include "TransformBuffer.h"
#include "Sphere.h"
#include "VertexBuffer.h"
#include "InputLayout.h"
#include "Topology.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "IndexBuffer.h"
#include "../Core/Graphics.h"
#include "BindableManager.h"
#include "../Render/RenderManager.h"

namespace Engine
{
	PointLight::PointLight() :
		Drawable()
		, pPointCBuffer(StaticFindBindable<PixelCBuffer<POINTLIGHT>>("PointLight"))
		, pVSPointCBuffer(StaticFindBindable<VertexCBuffer<POINTLIGHT>>("PointLight"))
	{
		m_tOrthoInfo.fLeft = -2500.f;
		m_tOrthoInfo.fRight = 2500.f;
		m_tOrthoInfo.fTop = 2500.f;
		m_tOrthoInfo.fBottom = -2500.f;
		m_tOrthoInfo.fNear = 0.1f;
		m_tOrthoInfo.fFar = 5000.f;

		SetBindableType(Engine::BINDABLE_TYPE::LIGHT);

		Reset();

		if (pPointCBuffer)
		{
			pPointCBuffer = std::static_pointer_cast<PixelCBuffer<POINTLIGHT>>(pPointCBuffer->Clone());
		}

		if (pVSPointCBuffer)
		{
			pVSPointCBuffer = std::static_pointer_cast<VertexCBuffer<POINTLIGHT>>(pVSPointCBuffer->Clone());

			pVSPointCBuffer->SetBuffer(pPointCBuffer->GetBuffer());
		}

		AddChild(pVSPointCBuffer);

		AddChild(pPointCBuffer);

		const std::shared_ptr<Drawable>& pChild = CreateBindable<Drawable>("sphere");

		if (pChild != nullptr)
		{
			std::string name = "Sphere";

			name += std::to_string(8);

			name += "_";

			name += std::to_string(8);

			//std::shared_ptr<VertexBuffer<VERTEX>> pVertexBuffer = StaticFindBindable<VertexBuffer<VERTEX>>(name);

			//if (pVertexBuffer == nullptr)
			//{
			//	std::vector<VERTEX> vecVertex;

			//	Sphere::CreateSphereVertex<VERTEX>(8, 8, vecVertex);

			//	pVertexBuffer = StaticCreateBindable<VertexBuffer<VERTEX>>(name, &vecVertex[0], static_cast<int>(vecVertex.size()));
			//}

			//pChild->AddBind(pVertexBuffer);

			//std::shared_ptr<IndexBuffer> pIndexBuffer = StaticFindBindable<IndexBuffer>(name);

			//if (pIndexBuffer == nullptr)
			//{
			//	std::vector<unsigned int> vecIndex;

			//	Sphere::CreateSphereIndex(8, 8, vecIndex);

			//	pIndexBuffer = StaticCreateBindable<IndexBuffer>(name, vecIndex);
			//}

			//pChild->AddBind(pIndexBuffer);

			//pChild->SetIndexBuffer(pIndexBuffer);

			std::shared_ptr<VertexShader> pVertexShader = StaticFindBindable<VertexShader>("PointLightVS");

			if (pVertexShader == nullptr)
			{
				pVertexShader = StaticCreateBindable<VertexShader>("VertexShader VS", TEXT("VertexShader.hlsl"), "VS");
			}

			pChild->AddChild(pVertexShader);

			std::shared_ptr<PixelShader> pPixelShader = StaticFindBindable<PixelShader>("MultiPS");

			if (pPixelShader == nullptr)
			{
				pPixelShader = StaticCreateBindable<PixelShader>("PixelShader PS_White", TEXT("PixelShader.hlsl"), "PS_White");
			}

			pChild->AddChild(pPixelShader);

			pChild->FindAndAddBind<HullShader>("PointLightHS");

			pChild->FindAndAddBind<DomainShader>("PointLightDS");

			std::shared_ptr<InputLayout> pInputLayout = StaticFindBindable<InputLayout>("P");

			if (pInputLayout == nullptr)
			{
				D3D11_INPUT_ELEMENT_DESC desc = { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

				pInputLayout = StaticCreateBindable<InputLayout>("P", pVertexShader, &desc, static_cast<int>(sizeof(desc) / sizeof(D3D11_INPUT_ELEMENT_DESC)));
			}

			pChild->AddChild(pInputLayout);

			pChild->AddChild(StaticFindBindable<Topology>("1ControlPointPatch"));
		}

		StartImGui();

		NotUseShadow();
	}

	void PointLight::SetLightType(LIGHT_TYPE eType)
	{
		tPointLight.eLightType = eType;
	}

	LIGHT_TYPE PointLight::GetLightType() const
	{
		return tPointLight.eLightType;
	}

	void PointLight::SetIntensity(float fIntensity)
	{
		tPointLight.fIntensity = fIntensity;
	}

	const std::shared_ptr<PixelCBuffer<POINTLIGHT>>& PointLight::GetLightCBuffer() const
	{
		return pPointCBuffer;
	}

	const Matrix& PointLight::GetView() const
	{
		return matView;
	}

	const Matrix& PointLight::GetViewProject() const
	{
		return matViewProject;
	}

	const ORTHOINFO& PointLight::GetOrthoInfo() const
	{
		return m_tOrthoInfo;
	}

	void PointLight::SetOrthoInfo(const ORTHOINFO& tInfo)
	{
		m_tOrthoInfo = tInfo;
	}

	float PointLight::GetIntensity() const
	{
		return tPointLight.fIntensity;
	}

	Vector4 PointLight::GetLightColor() const
	{
		return tPointLight.color;
	}

	Vector4 PointLight::GetAmbientColor() const
	{
		return tPointLight.ambientColor;
	}

	void PointLight::SetLightColor(const Vector4& vColor)
	{
		tPointLight.color = vColor;
	}

	void PointLight::SetAmbientColor(const Vector4& vColor)
	{
		tPointLight.ambientColor = vColor;
	}

	float PointLight::GetConstantAttenuation() const
	{
		return tPointLight.fConstantAttenuation;
	}

	float PointLight::GetLinearAttenuation() const
	{
		return tPointLight.fLinearAttenuation;
	}

	float PointLight::GetQuadraticAttenuation() const
	{
		return tPointLight.fQuadraticAttenuation;
	}

	void PointLight::SetConstantAttenuation(float fAttenuation)
	{
		tPointLight.fConstantAttenuation = fAttenuation;
	}

	void PointLight::SetLinearAttenuation(float fAttenuation)
	{
		tPointLight.fLinearAttenuation = fAttenuation;
	}

	void PointLight::SetQuadraticAttenuation(float fAttenuation)
	{
		tPointLight.fQuadraticAttenuation = fAttenuation;
	}

	void PointLight::Reset()
	{
		__super::Reset();

		tPointLight.ambientColor.x = 2 / 255.f;
		tPointLight.ambientColor.y = 46 / 255.f;
		tPointLight.ambientColor.z = 79 / 255.f;
		tPointLight.ambientColor.w = 1.f;

		tPointLight.color.x = 1.f;
		tPointLight.color.y = 1.f;
		tPointLight.color.z = 1.f;
		tPointLight.color.w = 1.f;

		tPointLight.fConstantAttenuation = 1.f;
		tPointLight.fLinearAttenuation = 0.045f;
		tPointLight.fQuadraticAttenuation = 0.0075f;
		tPointLight.fIntensity = 10.f;

		tPointLight.eLightType = LIGHT_TYPE::POINT;

		matView[0][3] = 0.f;
		matView[1][3] = 0.f;
		matView[2][3] = 0.f;
		matView[3][3] = 1.f;
	}

	void PointLight::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		const std::shared_ptr<TransformBuffer>& pLightTransform = GetTransform();

		if (!pLightTransform)
		{
			return;
		}

		const Vector3& vAxisX = pLightTransform->GetAxis(AXIS_TYPE::X);
		const Vector3& vAxisY = pLightTransform->GetAxis(AXIS_TYPE::Y);
		const Vector3& vAxisZ = pLightTransform->GetAxis(AXIS_TYPE::Z);

		const Vector3& vLightPos = pLightTransform->GetPosition();

		matView[0][0] = vAxisX.x;
		matView[0][1] = vAxisY.x;
		matView[0][2] = vAxisZ.x;
		matView[1][0] = vAxisX.y;
		matView[1][1] = vAxisY.y;
		matView[1][2] = vAxisZ.y;
		matView[2][0] = vAxisX.z;
		matView[2][1] = vAxisY.z;
		matView[2][2] = vAxisZ.z;
		matView[3].x = -vLightPos.Dot(vAxisX);
		matView[3].y = -vLightPos.Dot(vAxisY);
		matView[3].z = -vLightPos.Dot(vAxisZ);

		matViewProject = matView * Matrix::OthorGraphicLH(m_tOrthoInfo.fLeft, m_tOrthoInfo.fRight, m_tOrthoInfo.fTop, m_tOrthoInfo.fBottom, m_tOrthoInfo.fLeft, m_tOrthoInfo.fFar);
		// (R T)^-1 = T^-1 * R' =	1	0	0	0	*	Ux	Vx	Wx	0	=	Ux		Vx		Wx		0
		//							0	1	0	0		Uy	Vy	Wy	0		Uy		Vy		Wy		0
		//							0	0	1	0		Uz	Vz	Wz	0		Uz		Vz		Wz		0
		//							-x	-y	-z	1		0	0	0	1		-U.P	-V.P	-W.P	1

		tPointLight.pos = Graphics::GetInst()->GetView().TransformCoord(GetTransform()->GetPosition());
		tPointLight.dir = Graphics::GetInst()->GetView().TransformNormal(GetTransform()->GetAxis(AXIS_TYPE::Z));

		pVSPointCBuffer->UpdateBuffer(tPointLight);
	}

	void PointLight::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		RenderManager::GetInst()->AddLight(std::static_pointer_cast<PointLight>(std::enable_shared_from_this<CRef>::shared_from_this()));
	}

	void PointLight::Bind()
	{
		__super::Bind();
	}

	std::shared_ptr<Bindable> PointLight::Clone()
	{
		return std::make_shared<PointLight>(*this);
	}
}