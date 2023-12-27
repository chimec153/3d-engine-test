#include "Drawable.h"
#include "Bindable.h"
#include "../Core/Graphics.h"
#include "TransformBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"
#include "../Core/PathManager.h"
#include "VertexBuffer.h"
#include "FbxLoader.h"
#include "Texture.h"
#include "Box.h"
#include "../Shader/ShaderManager.h"
#include "BindableManager.h"
#include "../Render/RenderManager.h"
#include "PointLight.h"
#include "ColliderSphere.h"
#include "../Collision/CollisionManager.h"
#include "PixelShader.h"
#include "VertexShader.h"
#include "InputLayout.h"
#include "Topology.h"
#include "../Animation/Sequence.h"
#include "Animation.h"
#include "../Animation/Skeleton.h"
#include "Mesh.h"
#include "ColliderMesh.h"
#include "NavMesh.h"
#include "Agent.h"
#include "Mesh.h"

namespace Engine
{
	Drawable::Drawable() :
		Bindable()
		, m_pTransform(nullptr)
		, m_pMaterial(nullptr)
		, m_pMesh(nullptr)
		, m_pVertexShader(nullptr)
		, m_pPixelShader(nullptr)
		, m_pCollider(nullptr)
		, m_pAnimation(nullptr)
		, m_tSphereInfo()
		, m_bInLightViewFrustum(false)
		, m_eBoundingVolumeType()
		, m_bUseInstance(true)
		, m_bUseShadow(true)
		, m_pInstancing(nullptr)
		, m_iInstID(-1)
		, m_iParentJointCount(0)
		, m_eRenderLayer(RENDER_LAYER::OPACUE)
		, m_iInstanceKey()
	{
		SetBindableType(BINDABLE_TYPE::DRAWABLE);
		SetObjectType(OBJECT_TYPE::DRAW);
	}

	Drawable::Drawable(const Drawable& drawable) :
		Bindable(drawable)
		, m_pTransform(std::static_pointer_cast<Transform>(FindChild(BINDABLE_TYPE::TRANSFORM)))
		, m_pMaterial(std::static_pointer_cast<Material>(FindChild(BINDABLE_TYPE::MATERIAL)))
		, m_vecTexture(drawable.m_vecTexture)
		, m_pMesh(std::static_pointer_cast<Mesh>(FindChild(BINDABLE_TYPE::MESH)))
		, m_pVertexShader(drawable.m_pVertexShader)
		, m_pPixelShader(drawable.m_pPixelShader)
		, m_pCollider(std::static_pointer_cast<Collider>(FindChild(OBJECT_TYPE::COLLIDER)))
		, m_pAnimation(std::static_pointer_cast<Animation>(FindChild(BINDABLE_TYPE::ANIMATION)))
		, m_pAgent(drawable.m_pAgent ? std::static_pointer_cast<Agent>(drawable.m_pAgent->Clone()) : nullptr)
		, m_tSphereInfo(drawable.m_tSphereInfo)
		, m_bInLightViewFrustum(drawable.m_bInLightViewFrustum)
		, m_eBoundingVolumeType(drawable.m_eBoundingVolumeType)
		, m_bUseInstance(drawable.m_bUseInstance)
		, m_bUseShadow(drawable.m_bUseShadow)
		, m_pInstancing(nullptr)
		, m_iInstID(-1)
		, m_iParentJointCount(0)
		, m_eRenderLayer(drawable.m_eRenderLayer)
		, m_iInstanceKey(drawable.m_iInstanceKey)
	{
		SetTransform(m_pTransform);

		if (m_pAnimation)
		{
			m_pAnimation->SetOwner(this);
		}

		if (m_pAgent)
		{
			m_pAgent->SetTransform(GetTransform());
		}
	}

	Drawable::~Drawable()
	{
		CollisionManager::GetInst()->DeleteDrawable(this);
	}

	void Drawable::SetTransform(const std::shared_ptr<class Transform>& pTransform)
	{
		m_pTransform = pTransform;

		if (GetParent())
		{
			std::shared_ptr<Transform> pParentTransform = GetParent()->FindChild<Transform>();

			m_pTransform->SetParentTransform(pParentTransform.get());

			if (pParentTransform != nullptr)
			{
				pParentTransform->AddChildTransform(m_pTransform.get());
			}
		}

		std::list<std::shared_ptr<Bindable>>::const_iterator iter = GetChildList().begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = GetChildList().end();

		for (; iter != iterEnd; ++iter)
		{
			std::shared_ptr<Transform> pChildTransform = (*iter)->FindChild<Transform>();

			if (pChildTransform != nullptr)
			{
				pChildTransform->SetParentTransform(m_pTransform.get());

				if (m_pTransform != nullptr)
				{
					m_pTransform->AddChildTransform(pChildTransform.get());
				}
			}
		}
	}

	void Drawable::AddChild(const std::vector<std::shared_ptr<class Bindable>>& bind)
	{
		for (size_t i = 0; i < bind.size(); ++i)
		{
			AddChild(bind[i]);
		}
	}

	std::shared_ptr<Transform> Drawable::GetTransform() const
	{
		return m_pTransform;
	}

	std::shared_ptr<Material> Drawable::GetMaterial() const
	{
		return m_pMaterial;
	}

	void Drawable::SetMaterial(const std::shared_ptr<Material>& pMaterial)
	{
		m_pMaterial = pMaterial;

		UpdateInstanceKey();
	}

	void Drawable::AddChild(const std::shared_ptr<class Bindable>& bind)
	{
		assert(bind != nullptr);

		switch (bind->GetBindableType())
		{
		case BINDABLE_TYPE::MESH:
			SetMesh(std::static_pointer_cast<Mesh>(bind));
			break;
		case BINDABLE_TYPE::VERTEX_SHADER:
			SetVertexShader(std::static_pointer_cast<VertexShader>(bind));
			break;
		case BINDABLE_TYPE::PIXEL_SHADER:
			SetPixelShader(std::static_pointer_cast<PixelShader>(bind));
			break;
		case BINDABLE_TYPE::TEXTURE:
			AddTexture(std::static_pointer_cast<Texture>(bind));
			break;
		case BINDABLE_TYPE::MATERIAL:
			SetMaterial(std::static_pointer_cast<Material>(bind));
			break;
		case BINDABLE_TYPE::TRANSFORM:
			SetTransform(std::static_pointer_cast<Transform>(bind));
			break;
		case BINDABLE_TYPE::ANIMATION:
			SetAnimation(std::static_pointer_cast<Animation>(bind));
			break;
		default:
			break;
		}

		AddDrawable(bind);

		__super::AddChild(bind);
	}

	void Drawable::AddDrawable(const std::shared_ptr<class Bindable>& pChild)
	{
		std::vector<std::shared_ptr<Transform>> vecChildTransform;

		pChild->FindChilds<Transform>(vecChildTransform);

		for (int i = 0; i < static_cast<int>(vecChildTransform.size()); ++i)
		{
			vecChildTransform[i]->SetParentTransform(m_pTransform.get());

			if (m_pTransform != nullptr)
			{
				m_pTransform->AddChildTransform(vecChildTransform[i].get());
			}
		}
	}

