#include "Cloth.h"
#include "Mesh.h"
#include "BindableManager.h"
#include "ColliderSphere.h"

Engine::Cloth::Cloth()
{
}

Engine::Cloth::Cloth(int iWidth, int iHeight, float fSpring, float fSpringShear, float fSpringDistance, float fDamper, float fDamperShear, float fDamperDistance, float fDistance, float fMass)	:
	m_iWidth(iWidth)
	, m_iHeight(iHeight)
	, m_fSpring(fSpring)
	, m_fSpringShear(fSpringShear)
	, m_fSpringDistance(fSpringDistance)
	, m_fDamper(fDamper)
	, m_fDamperShear(fDamperShear)
	, m_fDamperDistance(fDamperDistance)
	, m_fDistance(fDistance)
	, m_fMass(fMass)
	, m_bSwitch(false)
	, m_fWind(0.f)
	, m_vWind()
	, m_pCollider(CreateBindable<ColliderSphere>("ClothCollider"))
{
	SetBindableType(BINDABLE_TYPE::CLOTH);

	m_vecPosition.resize(m_iWidth * m_iHeight);
	m_vecPrevPosition.resize(m_iWidth * m_iHeight);
	m_vecVelocity.resize(m_iWidth * m_iHeight);
	m_vecForce.resize(m_iWidth * m_iHeight);

	FindAndAddBind<class Topology>("TriangleList");
	FindAndAddBind<class InputLayout>(STANDARD_INPUT_LAYOUT);
	FindAndAddBind<VertexShader>("anisotropic_microfacet VSNoSkin");
	FindAndAddBind<PixelShader>("anisotropic_microfacet PS_NoSpecMapNoNormalMap");

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	AddChild(pMaterial->Clone());

#ifdef _DEBUG
	FindAndAddBind<RasterizerState>("CullNone");
#else
	FindAndAddBind<RasterizerState>("CullNone");
#endif

	Ready();
}

void Engine::Cloth::SetWindHeavyness(float fWind)
{
	m_fWind = fWind;
}
void Engine::Cloth::SetWind(const Vector3& vWind)
{
	m_vWind = vWind;
}
float Engine::Cloth::GetWindHeavyness() const
{
	return m_fWind;
}
//F = k * (|Q - P| - d) * (Q - P)/|Q - P|
// F = k * (dQ / dt - dP / dt)
// d, 2d, d * sqrt(2)
//F = mg + k * (W - V) . N
// P' = P0 + V0 * d_t + F / m * d_t^2 / 2
void Engine::Cloth::FixedUpdate(float fDeltaTime)
{
	UpdateForce();

	UpdatePosition(fDeltaTime);

	m_bSwitch ^= true;
}

void Engine::Cloth::Save(FILE* pFile)
{
	__super::Save(pFile);

	fwrite(&m_iWidth, 4, 1, pFile);
	fwrite(&m_iHeight, 4, 1, pFile);

	if (m_iWidth * m_iHeight > 0)
	{
		fwrite(&m_vecPosition[0], sizeof(VertexStandard), m_iWidth * m_iHeight, pFile);
		fwrite(&m_vecPrevPosition[0], sizeof(VertexStandard), m_iWidth * m_iHeight, pFile);
		fwrite(&m_vecVelocity[0], 12, m_iWidth * m_iHeight, pFile);
		fwrite(&m_vecForce[0], 12, m_iWidth * m_iHeight, pFile);

		if (m_iWidth > 1 && m_iHeight > 1)
		{
			fwrite(&m_vecIndex[0], 4, (m_iWidth - 1) * (m_iHeight - 1) * 6, pFile);
		}
	}

	fwrite(&m_fSpring, 4, 1, pFile);
	fwrite(&m_fSpringShear, 4, 1, pFile);
	fwrite(&m_fSpringDistance, 4, 1, pFile);
	fwrite(&m_fDamper, 4, 1, pFile);
	fwrite(&m_fDamperShear, 4, 1, pFile);
	fwrite(&m_fDamperDistance, 4, 1, pFile);
	fwrite(&m_fDistance, 4, 1, pFile);
	fwrite(&m_fMass, 4, 1, pFile);
	fwrite(&m_bSwitch, 1, 1, pFile);
	fwrite(&m_fWind, 4, 1, pFile);
	fwrite(&m_vWind, 12, 1, pFile);
}

