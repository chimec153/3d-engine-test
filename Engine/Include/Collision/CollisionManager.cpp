#include "CollisionManager.h"
#include "../Bindable/Collider.h"
#include "../Bindable/Drawable.h"
#include "../Bindable/Transform.h"
#include "../Bindable/Camera.h"
#ifdef _DEBUG
#include "../Bindable/Material.h"
#endif

namespace Engine
{
	CollisionManager* CollisionManager::m_pInst = nullptr;

	CollisionManager::CollisionManager() :
		fAcutenessThreshold(cosf(DegToRad(30.f)))
	{
		/*PSPACE pSpace[4] = {};

		for (int i = 0; i < 4; ++i)
		{
			pSpace[i] = CreateChildSpace(m_pSpace, ((i & 0b10) << 1) | 0b10 | (i & 0b1));
		}

		for (int i = 0; i < 4; ++i)
		{
			pSpace[i]->bDelete = false;

			std::vector<Vector3> vecPos;

			for (int j = 0; j < 6; ++j)
			{
				Vector3 vPos = pSpace[i]->vPos;

				vPos[0] += pSpace[i]->fSize / 2.f * (1 - 2 * (i / 2));

				vPos[1] += sinf(PI / 3 * -j* (1 - 2 * (i / 2))) * pSpace[i]->fSize / 8.f;
				vPos[2] += cosf(PI / 3 * -j* (1 - 2 * (i / 2))) * pSpace[i]->fSize / 8.f;

				vecPos.push_back(vPos);
			}

			PORTAL tPortal(vecPos);

			tPortal.pSpace = pSpace[i + 2 * (1 - 2 * (i / 2))];

			pSpace[i]->PortalList.push_back(tPortal);

			vecPos.clear();

			for (int j = 0; j < 6; ++j)
			{
				Vector3 vPos = pSpace[i]->vPos;

				vPos[2] += pSpace[i]->fSize / 2.f * (1 - 2 * (i % 2));

				vPos[1] += sinf(PI / 3 * (1 - 2 * (i % 2)) * j) * pSpace[i]->fSize / 8.f;
				vPos[0] += cosf(PI / 3 * (1 - 2 * (i % 2)) * j) * pSpace[i]->fSize / 8.f;

				vecPos.push_back(vPos);
			}

			PORTAL _tPortal(vecPos);

			_tPortal.pSpace = pSpace[i + (1 - 2 * (i % 2))];

			pSpace[i]->PortalList.push_back(_tPortal);
		}*/
	}

	CollisionManager::~CollisionManager()
	{
	}

	void CollisionManager::AddDrawable(class Drawable* pDrawable)
	{
		const std::shared_ptr<class Transform>& pTransform = pDrawable->GetTransform();

		if (!pTransform)
		{
			return;
		}

		int iType = static_cast<int>(pTransform->GetCameraType());

		SPACE* pPrevSpace = m_tChannel[iType].FindSpaceAndErase(pDrawable);

		if (pPrevSpace)
		{
			std::list<Drawable*>::iterator iter = pPrevSpace->DrawableList.begin();
			std::list<Drawable*>::iterator iterEnd = pPrevSpace->DrawableList.end();

			for (; iter != iterEnd; ++iter)
			{
				if (*iter == pDrawable)
				{
					pPrevSpace->DrawableList.erase(iter);
					break;
				}
			}

			if (!pPrevSpace->GetTotalDrawableCount() && pPrevSpace->bDelete)
			{
				if (pPrevSpace->pParent)
				{
					for (int i = 0; i < 8; ++i)
					{
						if (pPrevSpace->pParent->pChild[i].get() == pPrevSpace)
						{
							pPrevSpace->pParent->pChild[i] = nullptr;
							break;
						}
					}
				}
			}
		}

		Vector4 vSphereInfo = pDrawable->GetSphereInfo();

		vSphereInfo = pTransform->GetTransformMatrix().TransformCoord({ vSphereInfo.x, vSphereInfo.y, vSphereInfo.z });

		SPACE* _pSpace = m_tChannel[iType].m_pSpace;

		while (_pSpace->fSize > 64.f)
		{
			bool bLeft = _pSpace->IsLeft(vSphereInfo);

			if (bLeft && _pSpace->IsRight(vSphereInfo))
			{
				break;
			}

			bool bBottom = _pSpace->IsBottom(vSphereInfo);

			if (bBottom && _pSpace->IsTop(vSphereInfo))
			{
				break;
			}

			bool bNear = _pSpace->IsNear(vSphereInfo);

			if (bNear && _pSpace->IsFar(vSphereInfo))
			{
				break;
			}

			int iIndex = 4 * !bLeft + static_cast<int>(SPACE_DIR::LTN) * !bBottom + !bNear;

			if (!_pSpace->pChild[iIndex])
			{
				CreateChildSpace(_pSpace, iIndex);
			}

			_pSpace = _pSpace->pChild[iIndex].get();
		}

		_pSpace->DrawableList.push_back(pDrawable);
		m_tChannel[iType].m_mapDrawable.insert(std::make_pair(pDrawable, _pSpace));
	}

