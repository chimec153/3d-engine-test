#pragma once

#include "../Types.h"
#ifdef _DEBUG
#include "../Bindable/Drawable.h"
#include "../Scene/SceneManager.h"
#include "../Scene/Scene.h"
#endif

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
		std::list<std::shared_ptr<class Drawable>>	DrawableList;
		std::list<struct _tagPortal>	PortalList;
		bool bDelete;
#ifdef _DEBUG
		//class std::shared_ptr<Drawable> pDebugBox;
#endif
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
#ifdef _DEBUG
			//, pDebugBox(nullptr)
#endif
			, bCheck(false)
		{
#ifdef _DEBUG
			/*Scene* pScene = SceneManager::GetInst()->GetScene();

			pDebugBox = pScene->CreateCloneDrawable("TextureBox", "TextureBox", pScene->FindLayer(DEFAULT_LAYER));

			const std::shared_ptr<Transform>& pTransform = pDebugBox->GetTransform();

			pTransform->SetScale(Vector3{fSize, fSize , fSize });
			pTransform->SetPosition(vPos);*/
#endif
		}

		~_tagSpace()
		{
#ifdef _DEBUG
			//pDebugBox->InActivate();
#endif
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

		template <int T>
		int GetTotalDrawableCountSub(int iPrevCount)	const
		{
			return pChild[T] ? 
				pChild[T]->GetTotalDrawableCount(iPrevCount + static_cast<int>(DrawableList.size())) :
				GetTotalDrawableCountSub<T - 1>(iPrevCount);
		}

		template <>
		int GetTotalDrawableCountSub<0>(int iPrevCount)	const
		{
			return pChild[0] ?
				pChild[0]->GetTotalDrawableCount(iPrevCount + static_cast<int>(DrawableList.size())) :
				iPrevCount + static_cast<int>(DrawableList.size());
		}

		int GetTotalDrawableCount(int iPrevCount = 0)	const
		{
			return GetTotalDrawableCountSub<7>(iPrevCount);
		}
	}SPACE, * PSPACE;


	ENGINE_DLL typedef struct _tagPortal
	{
		std::vector<Vector3>	vecPos;
		_tagSpace* pSpace;
#ifdef _DEBUG
		//std::shared_ptr<class Drawable> pDebugLine;
#endif

		_tagPortal(const std::vector<Vector3>& vecPos) :
			vecPos(vecPos)
			, pSpace(nullptr)
#ifdef _DEBUG
			//, pDebugLine(nullptr)
#endif
		{
#ifdef _DEBUG
			/*Scene* pScene = SceneManager::GetInst()->GetScene();

			pDebugLine = pScene->CreateCloneDrawable("Line", "Line", pScene->FindLayer(DEFAULT_LAYER));

			std::vector<VertexTexture> vecVertex;

			std::vector<unsigned int> vecIndex;

			for (size_t i = 0; i < vecPos.size(); ++i)
			{
				VertexTexture vertex = {};

				vertex.pos = vecPos[i];

				vecVertex.push_back(vertex);

				vecIndex.push_back(static_cast<unsigned int>(i));
			}

			vecIndex.push_back(0);

			 = *Graphics::GetInst();

			pDebugLine->CreateBindable<VertexBuffer>("Line", vecVertex);
			pDebugLine->CreateBindable<IndexBuffer>("LineIndex", vecIndex);*/
#endif

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
		SPACE* m_pSpace;
		std::list<class Collider*>	m_ColliderList;
		std::unordered_map<class Drawable*, SPACE*>	m_mapDrawable;
		float fAcutenessThreshold;
	public:
		void AddDrawable(const std::shared_ptr<class Drawable>& pDrawable);
		void AddCollider(class Collider* pCollider);
		void VisibleTest();
		PSPACE CreateChildSpace(SPACE* pParent, int iIndex)	const;
		void VisibleTest(PSPACE pSpace, const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* vecLocalPlanes = nullptr);
		void PortalVisibleTest(PSPACE pSpace, const std::vector<Vector4>& vecPlanes);
		bool VisibleTestNoRecursive(PSPACE pSpace, const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* vecLocalPlanes = nullptr);

	public:
		void Collision(float fDeltaTime);

	private:
		SPACE* FindSpaceAndErase(const std::shared_ptr<class Drawable>& pDrawable);
	};

}