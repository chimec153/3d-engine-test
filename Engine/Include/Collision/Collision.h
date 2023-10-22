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

	public:
		static bool CollisionLineToSphere(class ColliderLine* pSrc, class ColliderSphere* pDest);
		static bool CollisionSphereToSphere(class ColliderSphere* pSrc, class ColliderSphere* pDest, float fDeltaTime);
		static bool CollisionLineToMesh(class ColliderLine* pSrc, class ColliderMesh* pDest);
	};

}