	void CollisionManager::AddCollider(Collider* pCollider)
	{
		for (int i = 0; i < log2<static_cast<int>(COLLISION_CHANNEL::END) - 1, 31>() + 1; ++i)
		{
			if (static_cast<int>(pCollider->GetChannel()) & (i + 1))
			{
				m_tChannel[i].m_ColliderList.push_back(pCollider);
			}
		}
	}

	void CollisionManager::VisibleTest(CAMERA_TYPE eType)
	{
		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera(eType);

		if (!pCamera) 
		{
			return;
		}

		float fAngle = pCamera->GetAngle();

		float fBeta = atanf(tanf(fAngle) / pCamera->GetRatio());

		const Matrix& matCameraTransform = pCamera->GetTransform()->GetTransformMatrix();

		const Vector3& vCameraPos = pCamera->GetTransform()->GetPosition();

		Vector3 vNearNormal = matCameraTransform.TransformNormal({ 0.f, 0.f, 1.f });
		Vector3 vLeftNormal = matCameraTransform.TransformNormal({ cosf(fAngle), 0.f, sinf(fAngle) });
		Vector3 vRightNormal = matCameraTransform.TransformNormal({ -cosf(fAngle), 0.f, sinf(fAngle) });
		Vector3 vTopNormal = matCameraTransform.TransformNormal({ 0.f, -cosf(fBeta), sinf(fBeta) });
		Vector3 vBottomNormal = matCameraTransform.TransformNormal({ 0.f, cosf(fBeta), sinf(fBeta) });

		Vector3 vNearPos = matCameraTransform.TransformCoord({ 0.f, 0.f, pCamera->GetNear() });

		std::vector<Vector4> vecPlanes;

		vecPlanes.push_back({ vNearNormal, -vNearNormal.Dot(vNearPos) });
		vecPlanes.push_back({ vLeftNormal, -vLeftNormal.Dot(vCameraPos) });
		vecPlanes.push_back({ vRightNormal, -vRightNormal.Dot(vCameraPos) });
		vecPlanes.push_back({ vTopNormal, -vTopNormal.Dot(vCameraPos) });
		vecPlanes.push_back({ vBottomNormal, -vBottomNormal.Dot(vCameraPos) });

		float fNearRadius = (abs(vNearNormal.x) + abs(vNearNormal.y) + abs(vNearNormal.z)) / 2.f;
		float fLeftRadius = (abs(vLeftNormal.x) + abs(vLeftNormal.y) + abs(vLeftNormal.z)) / 2.f;
		float fRightRadius = (abs(vRightNormal.x) + abs(vRightNormal.y) + abs(vRightNormal.z)) / 2.f;
		float fTopRadius = (abs(vTopNormal.x) + abs(vTopNormal.y) + abs(vTopNormal.z)) / 2.f;
		float fBottomRadius = (abs(vBottomNormal.x) + abs(vBottomNormal.y) + abs(vBottomNormal.z)) / 2.f;

		VisibleTest(m_tChannel[static_cast<int>(eType)].m_pSpace, vecPlanes);

		std::list<PSPACE> SpaceList;

		SpaceList.push_back(m_tChannel[static_cast<int>(eType)].m_pSpace);

		while (!SpaceList.empty())
		{
			PSPACE pSpace = SpaceList.back();

			SpaceList.pop_back();

			pSpace->bCheck = false;

			for (int i = 0; i < 8; ++i)
			{
				if (pSpace->pChild[i])
				{
					SpaceList.push_back(pSpace->pChild[i].get());
				}
			}
		}


#ifdef _DEBUG
		/*SpaceList.push_back(m_pSpace);

		while (!SpaceList.empty())
		{
			PSPACE pSpace = SpaceList.back();

			SpaceList.pop_back();

			for (int i = 0; i < 8; ++i)
			{
				if (pSpace->pChild[i])
				{
					SpaceList.push_back(pSpace->pChild[i]);
				}
			}

			pSpace->pDebugBox->GetMaterial()->SetDiffuseColor(1.f, 0.f, 0.f, 1.f);
		}*/
#endif

#ifdef _DEBUG
		//pSpace->pDebugBox->GetMaterial()->SetDiffuseColor(0.f, 1.f, 0.f, 1.f);
#endif

#ifdef _DEBUG
	/*SpaceList.push_back(m_pSpace);

	while (!SpaceList.empty())
	{
		PSPACE pSpace = SpaceList.back();

		SpaceList.pop_back();

		for (int i = 0; i < 8; ++i)
		{
			if (pSpace->pChild[i])
			{
				SpaceList.push_back(pSpace->pChild[i]);
			}
		}

		if (pSpace->fSize >= m_pSpace->fSize / 2.f || true)
		{
			pSpace->pDebugBox->InViewFrustum();
			pSpace->pDebugBox->GetMaterial()->SetDiffuseColor(0.f, 1.f, 0.f, 1.f);
		}
		else
		{
			pSpace->pDebugBox->OutViewFrustum();
		}

		std::list<PORTAL>::iterator iter = pSpace->PortalList.begin();
		std::list<PORTAL>::iterator iterEnd = pSpace->PortalList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter).pDebugLine)
			{
				(*iter).pDebugLine->InViewFrustum();
			}
		}
	}*/
#endif
	}

