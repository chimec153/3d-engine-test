#pragma once

#include "../Types.h"

namespace Engine
{
	enum class SPACE_DIR
	{
		LBN,
		LBF,
		LTN,
		LTF,
		RBN,
		RBF,
		RTN,
		RTF,
		END
	};

	ENGINE_DLL typedef struct _tagSpace
	{
		Vector3	vPos;
		float	fSize;
		std::unique_ptr<_tagSpace> pChild[static_cast<int>(SPACE_DIR::END)];
		_tagSpace* pParent;
		// Phase E7 — DrawableList removed; no live Drawable instances.
		std::list<struct _tagPortal>	PortalList;
		bool bDelete;
		bool bCheck;

		_tagSpace() :
			vPos()
			, fSize()
			, pChild()
			, pParent(nullptr)
			, bDelete(true)
			, bCheck(false)
		{
		}

		_tagSpace(const Vector3& vPos, float fSize) :
			vPos(vPos)
			, fSize(fSize)
			, pChild()
			, pParent(nullptr)
			, bDelete(true)
			, bCheck(false)
		{
		}

		~_tagSpace()
		{
		}

		bool IsRight(const Vector4& vSphereInfo)	const
		{
			return vSphereInfo.x + vSphereInfo.w - vPos.x >= 0.f;
		}

		bool IsLeft(const Vector4& vSphereInfo)	const
		{
			return -vSphereInfo.x + vSphereInfo.w + vPos.x >= 0.f;
		}

		bool IsBottom(const Vector4& vSphereInfo)	const
		{
			return -vSphereInfo.y + vSphereInfo.w + vPos.y >= 0.f;
		}

		bool IsTop(const Vector4& vSphereInfo)	const
		{
			return vSphereInfo.y + vSphereInfo.w - vPos.y >= 0.f;
		}

		bool IsNear(const Vector4& vSphereInfo)	const
		{
			return -vSphereInfo.z + vSphereInfo.w + vPos.z >= 0.f;
		}

		bool IsFar(const Vector4& vSphereInfo)	const
		{
			return vSphereInfo.z + vSphereInfo.w - vPos.z >= 0.f;
		}

		// Phase E7 — GetTotalDrawableCount + helper templates removed
		// (DrawableList is gone).
	}SPACE, * PSPACE;


	ENGINE_DLL typedef struct _tagPortal
	{
		std::vector<Vector3>	vecPos;
		_tagSpace* pSpace;

		_tagPortal(const std::vector<Vector3>& vecPos) :
			vecPos(vecPos)
			, pSpace(nullptr)
		{
			// Phase E7 — debug-line construction removed; depended on the
			// Drawable / CreateCloneDrawable path that no longer exists.
		}
	}PORTAL, * PPORTAL;

	class ENGINE_DLL CollisionManager
	{
	private:
		CollisionManager();
		~CollisionManager();

	private:
		static CollisionManager* m_pInst;

	public:
		static CollisionManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new CollisionManager;
			}

			return m_pInst;
		}

		static void DestroyInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		typedef struct _tagCollisionChannel
		{
			SPACE* m_pSpace;
			std::list<class Collider*>	m_ColliderList;
			// Phase E7 — m_mapDrawable + FindSpaceAndErase + DeleteDrawable
			// removed (Drawable*-keyed spatial hash dropped).

			_tagCollisionChannel() :
				m_pSpace(dbg_new SPACE({}, 4096.f))
			{
			}

			~_tagCollisionChannel()
			{
				SAFE_DELETE(m_pSpace);
			}
		}COLLISIONCHANNEL, *PCOLLISIONCHANEEL;

		template <int I, int C>
		constexpr static int log2()
		{
			if constexpr(static_cast<bool>((1 << C) & I))
			{
				return C;
			}

			return log2<I, C - 1>();
		}

		template <>
		constexpr static int log2<static_cast<int>(COLLISION_CHANNEL::END) - 1, 0>()
		{
			return 0;
		}

		COLLISIONCHANNEL m_tChannel[log2<static_cast<int>(COLLISION_CHANNEL::END) - 1, 31>() + 1];
		float fAcutenessThreshold;

	public:
		// Phase E5 — AddDrawable removed (no live Drawable instances).
		void AddCollider(class Collider* pCollider);
		void VisibleTest(CAMERA_TYPE eType);
		PSPACE CreateChildSpace(SPACE* pParent, int iIndex)	const;
		void VisibleTest(PSPACE pSpace, const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* vecLocalPlanes = nullptr);
		void PortalVisibleTest(PSPACE pSpace, const std::vector<Vector4>& vecPlanes);
		bool VisibleTestNoRecursive(PSPACE pSpace, const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* vecLocalPlanes = nullptr);
		// Phase E7 — DeleteDrawable removed.
		void Collision(CAMERA_TYPE eType, float fDeltaTime);

	public:
		void Collision(float fDeltaTime);
		void VisibleTest();
	};

}