void Engine::Cloth::Load(FILE* pFile)
{
	__super::Load(pFile);

	fread(&m_iWidth, 4, 1, pFile);
	fread(&m_iHeight, 4, 1, pFile);

	if (m_iWidth * m_iHeight > 0)
	{
		m_vecPosition.resize(m_iWidth * m_iHeight);
		m_vecPrevPosition.resize(m_iWidth * m_iHeight);
		m_vecVelocity.resize(m_iWidth * m_iHeight);
		m_vecForce.resize(m_iWidth * m_iHeight);

		fread(&m_vecPosition[0], sizeof(VertexStandard), m_iWidth * m_iHeight, pFile);
		fread(&m_vecPrevPosition[0], sizeof(VertexStandard), m_iWidth * m_iHeight, pFile);
		fread(&m_vecVelocity[0], sizeof(VertexStandard), m_iWidth * m_iHeight, pFile);
		fread(&m_vecForce[0], sizeof(VertexStandard), m_iWidth * m_iHeight, pFile);

		if (m_iWidth > 1 && m_iHeight > 1)
		{
			m_vecIndex.resize((m_iWidth - 1) * (m_iHeight - 1) * 6);

			fread(&m_vecIndex[0], 4, (m_iWidth - 1) * (m_iHeight - 1) * 6, pFile);
		}
	}

	fread(&m_fSpring, 4, 1, pFile);
	fread(&m_fSpringShear, 4, 1, pFile);
	fread(&m_fSpringDistance, 4, 1, pFile);
	fread(&m_fDamper, 4, 1, pFile);
	fread(&m_fDamperShear, 4, 1, pFile);
	fread(&m_fDamperDistance, 4, 1, pFile);
	fread(&m_fDistance, 4, 1, pFile);
	fread(&m_fMass, 4, 1, pFile);
	fread(&m_bSwitch, 1, 1, pFile);
	fread(&m_fWind, 4, 1, pFile);
	fread(&m_vWind, 12, 1, pFile);

	m_pCollider = std::static_pointer_cast<ColliderSphere>(FindChild("ClothCollider"));

	assert(m_pCollider);

	Ready();
}

Engine::Vector3 Engine::Cloth::GetSpringForce(int iSrcIndex, int iDestIndex, float fSpring, float fDist) const
{
	if (m_bSwitch)
	{
		const Vector3& vDist = (m_vecPrevPosition[iDestIndex].pos - m_vecPrevPosition[iSrcIndex].pos);

		float fLength = vDist.Length();

		return fSpring * (vDist.Length() - fDist) * vDist / fLength;
	}
	else
	{
		const Vector3& vDist = (m_vecPosition[iDestIndex].pos - m_vecPosition[iSrcIndex].pos);

		float fLength = vDist.Length();

		return fSpring * (vDist.Length() - fDist) * vDist / fLength;
	}
}

void Engine::Cloth::ApplySpringForce(int iSrcIndex, int iDestIndex, float fSpring, float fDist)
{
	const Vector3& vForce = GetSpringForce(iSrcIndex, iDestIndex, fSpring, fDist);

	m_vecForce[iSrcIndex] += vForce;

	if (iSrcIndex % m_iWidth == 0) {
		m_vecForce[iDestIndex] -= vForce * 2.f;
	}
	else {
		m_vecForce[iDestIndex] -= vForce;
	}
}

void Engine::Cloth::ApplyDampForce(int iSrcIndex, int iDestIndex, float fDamp)
{
	const Vector3& vForce = fDamp * (m_vecVelocity[iDestIndex] - m_vecVelocity[iSrcIndex]);

	m_vecForce[iSrcIndex] += vForce;

	if (iSrcIndex % m_iWidth == 0) {
		m_vecForce[iDestIndex] -= vForce * 2.f;
	}
	else {
		m_vecForce[iDestIndex] -= vForce;
	}
}