	PSPACE CollisionManager::CreateChildSpace(SPACE* _pSpace, int iIndex)	const
	{
		assert(_pSpace->pChild[iIndex].get() == nullptr);

		_pSpace->pChild[iIndex] = std::make_unique<SPACE>(_pSpace->vPos -
			Vector3((-2 * ((iIndex & 0b100) >> 2) + 1) * _pSpace->fSize / 4.f,
				(-2 * ((iIndex & 0b10) >> 1) + 1) * _pSpace->fSize / 4.f,
				(-2 * (iIndex & 0b1) + 1) * _pSpace->fSize / 4.f),
			_pSpace->fSize / 2.f);

		_pSpace->pChild[iIndex]->pParent = _pSpace;

		return _pSpace->pChild[iIndex].get();
	}

	void CollisionManager::VisibleTest(PSPACE pSpace, const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* pvecLocalPlanes)
	{
		if (!VisibleTestNoRecursive(pSpace, vecPlanes, pvecLocalPlanes))
		{
			return;
		}

		for (int i = 0; i < 8; ++i)
		{
			if (!pSpace->pChild[i])
			{
				continue;
			}

			if (pSpace->pChild[i]->PortalList.empty())
			{
				VisibleTest(pSpace->pChild[i].get(), vecPlanes);
			}
			else
			{
				const Vector3& vCamPos = Graphics::GetInst()->GetCamera()->GetTransform()->GetPosition();

				if (pSpace->pChild[i]->vPos.x + pSpace->pChild[i]->fSize / 2.f >= vCamPos.x &&
					pSpace->pChild[i]->vPos.x - pSpace->pChild[i]->fSize / 2.f <= vCamPos.x &&
					pSpace->pChild[i]->vPos.y + pSpace->pChild[i]->fSize / 2.f >= vCamPos.y &&
					pSpace->pChild[i]->vPos.y - pSpace->pChild[i]->fSize / 2.f <= vCamPos.y &&
					pSpace->pChild[i]->vPos.z + pSpace->pChild[i]->fSize / 2.f >= vCamPos.z &&
					pSpace->pChild[i]->vPos.z - pSpace->pChild[i]->fSize / 2.f <= vCamPos.z)
				{
					VisibleTest(pSpace->pChild[i].get(), vecPlanes);

					PortalVisibleTest(pSpace->pChild[i].get(), vecPlanes);
				}
			}
		}
	}

