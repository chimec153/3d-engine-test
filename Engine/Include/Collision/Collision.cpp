#include "Collision.h"
#include "../Bindable/ColliderLine.h"
#include "../Bindable/ColliderSphere.h"
#include "../Bindable/Transform.h"
#include "../Bindable/ColliderMesh.h"
#include "../Bindable/ColliderOBB.h"

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

		float fA = vA.Length();

		if (fA > tSrc.fRadius + tDest.fRadius)
		{
			return false;
		}

		const Vector3& vDestPos = tDest.vCenter + vDestVelocity * fDeltaTime;

		vCross = tDest.vCenter + vA / fA * tDest.fRadius;

		return true;
	}

	bool Collision::CollisionLineToMesh(const LINECOLLIDERINFO& tLineInfo, const PMESHCOLLIDERINFO pMeshColliderInfo, Vector3& vCross)
	{
		Vector3 D = tLineInfo.vDir;

		float min_w = FLT_MAX;

		for (int i = 0; i < pMeshColliderInfo->vecIndex.size(); i += 3)
		{
			Vector3 PV0 = tLineInfo.vStart - Vector3(pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3], pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3 + 1], pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3 + 2]);

			Vector3 U = { pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i + 1] * 3] - pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3],pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i + 1] * 3 + 1] - pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3 + 1], pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i + 1] * 3 + 2] - pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3 + 2] };
			Vector3 V = { pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i + 2] * 3] - pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3],pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i + 2] * 3 + 1] - pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3 + 1], pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i + 2] * 3 + 2] - pMeshColliderInfo->vecPoint[pMeshColliderInfo->vecIndex[i] * 3 + 2] };

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
				vCross = tLineInfo.vStart + D * w;
			}
		}

		return min_w != FLT_MAX;
	}

	bool Collision::CollisionLineToTerrain(const LINECOLLIDERINFO& tSrc, const PMESHCOLLIDERINFO tDest, Vector3& vCross)
	{
		int iSize =static_cast<int>( tDest->vecPoint.size());

		float width = FLT_MIN;
		float height = FLT_MIN;
		float min_x = FLT_MAX;
		float min_z = FLT_MAX;

		for (int i = 0; i < iSize / 3; ++i)
		{
			if (width < tDest->vecPoint[i * 3])
			{
				width = tDest->vecPoint[i * 3];
			}

			if (height < tDest->vecPoint[i * 3 + 2])
			{
				height = tDest->vecPoint[i * 3 + 2];
			}

			if (min_x > tDest->vecPoint[i * 3])
			{
				min_x = tDest->vecPoint[i * 3];
			}

			if (min_z > tDest->vecPoint[i * 3 + 2])
			{
				min_z = tDest->vecPoint[i * 3 + 2];
			}
		}

		Vector3 s = tSrc.vStart - Vector3(min_x, 0.f, min_z);
		Vector3 d = tSrc.vDir;

		bool bMainAxisZ = abs(d.x) < abs(d.z);

		if (bMainAxisZ)
		{
			if (d.x != 0.f)
			{
				if (d.z < 0.f)
				{
					d = -d;
				}
					// z = a * x + b
					// 
					// a = d.z / d.x
					// 
					// b = sz - d.z / d.x * sx
					// 
					// z = d.z / d.x * x + sz - d.z / d.x * sx
					// 
					// z = 0, x = (dz / dx * sx - sz) /dz /dx
					//
					// x = 0, z = sz - d.z / d.x * sx
					// x = width, z = d.z / d.x * width + sz - d.z / d.x * sx
					// 
					// start_z = d.z / d.x * x + sz - d.z / d.x * sx
					// x = (start_z - sz + d.z / d.x * sx) / (d.z / d.x)
					// 
					//

				float start_x = (d.z / d.x * s.x - s.z) / (d.z / d.x);

				if (start_x >= 0.f && start_x < width)
				{
					s.x = start_x;
					s.z = 0.f;
				}
				else
				{
					float start_z = s.z - d.z / d.x * s.x;

					if (start_z >= 0.f && start_z < height)
					{
						start_z = static_cast<float>(static_cast<int>(start_z));

						s.x = (start_z - s.z + d.z / d.x * s.x) / (d.z / d.x);
						s.z = start_z;
					}
					else
					{
						start_z += d.z / d.x * width;

						if (start_z < 0.f || start_z >= height)
						{
							return false;
						}

						start_z = static_cast<float>(static_cast<int>(start_z));

						s.x = (start_z - s.z + d.z / d.x * s.x) / (d.z / d.x);
						s.z = start_z;
					}
				}
			}
		}
		else
		{
			if (d.z != 0.f)
			{
				if (d.x < 0.f)
				{
					d = -d;
				}
				// z = d.z / d.x * x + sz - d.z / d.x * sx
				// 
				// x = 0, z = sz - d.z / d.x * sx
				// 
				// z = 0, 
				// x = (d.z / d.x * sx - sz) / d.z / d.x,
				// 
				// z = height,
				// x = (height + d.z / d.x * sx - sz) / d.z / d.x
				// 
				// z = d.z / d.x * start_x + sz - d.z / d.x * sx
				//

				float start_z = (s.z - d.z / d.x * s.x);

				if (start_z >= 0.f && start_z < height)
				{
					s.z = start_z;
					s.x = 0.f;
				}
				else
				{
					float start_x = (d.z / d.x * s.x - s.z) / (d.z / d.x);

					if (start_x >= 0.f && start_x < width)
					{
						start_x = static_cast<float>(static_cast<int>(start_x));

						s.z = d.z / d.x * start_x + s.z - d.z / d.x * s.x;
						s.x = start_x;
					}
					else
					{
						start_x = (height + d.z / d.x * s.x - s.z) / (d.z / d.x);

						if (start_x < 0.f || start_x >= width)
						{
							return false;
						}

						start_x = static_cast<float>(static_cast<int>(start_x));

						s.z = d.z / d.x * start_x + s.z - d.z / d.x * s.x;
						s.x = start_x;
					}
				}
			}
		}

		float& fMainPos = bMainAxisZ ? s.z : s.x;
		float& fSubPos = bMainAxisZ ? s.x : s.z;

		float diff = bMainAxisZ ? (d.z == 0.f ? 0.f : d.x / d.z) : (d.x == 0.f ? 0.f : d.z / d.x);

		float fMinDist = FLT_MAX;
		Vector3 vMinCross = {};

		if (!diff)
		{
			return false;
		}

		while (true)
		{
			if (s.x < 0.f && d.x < 0.f)
			{
				break;
			}

			if (s.x > width && d.x > 0.f)
			{
				break;
			}

			if (s.z < 0.f && d.z < 0.f)
			{
				break;
			}

			if (s.z > height && d.z > 0.f)
			{
				break;
			}
			
			if (s.x >= 0.f && s.z >= 0.f && s.x < width && s.z < height)
			{
				if (CollisionLineToQuad(tSrc, tDest, static_cast<int>(s.x) + static_cast<int>((height - static_cast<int>(s.z) - 1.f) * width), vCross))
				{
					float fDist = (vCross - tSrc.vStart).Length();

					if (fDist < fMinDist) 
					{
						fMinDist = fDist;
						vMinCross = vCross;
					}
				}
			}

			if (ceilf(fSubPos + diff) != ceilf(fSubPos))
			{
				fSubPos += diff;

				if (s.x >= 0.f && s.z >= 0.f && s.x < width && s.z < height)
				{
					if (CollisionLineToQuad(tSrc, tDest, static_cast<int>(s.x) + static_cast<int>((height - static_cast<int>(s.z) - 1.f) * width), vCross))
					{
						float fDist = (vCross - tSrc.vStart).Length();

						if (fDist < fMinDist)
						{
							fMinDist = fDist;
							vMinCross = vCross;
						}
					}
				}

				fMainPos += 1.f;
			}
			else
			{
				fMainPos += 1.f;
				fSubPos += diff;
			}
		}

		vCross = vMinCross;

		return fMinDist != FLT_MAX;
	}

	bool Collision::CollisionOBBToSphere(const OBBINFO& tSrc, const SPHERECOLLIDERINFO tDest, Vector3& vCross)
	{
		float fLengthX = tSrc.vAxis[0].Length();
		float fLengthY = tSrc.vAxis[1].Length();
		float fLengthZ = tSrc.vAxis[2].Length();

		const Vector3& vAxisX = tSrc.vAxis[0] / fLengthX;
		const Vector3& vAxisY = tSrc.vAxis[1] / fLengthY;
		const Vector3& vAxisZ = tSrc.vAxis[2] / fLengthZ;

		Matrix matRot = {};

		matRot[0] = vAxisX;
		matRot[1] = vAxisY;
		matRot[2] = vAxisZ;
		matRot[3][3] = 1.f;

		matRot.Transpose();

		const Vector3& vSphereCenter = matRot.TransformCoord(tDest.vCenter - tSrc.vCenter);

		Vector3 e = Vector3(std::max(-fLengthX / 2.f - vSphereCenter.x, 0.f), std::max(-fLengthY / 2.f - vSphereCenter.y, 0.f), std::max(-fLengthZ / 2.f - vSphereCenter.z, 0.f));

		e += Vector3(std::max(vSphereCenter.x - fLengthX / 2.f, 0.f), std::max(vSphereCenter.y - fLengthY / 2.f, 0.f), std::max(vSphereCenter.z - fLengthZ / 2.f, 0.f));

		return e.Dot(e) <= tDest.fRadius * tDest.fRadius;
	}

	bool Collision::CollisionOBBToOBB(const OBBINFO& tSrc, const OBBINFO& tDest, Vector3& vCross)
	{
		const Vector3& vDist = tSrc.vCenter - tDest.vCenter;

		float fSrcAxisXLength = tSrc.vAxis[0].Length();

		const Vector3& vSrcAxisX = tSrc.vAxis[0] / fSrcAxisXLength;

		float fSrcAxisXDist = vSrcAxisX.Dot(vDist);

		float fDotXX = abs(vSrcAxisX.Dot(tDest.vAxis[0]));

		float fDotXY = abs(vSrcAxisX.Dot(tDest.vAxis[1]));

		float fDotXZ = abs(vSrcAxisX.Dot(tDest.vAxis[2]));

		if (fDotXX + fDotXY + fDotXZ + fSrcAxisXLength < fSrcAxisXDist * 2.f)
		{
			return false;
		}

		float fSrcAxisYLength = tSrc.vAxis[1].Length();

		const Vector3& vSrcAxisY = tSrc.vAxis[1] / fSrcAxisYLength;

		float fSrcAxisYDist = vSrcAxisY.Dot(vDist);

		float fDotYX = abs(vSrcAxisY.Dot(tDest.vAxis[0]));

		float fDotYY = abs(vSrcAxisY.Dot(tDest.vAxis[1]));

		float fDotYZ = abs(vSrcAxisY.Dot(tDest.vAxis[2]));

		if (fDotYX + fDotYY + fDotYZ + fSrcAxisYLength < fSrcAxisYDist * 2.f)
		{
			return false;
		}

		float fSrcAxisZLength = tSrc.vAxis[2].Length();

		const Vector3& vSrcAxisZ = tSrc.vAxis[2] / fSrcAxisZLength;

		float fSrcAxisZDist = vSrcAxisZ.Dot(vDist);

		float fDotZX = abs(vSrcAxisZ.Dot(tDest.vAxis[0]));

		float fDotZY = abs(vSrcAxisZ.Dot(tDest.vAxis[1]));

		float fDotZZ = abs(vSrcAxisZ.Dot(tDest.vAxis[2]));

		if (fDotZX + fDotZY + fDotZZ + fSrcAxisZLength < fSrcAxisZDist * 2.f)
		{
			return false;
		}

		float fDestAxisXLength = tDest.vAxis[0].Length();

		const Vector3& vDestAxisX = tDest.vAxis[0] / fDestAxisXLength;

		float fDestAxisXDist = vDestAxisX.Dot(vDist);

		if (abs(vDestAxisX.Dot(tSrc.vAxis[0])) + abs(vDestAxisX.Dot(tSrc.vAxis[1])) + abs(vDestAxisX.Dot(tSrc.vAxis[2])) + fDestAxisXLength < fDestAxisXDist * 2.f)
		{
			return false;
		}

		float fDestAxisYLength = tDest.vAxis[1].Length();

		const Vector3& vDestAxisY = tDest.vAxis[1] / fDestAxisYLength;

		float fDestAxisYDist = vDestAxisY.Dot(vDist);

		if (abs(vDestAxisY.Dot(tSrc.vAxis[0])) + abs(vDestAxisY.Dot(tSrc.vAxis[1])) + abs(vDestAxisY.Dot(tSrc.vAxis[2])) + fDestAxisYLength < fDestAxisYDist * 2.f)
		{
			return false;
		}

		float fDestAxisZLength = tDest.vAxis[2].Length();

		const Vector3& vDestAxisZ = tDest.vAxis[2] / fDestAxisZLength;

		float fDestAxisZDist = vDestAxisZ.Dot(vDist);

		if (abs(vDestAxisZ.Dot(tSrc.vAxis[0])) + abs(vDestAxisZ.Dot(tSrc.vAxis[1])) + abs(vDestAxisZ.Dot(tSrc.vAxis[2])) + fDestAxisZLength < fDestAxisZDist * 2.f)
		{
			return false;
		}

		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				Vector3 vAxis = tSrc.vAxis[i].Cross(tDest.vAxis[j]);

				float fLength = vAxis.Length();

				if (fLength)
				{
					vAxis /= fLength;

					if (abs(vAxis.Dot(tSrc.vAxis[0])) + abs(vAxis.Dot(tSrc.vAxis[1])) + abs(vAxis.Dot(tSrc.vAxis[2]))
						+ abs(vAxis.Dot(tDest.vAxis[0])) + abs(vAxis.Dot(tDest.vAxis[1])) + abs(vAxis.Dot(tDest.vAxis[2])) < vAxis.Dot(vDist) * 2.f)
					{
						return false;
					}
				}
			}
		}

		return true;
	}

	bool Collision::CollisionOBBToLine(const OBBINFO& tSrc, const LINECOLLIDERINFO& tDest, Vector3& vCross)
	{
		const Vector3& p = tSrc.vCenter - tDest.vStart;

		Vector3 vHalfLength = {tSrc.vAxis[0].Length() * 0.5f, tSrc.vAxis[1].Length() * 0.5f, tSrc.vAxis[2].Length() * 0.5f };

		Vector3 vAxis[3] = {};

		vAxis[0] = tSrc.vAxis[0] / tSrc.vAxis[0].Length();
		vAxis[1] = tSrc.vAxis[1] / tSrc.vAxis[1].Length();
		vAxis[2] = tSrc.vAxis[2] / tSrc.vAxis[2].Length();

		float tmin = -FLT_MAX;
		float tmax = FLT_MAX;

		for (int i = 0; i < 3; ++i)
		{
			float e = vAxis[i].Dot(p);
			float f = vAxis[i].Dot(tDest.vDir);

			if (abs(f) > epsilon)
			{
				f = 1.f / f;

				float t1 = (e + vHalfLength[i]) * f;
				float t2 = (e - vHalfLength[i]) * f;

				if (t1 > t2)
				{
					tmin = t2 > tmin ? t2 : tmin;

					tmax = t1 < tmax ? t1 : tmax;
				}
				else
				{
					tmin = t1 > tmin ? t1 : tmin;

					tmax = t2 < tmax ? t2 : tmax;
				}

				if (tmin > tmax)
				{
					return false;
				}

				if (tmax < 0.f)
				{
					return false;
				}
			}
			else if (-e - vHalfLength[i] > 0.f || -e + vHalfLength[i] < 0.f)
			{
				return false;
			}
		}

		if (tmin > 0.f)
		{
			vCross = tDest.vStart + tDest.vDir * tmin;
			return true;
		}

		vCross = tDest.vStart + tDest.vDir * tmax;

		return true;
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

		// Phase B.4 — Collider→Component, owners via GetOwner.
		Drawable* pSrcOwner = pSrc->GetOwner();
		Drawable* pDstOwner = pDest->GetOwner();
		if (!pSrcOwner || !pDstOwner) return false;

		Vector3 vSrcVel = pSrcOwner->GetTransform() ? pSrcOwner->GetTransform()->GetVelocity() : Vector3{};
		Vector3 vDstVel = pDstOwner->GetTransform() ? pDstOwner->GetTransform()->GetVelocity() : Vector3{};

		if (CollisionSphereToSphere(pSrc->GetInfo(), pDest->GetInfo(), vSrcVel, vDstVel, fDeltaTime, vCross))
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
	bool Collision::CollisionLineToTerrain(ColliderLine* pSrc, ColliderMesh* pDest)
	{
		Vector3 vCross = {};

		if (CollisionLineToTerrain(pSrc->GetInfo(), pDest->GetInfo(), vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);

			return true;
		}

		return false;
	}
	bool Collision::CollisionOBBToSphere(ColliderOBB* pSrc, ColliderSphere* pDest)
	{
		Vector3 vCross = {};

		if (CollisionOBBToSphere(pSrc->GetInfo(), pDest->GetInfo(), vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);
			return true;
		}

		return false;
	}
	bool Collision::CollisionOBBToOBB(ColliderOBB* pSrc, ColliderOBB* pDest)
	{
		Vector3 vCross = {};

		if (CollisionOBBToOBB(pSrc->GetInfo(), pDest->GetInfo(), vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);
			return true;
		}

		return false;
	}
	bool Collision::CollisionOBBToLine(ColliderOBB* pSrc, ColliderLine* pDest)
	{
		Vector3 vCross = {};

		if (CollisionOBBToLine(pSrc->GetInfo(), pDest->GetInfo(), vCross))
		{
			pSrc->SetCross(vCross);
			pDest->SetCross(vCross);
			return true;
		}

		return false;
	}
	// s =	px	vx	dx
	//		py	vy	dy
	//		pz	vz	dz
	//
	//		ux	vx	dx
	//		uy	vy	dy
	//		uz	vz	dz
	//
	bool Collision::CollisionLineToTriangle(const LINECOLLIDERINFO& tLine, const Vector3& p0, const Vector3& p1, const Vector3& p2, Vector3& vCross)
	{
		const Vector3& u = p1 - p0;
		const Vector3& v = p2 - p0;
		const Vector3& d = -tLine.vDir;
		const Vector3& p = tLine.vStart - p0;

		float determinent = u.x * (v.y * d.z - v.z * d.y)+ v.x * (u.z * d.y - u.y * d.z) + d.x * (u.y * v.z - v.y * u.z);

		if(!determinent)
		{
			return false;
		}

		float determinent_x = p.x * (v.y * d.z - v.z * d.y) + v.x * (p.z * d.y - p.y * d.z) + d.x * (p.y * v.z - v.y * p.z);

		float s = determinent_x / determinent;

		if (s < 0.f)
		{
			return false;
		}

		float determinent_y = u.x * (p.y * d.z - p.z * d.y) + p.x * (u.z * d.y - u.y * d.z) + d.x * (u.y * p.z - p.y * u.z);

		float t = determinent_y / determinent;

		if (t < 0.f || s + t > 1.f)
		{
			return false;
		}

		float determinent_z = u.x * (v.y * p.z - v.z * p.y) + v.x * (u.z * p.y - u.y * p.z) + p.x * (u.y * v.z - v.y * u.z);

		float k = determinent_z / determinent;

		if (k < 0.f)
		{
			return false;
		}

		vCross = tLine.vStart + k * tLine.vDir;

		return true;
	}
	bool Collision::CollisionLineToQuad(const LINECOLLIDERINFO& tSrc, const PMESHCOLLIDERINFO tDest, int index, Vector3& vCross)
	{

		const Vector3& p0 = Vector3(
			tDest->vecPoint[tDest->vecIndex[index * 6] * 3],
			tDest->vecPoint[tDest->vecIndex[index * 6] * 3 + 1],
			tDest->vecPoint[tDest->vecIndex[index * 6] * 3 + 2]);
		const Vector3& p1 = Vector3(
			tDest->vecPoint[tDest->vecIndex[index * 6 + 1] * 3],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 1] * 3 + 1],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 1] * 3 + 2]);
		const Vector3& p2 = Vector3(
			tDest->vecPoint[tDest->vecIndex[index * 6 + 2] * 3],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 2] * 3 + 1],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 2] * 3 + 2]);

		if (CollisionLineToTriangle(tSrc, p0, p1, p2, vCross))
		{
			return true;
		}

		const Vector3& p3 = Vector3(
			tDest->vecPoint[tDest->vecIndex[index * 6 + 3] * 3],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 3] * 3 + 1],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 3] * 3 + 2]);
		const Vector3& p4 = Vector3(
			tDest->vecPoint[tDest->vecIndex[index * 6 + 4] * 3],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 4] * 3 + 1],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 4] * 3 + 2]);
		const Vector3& p5 = Vector3(
			tDest->vecPoint[tDest->vecIndex[index * 6 + 5] * 3],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 5] * 3 + 1],
			tDest->vecPoint[tDest->vecIndex[index * 6 + 5] * 3 + 2]);

		if (CollisionLineToTriangle(tSrc, p3, p4, p5, vCross))
		{
			return true;
		}

		return false;
	}
}