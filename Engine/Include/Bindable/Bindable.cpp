#include "Bindable.h"
#include "../Core/Graphics.h"
#include "Transform.h"
#include "Agent.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexShader.h"
#include "HullShader.h"
#include "DomainShader.h"
#include "GeometryShader.h"
#include "PixelShader.h"
#include "Texture.h"
#include "Material.h"
#include "InputLayout.h"
#include "Topology.h"
#include "Mesh.h"
#include "Terrain.h"
#include "ColliderLine.h"
#include "ColliderSphere.h"
#include "ColliderMesh.h"
#include "Animation.h"
#include "PointLight.h"
#include "NavMesh.h"
#include "Particle.h"
#include "PaperBurn.h"
#include "Fluid.h"
#include "SkyBox.h"
#include "Cloth.h"
#include "Decal.h"
#include "BindableManager.h"
#include "Camera.h"
#include "BlendState.h"
#include "DepthStencilState.h"
#include "Mouse.h"
#include "ColliderOBB.h"

namespace Engine
{
	Bindable::Bindable() :
		m_eBindableType(BINDABLE_TYPE::NONE)
		, m_eObjectType(OBJECT_TYPE::BIND)
		, m_pParent(nullptr)
		, m_pScene(nullptr)
		, m_pLayer(nullptr)
	{
	}