	void CollisionManager::PortalVisibleTest(PSPACE pPortalSpace, const std::vector<Vector4>& vecPlanes)
	{
		pPortalSpace->bCheck = true;

		std::list<PORTAL>::iterator iter = pPortalSpace->PortalList.begin();
		std::list<PORTAL>::iterator iterEnd = pPortalSpace->PortalList.end();

		for (; iter != iterEnd; ++iter)
		{
			std::vector<Vector4> vecPlane;

			vecPlane.push_back(vecPlanes[0]);

			std::vector<Vector3> _vecPos = (*iter).vecPos;

			for (size_t j = 0; j < vecPlanes.size(); ++j)
			{
				std::vector<bool> vecCheck;
				bool bFind = false;

				int iStart = -1;
				int iZeroStart = -1;

				float fDot = _vecPos.front().Dot(vecPlanes[j]) + vecPlanes[j].w;

				bool bPrev = fDot <= -0.f;
				vecCheck.push_back(bPrev);

				if (!bPrev)
				{
					iStart = 0;
				}
				else
				{
					iZeroStart = 0;
				}

				for (int k = 1; k < static_cast<int>(_vecPos.size()); ++k)
				{
					float fDot = _vecPos[k].Dot(vecPlanes[j]) + vecPlanes[j].w;

					bool bCurrent = fDot <= -0.f;

					if (bCurrent)
					{
						bPrev = false;

						if (iZeroStart == -1)
						{
							iZeroStart = k;
						}
					}
					else
					{
						if (iStart == -1)
						{
							iStart = k;
						}

						bPrev = true;
					}

					bPrev = bCurrent;

					vecCheck.push_back(bCurrent);
				}

				if (iStart == -1)
				{
					vecPlane.clear();
					break;
				}

				if (iZeroStart == -1)
				{
					continue;
				}

				std::vector<Vector3> vecPos;

				bPrev = vecCheck[iStart];

				vecPos.push_back(_vecPos[iStart]);

				for (int k = 1; k < static_cast<int>(_vecPos.size()); ++k)
				{
					int iIndex = (k + iStart) % static_cast<int>(_vecPos.size());

					if (bPrev && vecCheck[iIndex])
					{
						vecPos.push_back(_vecPos[iIndex]);
					}

					else if (bPrev != vecCheck[iIndex])
					{
						Vector3 vPrevPos = _vecPos[(iIndex + static_cast<int>(_vecPos.size()) - 1) % static_cast<int>(_vecPos.size())];

						float fDotPrev = vecPlanes[j].DotPoint(vPrevPos);

						float fDot = vecPlanes[j].DotPoint(_vecPos[iIndex]);

						if ((-0.001f >= fDotPrev || fDotPrev > 0.f) &&
							(-0.001f >= fDot || fDot > 0.f))
						{
							Vector3 vV = (_vecPos[iIndex] - vPrevPos);

							float t = -vecPlanes[j].DotPoint(vPrevPos) / (vecPlanes[j].DotVector(vV));

							Vector3 vCrossPos = vV * t + vPrevPos;

							vecPos.push_back(vCrossPos);
						}

						if (!vecCheck[iIndex])
						{
							vecPos.push_back(_vecPos[iIndex]);
						}
					}
					else
					{
						vecPos.push_back(_vecPos[iIndex]);
					}

					bPrev = vecCheck[iIndex];
				}

				_vecPos = vecPos;
			}

			if (vecPlane.empty())
			{
				continue;
			}

			const Vector3& vCamPos = Graphics::GetInst()->GetCamera()->GetTransform()->GetPosition();

			for (size_t j = 0; j < _vecPos.size(); ++j)
			{
				Vector3 vVector = _vecPos[j] - _vecPos[(j + 1) % _vecPos.size()];

				float fLength = vVector.Length();

				if (fLength * fLength < 0.001f)
				{
					continue;
				}

				Vector3 V1 = _vecPos[j] - vCamPos;

				V1.Normalize();

				Vector3 V2 = _vecPos[(j + 1) % _vecPos.size()] - vCamPos;

				V2.Normalize();

				Vector3 vNormal = V1.Cross(V2);

				vNormal.Normalize();

				vecPlane.push_back({ vNormal, -vNormal.Dot(vCamPos) });
			}

			std::vector<Vector4> vecLocalPlane;

			for (size_t j = 1; j < vecPlane.size(); ++j)
			{
				const Vector4& vPlane1 = vecPlane[j];
				const Vector4& vPlane2 = vecPlane[(j + 1 + (j == vecPlane.size() - 1)) % vecPlane.size()];

				Vector3 vNormal1 = { vPlane1.x, vPlane1.y, vPlane1.z };
				Vector3 vNormal2 = { vPlane2.x, vPlane2.y, vPlane2.z };

				float fCos = vNormal1.Dot(vNormal2);

				if (fCos > fAcutenessThreshold)
				{
					Vector3 vA = vNormal1 + vNormal2;
					Vector3 vB = vNormal1.Cross(vNormal2);

					vA.Normalize();

					float fLength = vB.Length();

					if (!fLength)
					{
						continue;
					}

					vB /= fLength;

					Vector3 vNormal = vA - vB.Dot(vA) * vB;

					vNormal.Normalize();

					vecLocalPlane.push_back({ vNormal, -vNormal.Dot(vCamPos) });
				}
			}

			if (!(*iter).pSpace->bCheck)
			{
				VisibleTest((*iter).pSpace, vecPlane, &vecLocalPlane);

				PortalVisibleTest((*iter).pSpace, vecPlane);
			}
		}
	}

