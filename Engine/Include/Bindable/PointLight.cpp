#include "PointLight.h"
#include "Transform.h"
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
#include "Camera.h"

namespace Engine
{
	PointLight::PointLight() :
		Component()
		, pPointCBuffer(StaticFindBindable<ConstantBuffer<POINTLIGHT>>("PointLightCBuffer"))
		, tPointLight{}                  // zero-init: prevents garbage intensity/color
		, matView(Matrix::matIdentity)
		, matViewProject(Matrix::matIdentity)
	{
		SetComponentType(COMPONENT_TYPE::LIGHT);

		// Sensible defaults — without these, GameScene-created lights that
		// only call SetLightType end up with garbage in the cbuffer and the
		// deferred PS_Multi sees C = garbage * garbage → screen-wide blowout.
		tPointLight.color        = { 1.f, 1.f, 1.f, 1.f };
		tPointLight.ambientColor = { 0.2f, 0.2f, 0.2f, 1.f };
		tPointLight.dir          = { 0.3f, -0.7f, 0.5f };  // pointing down/forward
		tPointLight.fIntensity   = 1.f;
		tPointLight.fConstantAttenuation  = 1.f;
		tPointLight.fLinearAttenuation    = 0.f;
		tPointLight.fQuadraticAttenuation = 0.f;
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

	const std::shared_ptr<ConstantBuffer<POINTLIGHT>>& PointLight::GetLightCBuffer() const
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

	float PointLight::GetSpotConeExponent() const
	{
		return tPointLight.fSpotConeExponent;
	}

	void PointLight::SetSpotConeExponent(float fExponent)
	{
		tPointLight.fSpotConeExponent = fExponent;
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
		tPointLight.fIntensity = 1.f;
		// Default cone exponent: moderate spot. Inspector slider exposes
		// this so per-light tuning is straightforward.
		tPointLight.fSpotConeExponent = 8.f;

		tPointLight.eLightType = LIGHT_TYPE::POINT;

		matView[0][3] = 0.f;
		matView[1][3] = 0.f;
		matView[2][3] = 0.f;
		matView[3][3] = 1.f;
	}

	bool PointLight::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		m_tOrthoInfo.fLeft = -2500.f;
		m_tOrthoInfo.fRight = 2500.f;
		m_tOrthoInfo.fTop = 2500.f;
		m_tOrthoInfo.fBottom = -2500.f;
		m_tOrthoInfo.fNear = 0.1f;
		m_tOrthoInfo.fFar = 5000.f;

		Reset();

		m_pTransform = std::make_shared<Transform>();
		AddChild(m_pTransform);

		// Phase B.7 — old code created a child "sphere" Drawable with
		// VS/HS/DS/IL/Topology configured but never added a Mesh, so it
		// never actually rendered. Removed as dead code.

		return true;
	}

	void PointLight::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		const std::shared_ptr<Transform>& pLightTransform = GetTransform();

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

		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();

		if (pCamera)
		{
			const Matrix& matView = pCamera->GetView();

			tPointLight.pos = matView.TransformCoord(GetTransform()->GetPosition());
			tPointLight.dir = matView.TransformNormal(GetTransform()->GetAxis(AXIS_TYPE::Z));
		}
	}

	void PointLight::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		RenderManager::GetInst()->AddLight(std::static_pointer_cast<PointLight>(std::enable_shared_from_this<CRef>::shared_from_this()));
	}

	void PointLight::Bind()
	{
		pPointCBuffer->UpdateBuffer(tPointLight);
		pPointCBuffer->Bind();
		// Phase B.7 — Component has no Bind to delegate to. The CB upload
		// + bind is the entire job for the deferred lighting pass.
	}

	void PointLight::PostBind()
	{
		// No-op for now — RenderManager calls this for symmetry with the
		// old Drawable::Bind/PostBind pair, but light CB doesn't need a
		// cleanup step.
	}

	std::shared_ptr<Component> PointLight::Clone()
	{
		return std::make_shared<PointLight>(*this);
	}
	void PointLight::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&tPointLight, sizeof(POINTLIGHT), 1, pFile);
		fwrite(&m_tOrthoInfo, sizeof(ORTHOINFO), 1, pFile);

		bool bMainLight = Graphics::GetInst()->GetLight().get() == this;

		fwrite(&bMainLight, 1, 1, pFile);
	}
	void PointLight::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&tPointLight, sizeof(POINTLIGHT), 1, pFile);
		fread(&m_tOrthoInfo, sizeof(ORTHOINFO), 1, pFile);

		bool bMainLight = false;

		fread(&bMainLight, 1, 1, pFile);

		if (bMainLight)
		{
			Graphics::GetInst()->SetLight(std::static_pointer_cast<PointLight>(shared_from_this()));
		}
	}
}