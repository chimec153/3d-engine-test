#pragma once

#include "../Types.h"

namespace Engine
{
	class ENGINE_DLL Collision
	{
	private:
		static bool CollisionLineToSphere(const LINECOLLIDERINFO& tSrc, const SPHERECOLLIDERINFO& tDest, Vector3& vCross);
		static bool CollisionSphereToSphere(const SPHERECOLLIDERINFO& tSrc, const SPHERECOLLIDERINFO& tDest, const Vector3& vSrcVelocity, const Vector3& vDestVelocity, float fDeltaTime, Vector3& vCross);
		static bool CollisionLineToMesh(const LINECOLLIDERINFO& tSrc, const PMESHCOLLIDERINFO tDest, Vector3& vCross);
		static bool CollisionLineToTerrain(const LINECOLLIDERINFO& tSrc, const PMESHCOLLIDERINFO tDest, Vector3& vCross);
		static bool CollisionOBBToSphere(const OBBINFO& tSrc, const SPHERECOLLIDERINFO tDest, Vector3& vCross);
		static bool CollisionOBBToOBB(const OBBINFO& tSrc, const OBBINFO& tDest, Vector3& vCross);
		static bool CollisionOBBToLine(const OBBINFO& tSrc, const LINECOLLIDERINFO& tDest, Vector3& vCross);

	public:
		static bool CollisionLineToSphere(class ColliderLine* pSrc, class ColliderSphere* pDest);
		static bool CollisionSphereToSphere(class ColliderSphere* pSrc, class ColliderSphere* pDest, float fDeltaTime);
		static bool CollisionLineToMesh(class ColliderLine* pSrc, class ColliderMesh* pDest);
		static bool CollisionLineToTerrain(class ColliderLine* pSrc, class ColliderMesh* pDest);
		static bool CollisionOBBToSphere(class ColliderOBB* pSrc, class ColliderSphere* pDest);
		static bool CollisionOBBToOBB(class ColliderOBB* pSrc, class ColliderOBB* pDest);
		static bool CollisionOBBToLine(class ColliderOBB* pSrc, class ColliderLine* pDest);

	private:
		static bool CollisionLineToTriangle(const LINECOLLIDERINFO& tLine, const Vector3& p0, const Vector3& p1, const Vector3& p2, Vector3& vCross);
		static bool CollisionLineToQuad(const LINECOLLIDERINFO& tLine, const PMESHCOLLIDERINFO tDest, int index, Vector3& vCross);
	};

}