	void Drawable::GetInstData(char* pData, int iSize)	const
	{
		if (!m_pTransform)
		{
			return;
		}

		const TRANSFORMBUFFER& tBuffer = m_pTransform->GetBuffer();

		memcpy_s(pData, iSize, &tBuffer, 192);
		pData += 192;
		iSize -= 192;

		if (iSize <= 0)
		{
			return;
		}

		std::shared_ptr<Material> pMaterial = m_pMaterial;

		if (!pMaterial)
		{
			if (m_pMesh)
			{
				pMaterial = m_pMesh->GetMaterial();
			}

			if (!pMaterial)
			{
				return;
			}
		}

		const MATERIAL& material = pMaterial->GetMaterial();

		memcpy_s(pData, iSize, &material.diffuseColor, 16);
		pData += 16;
		iSize -= 16;
		memcpy_s(pData, iSize, &material.specularColor, 16);
		pData += 16;
		iSize -= 16;
		memcpy_s(pData, iSize, &material.vRoughness, 8);
		pData += 8;
		iSize -= 8;
		memcpy_s(pData, iSize, &material.fFraction, 4);
		pData += 4;
		iSize -= 4;

		if (iSize <= 0)
		{
			return;
		}

		memcpy_s(pData, iSize, &tBuffer.matJoint, 64);
		pData += 64;
		iSize -= 64;

		memcpy_s(pData, iSize, &m_iInstID, 4);
		pData += 4;
		iSize -= 4;

		memcpy_s(pData, iSize, &tBuffer.iJointSocket, 4);
		pData += 4;
		iSize -= 4;

		memcpy_s(pData, iSize, &m_iParentJointCount, 4);
		pData += 4;
		iSize -= 4;

		return;
	}

	void Drawable::AddTexture(const std::shared_ptr<Texture>& pTexture)
	{
		m_vecTexture.push_back(pTexture);

		UpdateInstanceKey();
	}

	const std::vector<std::shared_ptr<Texture>>& Drawable::GetTextures() const
	{
		return m_vecTexture;
	}

	const std::shared_ptr<Mesh>& Drawable::GetMesh() const
	{
		return m_pMesh;
	}

	void Drawable::SetMesh(const std::shared_ptr<Mesh>& pBuffer)
	{
		m_pMesh = pBuffer;

		UpdateInstanceKey();
	}

	const std::shared_ptr<VertexShader>& Drawable::GetVertexShader() const
	{
		return m_pVertexShader;
	}

	void Drawable::SetVertexShader(const std::shared_ptr<VertexShader>& pShader)
	{
		m_pVertexShader = pShader;

		UpdateInstanceKey();
	}

	const std::shared_ptr<PixelShader>& Drawable::GetPixelShader() const
	{
		return m_pPixelShader;
	}

	void Drawable::SetPixelShader(const std::shared_ptr<PixelShader>& pShader)
	{
		m_pPixelShader = pShader;

		UpdateInstanceKey();
	}

	void Drawable::SetCollider(const std::shared_ptr<Collider>& pCollider)
	{
		m_pCollider = pCollider;
	}

	void Drawable::SetAnimation(std::shared_ptr<Animation> pAnimation)
	{
		m_pAnimation = pAnimation;

		if (m_pAnimation)
		{
			m_pAnimation->SetOwner(this);
		}

		UpdateInstanceKey();
	}

	std::shared_ptr<Animation> Drawable::GetAnimation() const
	{
		return m_pAnimation;
	}

	void Drawable::SetRenderLayer(RENDER_LAYER eLayer)
	{
		m_eRenderLayer = eLayer;
	}

	RENDER_LAYER Drawable::GetRenderLayer() const
	{
		return m_eRenderLayer;
	}

	void Drawable::AddSeqeunces(const std::vector<FbxLoader::SEQUENCE>& vecSequance, const std::string& strSeq)
	{
		for (size_t j = 0; j < vecSequance.size(); ++j)
		{
			std::shared_ptr<Sequence> pSequence = std::make_shared<Sequence>();

			std::vector<FbxLoader::FBXBONEKEYFRAME> vecKeyFrame;

			pSequence->SetTag(vecSequance[j].strTag + strSeq);

			if (vecKeyFrame.size() < vecSequance[j].vecBoneKeyFrame.size())
			{
				vecKeyFrame.resize(vecSequance[j].vecBoneKeyFrame.size());
			}

			for (size_t k = 0; k < vecSequance[j].vecBoneKeyFrame.size(); ++k)
			{
				for (size_t m = 0; m < vecSequance[j].vecBoneKeyFrame[k].vecKeyFrame.size(); ++m)
				{
					vecKeyFrame[k].vecKeyFrame.push_back(vecSequance[j].vecBoneKeyFrame[k].vecKeyFrame[m]);
				}
			}

			if (vecKeyFrame.empty())
			{
				continue;
			}

			if (!pSequence->SetSequance(vecKeyFrame))
			{
				continue;
			}

			m_pAnimation->AddSequance(pSequence->GetTag(), pSequence);
		}
	}

	void Drawable::SetAgent(std::shared_ptr<Engine::Agent> pAgent)
	{
		m_pAgent = pAgent;
	}

	void Drawable::Move(const Engine::Vector3& pos)
	{
		if (m_pAgent)
		{
			m_pAgent->SetTargetPos(pos);
		}
	}

	std::shared_ptr<Agent> Drawable::GetAgent() const
	{
		return m_pAgent;
	}

	size_t Drawable::GetInstanceKey() const
	{
		return m_iInstanceKey;
	}

	void Drawable::UpdateInstanceKey()
	{
		std::hash<std::string> hs;

		m_iInstanceKey = 1;

		m_iInstanceKey *= m_pMesh ? hs(m_pMesh->GetTag()) : 1;

		m_iInstanceKey *= m_pMaterial ? hs(m_pMaterial->GetTag()) : 1;

		m_iInstanceKey *= m_pVertexShader ? hs(m_pVertexShader->GetTag()) : 1;

		m_iInstanceKey *= m_pPixelShader ? hs(m_pPixelShader->GetTag()) : 1;

		m_iInstanceKey *= m_pAnimation ? hs(m_pAnimation->GetTag()) : 1;

		for (int i = 0; i < static_cast<int>(m_vecTexture.size()); ++i)
		{
			m_iInstanceKey *= hs(m_vecTexture[i]->GetTag());
		}
	}

	bool Drawable::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		AddChild(std::make_shared<Transform>());