void Engine::Cloth::UpdateForce()
{
	memset(&m_vecForce[0].x, 0, 12 * m_iWidth * m_iHeight);

	for (int j = 0; j < m_iHeight; ++j)
	{
		for (int i = 0; i < m_iWidth; ++i)
		{
			int iIndex = i + j * m_iWidth;

			if (i < m_iWidth - 1)
			{
				int iRightIndex = iIndex + 1;

				ApplySpringForce(iIndex, iRightIndex, m_fSpring, m_fDistance);

				ApplyDampForce(iIndex, iRightIndex, m_fDamper);

				if (i < m_iWidth - 2)
				{
					ApplySpringForce(iIndex, iRightIndex + 1, m_fSpringDistance, m_fDistance * 2.f);

					ApplyDampForce(iIndex, iRightIndex + 1, m_fDamperDistance);
				}

				if (j < m_iHeight - 1)
				{
					int iDownIndex = iIndex + m_iWidth;

					ApplySpringForce(iIndex, iDownIndex, m_fSpring, m_fDistance);

					ApplyDampForce(iIndex, iDownIndex, m_fDamper);

					if (j < m_iHeight - 2)
					{
						ApplySpringForce(iIndex, iDownIndex + m_iWidth, m_fSpringDistance, m_fDistance * 2.f);

						ApplyDampForce(iIndex, iDownIndex + m_iWidth, m_fDamperDistance);
					}

					int iRightDownIndex = iRightIndex + m_iWidth;

					ApplySpringForce(iIndex, iRightDownIndex, m_fSpringShear, m_fDistance * 1.41421354f);

					ApplyDampForce(iIndex, iRightDownIndex, m_fDamperShear);
				}
			}
			else if (j < m_iHeight - 1)
			{
				int iDownIndex = iIndex + m_iWidth;

				ApplySpringForce(iIndex, iDownIndex, m_fSpring, m_fDistance);

				ApplyDampForce(iIndex, iDownIndex, m_fDamper);

				if (j < m_iHeight - 2)
				{
					ApplySpringForce(iIndex, iDownIndex + m_iWidth, m_fSpringDistance, m_fDistance * 2.f);

					ApplyDampForce(iIndex, iDownIndex + m_iWidth, m_fDamperDistance);
				}
			}

			if (i != 0 && j != m_iHeight - 1)
			{
				ApplySpringForce(iIndex, iIndex - 1 + m_iWidth, m_fSpringShear, m_fDistance * 1.41421354f);

				ApplyDampForce(iIndex, iIndex - 1 + m_iWidth, m_fDamperShear);
			}
		}
	}
}

void Engine::Cloth::UpdatePosition(float fDeltaTime)
{
	std::vector<VertexStandard>& vecNewPosition = GetPrevVertexs();
	std::vector<VertexStandard>& vecCurrentPosition = GetCurrentVertexs();

	for (int i = 0; i < static_cast<int>(vecNewPosition.size()); ++i)
	{
		float fNormalLength = vecCurrentPosition[i].normal.Length();

		const Vector3& vForce = m_fMass * Vector3(0.f, -9.81f, 0.f) + (fNormalLength ? m_fWind * (m_vWind - m_vecVelocity[i]).Dot(vecCurrentPosition[i].normal) * vecCurrentPosition[i].normal / fNormalLength : Vector3());

		if (i % m_iWidth == 0)
		{
			vecNewPosition[i].pos = vecCurrentPosition[i].pos;
		}
		else
		{
			vecNewPosition[i].pos = vecCurrentPosition[i].pos + m_vecVelocity[i] * fDeltaTime + fDeltaTime * fDeltaTime / 2.f * (m_vecForce[i] + vForce) / m_fMass;
		}

		m_vecVelocity[i] = (vecNewPosition[i].pos - vecCurrentPosition[i].pos) / fDeltaTime;
	}

	SetNormals(vecNewPosition, m_vecIndex);

	SetTangent(vecNewPosition, m_vecIndex);

	const Engine::Vector4& vSphere = GetBoundingSphere<VertexStandard>(vecNewPosition);

	m_pCollider->SetRadius(vSphere.w);
	m_pCollider->SetOffset(vSphere);

	m_pMesh->SetVertexBuffer(0, &vecNewPosition[0], static_cast<int>(sizeof(VertexStandard) * vecNewPosition.size()));
}

