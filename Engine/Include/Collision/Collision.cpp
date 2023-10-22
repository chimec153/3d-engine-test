#include "Collision.h"
#include "../Bindable/ColliderLine.h"
#include "../Bindable/ColliderSphere.h"
#include "../Bindable/TransformBuffer.h"
#include "../Bindable/ColliderMesh.h"

namespace Engine
{
	bool Collision::CollisionLineToSphere(const LINECOLLIDERINFO& tSrc, const SPHERECOLLIDERINFO& tDest, Vector3& vCross)
	{
		Vector3 vCtoP = tSrc.vStart - tDest.vCenter;

		float fLength = tSrc.vDir.Dot(vCtoP);

		float D = fLength * fLength - vCtoP.Dot(vCtoP) + tDest.fRadius * tDest.fRadius;

		if (D < 0.f)
		{
			return false;
		}

		float t1 = -fLength + sqrtf(D);
		float t2 = -fLength - sqrtf(D);

		if (t1 >= 0.f)
		{
			if (t1 < t2)
			{
				vCross = tSrc.vStart + tSrc.vDir * t1;

				return true;
			}

			vCross = tSrc.vStart + tSrc.vDir * t2;

			return true;
		}
		else if (t2 >= 0.f)
		{
			vCross = tSrc.vStart + tSrc.vDir * t2;
			return true;
		}

		return false;
	}

	bool Collision::CollisionSphereToSphere(const SPHERECOLLIDERINFO& tSrc, const SPHERECOLLIDERINFO& tDest, const Vector3& vSrcVelocity, const Vector3& vDestVelocity, float fDeltaTime, Vector3& vCross)
	{
		const Vector3& vA = tSrc.vCenter - tDest.vCenter;
		const Vector3& vB = vSrcVelocity - vDestVelocity;

		float fDot = vA.Dot(vB);

		float fA = vA.Length();

		float fB = vB.Length();

		float fLength = fB * fB;

		if (!fLength)
		{
			return false;
		}

		float fD = fDot * fDot - fLength * (fA * fA - (tSrc.fRadius + tDest.fRadius) * (tSrc.fRadius + tDest.fRadius));

		if (fD < 0.f)
		{
			return false;
		}

		float t = (-fDot - sqrtf(fD)) / fLength;

		if (t >= 0.f && t <= 1.f)
		{
			const Vector3& vDestPos = tDest.vCenter + vDestVelocity * fDeltaTime;

			vCross = ((tSrc.vCenter + vSrcVelocity * fDeltaTime) - vDestPos).Normalize() * tDest.fRadius + vDestPos;

			return true;
		}

		return false;
	}

	bool Collision::CollisionLineToMesh(const LINECOLLIDERINFO& tSrc, const PMESHCOLLIDERINFO pDest, Vector3& vCross)
	{
		Vector3 D = tSrc.vDir;

		float min_w = FLT_MAX;

		for (int i = 0; i < pDest->vecIndex.size(); i += 3)
		{
			Vector3 PV0 = tSrc.vStart - Vector3(pDest->vecPoint[pDest->vecIndex[i] * 3], pDest->vecPoint[pDest->vecIndex[i] * 3 + 1], pDest->vecPoint[pDest->vecIndex[i] * 3 + 2]);

			Vector3 U = { pDest->vecPoint[pDest->vecIndex[i + 1] * 3] - pDest->vecPoint[pDest->vecIndex[i] * 3],pDest->vecPoint[pDest->vecIndex[i + 1] * 3 + 1] - pDest->vecPoint[pDest->vecIndex[i] * 3 + 1], pDest->vecPoint[pDest->vecIndex[i + 1] * 3 + 2] - pDest->vecPoint[pDest->vecIndex[i] * 3 + 2] };
			Vector3 V = { pDest->vecPoint[pDest->vecIndex[i + 2] * 3] - pDest->vecPoint[pDest->vecIndex[i] * 3],pDest->vecPoint[pDest->vecIndex[i + 2] * 3 + 1] - pDest->vecPoint[pDest->vecIndex[i] * 3 + 1], pDest->vecPoint[pDest->vecIndex[i + 2] * 3 + 2] - pDest->vecPoint[pDest->vecIndex[i] * 3 + 2] };

			float fDet = U.x * (V.y * -D.z - V.z * -D.y) - U.y * (V.x * -D.z - V.z * -D.x) + U.z * (V.x * -D.y - V.y * -D.x);

			float fDet_z = U.x * (V.y * PV0.z - V.z * PV0.y) - U.y * (V.x * PV0.z - V.z * PV0.x) + U.z * (V.x * PV0.y - V.y * PV0.x);

			float w = fDet_z / fDet;

			if (w < 0.f)
			{
				continue;
			}

			float fDet_x = PV0.x * (V.y * -D.z - V.z * -D.y) - PV0.y * (V.x * -D.z - V.z * -D.x) + PV0.z * (V.x * -D.y - V.y * -D.x);

			float fDet_y = U.x * (PV0.y * -D.z - PV0.z * -D.y) - U.y * (PV0.x * -D.z - PV0.z * -D.x) + U.z * (PV0.x * -D.y - PV0.y * -D.x);

			float t = fDet_x / fDet;

			float s = fDet_y / fDet;

			if (t >= 0 && s >= 0 && s + t <= 1 &&
				w < min_w)
			{
				min_w = w;
				vCross = tSrc.vStart + D * w;
			}
		}

		return min_w != FLT_MAX;
	}

	bool Collision::CollisionLineToSphere(ColliderLine* pSrc, ColliderSphere* pDest)
	{
		Vector3 vCross;

		if (CollisionLineToSphere(pSrc->GetInfo(), pDest->GetInfo(), vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);

			return true;
		}

		return false;
	}

	bool Collision::CollisionSphereToSphere(ColliderSphere* pSrc, ColliderSphere* pDest, float fDeltaTime)
	{
		Vector3 vCross;

		if (CollisionSphereToSphere(pSrc->GetInfo(), pDest->GetInfo(), static_cast<Drawable*>(pSrc->GetParent())->GetTransform()->GetVelocity(), static_cast<Drawable*>(pDest->GetParent())->GetTransform()->GetVelocity(), fDeltaTime, vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);

			return true;
		}

		return false;
	}
	bool Collision::CollisionLineToMesh(ColliderLine* pSrc, ColliderMesh* pDest)
	{
		Vector3 vCross = {};

		if (CollisionLineToMesh(pSrc->GetInfo(), pDest->GetInfo(), vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);
			return true;
		}

		return false;
	}
}