	Bindable::Bindable(const Bindable& bindable) :
		CRef(bindable)
		, m_eBindableType(bindable.m_eBindableType)
		, m_eObjectType(bindable.m_eObjectType)
		, m_pParent(nullptr)
		, m_pScene(nullptr)
		, m_pLayer(nullptr)
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = bindable.m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = bindable.m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			AddChild((*iter)->Clone());
		}
	}

	Bindable::~Bindable()
	{
	}


	void Bindable::SetBindableType(BINDABLE_TYPE eType)
	{
		m_eBindableType = eType;
	}

	BINDABLE_TYPE Bindable::GetBindableType() const
	{
		return m_eBindableType;
	}

	Bindable* Bindable::GetParent() const
	{
		return m_pParent;
	}

	void Bindable::SetParent(Bindable* pParent)
	{
		m_pParent = pParent;
	}

	const std::list<std::shared_ptr<Bindable>>& Bindable::GetChildList() const
	{
		return m_ChildList;
	}

	void Bindable::AddChild(const std::shared_ptr<class Bindable>& pChild)
	{
		// Phase E7 — parent-Drawable AddDrawable walk removed. There are no
		// live Drawable instances anymore; the loop never fired in
		// practice. Bindable child-list bookkeeping remains.
		pChild->SetParent(this);

		m_ChildList.push_back(pChild);
	}

	void Bindable::SetObjectType(OBJECT_TYPE eType)
	{
		m_eObjectType = eType;
	}

	OBJECT_TYPE Bindable::GetObjectType() const
	{
		return m_eObjectType;
	}


	std::shared_ptr<Bindable> Bindable::FindChild(BINDABLE_TYPE eType) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetBindableType() == eType)
			{
				return *iter;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Bindable::FindChilds(BINDABLE_TYPE eType, std::vector<std::shared_ptr<Bindable>>& vecBindables) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetBindableType() == eType)
			{
				vecBindables.push_back(*iter);
			}
		}
	}

	std::shared_ptr<Bindable> Bindable::FindChild(const std::string& strTag) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetTag() == strTag)
			{
				return *iter;
			}

			std::shared_ptr<Bindable> pChild = (*iter)->FindChild(strTag);

			if (pChild != nullptr)
			{
				return pChild;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Bindable::DeleteChild(std::shared_ptr<Bindable> pBindable)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter) == pBindable)
			{
				(*iter)->SetParent(nullptr);

				m_ChildList.erase(iter);
				return;
			}
		}
	}

	std::shared_ptr<Bindable> Bindable::FindChild(OBJECT_TYPE eType) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetObjectType() == eType)
			{
				return *iter;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Bindable::SetScene(Scene* pScene)
	{
		m_pScene = pScene;
	}

	void Bindable::SetLayer(Layer* pLayer)
	{
		m_pLayer = pLayer;
	}

	Scene* Bindable::GetScene() const
	{
		return m_pScene;
	}

	Layer* Bindable::GetLayer() const
	{
		return m_pLayer;
	}

	bool Bindable::Init()
	{
		return true;
	}

	void Bindable::Start()
	{
	}

	void Bindable::Input(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->Input(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::Update(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->Update(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::FixedUpdate(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				iterEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->FixedUpdate(fDeltaTime);
			++iter;
		}
	}

	void Bindable::Collision(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->Collision(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::PostUpdate(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->PostUpdate(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::PreDraw(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				iterEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->PreDraw(fDeltaTime);
			++iter;
		}
	}

	void Bindable::Bind()
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

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
	}

	void Bindable::PostBind()
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->PostBind();
		}
	}

	std::shared_ptr<Bindable> Bindable::Clone()
	{
		return nullptr;
	}

	void Bindable::Reset()
	{
	}

	void Bindable::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_eObjectType, 4, 1, pFile);

		int iChildCount = static_cast<int>(m_ChildList.size());

		fwrite(&iChildCount, 4, 1, pFile);

		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			BINDABLE_TYPE eType = (*iter)->GetBindableType();

			fwrite(&eType, 4, 1, pFile);

			switch (eType)
			{
			case Engine::BINDABLE_TYPE::VERTEX_BUFFER:
			case Engine::BINDABLE_TYPE::INDEX_BUFFER:
			case Engine::BINDABLE_TYPE::VERTEX_SHADER:
			case Engine::BINDABLE_TYPE::HULL_SHADER:
			case Engine::BINDABLE_TYPE::DOMAIN_SHADER:
			case Engine::BINDABLE_TYPE::GEOMETRY_SHADER:
			case Engine::BINDABLE_TYPE::PIXEL_SHADER:
			case Engine::BINDABLE_TYPE::TEXTURE:
			case Engine::BINDABLE_TYPE::INPUTLAYOUT:
			case Engine::BINDABLE_TYPE::TOPOLOGY:
			case Engine::BINDABLE_TYPE::MESH:
			case Engine::BINDABLE_TYPE::BLEND_STATE:
			case Engine::BINDABLE_TYPE::DEPTH_STENCIL_STATE:
			case Engine::BINDABLE_TYPE::RASTERIZER_STATE:
			{
				(*iter)->CRef::Save(pFile);
			}
				break;
			case Engine::BINDABLE_TYPE::MATERIAL:
			case Engine::BINDABLE_TYPE::TRANSFORM:
			case Engine::BINDABLE_TYPE::TERRAIN:
			case Engine::BINDABLE_TYPE::COLLIDER_LINE:
			case Engine::BINDABLE_TYPE::COLLIDER_SPHERE:
			case Engine::BINDABLE_TYPE::COLLIDER_MESH:
			case Engine::BINDABLE_TYPE::ANIMATION:
			case Engine::BINDABLE_TYPE::AGENT:
			case Engine::BINDABLE_TYPE::NAV_MESH:
			case Engine::BINDABLE_TYPE::LIGHT:
			case Engine::BINDABLE_TYPE::PARTICLE:
			case Engine::BINDABLE_TYPE::DECAL:
			case Engine::BINDABLE_TYPE::PAPERBURN:
			case Engine::BINDABLE_TYPE::FLUID:
			case Engine::BINDABLE_TYPE::SKYBOX:
			case Engine::BINDABLE_TYPE::CLOTH:
			case Engine::BINDABLE_TYPE::CAMERA:
			case Engine::BINDABLE_TYPE::DRAWABLE:
				(*iter)->Save(pFile);
				break;
			default:
				assert(false);
			}
		}
	}

	void Bindable::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_eObjectType, 4, 1, pFile);

		int iChildCount = 0;

		fread(&iChildCount, 4, 1, pFile);

		for (int i = 0; i < iChildCount; ++i)
		{
			BINDABLE_TYPE eType = BINDABLE_TYPE::NONE;

			fread(&eType, 4, 1, pFile);

			// Phase E7 — Transform-as-Bindable records in legacy save files.
			// No live Drawable hosts remain; just consume the bytes via a
			// temporary so the file pointer advances correctly.
			if (eType == BINDABLE_TYPE::TRANSFORM)
			{
				Transform tmp;
				tmp.Load(pFile);
				continue;
			}

			std::shared_ptr<Bindable> pBindable = CreateBindable(eType);

			if (!pBindable)
			{
				int iLength = 0;

				fread(&iLength, 4, 1, pFile);

				if (iLength)
				{
					std::unique_ptr<char[]> strBind = std::make_unique<char[]>(iLength + 1);

					strBind[iLength] = 0;

					fread(strBind.get(), 1, iLength, pFile);

					pBindable = FindBindable(eType, strBind.get());

					if (!pBindable)
					{
						continue;
					}
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				pBindable->SetScene(m_pScene);

				pBindable->SetLayer(m_pLayer);

				pBindable->SetParent(this);

				pBindable->Load(pFile);
			}
			
			AddChild(pBindable);
		}
	}

	std::shared_ptr<Bindable> Bindable::CreateBindable(BINDABLE_TYPE eType)
	{
		switch (eType)
		{
		case Engine::BINDABLE_TYPE::VERTEX_BUFFER:
			return nullptr;
		case Engine::BINDABLE_TYPE::INDEX_BUFFER:
			return nullptr;
		case Engine::BINDABLE_TYPE::VERTEX_SHADER:
			return nullptr;
		case Engine::BINDABLE_TYPE::HULL_SHADER:
			return nullptr;
		case Engine::BINDABLE_TYPE::DOMAIN_SHADER:
			return nullptr;
		case Engine::BINDABLE_TYPE::GEOMETRY_SHADER:
			return nullptr;
		case Engine::BINDABLE_TYPE::PIXEL_SHADER:
			return nullptr;
		case Engine::BINDABLE_TYPE::TEXTURE:
			return nullptr;
		case Engine::BINDABLE_TYPE::MATERIAL:
			return std::make_shared<Material>();
		case Engine::BINDABLE_TYPE::TRANSFORM:
			// Phase B.3 — Transform migrated to Component. Save records
			// from old files are handled in Bindable::Load via a special
			// case (apply to owning Drawable's existing transform).
			return nullptr;
		case Engine::BINDABLE_TYPE::INPUTLAYOUT:
			return nullptr;
		case Engine::BINDABLE_TYPE::TOPOLOGY:
			return nullptr;
		case Engine::BINDABLE_TYPE::MESH:
			return nullptr;
		case Engine::BINDABLE_TYPE::TERRAIN:
			// Phase E5 — Terrain migrated to GameObject.
			return nullptr;
		case Engine::BINDABLE_TYPE::COLLIDER_LINE:
		case Engine::BINDABLE_TYPE::COLLIDER_SPHERE:
		case Engine::BINDABLE_TYPE::COLLIDER_MESH:
		case Engine::BINDABLE_TYPE::COLLIDER_OBB:
			// Phase B.4 — Collider hierarchy migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::ANIMATION:
			// Phase E3 — Animation migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::AGENT:
			// Phase B.4 — Agent migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::NAV_MESH:
			// Phase B.4 — NavMesh migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::LIGHT:
			// Phase B.7 — PointLight migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::PARTICLE:
			// Phase E5 — Particle migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::DECAL:
			// Phase E5 — Decal migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::PAPERBURN:
			// Phase E5 — PaperBurn migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::FLUID:
			// Phase E5 — Fluid migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::SKYBOX:
			// Phase E5 — SkyBox migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::CLOTH:
			// Phase E5 — Cloth migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::CAMERA:
			// Phase B.5 — Camera migrated to Component.
			return nullptr;
		case Engine::BINDABLE_TYPE::DRAWABLE:
			// Phase E7 — DRAWABLE factory entry kept for the BINDABLE_TYPE
			// enum value but no longer instantiates a Drawable. Live entities
			// are GameObjects today; legacy scenes would have stored Drawables
			// here, but Layer::Load no longer drives this path (count=0).
			return nullptr;
		case Engine::BINDABLE_TYPE::BLEND_STATE:
			return nullptr;
		case Engine::BINDABLE_TYPE::DEPTH_STENCIL_STATE:
			return nullptr;
		case Engine::BINDABLE_TYPE::RASTERIZER_STATE:
			return nullptr;
		case Engine::BINDABLE_TYPE::MOUSE:
			// Phase B.6 — Mouse migrated to Component.
			return nullptr;
		default:
			assert(false);
		}

		return std::shared_ptr<Bindable>();
	}
	std::shared_ptr<Bindable> Bindable::FindBindable(BINDABLE_TYPE eType, const std::string& strBind)
	{
		switch (eType)
		{
		case Engine::BINDABLE_TYPE::VERTEX_BUFFER:
			return StaticFindBindable<VertexBuffer>(strBind);
		case Engine::BINDABLE_TYPE::INDEX_BUFFER:
			return StaticFindBindable<IndexBuffer>(strBind);
		case Engine::BINDABLE_TYPE::VERTEX_SHADER:
			return StaticFindBindable<VertexShader>(strBind);
		case Engine::BINDABLE_TYPE::HULL_SHADER:
			return StaticFindBindable<HullShader>(strBind);
		case Engine::BINDABLE_TYPE::DOMAIN_SHADER:
			return StaticFindBindable<DomainShader>(strBind);
		case Engine::BINDABLE_TYPE::GEOMETRY_SHADER:
			return StaticFindBindable<GeometryShader>(strBind);
		case Engine::BINDABLE_TYPE::PIXEL_SHADER:
			return StaticFindBindable<PixelShader>(strBind);
		case Engine::BINDABLE_TYPE::TEXTURE:
			return StaticFindBindable<Texture>(strBind);
		case Engine::BINDABLE_TYPE::MATERIAL:
			break;
		case Engine::BINDABLE_TYPE::TRANSFORM:
			break;
		case Engine::BINDABLE_TYPE::INPUTLAYOUT:
			return StaticFindBindable<InputLayout>(strBind);
		case Engine::BINDABLE_TYPE::TOPOLOGY:
			return StaticFindBindable<Topology>(strBind);
		case Engine::BINDABLE_TYPE::MESH:
			return StaticFindBindable<Mesh>(strBind);
		case Engine::BINDABLE_TYPE::TERRAIN:
			break;
		case Engine::BINDABLE_TYPE::COLLIDER_LINE:
			break;
		case Engine::BINDABLE_TYPE::COLLIDER_SPHERE:
			break;
		case Engine::BINDABLE_TYPE::COLLIDER_MESH:
			break;
		case Engine::BINDABLE_TYPE::COLLIDER_OBB:
			break;
		case Engine::BINDABLE_TYPE::ANIMATION:
			break;
		case Engine::BINDABLE_TYPE::AGENT:
			break;
		case Engine::BINDABLE_TYPE::NAV_MESH:
			break;
		case Engine::BINDABLE_TYPE::LIGHT:
			break;
		case Engine::BINDABLE_TYPE::PARTICLE:
			break;
		case Engine::BINDABLE_TYPE::DECAL:
			break;
		case Engine::BINDABLE_TYPE::PAPERBURN:
			break;
		case Engine::BINDABLE_TYPE::FLUID:
			break;
		case Engine::BINDABLE_TYPE::SKYBOX:
			break;
		case Engine::BINDABLE_TYPE::CLOTH:
			break;
		case Engine::BINDABLE_TYPE::CAMERA:
			break;
		case Engine::BINDABLE_TYPE::DRAWABLE:
			break;
		case Engine::BINDABLE_TYPE::BLEND_STATE:
			return StaticFindBindable<BlendState>(strBind);
			break;
		case Engine::BINDABLE_TYPE::DEPTH_STENCIL_STATE:
			return StaticFindBindable<DepthStencilState>(strBind);
			break;
		case Engine::BINDABLE_TYPE::RASTERIZER_STATE:
			return StaticFindBindable<RasterizerState>(strBind);
			break;
		case Engine::BINDABLE_TYPE::MOUSE:
			break;
		default:
			assert(false);
		}
		return std::shared_ptr<Bindable>();
	}
}