void Engine::Cloth::CreateVertexAndIndex()
{
	std::vector<VertexStandard>& vecVertex = GetCurrentVertexs();

	for (int j = 0; j < m_iHeight; ++j)
	{
		for (int i = 0; i < m_iWidth; ++i)
		{
			vecVertex[i + j * m_iWidth].pos.x = i * m_fDistance;
			vecVertex[i + j * m_iWidth].pos.y = (m_iHeight - 1 - j) * m_fDistance;

			vecVertex[i + j * m_iWidth].uv.x = i / static_cast<float>(m_iWidth - 1);
			vecVertex[i + j * m_iWidth].uv.y = j / static_cast<float>(m_iHeight - 1);
		}
	}

	for (int j = 0; j < m_iHeight - 1; ++j)
	{
		for (int i = 0; i < m_iWidth - 1; ++i)
		{
			m_vecIndex.push_back(i + j * m_iWidth);
			m_vecIndex.push_back(i + 1 + j * m_iWidth);
			m_vecIndex.push_back(i + (j + 1) * m_iWidth);

			m_vecIndex.push_back(i + 1 + j * m_iWidth);
			m_vecIndex.push_back(i + 1 + (j + 1) * m_iWidth);
			m_vecIndex.push_back(i + (j + 1) * m_iWidth);
		}
	}

	SetNormals(vecVertex, m_vecIndex);

	SetTangent(vecVertex, m_vecIndex);

	GetPrevVertexs() = vecVertex;

	m_pMesh = CreateBindable<Mesh>("ClothMesh", vecVertex, m_vecIndex, D3D11_USAGE_DYNAMIC);
}

std::vector<Engine::VertexStandard>& Engine::Cloth::GetCurrentVertexs()
{
	if (m_bSwitch)
	{
		return m_vecPrevPosition;
	}
	else
	{
		return m_vecPosition;
	}
}

std::vector<Engine::VertexStandard>& Engine::Cloth::GetPrevVertexs()
{
	if (m_bSwitch)
	{
		return m_vecPosition;
	}
	else
	{
		return m_vecPrevPosition;
	}
}

void Engine::Cloth::CollisionStay(Collider* pSrc, Collider* pDest, float fDeltaTime)
{
	Collider* _pDest = nullptr;

	if (m_pCollider.get() == pSrc)
	{
		_pDest = pDest;
	}
	else
	{
		_pDest = pSrc;
	}

	switch (_pDest->GetColliderType())
	{
	case Engine::COLLIDER_TYPE::NONE:
		break;
	case Engine::COLLIDER_TYPE::LINE:
		break;
	case Engine::COLLIDER_TYPE::SPHERE:
	{
		SPHERECOLLIDERINFO tInfo = static_cast<ColliderSphere*>(_pDest)->GetInfo();

		std::vector<VertexStandard>& vecVertex = GetCurrentVertexs();

		for (int i = 0; i < static_cast<int>(vecVertex.size()); ++i)
		{
			if ((vecVertex[i].pos - tInfo.vCenter).Length() < tInfo.fRadius)
			{
				const Vector3& vSub = (vecVertex[i].pos - tInfo.vCenter);

				float fLength = (vecVertex[i].pos - tInfo.vCenter).Length();

				if (fLength != 0.f)
				{
					vecVertex[i].pos = tInfo.vCenter + vSub / fLength * tInfo.fRadius;
				}
			}
		}
	}
		break;
	case Engine::COLLIDER_TYPE::MESH:
		break;
	case Engine::COLLIDER_TYPE::END:
		break;
	default:
		break;
	}
}

void Engine::Cloth::Ready()
{
	CreateVertexAndIndex();

	if (m_pCollider)
	{
		m_pCollider->SetCallBack(COLLISION_TYPE::STAY, this, &Cloth::CollisionStay);
	}
}