	bool CollisionManager::VisibleTestNoRecursive(PSPACE pSpace, const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* pvecLocalPlanes)
	{
		for (int i = 0; i < vecPlanes.size(); ++i)
		{
			if (vecPlanes[i].Dot({ pSpace->vPos , 1.f }) + pSpace->fSize * (abs(vecPlanes[i].x) + abs(vecPlanes[i].y) + abs(vecPlanes[i].z)) / 2.f < 0.f)
			{
				return false;
			}
		}

		std::list<Drawable*>::iterator iter = pSpace->DrawableList.begin();
		std::list<Drawable*>::iterator iterEnd = pSpace->DrawableList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->UpdateInViewFrustum(vecPlanes, pvecLocalPlanes);
			++iter;
		}

		return true;
	}

	void CollisionManager::DeleteDrawable(Drawable* pDrawable)
	{
		std::shared_ptr<Transform> pTransform = pDrawable->GetTransform();

		m_tChannel[pTransform ? static_cast<int>(pTransform->GetCameraType()) : 0].DeleteDrawable(pDrawable);
	}

	void CollisionManager::Collision(CAMERA_TYPE eType, float fDeltaTime)
	{
		if (m_tChannel[static_cast<int>(eType)].m_ColliderList.size() < 2)
		{
			m_tChannel[static_cast<int>(eType)].m_ColliderList.clear();
			return;
		}

		std::list<Collider*>::iterator iterSrc = m_tChannel[static_cast<int>(eType)].m_ColliderList.begin();
		std::list<Collider*>::iterator iterEnd = m_tChannel[static_cast<int>(eType)].m_ColliderList.end();

		--iterEnd;

		for (; iterSrc != iterEnd; ++iterSrc)
		{
			std::list<Collider*>::iterator iterDest = iterSrc;
			std::list<Collider*>::iterator iterDestEnd = m_tChannel[static_cast<int>(eType)].m_ColliderList.end();

			++iterDest;

			for (; iterDest != iterDestEnd; ++iterDest)
			{
				if ((*iterSrc)->Collision(*iterDest, fDeltaTime))
				{
					if (!(*iterSrc)->HasPrevCollider(*iterDest))
					{
						(*iterSrc)->AddPrevCollider(*iterDest);
						(*iterDest)->AddPrevCollider(*iterSrc);

						(*iterSrc)->Call(COLLISION_TYPE::BEGIN, *iterDest, fDeltaTime);
						(*iterDest)->Call(COLLISION_TYPE::BEGIN, *iterSrc, fDeltaTime);
					}
					else
					{
						(*iterSrc)->Call(COLLISION_TYPE::STAY, *iterDest, fDeltaTime);
						(*iterDest)->Call(COLLISION_TYPE::STAY, *iterSrc, fDeltaTime);
					}
				}
				else if ((*iterSrc)->HasPrevCollider(*iterDest))
				{
					(*iterSrc)->DeletePrevCollider(*iterDest);
					(*iterDest)->DeletePrevCollider(*iterSrc);

					(*iterSrc)->Call(COLLISION_TYPE::LAST, *iterDest, fDeltaTime);
					(*iterDest)->Call(COLLISION_TYPE::LAST, *iterSrc, fDeltaTime);
				}
			}
		}

		m_tChannel[static_cast<int>(eType)].m_ColliderList.clear();
	}

	void CollisionManager::Collision(float fDeltaTime)
	{
		for (int i = 0; i < static_cast<int>(CAMERA_TYPE::END); ++i)
		{
			Collision(static_cast<CAMERA_TYPE>(i), fDeltaTime);
		}
	}

	void CollisionManager::VisibleTest()
	{
		for (int i = 0; i < static_cast<int>(CAMERA_TYPE::END); ++i)
		{
			VisibleTest(static_cast<CAMERA_TYPE>(i));
		}
	}

	SPACE* CollisionManager::_tagCollisionChannel::FindSpaceAndErase(Drawable* pDrawable)
	{
		std::unordered_map<Drawable*, PSPACE>::const_iterator iter = m_mapDrawable.find(pDrawable);

		if (iter == m_mapDrawable.end())
		{
			return nullptr;
		}

		SPACE* pSpace = iter->second;

		m_mapDrawable.erase(iter);

		return pSpace;
	}
	void CollisionManager::_tagCollisionChannel::DeleteDrawable(Drawable* pDrawable)
	{
		PSPACE pSpace = FindSpaceAndErase(pDrawable);

		if (!pSpace)
		{
			return;
		}

		std::list<Drawable*>::iterator iter = pSpace->DrawableList.begin();
		std::list<Drawable*>::iterator iterEnd = pSpace->DrawableList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter) == pDrawable)
			{
				pSpace->DrawableList.erase(iter);
				return;
			}
		}
	}
}