		return true;
	}

	void Drawable::Start()
	{
		CollisionManager::GetInst()->AddDrawable(this);
	}

	void Drawable::Input(float fDeltaTime)
	{
		__super::Input(fDeltaTime);
	}

	void Drawable::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);
	}

	void Drawable::Collision(float fDeltaTime)
	{
		m_bInViewFrustum = true;

		if (m_pTransform && m_pTransform->GetVelocity() != 0.f)
		{
			CollisionManager::GetInst()->AddDrawable(this);
		}

		__super::Collision(fDeltaTime);
	}

	void Drawable::PreDraw(float fDeltaTime)
	{
		if (m_pTransform)
		{
			m_pTransform->SetVelocity({});
		}

		if (m_bUseShadow)
		{
			UpdateInLightViewFrustum();
		}

		RenderManager::GetInst()->AddDrawable(std::static_pointer_cast<Drawable>(shared_from_this()));

		__super::PreDraw(fDeltaTime);
	}

	void Drawable::Bind()
	{
		const std::list<std::shared_ptr<Bindable>>& ChildList = GetChildList();

		std::list<std::shared_ptr<Bindable>>::const_iterator iter = ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			switch ((*iter)->GetObjectType())
			{
			case Engine::OBJECT_TYPE::BIND:
			case Engine::OBJECT_TYPE::COLLIDER:
				(*iter)->Bind();
				break;
			}
		}

		if (m_pMesh)
		{
			m_pMesh->Draw();
		}

		PostBind();

	}

	void Drawable::DrawShadow()
	{
		BindExceptShader();

		if (m_pMesh)
		{
			m_pMesh->Draw();
		}

		if (m_pAnimation)
		{
			m_pAnimation->GetFinalBuffer()->ResetSRV(30);
		}
	}

	std::shared_ptr<Bindable> Drawable::Clone()
	{
		return std::make_shared<Drawable>(*this);
	}

	void Drawable::BindExceptShader()
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = GetChildList().begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = GetChildList().end();

		for (; iter != iterEnd; ++iter)
		{
			switch ((*iter)->GetBindableType())
			{
			case BINDABLE_TYPE::VERTEX_BUFFER:
			case BINDABLE_TYPE::INDEX_BUFFER:
			case BINDABLE_TYPE::TRANSFORM:
			case BINDABLE_TYPE::INPUTLAYOUT:
			case BINDABLE_TYPE::TOPOLOGY:
				(*iter)->Bind();
			}
		}

		if (m_pAnimation)
		{
			m_pAnimation->GetFinalBuffer()->SetSRV(30);
		}
	}

	void Drawable::PostBindExceptShader()
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = GetChildList().begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = GetChildList().end();

		for (; iter != iterEnd; ++iter)
		{
			switch ((*iter)->GetBindableType())
			{
			case BINDABLE_TYPE::VERTEX_BUFFER:
			case BINDABLE_TYPE::INDEX_BUFFER:
			case BINDABLE_TYPE::TRANSFORM:
			case BINDABLE_TYPE::INPUTLAYOUT:
			case BINDABLE_TYPE::TOPOLOGY:
				(*iter)->PostBind();
			}
		}
	}

	void Drawable::PostBind()
	{
		const std::list<std::shared_ptr<Bindable>>& ChildList = GetChildList();

		std::list<std::shared_ptr<Bindable>>::const_iterator iter = ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			switch ((*iter)->GetObjectType())
			{
			case Engine::OBJECT_TYPE::BIND:
				(*iter)->PostBind();
				break;
			}
		}
	}

	void Drawable::Reset()
	{
		if (m_pTransform == nullptr)
		{
			return;
		}

		m_pTransform->Reset();
	}

	void Drawable::CheckRangeAndMove()
	{
		m_pTransform->SetX(m_pTransform->GetX() < -50.f ? 0.f : m_pTransform->GetX());
		m_pTransform->SetY(m_pTransform->GetY() < -50.f ? 0.f : m_pTransform->GetY());
		m_pTransform->SetZ(m_pTransform->GetZ() < -50.f ? 0.f : m_pTransform->GetZ());

		m_pTransform->SetX(m_pTransform->GetX() > 50.f ? 0.f : m_pTransform->GetX());
		m_pTransform->SetY(m_pTransform->GetY() > 50.f ? 0.f : m_pTransform->GetY());
		m_pTransform->SetZ(m_pTransform->GetZ() > 50.f ? 0.f : m_pTransform->GetZ());
	}

	bool Drawable::UpdateInViewFrustum(const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* pvecLocalPlanes)
	{
		if (!m_pTransform)
		{
			return m_bInViewFrustum;
		}

		switch (m_eBoundingVolumeType)
		{
		case BOUNDING_VOLUME_TYPE::NONE:
			break;
		case BOUNDING_VOLUME_TYPE::SPHERE:
			m_bInViewFrustum = CollisionFrustumAndSphere(vecPlanes, m_tSphereInfo, pvecLocalPlanes);
			break;
		case BOUNDING_VOLUME_TYPE::BOX:
			break;
		case BOUNDING_VOLUME_TYPE::ELIPSOID:
			break;
		case BOUNDING_VOLUME_TYPE::CYLINDER:
			break;
		case BOUNDING_VOLUME_TYPE::END:
			break;
		default:
			break;
		}

		return m_bInViewFrustum;
	}

	bool Drawable::CollisionFrustumAndSphere(const std::vector<Vector4>& vecPlanes, const Vector4& tSphereInfo, const std::vector<Vector4>* pvecLocalPlanes) const
	{
		const Vector3& vPos = m_pTransform->GetTransformMatrix().TransformCoord(Vector3{ tSphereInfo.x, tSphereInfo.y, tSphereInfo.z });

		for (size_t i = 0; i < vecPlanes.size(); ++i)
		{
			if (vecPlanes[i].DotPoint(vPos) + tSphereInfo.w < 0.f)
			{
				return false;
			}
		}

		if (!pvecLocalPlanes)
		{
			return true;
		}

		for (size_t i = 0; i < pvecLocalPlanes->size(); ++i)
		{
			if ((*pvecLocalPlanes)[i].DotPoint(vPos) + tSphereInfo.w < 0.f)
			{
				return false;
			}
		}

		return true;
	}

	bool Drawable::CollisionFrustumAndBox(const OBBINFO& tBoxInfo) const
	{
		const Matrix& matWV = m_pTransform->GetTransformMatrix() * Graphics::GetInst()->GetView();

		const Vector3& vViewPos = matWV.TransformCoord(tBoxInfo.vCenter);

		Vector3 vAxis[3] =
		{
			matWV.TransformNormal(tBoxInfo.vAxis[0]),
			matWV.TransformNormal(tBoxInfo.vAxis[1]),
			matWV.TransformNormal(tBoxInfo.vAxis[2]),
		};

		Vector3 vNormalNear = { 0.f,0.f,1.f };

		float fRadiusNear = (abs(vNormalNear.Dot(vAxis[0])) + abs(vNormalNear.Dot(vAxis[1])) + abs(vNormalNear.Dot(vAxis[2]))) / 2.f;

		if (vViewPos.Dot(vNormalNear) + fRadiusNear < 0.f)
		{
			return false;
		}

		float fAngle = Graphics::GetInst()->GetAngle();

		float fCos = cosf(fAngle);
		float fSin = sinf(fAngle);

		Vector3 vNormalLeft = { fCos, 0.f, fSin };
		Vector3 vNormalRight = { -fCos, 0.f, fSin };

		float fRadiusLeft = (abs(vNormalLeft.Dot(vAxis[0])) + abs(vNormalLeft.Dot(vAxis[1])) + abs(vNormalLeft.Dot(vAxis[2]))) / 2.f;
		float fRadiusRight = (abs(vNormalRight.Dot(vAxis[0])) + abs(vNormalRight.Dot(vAxis[1])) + abs(vNormalRight.Dot(vAxis[2]))) / 2.f;

		if (vViewPos.Dot(vNormalLeft) + fRadiusLeft < 0.f ||
			vViewPos.Dot(vNormalRight) + fRadiusRight < 0.f)
		{
			return false;
		}

		float fRatio = Graphics::GetInst()->GetRatio();

		float fBeta = atanf(tanf(fAngle) / fRatio);

		Vector3 vNormalTop = { 0.f, -cosf(fBeta), sinf(fBeta) };
		Vector3 vNormalBottom = { 0.f, cosf(fBeta), sinf(fBeta) };

		float fRadiusTop = (abs(vNormalTop.Dot(vAxis[0])) + abs(vNormalTop.Dot(vAxis[1])) + abs(vNormalTop.Dot(vAxis[2]))) / 2.f;
		float fRadiusBottom = (abs(vNormalBottom.Dot(vAxis[0])) + abs(vNormalBottom.Dot(vAxis[1])) + abs(vNormalBottom.Dot(vAxis[2]))) / 2.f;

		return vViewPos.Dot(vNormalTop) + fRadiusTop >= 0.f &&
			vViewPos.Dot(vNormalBottom) + fRadiusBottom >= 0.f;
	}

	bool Drawable::CollisionFrustumAndElipsoid(const ELIPSOIDINFO& tElipsoid) const
	{
		const Matrix& matWV = m_pTransform->GetWV();

		const Vector3& vViewPos = matWV.TransformCoord(tElipsoid.vCenter);

		Vector3 vAxis[3] =
		{
			matWV.TransformNormal(tElipsoid.vRST[0]),
			matWV.TransformNormal(tElipsoid.vRST[1]),
			matWV.TransformNormal(tElipsoid.vRST[2]),
		};

		Vector3 vNormalNear = { 0.f, 0.f, 1.f };

		float fDot1 = vNormalNear.Dot(vAxis[0]);

		float fRadiusNear = sqrtf(fDot1 * fDot1);

		return false;
	}

	void Drawable::UpdateInLightViewFrustum()
	{
		m_bInLightViewFrustum = false;

		if (!m_pTransform)
		{
			return;
		}

		const std::shared_ptr<PointLight>& pLight = Graphics::GetInst()->GetLight();

		if (!pLight)
		{
			return;
		}

		const ORTHOINFO& tInfo = pLight->GetOrthoInfo();

		const Matrix& matWV = m_pTransform->GetTransformMatrix() * pLight->GetView();

		const Vector3& vViewPos = matWV.TransformCoord({ m_tSphereInfo.x, m_tSphereInfo.y, m_tSphereInfo.z });

		Vector4 vLeft = { 1.f, 0.f, 0.f, -tInfo.fLeft };
		Vector4 vRight = { -1.f, 0.f, 0.f, tInfo.fRight };

		Vector4 vBottom = { 0.f, 1.f, 0.f, tInfo.fBottom };
		Vector4 vTop = { 0.f, -1.f, 0.f, tInfo.fTop };

		Vector4 vNear = { 0.f,0.f,  1.f, -tInfo.fNear };
		Vector4 vFar = { 0.f, 0.f, -1.f, tInfo.fFar };

		if (vViewPos.x + m_tSphereInfo.w - tInfo.fLeft < 0.f)
		{
			return;
		}

		if (-vViewPos.x + m_tSphereInfo.w + tInfo.fRight < 0.f)
		{
			return;
		}

		if (vViewPos.y + m_tSphereInfo.w - tInfo.fBottom < 0.f)
		{
			return;
		}

		if (-vViewPos.y + m_tSphereInfo.w + tInfo.fTop < 0.f)
		{
			return;
		}

		if (vViewPos.z + m_tSphereInfo.w - tInfo.fNear < 0.f)
		{
			return;
		}

		if (-vViewPos.z + m_tSphereInfo.w + tInfo.fFar < 0.f)
		{
			return;
		}

		m_bInLightViewFrustum = true;
	}

	bool Drawable::IsInViewFrustum() const
	{
		return m_bInViewFrustum;
	}

	bool Drawable::IsInLightViewfFrustum() const
	{
		return m_bInLightViewFrustum;
	}

	void Drawable::InViewFrustum()
	{
		m_bInViewFrustum = true;
	}

	void Drawable::OutViewFrustum()
	{
		m_bInViewFrustum = false;
	}

	void Drawable::InLightViewFrustum()
	{
		m_bInLightViewFrustum = true;
	}

	void Drawable::OutLightViewFrustum()
	{
		m_bInLightViewFrustum = false;
	}

	void Drawable::Load(const TCHAR* pFileName, const std::string& strPathKey)
	{
		TCHAR strExt[_MAX_EXT] = {};

		_wsplitpath_s(pFileName, nullptr, 0, nullptr, 0, nullptr, 0, strExt, _MAX_EXT);

		_wcsupr_s(strExt);

		if (!wcscmp(strExt, TEXT(".OBJ")))
		{
			LoadOBJ(pFileName, strPathKey);
		}

		else if (!wcscmp(strExt, TEXT(".FBX")))
		{
			LoadFBX(pFileName, strPathKey);
		}

		else
		{
			assert(false);
		}
	}

	void Drawable::Load(const char* pFileName)
	{
		char strMesh[MAX_PATH] = {};

		strcpy_s(strMesh, pFileName);

		strcat_s(strMesh, ".mesh");

		std::shared_ptr<Mesh> pMesh = CreateBindable<Mesh>("mesh", strMesh, MESH_PATH);

		std::shared_ptr<Animation> pAnimation = CreateBindable<Animation>("Animation");

		std::shared_ptr<Sequence> pSequence = std::make_shared<Sequence>();

		char strSequence[MAX_PATH] = {};

		strcpy_s(strSequence, pFileName);

		strcat_s(strSequence, "Take 001.seq");

		pSequence->LoadFromPath(strSequence, MESH_PATH);

		std::shared_ptr<Skeleton> pSkeleton = std::make_shared<Skeleton>();

		char strSkeleton[MAX_PATH] = {};

		strcpy_s(strSkeleton, pFileName);

		strcat_s(strSkeleton, ".skel");

		pSkeleton->LoadFromPath(strSkeleton, MESH_PATH);

		pAnimation->SetSkeleton(pSkeleton);

		pAnimation->AddSequance(pSequence->GetTag(), pSequence);

		FindAndAddBind<VertexShader>("anisotropic_microfacet VSSkin");

		FindAndAddBind<PixelShader>("anisotropic_microfacet PS");

		FindAndAddBind<InputLayout>("Standard");

		FindAndAddBind<Topology>("TriangleList");
	}

	void Drawable::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_eRenderLayer, 4, 1, pFile);
		fwrite(&m_tSphereInfo, 16, 1, pFile);
		fwrite(&m_bInViewFrustum, 1, 1, pFile);
		fwrite(&m_bInLightViewFrustum, 1, 1, pFile);
		fwrite(&m_eBoundingVolumeType, 4, 1, pFile);
		fwrite(&m_bUseInstance, 1, 1, pFile);
		fwrite(&m_bUseShadow, 1, 1, pFile);

		Bindable* pParent = nullptr;

		bool bAgent = static_cast<bool>(m_pAgent);

		fwrite(&bAgent, 1, 1, pFile);

		if (bAgent)
		{
			int iLength = m_pAgent->GetTag().length();

			fwrite(&iLength, 4, 1, pFile);

			if (iLength)
			{
				fwrite(m_pAgent->GetTag().c_str(), 1, iLength, pFile);
			}
		}
	}

	void Drawable::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_eRenderLayer, 4, 1, pFile);
		fread(&m_tSphereInfo, 16, 1, pFile);
		fread(&m_bInViewFrustum, 1, 1, pFile);
		fread(&m_bInLightViewFrustum, 1, 1, pFile);
		fread(&m_eBoundingVolumeType, 4, 1, pFile);
		fread(&m_bUseInstance, 1, 1, pFile);
		fread(&m_bUseShadow, 1, 1, pFile);

		bool bAgent = false;

		fread(&bAgent, 1, 1, pFile);

		if (bAgent)
		{
			int iLength = 0;

			fread(&iLength, 4, 1, pFile);

			if (iLength)
			{
				std::unique_ptr<char[]> strTag = std::make_unique<char[]>(iLength + 1);

				strTag[iLength] = 0;

				fread(strTag.get(), 1, iLength, pFile);

				std::shared_ptr<Agent> pAgent = std::static_pointer_cast<Agent>(GetScene()->FindBindable(strTag.get()));

				if (pAgent)
				{
					m_pAgent = pAgent;
				}

				if (m_pAgent)
				{
					m_pAgent->SetTransform(GetTransform());
				}
			}
		}
	}

	void Drawable::Parse(const char* pResult)
	{
	}

	void Drawable::LoadOBJ(const TCHAR* pFileName, const std::string& strPathKey)
	{
		TCHAR strFullPath[MAX_PATH] = {};

		const TCHAR* pPath = CPathManager::GetInst()->FindPath(strPathKey);

		if (pPath)
		{
			wcscpy_s(strFullPath, pPath);
		}

		wcscat_s(strFullPath, pFileName);

		char strFull[MAX_PATH] = {};

#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, strFullPath, -1, strFull, MAX_PATH, 0, 0);
#else
		strcpy_s(strFull, strFullPath);
#endif

		FILE* pFile = nullptr;

		fopen_s(&pFile, strFull, "rt");

		if (pFile)
		{
			bool bSame = true;

			std::vector<VertexStandard> vecVertex;
			std::vector<std::vector<VertexStandard>> vecTotalVertex;
			std::vector<Vector3> vecPos;
			std::vector<unsigned int> vecSubIndex;
			std::vector<std::vector<unsigned int>> vecIndex;
			std::vector<std::vector<std::vector<unsigned int>>> vecTotalIndex;

			std::vector<std::shared_ptr<Texture>> vecTexture;
			std::vector<std::vector<std::shared_ptr<Texture>>> vecTotalTexture;

			std::vector<DirectX::XMFLOAT2> vecUV;
			std::vector<Vector3> vecNormal;

			bool bHasNormal = false;
			bool bHasUV = false;
			int iNormalCount = 0;

			std::vector<MATERIALINFO> vecMaterial;

			std::vector<MATERIALINFO> vecUseMaterial;

			int iPrevPos = 0;

			int iPrevUV = 0;

			int iPrevNormal = 0;

			bool bPath = true;

			int iTexture = 0;

			while (bPath)
			{
				char strLine[MAX_PATH] = {};

				char* pResult = fgets(strLine, MAX_PATH, pFile);

				if (!pResult || !strcmp(pResult, ""))
				{
					break;
				}

				switch (pResult[0])
				{
				case '#':
					break;
				case 'n':
				case 'o':
				case 'g':
				{
					char* pContext = nullptr;

					char* _pResult = strtok_s(pResult, " ", &pContext);

					if (pContext)
					{
						pContext[strlen(pContext) - 1] = 0;

						if (vecSubIndex.size())
						{
							iPrevPos = static_cast<int>(vecPos.size());
							iPrevUV = static_cast<int>(vecUV.size());
							iPrevNormal = static_cast<int>(vecNormal.size());

							vecIndex.push_back(vecSubIndex);

							std::vector<std::vector<unsigned int>> _vecSubIndex;

							_vecSubIndex.push_back(vecSubIndex);

							vecTotalIndex.push_back(_vecSubIndex);

							vecTotalTexture.push_back(vecTexture);

							vecTotalVertex.push_back(vecVertex);

							vecVertex.clear();

							vecSubIndex.clear();

							vecTexture.clear();
						}
					}
				}
				break;
				case 'v':

					switch (pResult[1])
					{
					case 't':
					{
						bHasUV = true;

						char* pContext = nullptr;
						char* _pResult = strtok_s(pResult, " ", &pContext);

						_pResult = strtok_s(nullptr, " ", &pContext);

						float fU = (float)atof(_pResult);

						_pResult = strtok_s(nullptr, " ", &pContext);

						float fV = (float)atof(_pResult);

						vecUV.push_back({ fU, fV });
					}
					break;
					case 'n':
					{
						bHasNormal = true;

						char* pContext = nullptr;
						char* _pResult = strtok_s(pResult, " ", &pContext);

						Vector3 vNormal;

						for (int i = 0; i < 3; ++i)
						{
							_pResult = strtok_s(nullptr, " ", &pContext);

							vNormal[i] = (float)atof(_pResult);
						}

						vecNormal.push_back(vNormal);

						++iNormalCount;
					}
					break;
					default:
					{
						char* pContext = nullptr;
						char* _pResult = strtok_s(pResult, " ", &pContext);

						Vector3 position;

						for (int i = 0; i < 3; ++i)
						{
							_pResult = strtok_s(nullptr, " ", &pContext);

							position[i] = (float)atof(_pResult);
						}

						vecPos.push_back(position);
					}
					break;
					}
					break;
				case 'f':
				{
					char* pContext = nullptr;
					char* _pResult = strtok_s(pResult, " ", &pContext);

					std::vector<int> vecVertexSub;

					while (true)
					{
						_pResult = strtok_s(nullptr, " ", &pContext);

						if (!_pResult)
						{
							break;
						}

						char* _pContext = nullptr;

						char* __pResult = strtok_s(_pResult, "/", &_pContext);

						char* __pResult2 = strtok_s(nullptr, "/", &_pContext);

						unsigned int iVertex = atoi(__pResult);

						if (!iVertex)
						{
							break;
						}

						unsigned int iIndex = 0;

						if (__pResult2)
						{
							iIndex = atoi(__pResult2);
						}

						unsigned int iNormalIndex = 0;

						if (_pContext)
						{
							iNormalIndex = atoi(_pContext);
						}

						if (bSame &&
							vecPos.size() - iPrevPos == vecUV.size() - iPrevUV &&
							vecNormal.size() - iPrevNormal == vecUV.size() - iPrevUV)
						{
							if (vecVertex.size() < iVertex)
							{
								vecVertex.resize(iVertex);
							}

							vecVertex[iVertex - 1].pos = vecPos[iVertex - 1];

							if (bHasUV && iIndex)
							{
								vecVertex[iVertex - 1].uv.x = vecUV[iIndex - 1].x;
								vecVertex[iVertex - 1].uv.y = 1.f - vecUV[iIndex - 1].y;
							}

							if (bHasNormal && iNormalIndex)
							{
								vecVertex[iVertex - 1].normal = vecNormal[iNormalIndex - 1];
							}

							vecVertexSub.push_back(iVertex - 1);
						}
						else
						{
							bSame = false;

							VertexStandard vVertex;

							vVertex.pos = vecPos[iVertex - 1];

							if (bHasUV)
							{
								vVertex.uv.x = vecUV[iIndex - 1].x;
								vVertex.uv.y = 1.f - vecUV[iIndex - 1].y;
							}

							if (bHasNormal)
							{
								vVertex.normal = vecNormal[iNormalIndex - 1];
							}

							vecVertexSub.push_back(static_cast<int>(vecVertex.size()));

							vecVertex.push_back(vVertex);
						}
					}

					switch (vecVertexSub.size())
					{
					case 3:
						for (size_t i = 0; i < vecVertexSub.size(); ++i)
						{
							vecSubIndex.push_back(vecVertexSub[i]);
						}

						break;
					case 4:
						vecSubIndex.push_back(vecVertexSub[0]);
						vecSubIndex.push_back(vecVertexSub[1]);
						vecSubIndex.push_back(vecVertexSub[2]);

						vecSubIndex.push_back(vecVertexSub[0]);
						vecSubIndex.push_back(vecVertexSub[2]);
						vecSubIndex.push_back(vecVertexSub[3]);
						break;
					default:
						assert(false);
						break;
					}

				}
				break;
				case 'm':
				{
					char* pContext = nullptr;

					char* _pResult = strtok_s(pResult, " ", &pContext);

					if (!strcmp(_pResult, "mtllib"))
					{
						int iLength = static_cast<int>(strlen(strFull));

						for (int i = iLength - 1; i >= 0; --i)
						{
							if (strFull[i] == '/' || strFull[i] == '\\')
							{
								memset(strFull + i + 1, 0, iLength - i);
								break;
							}
						}

						strcat_s(strFull, pContext);

						strFull[strlen(strFull) - 1] = 0;

						vecMaterial = LoadOBJMaterialFromFullPath(strFull);
					}
				}
				break;
				case 'u':
				{
					char* pContext = nullptr;

					char* _pResult = strtok_s(pResult, " ", &pContext);

					if (!strcmp(_pResult, "usemtl"))
					{
						bool bFind = false;

						pContext[strlen(pContext) - 1] = 0;

						for (size_t i = 0; i < vecMaterial.size(); ++i)
						{
							if (vecMaterial[i].pMaterial->GetTag() == pContext)
							{
								bFind = true;

								const std::shared_ptr<Material>& pMaterial = std::static_pointer_cast<Material>(vecMaterial[i].pMaterial->Clone());

								for (size_t j = 0; j < vecMaterial[i].vecTexture.size(); ++j)
								{
									iTexture |= 1 << static_cast<std::shared_ptr<Texture>>(vecMaterial[i].vecTexture[j])->GetSlot();

									vecTexture.push_back(vecMaterial[i].vecTexture[j]);
								}

								break;
							}
						}

						assert(bFind);
					}
				}
				break;
				default:
					break;
				}
			}

			std::vector<unsigned int> _vecIndex;

			for (size_t i = 0; i < vecIndex.size(); ++i)
			{
				size_t iSize = _vecIndex.size();
				size_t iAddSize = vecIndex[i].size();

				_vecIndex.resize(iSize + iAddSize);

				memcpy_s(&_vecIndex[iSize], 4 * iAddSize, &vecIndex[i][0], 4 * iAddSize);
			}

			if (!bHasNormal)
			{
				SetNormals<VertexStandard>(vecTotalVertex, vecIndex);
			}

			SetTangent<VertexStandard>(vecTotalVertex, vecIndex);

			const std::shared_ptr<Mesh>& pMesh = StaticCreateBindable<Mesh>(GetTag(), vecTotalVertex, vecTotalIndex);

			pMesh->SetTextures(vecTotalTexture);

			AddChild(pMesh);

			switch (iTexture)
			{
			case 0:
			{
				//const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong_NoDiffuseNoNormalNoSpec");
				const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoDiffuseNoNormalNoSpec");

				if (pvecBindable)
				{
					AddChild(*pvecBindable);
				}
			}
			break;
			case 1:
			{
				const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoSpecNoNormal");
				//const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong_NoNormalNoSpec");

				if (pvecBindable)
				{
					AddChild(*pvecBindable);
				}
			}
			break;
			case 1 | 2:
			{
				const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoSpec");

				if (pvecBindable)
				{
					AddChild(*pvecBindable);
				}
			}
			break;
			case 1 | 4:
			{
				const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoNormal");
				//const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong_NoNormal");

				if (pvecBindable)
				{
					AddChild(*pvecBindable);
				}
			}
			break;
			case 1 | 2 | 4:
			{
				const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet");
				//const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong");

				if (pvecBindable)
				{
					AddChild(*pvecBindable);
				}
			}
			break;
			default:
				assert(false);
				break;
			}

			const Vector4& vSphereInfo = GetBoundingSphere(vecVertex);

			SetBoundingSphereInfo(vSphereInfo);

			FindAndAddBind<Topology>("TriangleList");

			FindAndAddBind<Material>("Material");

			fclose(pFile);
		}
	}

	std::vector<Drawable::MATERIALINFO> Drawable::LoadOBJMaterial(const char* pFileName, const std::string& strPathKey)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPathKey);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFileName);

		return LoadOBJMaterialFromFullPath(strFullPath);
	}

	std::vector<Drawable::MATERIALINFO> Drawable::LoadOBJMaterialFromFullPath(const char* strFullPath)
	{
		std::vector<MATERIALINFO> vecMaterial;

		PMATERIALINFO pCurrentMaterial = nullptr;

		FILE* pFile = nullptr;

		fopen_s(&pFile, strFullPath, "rt");

		if (pFile)
		{
			while (true)
			{
				char _strLine[MAX_PATH] = {};

				fgets(_strLine, MAX_PATH, pFile);

				char* strLine = _strLine;

				if (!strcmp(strLine, ""))
				{
					break;
				}

				if (strLine[0] == '#')
				{
					continue;
				}

				else if (strLine[0] == '\t')
				{
					++strLine;
				}

				char* pContext = nullptr;

				char* pResult = strtok_s(strLine, " ", &pContext);

				if (!strcmp(pResult, "newmtl"))
				{
					vecMaterial.emplace_back();

					pCurrentMaterial = &vecMaterial.back();

					if (pContext)
					{
						pContext[strlen(pContext) - 1] = 0;

						pCurrentMaterial->pMaterial = StaticCreateBindable<Material>(pContext);
					}
				}

				else if (!strcmp(pResult, "Ns"))
				{
					pCurrentMaterial->pMaterial->SetShininess(static_cast<float>(atof(pContext)));
				}

				else if (!strcmp(pResult, "Ka"))
				{
					Vector3 vColor;

					for (int i = 0; i < 3; ++i)
					{
						char* pResult = strtok_s(nullptr, " ", &pContext);

						vColor[i] = static_cast<float>(atof(pResult));
					}

					pCurrentMaterial->pMaterial->SetAmbientColor(vColor.x, vColor.y, vColor.z, 1.f);
				}

				else if (!strcmp(pResult, "Kd"))
				{
					Vector3 vColor;

					for (int i = 0; i < 3; ++i)
					{
						char* pResult = strtok_s(nullptr, " ", &pContext);

						vColor[i] = static_cast<float>(atof(pResult));
					}

					pCurrentMaterial->pMaterial->SetDiffuseColor(vColor.x, vColor.y, vColor.z, 1.f);
				}

				else if (!strcmp(pResult, "Ks"))
				{
					Vector3 vColor;

					for (int i = 0; i < 3; ++i)
					{
						char* pResult = strtok_s(nullptr, " ", &pContext);

						vColor[i] = static_cast<float>(atof(pResult));
					}

					pCurrentMaterial->pMaterial->SetSpecularColor(vColor.x, vColor.y, vColor.z, 1.f);
				}

				else if (!strcmp(pResult, "Ke"))
				{
					Vector4 vColor = { 0.f, 0.f, 0.f, 1.f };

					for (int i = 0; i < 3; ++i)
					{
						char* pResult = strtok_s(nullptr, " ", &pContext);

						vColor[i] = static_cast<float>(atof(pResult));
					}

					pCurrentMaterial->pMaterial->SetEmissiveColor(vColor);
				}

				else if (!strcmp(pResult, "map_Kd"))
				{
					pResult = strtok_s(nullptr, " ", &pContext);

					char pFullPath[MAX_PATH] = {};

					strcpy_s(pFullPath, strFullPath);

					int iLength = static_cast<int>(strlen(pFullPath));

					for (int i = iLength - 1; i >= 0; --i)
					{
						if (pFullPath[i] == '/' || pFullPath[i] == '\\')
						{
							memset(pFullPath + i + 1, 0, iLength - i);
							break;
						}
					}

					strcat_s(pFullPath, pResult);

					pFullPath[strlen(pFullPath) - 1] = 0;

					std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(pFullPath);

					if (pTexture == nullptr)
					{
						pTexture = StaticCreateBindable<Texture>(pFullPath, pFullPath);
					}

					pCurrentMaterial->vecTexture.push_back(pTexture);
				}

				else if (!strcmp(pResult, "map_Kn"))
				{
					pResult = strtok_s(nullptr, " ", &pContext);

					char pFullPath[MAX_PATH] = {};

					strcpy_s(pFullPath, strFullPath);

					int iLength = static_cast<int>(strlen(pFullPath));

					for (int i = iLength - 1; i >= 0; --i)
					{
						if (pFullPath[i] == '/' || pFullPath[i] == '\\')
						{
							memset(pFullPath + i + 1, 0, iLength - i);
							break;
						}
					}

					strcat_s(pFullPath, pResult);

					pFullPath[strlen(pFullPath) - 1] = 0;

					std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(pFullPath);

					if (pTexture == nullptr)
					{
						pTexture = StaticCreateBindable<Texture>(pFullPath, pFullPath, 1);
					}

					pCurrentMaterial->vecTexture.push_back(pTexture);
				}

				else if (!strcmp(pResult, "map_Ks"))
				{
					pResult = strtok_s(nullptr, " ", &pContext);

					char pFullPath[MAX_PATH] = {};

					strcpy_s(pFullPath, strFullPath);

					int iLength = static_cast<int>(strlen(pFullPath));

					for (int i = iLength - 1; i >= 0; --i)
					{
						if (pFullPath[i] == '/' || pFullPath[i] == '\\')
						{
							memset(pFullPath + i + 1, 0, iLength - i);
							break;
						}
					}

					strcat_s(pFullPath, pResult);

					pFullPath[strlen(pFullPath) - 1] = 0;

					std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(pFullPath);

					if (pTexture == nullptr)
					{
						pTexture = StaticCreateBindable<Texture>(pFullPath, pFullPath, 2);
					}

					pCurrentMaterial->vecTexture.push_back(pTexture);
				}
			}

			fclose(pFile);
		}

		return vecMaterial;
	}

	void Drawable::LoadFBX(const TCHAR* pFileName, const std::string& strPathKey)
	{
		FbxLoader loader;

		if (!loader.Init())
		{
			return;
		}

		char strFilePath[MAX_PATH] = {};

#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, pFileName, -1, strFilePath, MAX_PATH, nullptr, nullptr);
#else
		strcpy_s(strFilePath, pFileName);
#endif

		char strFileName[_MAX_FNAME] = {};

		char _strExt[_MAX_EXT] = {};

		_splitpath_s(strFilePath, nullptr, 0, nullptr, 0, strFileName, _MAX_FNAME, _strExt, _MAX_EXT);

		int iLength = static_cast<int>(strlen(strFilePath));

		for (int i = iLength - 1; i >= 0; --i)
		{
			if (strFilePath[i] == '/' || strFilePath[i] == '\\')
			{
				// 11
				// asdf/ad.wzt
				// 01234567890
				memset(strFilePath + i + 1, 0, iLength - i - 1);
				break;
			}
		}

		loader.LoadFile(pFileName, strPathKey);

		const FbxLoader::SKELETON& tSkeleton = loader.GetSkeleton();

		std::shared_ptr<Skeleton> pSkeleton = nullptr;

		if (tSkeleton.vecBone.size())
		{
			pSkeleton = std::make_shared<Skeleton>();

			pSkeleton->SetTag(strFileName);

			pSkeleton->SetBone(tSkeleton.vecBone);

			char strSkelPath[MAX_PATH] = {};

			strcat_s(strSkelPath, strFileName);

			strcat_s(strSkelPath, ".skel");

			pSkeleton->SaveFromPath(strSkelPath, MESH_PATH);
		}

		std::vector<std::vector<VertexStandard>> vecVertex;
		std::vector<std::vector<std::vector<unsigned int>>> vecIndex;

		for (int i = 0; i < loader.GetLODCount(); ++i)
		{
			vecVertex.push_back(loader.GetVertexData(i));
			vecIndex.push_back(loader.GetIndexData(i));
		}

		for (int i = 0; i< vecIndex.size(); ++i)
		{
			for (int j = 0; j < vecIndex[i].size(); ++j)
			{
				for (int k = 0; k < vecVertex.size(); ++k)
				{
					SetTangent(vecVertex[k], vecIndex[i][j]);
				}
			}
		}

		std::shared_ptr<Mesh> pMesh = StaticCreateBindable<Mesh>(strFileName, vecVertex, vecIndex);

		AddChild(pMesh);

		GetBoundingSphere<VertexStandard>(vecVertex);

		if (tSkeleton.vecBone.size())
		{
			CreateBindable<class Animation>("Animation");

			//int iLodCount = loader.GetLODCount();

			//for (int i = 0; i < iLodCount; ++i)
			//{
				const std::vector<FbxLoader::SEQUENCE>& vecSequance = loader.GetSequences();

				AddSeqeunces(vecSequance);
			//}

			const std::unordered_map<std::string, Animation::PSEQUENCEINFO>& mapSequence = m_pAnimation->GetSequences();

			std::unordered_map<std::string, Animation::PSEQUENCEINFO>::const_iterator iter = mapSequence.begin();
			std::unordered_map<std::string, Animation::PSEQUENCEINFO>::const_iterator iterEnd = mapSequence.end();

			for (; iter != iterEnd; ++iter)
			{
				char strSeqPath[MAX_PATH] = {};

				strcat_s(strSeqPath, strFileName);

				strcat_s(strSeqPath, iter->second->pSequence->GetTag().c_str());

				strcat_s(strSeqPath, ".seq");

				char* pPos = strstr(strSeqPath, "|");

				if (pPos)
				{
					*pPos = '_';
				}

				iter->second->pSequence->SaveFromPath(strSeqPath, MESH_PATH);
			}

			//for (int i = 0; i < loader.GetLODCount(); ++i)
			//{
			//	AddSeqeunces(loader.GetSequences(i), "_test");
			//}

			m_pAnimation->SetSkeleton(pSkeleton);

			FindAndAddBind<VertexShader>("anisotropic_microfacet VSSkin");
			FindAndAddBind<InputLayout>("Standard");
		}
		else
		{
			FindAndAddBind<VertexShader>("anisotropic_microfacet VSNoSkin");
			FindAndAddBind<InputLayout>("Standard");
		}
		FindAndAddBind<Topology>("TriangleList");

		std::vector<std::vector<std::shared_ptr<Texture>>> _vecTexture;

		std::vector<std::vector<std::shared_ptr<Material>>> _vecMaterial(loader.GetLODCount());

		int iTextureCount = 0;

		for (int i = 0; i < loader.GetLODCount(); ++i)
		{
			const std::vector<FbxLoader::TEXTUREINFO>& vecTextureInfo = loader.GetTextures(i);

			std::vector<std::shared_ptr<Texture>> vecTexture;

			for (size_t j = 0; j < vecTextureInfo.size(); ++j)
			{
				TCHAR strFullPath[MAX_PATH] = {};

#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, 0, vecTextureInfo[j].strFullPath.c_str(), -1, strFullPath, MAX_PATH);
#else
				strcpy_s(strFullPath, vecTextureInfo[i].strFullPath.c_str());
#endif

				std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(vecTextureInfo[j].strFullPath);

				if (pTexture != nullptr)
				{
					vecTexture.push_back(pTexture);
					continue;
				}

				++iTextureCount;

				switch (vecTextureInfo[j].type)
				{
				case fbxsdk::FbxLayerElement::EType::eTextureDiffuse:
					vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 0));
					break;
				case fbxsdk::FbxLayerElement::EType::eTextureEmissive:
					vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 3));
					break;
				case fbxsdk::FbxLayerElement::EType::eTextureAmbient:
					assert(false);
					break;
				case fbxsdk::FbxLayerElement::EType::eTextureSpecular:
					vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 2));
					break;
				case fbxsdk::FbxLayerElement::EType::eTextureNormalMap:
				case fbxsdk::FbxLayerElement::EType::eTextureBump:
					vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 1));
					break;
				default:
					break;
				}
		}

			pMesh->SetTextures(i, vecTexture);

			_vecTexture.push_back(vecTexture);

			const std::vector<FbxLoader::MATERIALINFO>& vecMaterial = loader.GetMaterials(i);

			for (int k = 0; k < vecMaterial.size(); ++k)
			{
				std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>(vecMaterial[k].name);

				if (pMaterial == nullptr)
				{
					pMaterial = StaticCreateBindable<Material>(vecMaterial[k].name);

					pMaterial->SetDiffuseColor(vecMaterial[k].tMaterial.diffuseColor);
					pMaterial->SetAmbientColor(vecMaterial[k].tMaterial.ambientColor);
					pMaterial->SetSpecularColor(vecMaterial[k].tMaterial.specularColor);
					pMaterial->SetEmissiveColor(vecMaterial[k].tMaterial.emissiveColor);
					pMaterial->SetShininess(vecMaterial[k].tMaterial.fSpecPower);
					//pMaterial->SetReflectivity(vecMaterial[k].tMaterial.fFraction);
					pMaterial->SetReflectivity(1.f);

					pMaterial = std::static_pointer_cast<Material>(pMaterial->Clone());

					pMesh->AddMaterial(i, pMaterial);
				}
				else
				{
					pMaterial = std::static_pointer_cast<Material>(pMaterial->Clone());

					pMesh->AddMaterial(i, pMaterial);
				}

				_vecMaterial[i].push_back(pMaterial);
			}
	}

		if (iTextureCount > 0)
		{
			FindAndAddBind<PixelShader>("anisotropic_microfacet PS");
		}
		else
		{
			FindAndAddBind<PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
		}

		char strFullPath[MAX_PATH] = {};

		strcat_s(strFullPath, strFileName);

		strcat_s(strFullPath, ".mesh");

		SaveMesh(vecVertex, vecIndex, _vecTexture, _vecMaterial, strFullPath);
}

	void Drawable::SaveMesh(const std::vector<std::vector<VertexStandard>>& vecVertex, const std::vector<std::vector<std::vector<unsigned int>>>& vecIndex,
		const std::vector<std::vector<std::shared_ptr<Texture>>>& vecTexture, const std::vector<std::vector<std::shared_ptr<Material>>>& vecMaterial, const char* pFilePath, const std::string& strPathKey)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPathKey);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		FILE* pFile = nullptr;

		fopen_s(&pFile, strFullPath, "wb");

		if (pFile)
		{
			int iContainerSize = static_cast<int>(vecVertex.size());

			fwrite(&iContainerSize, 4, 1, pFile);

			for (int i = 0; i < iContainerSize; ++i)
			{
				int iVertexSize = static_cast<int>(vecVertex[i].size());

				fwrite(&iVertexSize, 4, 1, pFile);
				fwrite(&vecVertex[i][0], sizeof(VertexStandard), vecVertex[i].size(), pFile);

				short iIndexSize = static_cast<short>(vecIndex[i].size());

				fwrite(&iIndexSize, 2, 1, pFile);

				for (int j = 0; j < iIndexSize; ++j)
				{
					int iIndexCount = static_cast<int>(vecIndex[i][j].size());

					fwrite(&iIndexCount, 4, 1, pFile);

					if (iIndexCount)
					{
						fwrite(&vecIndex[i][j][0], 4, vecIndex[i][j].size(), pFile);
					}
				}

				int iTextureCount = static_cast<int>(vecTexture[i].size());

				fwrite(&iTextureCount, 4, 1, pFile);

				for (int j = 0; j < iTextureCount; ++j)
				{
					vecTexture[i][j]->Save(pFile);
				}

				int iMaterial = static_cast<int>(vecMaterial[i].size());

				fwrite(&iMaterial, 1, 4, pFile);

				for (int j = 0; j < iMaterial; ++j)
				{
					if (vecMaterial[i][j])
					{
						vecMaterial[i][j]->Save(pFile);
					}
					else
					{
						assert(false);
					}
				}
			}

			fclose(pFile);
		}
	}

	void Drawable::LoadMesh(const char* pFilePath, const std::string& strPathKey)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPathKey);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		FILE* pFile = nullptr;

		fopen_s(&pFile, strFullPath, "rb");

		if (pFile)
		{
			unsigned int iContainerCount;

			fread(&iContainerCount, 4, 1, pFile);

			std::vector<std::vector<VertexStandard>> vecVertex(iContainerCount);
			std::vector<std::vector<std::vector<unsigned int>>> vecIndex(iContainerCount);

			for (unsigned int i = 0; i < iContainerCount; ++i)
			{
				unsigned int iVertexCount;

				fread(&iVertexCount, 4, 1, pFile);

				vecVertex[i].resize(iVertexCount);

				fread(&vecVertex[i][0], sizeof(VertexStandard), iVertexCount, pFile);

				short iIndexGroupCount;

				fread(&iIndexGroupCount, 2, 1, pFile);

				for (short j = 0; j < iIndexGroupCount; ++j)
				{
					unsigned int iIndexCount;

					fread(&iIndexCount, 4, 1, pFile);

					if (iIndexCount)
					{
						vecIndex[i].resize(iIndexCount);

						fread(&vecIndex[i][0], 4, iIndexCount, pFile);
					}
				}
			}

			char strName[_MAX_FNAME] = {};

			_splitpath_s(pFilePath, nullptr, 0, nullptr, 0, strName, _MAX_FNAME, nullptr, 0);

			StaticCreateBindable<Mesh>(strName, vecVertex, vecIndex);

			fclose(pFile);
		}
	}

	void Drawable::SetBoundingSphereInfo(const Vector4& vInfo)
	{
		m_tSphereInfo = vInfo;
	}

	const Vector4& Drawable::GetSphereInfo() const
	{
		return m_tSphereInfo;
	}

	bool Drawable::UseInstance() const
	{
		return m_bUseInstance;
	}

	void Drawable::NotUseInstance()
	{
		m_bUseInstance = false;
	}

	void Drawable::NotUseShadow()
	{
		m_bUseShadow = false;
	}
	void Drawable::SetInstancing(RenderInstancing* pInstancing)
	{
		m_pInstancing = pInstancing;
	}
	RenderInstancing* Drawable::GetInstancing() const
	{
		return m_pInstancing;
	}
	int Drawable::GetInstID() const
	{
		return m_iInstID;
	}
	void Drawable::SetInstID(int iID)
	{
		m_iInstID = iID;
	}
	void Drawable::SetParentJointCount(int iCount)
	{
		m_iParentJointCount = iCount;
	}
}