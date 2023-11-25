#include "ImguiManager.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "Core/Graphics.h"
#include "Input/Input.h"
#include "Core/Window.h"
#include "Scene/Layer.h"
#include "Bindable/Drawable.h"
#include "Core/PathManager.h"
#include <commdlg.h>
#include "Bindable/Mesh.h"
#include "Bindable/VertexShader.h"
#include "Bindable/HullShader.h"
#include "Bindable/DomainShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Texture.h"
#include "Bindable/Material.h"
#include "Bindable/TransformBuffer.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/ConstantBuffer.h"
#include "Bindable/Bindable.h"
#include "Bindable/BindableManager.h"
#include "Navigation/Detour/DetourNavMeshBuilder.h"
#include "Navigation/Detour/DetourNavMesh.h"
#include "Navigation/Detour/DetourNavMeshQuery.h"
#include "Navigation/Detour/DetourCrowd.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Bindable/ColliderMesh.h"
#include "Bindable/Agent.h"
#include "Bindable/NavMesh.h"
#include "Input/Input.h"
#include "../Object/Player.h"
#include "Bindable/Animation.h"
#include "Animation/Sequence.h"
#include "Animation/Skeleton.h"
#include "Bindable/PointLight.h"
#include "Bindable/Sphere.h"
#include "Render/MRT.h"
#include "Bindable/Particle.h"
#include "Bindable/Cloth.h"

ImguiManager* ImguiManager::m_pInst = nullptr;

ImguiManager::ImguiManager() :
	m_bDemoWindow(false)
	, m_pHeightField(nullptr)
	, m_pCompactHeightField(nullptr)
	, m_pContourSet(nullptr)
	, m_pPolyMesh(nullptr)
	, m_pPolyMeshDetail(nullptr)
	, m_bMode(true)
{
	m_fCellSize = 0.3f;
	m_fCellHeight = 0.2f;
	m_fAgentHeight = 2.f;
	m_fAgentRadius = 0.6f;
	m_fAgentClimb = 1.9f;
	m_fAgentSlopeAngle = 60.f;
	m_fMaxEdgeLen = 12.f;
	m_fMaxEdgeError = 1.3f;
	m_fRegionMinSize = 8;
	m_fRegionMergeSize = 20;
	m_fVertsPerPoly = 6.f;
	m_fDetailSampleDist = 6.f;
	m_fDetailSampleMaxError = 1.f;
}

ImguiManager::~ImguiManager()
{
	if (m_pPolyMesh)
	{
		rcFreePolyMesh(m_pPolyMesh);
		m_pPolyMesh = nullptr;
	}
	if (m_pPolyMeshDetail)
	{
		rcFreePolyMeshDetail(m_pPolyMeshDetail);
		m_pPolyMeshDetail = nullptr;
	}
	if (m_pContourSet)
	{
		rcFreeContourSet(m_pContourSet);
		m_pContourSet = nullptr;
	}
	if (m_pCompactHeightField)
	{
		rcFreeCompactHeightfield(m_pCompactHeightField);
		m_pCompactHeightField = nullptr;
	}
	if (m_pHeightField)
	{
		rcFreeHeightField(m_pHeightField);
		m_pHeightField = nullptr;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool ImguiManager::Init(HWND hwnd)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	if (!ImGui_ImplWin32_Init(hwnd))
	{
		assert(false);
		return false;
	}

	if (!ImGui_ImplDX11_Init(Engine::Graphics::GetInst()->GetDevice(), Engine::Graphics::GetInst()->GetDeviceContext()))
	{
		assert(false);
		return false;
	}

	return true;
}

void ImguiManager::Update(float fDeltaTime)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::UP, DIK_SPACE))
	{
		m_bDemoWindow ^= true;
	}

	ImGui::Begin("Timer");

	float fScale = Engine::Window::GetInst()->GetTimer()->GetScale();

	if (ImGui::SliderFloat("Scale", &fScale, 0.f, 5.f))
	{
		Engine::Window::GetInst()->GetTimer()->SetScale(fScale);
	}

	ImGui::SameLine();

	if (ImGui::Button("stop"))
	{
		Engine::Window::GetInst()->Stop();
	}

	ImGui::SameLine();

	if (ImGui::Button("resume"))
	{
		Engine::Window::GetInst()->Resume();
	}

	ImGui::End();
}

void ImguiManager::Render(float fDeltaTime)
{
	if (m_bDemoWindow)
	{
		ImGui::ShowDemoWindow();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void ImguiManager::DisableMouse()
{
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

void ImguiManager::EnableMouse()
{
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}

void ImguiManager::CRef_ImGuiWindow(std::shared_ptr<Engine::CRef> pRef)
{
	ImGui::Text("Name: %s", pRef->GetTag().c_str());

	ImGui::SameLine();

	bool bEnable = pRef->IsEnable();

	if (ImGui::Checkbox("Enable", &bEnable))
	{
		if (bEnable)
		{
			pRef->Enable();
		}
		else
		{
			pRef->Disable();
		}
	}

	ImGui::SameLine();

	bool bActive = pRef->IsActive();

	if (ImGui::Checkbox("Active", &bActive))
	{
		if (!bActive)
		{
			pRef->InActivate();
		}
	}
}

void ImguiManager::JointSocket_ImGuiWindow(std::shared_ptr<Engine::JointSocket> pRef, int _iIndex)
{
	CRef_ImGuiWindow(pRef);

	char pText[TEXT_LEN] = {};

	sprintf_s(pText, "%d parent index", _iIndex);

	int iIndex = pRef->GetParentIndex();

	if (ImGui::InputInt(pText, &iIndex))
	{
		pRef->SetParentIndex(iIndex);
	}

	Engine::Vector3 vScale = pRef->GetScale();
	Engine::Vector3 vPos = pRef->GetPosition();
	Engine::Vector3 vQuternion = pRef->GetRotation();

	sprintf_s(pText, "%d Joint Scale", _iIndex);

	if (ImGui::InputFloat3(pText, &vScale.x))
	{
		pRef->SetScale(vScale);
		pRef->UpdateJointMatrix();
	}

	sprintf_s(pText, "%d Joint Position", _iIndex);

	if (ImGui::InputFloat3(pText, &vPos.x))
	{
		pRef->SetPosition(vPos);
		pRef->UpdateJointMatrix();
	}

	sprintf_s(pText, "%d Joint Rotation", _iIndex);

	if (ImGui::InputFloat3(pText, &vQuternion.x))
	{
		pRef->SetRotation(vQuternion);
		pRef->UpdateJointMatrix();
	}
}
void ImguiManager::SceneWindow(Engine::Scene* pScene)
{


	//Layer_DrawListImgui();
}
//
//	void Skeleton::ImGuiWindow()
//	{
//		__super::ImGuiWindow();
//
//		static int iIndex = -1;
//
//		std::vector<const char*> vecSocket;
//		std::vector<std::shared_ptr<JointSocket>> _vecSocket;
//
//		SocketList::iterator iter = m_SocketList.begin();
//		SocketList::iterator iterEnd = m_SocketList.end();
//
//		for (; iter != iterEnd; ++iter)
//		{
//			vecSocket.push_back((*iter)->GetTag().c_str());
//			_vecSocket.push_back(*iter);
//		}
//
//		if (vecSocket.size())
//		{
//			ImGui::ListBox("sockek list", &iIndex, &vecSocket[0], static_cast<int>(vecSocket.size()));
//		}
//
//		if (iIndex >= 0 && iIndex < _vecSocket.size())
//		{
//			_vecSocket[iIndex]->ImGuiWindow();
//		}
//	}
//
//	void Camera::ImGuiWindow()
//	{
//		__super::ImGuiWindow();
//
//		ImGui::SliderFloat("Camera Speed", &m_fSpeed, 0.f, 500000.f);
//
//		ImGui::Checkbox("Control", &m_bControl);
//	}
//
//
void ImguiManager::Drawable_ShowImGuiWindow(std::shared_ptr<Engine::Bindable> pDrawable)
{
	static std::shared_ptr<Engine::Bindable> pSelect = nullptr;

	bool bSelect = false;

	if (ImGui::Begin("Node View"))
	{
		std::shared_ptr<Engine::Bindable> pResult = Drawable_ShowImGuiTree(pDrawable, bSelect);

		if (pResult)
		{
			pSelect = pResult;
		}
	}

	ImGui::End();

	if (pSelect != nullptr)
	{
		if (ImGui::Begin("Drawable", &bSelect))
		{
			Drawable_ImGuiWindow(pSelect);
		}
		ImGui::End();
	}
}

std::shared_ptr<Engine::Bindable> ImguiManager::Drawable_ShowImGuiTree(std::shared_ptr<Engine::Bindable> pDrawable, bool& bSelect)
{
	std::string strNode = "Node";

	strNode += pDrawable->GetTag();

	std::shared_ptr<Engine::Bindable> _pSelect = nullptr;

	if (ImGui::TreeNode(strNode.c_str()))
	{
		strNode += "_s";

		if (bSelect)
		{
		}

		if (ImGui::Selectable(strNode.c_str()))
		{
			_pSelect = pDrawable;
			bSelect = true;
		}
		const std::list<std::shared_ptr<Engine::Bindable>>& ChildList = pDrawable->GetChildList();
#ifdef _DEBUG
		static int iChildOffset = 0;


		if (ImGui::InputInt("child", &iChildOffset, 0, static_cast<int>(ChildList.size())))
		{
			std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iter = ChildList.begin();
			std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iterEnd = ChildList.end();

			for (int i = 0; iter != iterEnd; ++iter, ++i)
			{
				if (i >= iChildOffset)
				{
					(*iter)->Enable();
				}
				else
				{
					(*iter)->Disable();
				}
			}
		}
#endif

		std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iter = ChildList.begin();
		std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iterEnd = ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			bool _bSelect = bSelect;

			std::shared_ptr<Engine::Bindable> pSelect = Drawable_ShowImGuiTree(*iter, bSelect);

			if (pSelect != nullptr)
			{
				_pSelect = pSelect;
			}
		}

		ImGui::TreePop();
	}

	if (false)
	{
		return std::static_pointer_cast<Engine::Drawable>(pDrawable->shared_from_this());
	}

	return _pSelect;
}

void ImguiManager::LoadNavMesh(const TCHAR* pFullPath, class Engine::Scene* pScene)
{
	if (!pScene)
	{
		return;
	}

	Engine::FbxLoader loader;

	TCHAR strExt[_MAX_EXT] = {};

	_tsplitpath_s(pFullPath, nullptr, 0, nullptr, 0, nullptr, 0, strExt, _MAX_EXT);

	_tcsupr_s(strExt);

	if (!_tcscmp(strExt, TEXT(".OBJ")))
	{
		loader.LoadOBJ(pFullPath, "");
	}
	else if (!_tcscmp(strExt, TEXT(".FBX")))
	{
		loader.Init();

		loader.LoadFile(pFullPath, "");
	}

	int iCount = loader.GetLODCount();

	std::vector<float> vecPoint;
	std::vector<int> vecTris;
	Engine::Vector3 vMax = { FLT_MIN, FLT_MIN, FLT_MIN };
	Engine::Vector3 vMin = { FLT_MAX, FLT_MAX, FLT_MAX };

	std::vector<std::vector<Engine::VertexStandard>> vecVertexAll;
	std::vector<std::vector<std::vector<unsigned int>>> vecIndexAll;

	int iVertex = 0;

	for (int i = 0; i < iCount; ++i)
	{
		std::vector<Engine::VertexStandard> vecVertex = loader.GetVertexData(i);
		std::vector<std::vector<unsigned int>> vecIndex = loader.GetIndexData(i);

		for (size_t j = 0; j < vecVertex.size(); ++j)
		{
			vecPoint.push_back(vecVertex[j].pos.x);
			vecPoint.push_back(vecVertex[j].pos.y);
			vecPoint.push_back(vecVertex[j].pos.z);

			vMax.x = vecVertex[j].pos.x < vMax.x ? vMax.x : vecVertex[j].pos.x;
			vMax.y = vecVertex[j].pos.y < vMax.y ? vMax.y : vecVertex[j].pos.y;
			vMax.z = vecVertex[j].pos.z < vMax.z ? vMax.z : vecVertex[j].pos.z;

			vMin.x = vecVertex[j].pos.x > vMin.x ? vMin.x : vecVertex[j].pos.x;
			vMin.y = vecVertex[j].pos.y > vMin.y ? vMin.y : vecVertex[j].pos.y;
			vMin.z = vecVertex[j].pos.z > vMin.z ? vMin.z : vecVertex[j].pos.z;

			vecVertex[j].normal.x = 0.f;
			vecVertex[j].normal.y = 1.f;
			vecVertex[j].normal.z = 0.f;

			vecVertex[j].tangent.x = 1.f;
			vecVertex[j].tangent.y = 0.f;
			vecVertex[j].tangent.z = 0.f;
		}

		for (size_t j = 0; j < vecIndex.size(); ++j)
		{
			for (size_t k = 0; k < vecIndex[j].size(); ++k)
			{
				vecTris.push_back(static_cast<int>(vecIndex[j][k]) + iVertex);
			}
		}

		iVertex += static_cast<int>(vecVertex.size());

		vecVertexAll.push_back(vecVertex);
		vecIndexAll.push_back(vecIndex);
	}

	std::shared_ptr<Engine::Drawable> pNavigation = pScene->CreateDrawable<Engine::Drawable>("Navigation", pScene->FindLayer(DEFAULT_LAYER));

	if (!pNavigation)
	{
		return;
	}

	std::shared_ptr<Engine::ColliderMesh> pColliderMesh = pNavigation->CreateBindable<Engine::ColliderMesh>("ColliderMesh");

	pColliderMesh->SetInfo(vecPoint, vecTris);

	pNavigation->CreateBindable<Engine::Mesh>("NavMesh", vecVertexAll, vecIndexAll);

	pNavigation->FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
	pNavigation->FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoTexture");
	pNavigation->FindAndAddBind<Engine::InputLayout>("Standard");
	pNavigation->FindAndAddBind<Engine::Topology>("TriangleList");

	std::shared_ptr<Engine::Material> pMaterial = Engine::StaticFindBindable<Engine::Material>("Material");

	pNavigation->AddChild(pMaterial->Clone());

	m_pNavMesh = CreateNavMesh(vecPoint, vecTris, vMax, vMin);

	m_pNavMesh->SetTag("NavigationMesh");

	pNavigation->AddChild(m_pNavMesh);

	pColliderMesh->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &ImguiManager::CollisionStay);
}

void ImguiManager::LoadNavMesh(Engine::Scene* pScene, const TCHAR* pFilePath, const std::string& strPathKey)
{
	TCHAR strFullPath[MAX_PATH] = {};

	const TCHAR* pPath = Engine::CPathManager::GetInst()->FindPath(strPathKey);

	if (pPath)
	{
		_tcscpy_s(strFullPath, pPath);
	}

	_tcscat_s(strFullPath, pFilePath);

	LoadNavMesh(strFullPath, pScene);
}

std::shared_ptr<Engine::NavMesh> ImguiManager::CreateNavMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecTris, const Engine::Vector3& vMax, const Engine::Vector3& vMin)
{
	rcConfig config = {};

	memcpy_s(config.bmax, 12, &vMax.x, 12);
	memcpy_s(config.bmin, 12, &vMin.x, 12);

	config.cs = m_fCellSize;
	config.ch = m_fCellHeight;
	config.walkableSlopeAngle = m_fAgentSlopeAngle;
	config.walkableHeight = static_cast<int>(ceilf(m_fAgentHeight / config.ch));
	config.walkableRadius = static_cast<int>(ceilf(m_fAgentRadius / config.cs));
	config.walkableClimb = static_cast<int>(floorf(m_fAgentClimb / config.ch));
	config.maxEdgeLen = static_cast<int>(m_fMaxEdgeLen / m_fCellSize);
	config.maxSimplificationError = m_fMaxEdgeError;
	config.minRegionArea = static_cast<int>(m_fRegionMinSize * m_fRegionMinSize);
	config.mergeRegionArea = static_cast<int>(m_fRegionMergeSize * m_fRegionMergeSize);
	config.maxVertsPerPoly = static_cast<int>(m_fVertsPerPoly);
	config.borderSize = config.walkableRadius + 3;
	rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);
	config.detailSampleDist = m_fDetailSampleDist < 0.9f ? 0 : m_fCellSize * m_fDetailSampleDist;
	config.detailSampleMaxError = m_fCellHeight * m_fDetailSampleMaxError;

	config.bmin[0] -= config.borderSize * config.cs;
	config.bmin[2] -= config.borderSize * config.cs;
	config.bmax[0] += config.borderSize * config.cs;
	config.bmax[2] += config.borderSize * config.cs;

	m_pHeightField = rcAllocHeightfield();

	if (!m_pHeightField)
	{
		return nullptr;
	}

	if (!rcCreateHeightfield(&m_tContext, *m_pHeightField, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
	{
		return nullptr;
	}

	m_pTriAreas = std::make_unique<unsigned char[]>(vecTris.size() / 3);

	if (!m_pTriAreas)
	{
		return nullptr;
	}

	memset(m_pTriAreas.get(), 0, vecTris.size() / 3);

	rcMarkWalkableTriangles(&m_tContext, m_fAgentSlopeAngle, &vecPoint[0], static_cast<int>(vecPoint.size()), &vecTris[0], static_cast<int>(vecTris.size() / 3), m_pTriAreas.get());

	if (!rcRasterizeTriangles(&m_tContext, &vecPoint[0], static_cast<int>(vecPoint.size()), &vecTris[0], m_pTriAreas.get(), static_cast<int>(vecTris.size() / 3), *m_pHeightField, config.walkableClimb))
	{
		return nullptr;
	}

	m_pCompactHeightField = rcAllocCompactHeightfield();

	if (!m_pCompactHeightField)
	{
		return nullptr;
	}

	if (!rcBuildCompactHeightfield(&m_tContext, config.walkableHeight, config.walkableClimb, *m_pHeightField, *m_pCompactHeightField))
	{
		return nullptr;
	}

	if (!rcErodeWalkableArea(&m_tContext, config.walkableRadius, *m_pCompactHeightField))
	{
		return nullptr;
	}

	if (!rcBuildDistanceField(&m_tContext, *m_pCompactHeightField))
	{
		return nullptr;
	}

	if (!rcBuildRegions(&m_tContext, *m_pCompactHeightField, 0, config.minRegionArea, config.mergeRegionArea))
	{
		return nullptr;
	}

	m_pContourSet = rcAllocContourSet();

	if (!m_pContourSet)
	{
		return nullptr;
	}

	if (!rcBuildContours(&m_tContext, *m_pCompactHeightField, config.maxSimplificationError, config.maxEdgeLen, *m_pContourSet))
	{
		return nullptr;
	}

	m_pPolyMesh = rcAllocPolyMesh();

	if (!m_pPolyMesh)
	{
		return nullptr;
	}

	if (!rcBuildPolyMesh(&m_tContext, *m_pContourSet, config.maxVertsPerPoly, *m_pPolyMesh))
	{
		return nullptr;
	}

	m_pPolyMeshDetail = rcAllocPolyMeshDetail();

	if (!m_pPolyMeshDetail)
	{
		return nullptr;
	}

	if (!rcBuildPolyMeshDetail(&m_tContext, *m_pPolyMesh, *m_pCompactHeightField, config.detailSampleDist, config.detailSampleMaxError, *m_pPolyMeshDetail))
	{
		return nullptr;
	}

	for (int i = 0; i < m_pPolyMesh->npolys; ++i)
	{
		if (m_pPolyMesh->areas[i] == RC_WALKABLE_AREA)
		{
			m_pPolyMesh->areas[i] = 0;
		}

		if (m_pPolyMesh->areas[i] == 0)
		{
			m_pPolyMesh->flags[i] = 1;
		}
	}

	dtNavMeshCreateParams tParams = {};

	tParams.verts = m_pPolyMesh->verts;
	tParams.vertCount = m_pPolyMesh->nverts;
	tParams.polys = m_pPolyMesh->polys;
	tParams.polyAreas = m_pPolyMesh->areas;
	tParams.polyFlags = m_pPolyMesh->flags;
	tParams.polyCount = m_pPolyMesh->npolys;
	tParams.nvp = m_pPolyMesh->nvp;
	tParams.detailMeshes = m_pPolyMeshDetail->meshes;
	tParams.detailVerts = m_pPolyMeshDetail->verts;
	tParams.detailVertsCount = m_pPolyMeshDetail->nverts;
	tParams.detailTris = m_pPolyMeshDetail->tris;
	tParams.detailTriCount = m_pPolyMeshDetail->ntris;
	tParams.walkableHeight = m_fAgentHeight;
	tParams.walkableRadius = m_fAgentRadius;
	tParams.walkableClimb = m_fAgentClimb;
	memcpy_s(tParams.bmin, 12, m_pPolyMesh->bmin, 12);
	memcpy_s(tParams.bmax, 12, m_pPolyMesh->bmax, 12);
	tParams.cs = config.cs;
	tParams.ch = config.ch;
	tParams.buildBvTree = true;

	return std::make_shared<Engine::NavMesh>(tParams, m_fAgentRadius, m_fAgentHeight);
}

void ImguiManager::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{
	if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::LEFT))
	{
		Engine::Collider* pNavOwner = nullptr;

		if ((pSrc->GetBindableType() == Engine::BINDABLE_TYPE::COLLIDER_LINE && pSrc->GetTag() == "MouseLine"))
		{
			pNavOwner = pDest;
		}

		else if (pDest->GetBindableType() == Engine::BINDABLE_TYPE::COLLIDER_LINE && pDest->GetTag() == "MouseLine")
		{
			pNavOwner = pSrc;
		}

		if(pNavOwner)
		{
			if (m_bMode)
			{
				Engine::Scene* pScene = Engine::SceneManager::GetInst()->GetScene();

				std::shared_ptr<Engine::Layer> pLayer = pScene->FindLayer(DEFAULT_LAYER);

				char strPlayer[TEXT_LEN] = {};

				sprintf_s(strPlayer, "Player_%d", static_cast<int>(m_PlayerList.size()));

				std::shared_ptr<Player> pPlayer = std::static_pointer_cast<Player>(pScene->CreateCloneDrawable(strPlayer, "Player", pLayer, Engine::SCENE_TYPE::CURRENT));

				if (pPlayer)
				{
					Engine::Bindable* pParent = pNavOwner->GetParent();

					if (pParent)
					{
						std::shared_ptr<Engine::NavMesh> pNavMesh = std::static_pointer_cast<Engine::NavMesh>(pParent->FindChild(Engine::BINDABLE_TYPE::NAV_MESH));

						pPlayer->CreateAgent(pNavMesh, pSrc->GetCross());
					}
				}

				m_PlayerList.push_back(pPlayer);
			}
			else
			{
				std::list<std::shared_ptr<Player>>::const_iterator iter = m_PlayerList.begin();
				std::list<std::shared_ptr<Player>>::const_iterator iterEnd = m_PlayerList.end();

				for (; iter != iterEnd; ++iter)
				{
					(*iter)->Move(pSrc->GetCross());
				}
			}
		}
	}
}

std::vector<const char*> vecTypes = {
	"VERTEX_BUFFER",
	"INDEX_BUFFER",
	"VERTEX_SHADER",
	"HULL_SHADER",
	"DOMAIN_SHADER",
	"GEOMETRY_SHADER",
	"PIXEL_SHADER",
	"TEXTURE",
	"MATERIAL",
	"TRANSFORM",
	"INPUTLAYOUT",
	"TOPOLOGY",
	"MESH"
};

void ImguiManager::Drawable_ImGuiWindow(std::shared_ptr<Engine::Bindable> pDrawable)
{
	if (ImGui::Button("reset"))
	{
		pDrawable->Reset();
	}

	ImGui::SameLine();

	Engine::BINDABLE_TYPE eDrawableType = pDrawable->GetBindableType();

	switch (eDrawableType)
	{
	case Engine::BINDABLE_TYPE::TRANSFORM:
		TransformBuffer_ImGuiWindow(std::static_pointer_cast<Engine::Transform>(pDrawable));
		break;
	case Engine::BINDABLE_TYPE::MESH:
		Mesh_ImGuiWindow(std::static_pointer_cast<Engine::Mesh>(pDrawable));
		break;
	case Engine::BINDABLE_TYPE::MATERIAL:
		Material_ImGuiWindow(std::static_pointer_cast<Engine::Material>(pDrawable));
		break;
	case Engine::BINDABLE_TYPE::LIGHT:
		PointLight_ImGuiWindow(std::static_pointer_cast<Engine::PointLight>(pDrawable));
		break;
	case Engine::BINDABLE_TYPE::ANIMATION:
	{
		std::shared_ptr<Engine::Animation> pAnimation = std::static_pointer_cast<Engine::Animation>(pDrawable);

		std::shared_ptr<Engine::Skeleton> pSkeleton = pAnimation->GetSkeleton();

		if (pSkeleton)
		{
			const std::vector<Engine::PBONE>& vecJoint = pSkeleton->GetJoints();
			std::vector<int> vecDepth;

			for (size_t i = 0; i < vecJoint.size(); ++i)
			{
				if (vecJoint[i]->iParent != -1)
				{
					while (!vecDepth.empty())
					{
						if (vecDepth.back() == vecJoint[i]->iParent)
						{
							break;
						}

						vecDepth.pop_back();
					}
				}

				char strName[TEXT_LEN] = {};

				for (int j = 0; j < vecDepth.size(); ++j)
				{
					strcat_s(strName, "  ");
				}

				vecDepth.push_back(static_cast<int>(i));

				strcat_s(strName, vecJoint[i]->strName.c_str());

				char strIndex[TEXT_LEN] = {};

				sprintf_s(strIndex, " (index:%d)", static_cast<int>(i));

				strcat_s(strName, strIndex);

				ImGui::Text(strName);
			}

			const std::list<std::shared_ptr<Engine::JointSocket>>& socketList = pAnimation->GetSocketList();

			std::list<std::shared_ptr<Engine::JointSocket>>::const_iterator iter = socketList.begin();
			std::list<std::shared_ptr<Engine::JointSocket>>::const_iterator iterEnd = socketList.end();

			for (int i = 0; iter != iterEnd; ++iter, ++i)
			{
				JointSocket_ImGuiWindow(*iter, i);
			}
		}
		break;
	}
	case Engine::BINDABLE_TYPE::PARTICLE:
		Particle_ShowImGuiImage(std::static_pointer_cast<Engine::Particle>(pDrawable));
		break;
	case Engine::BINDABLE_TYPE::CLOTH:
		Cloth_ShowImguiWindow(std::static_pointer_cast<Engine::Cloth>(pDrawable));
		break;
	default:
		CRef_ImGuiWindow(pDrawable);
		break;
	}

	/*const Engine::Vector4& vSphereInfo = pDrawable->GetSphereInfo();

	ImGui::Text("%.f, %.f, %.f, %.f", vSphereInfo.x, vSphereInfo.y, vSphereInfo.z, vSphereInfo.w);*/

	const std::list<std::shared_ptr<Engine::Bindable>>& BindList = pDrawable->GetChildList();

	std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iter = BindList.begin();
	std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iterEnd = BindList.end();

	for (; iter != iterEnd; ++iter)
	{
		Drawable_ImGuiWindow(*iter);
	}

	static bool bOpen = false;
	static Engine::BINDABLE_TYPE eType = Engine::BINDABLE_TYPE::END;

	if (ImGui::Button("Add Bindable"))
	{
		bOpen = true;
	}

	static int iCurrent = 0;

	if (bOpen)
	{
		if (ImGui::ListBox("select type", &iCurrent, &vecTypes[0], static_cast<int>(vecTypes.size())))
		{
			bOpen = false;

			eType = static_cast<Engine::BINDABLE_TYPE>(iCurrent);
		}
	}

	if (eType != Engine::BINDABLE_TYPE::END)
	{
		static char strBindable[TEXT_LEN] = {};

		ImGui::InputText("bindable name", strBindable, TEXT_LEN);

		if (ImGui::Button("Create"))
		{
			switch (eType)
			{
			case Engine::BINDABLE_TYPE::VERTEX_BUFFER:
				//pDrawable->FindAndAddBind<Engine::VertexBuffer>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::INDEX_BUFFER:
				//pDrawable->FindAndAddBind<Engine::IndexBuffer>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::VERTEX_SHADER:
				pDrawable->FindAndAddBind<Engine::VertexShader>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::HULL_SHADER:
				pDrawable->FindAndAddBind<Engine::HullShader>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::DOMAIN_SHADER:
				pDrawable->FindAndAddBind<Engine::DomainShader>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::GEOMETRY_SHADER:
				break;
			case Engine::BINDABLE_TYPE::PIXEL_SHADER:
				pDrawable->FindAndAddBind<Engine::PixelShader>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::TEXTURE:
				pDrawable->FindAndAddBind<Engine::Texture>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::MATERIAL:
				pDrawable->FindAndAddBind<Engine::Material>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::TRANSFORM:
				pDrawable->FindAndAddBind<Engine::Transform>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::INPUTLAYOUT:
				pDrawable->FindAndAddBind<Engine::InputLayout>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::TOPOLOGY:
				pDrawable->FindAndAddBind<Engine::Topology>(strBindable);
				break;
			case Engine::BINDABLE_TYPE::MESH:
				pDrawable->FindAndAddBind<Engine::Mesh>(strBindable);
				break;
			}

			eType = Engine::BINDABLE_TYPE::END;
		}
	}
}

void ImguiManager::Material_ImGuiWindow(std::shared_ptr<Engine::Material> pMaterial)
{
	CRef_ImGuiWindow(pMaterial);

	const Engine::MATERIAL& tMaterial = pMaterial->GetMaterial();

	Engine::Vector4 vDiffuse = tMaterial.diffuseColor;

	ImGui::Text("Material");
	if (ImGui::ColorEdit4("diffuse", &vDiffuse.x))
	{
		pMaterial->SetDiffuseColor(vDiffuse);
	}

	Engine::Vector4 vAmbient = tMaterial.ambientColor;

	if (ImGui::ColorEdit4("ambient", &vAmbient.x))
	{
		pMaterial->SetAmbientColor(vAmbient);
	}

	Engine::Vector4 vSpecular = tMaterial.specularColor;

	if (ImGui::ColorEdit4("specular", &vSpecular.x))
	{
		pMaterial->SetSpecularColor(vSpecular);
	}

	Engine::Vector4 vEmissive = tMaterial.emissiveColor;

	if (ImGui::ColorEdit4("emissive", &vEmissive.x))
	{
		pMaterial->SetEmissiveColor(vEmissive);
	}

	float fSpecPower = tMaterial.fSpecPower;

	if (ImGui::SliderFloat("SpecularExponent", &fSpecPower, 0.f, 250.f))
	{
		pMaterial->SetShininess(fSpecPower);
	}

	float fFraction = tMaterial.fFraction;

	if (ImGui::SliderFloat("Fraction", &fFraction, 0.f, 1.f))
	{
		pMaterial->SetReflectivity(fFraction);
	}

	float fRoughnessX = tMaterial.vRoughness.x;

	if (ImGui::SliderFloat("RoughnessX", &fRoughnessX, 0.f, 1.f))
	{
		pMaterial->SetRoughnessX(fRoughnessX);
	}

	float fRoughnessY = tMaterial.vRoughness.y;

	if (ImGui::SliderFloat("RoughnessY", &fRoughnessY, 0.f, 1.f))
	{
		pMaterial->SetRoughnessY(fRoughnessY);
	}
}

void ImguiManager::Mesh_ImGuiWindow(std::shared_ptr<Engine::Mesh> pMesh)
{
	CRef_ImGuiWindow(pMesh);

	ImGui::Text("Mesh Count %d", pMesh->GetMeshCount());

#ifdef _DEBUG
	for (int i = 0; i < pMesh->GetMeshCount(); ++i)
	{
		char strLabel[TEXT_LEN] = {};

		sprintf_s(strLabel, "Enable Mesh %d", i + 1);

		bool bEnable = pMesh->IsMeshEnabled(i);

		if (ImGui::Checkbox(strLabel, &bEnable))
		{
			pMesh->ToggleMesh(i);
		}
	}
#endif
}

void ImguiManager::PointLight_ImGuiWindow(std::shared_ptr<Engine::PointLight> pLight)
{
	CRef_ImGuiWindow(pLight);

	float fIntencity = pLight->GetIntensity();

	if (ImGui::InputFloat("Light Intensity", &fIntencity, 0.f, 5000.f))
	{
		pLight->SetIntensity(fIntencity);
	}

	Engine::Vector4 vColor = pLight->GetLightColor();

	if (ImGui::ColorEdit4("color", &vColor.x))
	{
		pLight->SetLightColor(vColor);
	}

	Engine::Vector4 vAmbientColor = pLight->GetAmbientColor();

	if (ImGui::ColorEdit4("ambient", &vAmbientColor.x))
	{
		pLight->SetAmbientColor(vAmbientColor);
	}

	ImGui::Text("Attenutaion");

	float fConstant = pLight->GetConstantAttenuation();

	if (ImGui::SliderFloat("Constant", &fConstant, 0.f, 1.f))
	{
		pLight->SetConstantAttenuation(fConstant);
	}

	float fLinear = pLight->GetLinearAttenuation();

	if (ImGui::SliderFloat("Linear", &fLinear, 0.f, 1.f))
	{
		pLight->SetLinearAttenuation(fLinear);
	}

	float fQuadratic = pLight->GetQuadraticAttenuation();

	if (ImGui::SliderFloat("Quadratic", &fQuadratic, 0.f, 1.f))
	{
		pLight->SetQuadraticAttenuation(fQuadratic);
	}

	Engine::LIGHT_TYPE eLightType = pLight->GetLightType();

	if (ImGui::RadioButton("POINT", reinterpret_cast<int*>(&eLightType), static_cast<int>(Engine::LIGHT_TYPE::POINT)))
	{
		pLight->SetLightType(eLightType);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("SPOT", reinterpret_cast<int*>(&eLightType), static_cast<int>(Engine::LIGHT_TYPE::SPOT)))
	{
		pLight->SetLightType(eLightType);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("DIRECTIONAL", reinterpret_cast<int*>(&eLightType), static_cast<int>(Engine::LIGHT_TYPE::DIRECTIONAL)))
	{
		pLight->SetLightType(eLightType);
	}

	Engine::ORTHOINFO tOrthoInfo = pLight->GetOrthoInfo();

	if (ImGui::InputFloat("OrthoLeft", &tOrthoInfo.fLeft) ||
		ImGui::InputFloat("OrthoRight", &tOrthoInfo.fRight) ||
		ImGui::InputFloat("OrthoTop", &tOrthoInfo.fTop) ||
		ImGui::InputFloat("OrthoBottom", &tOrthoInfo.fBottom) ||
		ImGui::InputFloat("OrthoNear", &tOrthoInfo.fNear) ||
		ImGui::InputFloat("OrthoFar", &tOrthoInfo.fFar))
	{
		pLight->SetOrthoInfo(tOrthoInfo);
	}
}

void ImguiManager::Shader_ImGuiWindow(std::shared_ptr<Engine::Shader> pShader)
{
	CRef_ImGuiWindow(pShader);

	char strTitle[MAX_PATH];

	sprintf_s(strTitle, "Shader %s", pShader->GetTag().c_str());

	ImGui::Text(strTitle);

	ImGui::SameLine();

	if (ImGui::Button("ReLoad Shader"))
	{
		pShader->LoadShader();
	}

	ImGui::SameLine();

	ImGui::Text(pShader->GetEntry().get());
}

void ImguiManager::Sphere_ImGuiWindow(std::shared_ptr<Engine::Sphere> pSphere)
{
	CRef_ImGuiWindow(pSphere);

	Engine::Vector3 vDir = pSphere->GetDir();

	if (ImGui::InputFloat3("Dir", &vDir.x))
	{
		pSphere->SetDir(vDir);
	}

	float fSpeed = pSphere->GetSpeed();

	if (ImGui::InputFloat("Speed", &fSpeed))
	{
		pSphere->SetSpeed(fSpeed);
	}
}

//void Sequence_ImGuiWindow()
//{
//	__super::ImGuiWindow();

//	if (m_pSkeleton)
//	{
//		m_pSkeleton->ImGuiWindow();
//	}
//}

//void Texture_ImGuiWindow()
//{
//	__super::ImGuiWindow();

//	ImGui::Text("Texture Slot: %d", m_iSlot);
//}

void ImguiManager::TransformBuffer_ImGuiWindow(std::shared_ptr<Engine::Transform> pTransform)
{
	ImGui::Text("Transform");

	float x = pTransform->GetX();

	if (ImGui::InputFloat("x", &x))
	{
		pTransform->SetX(x);
	}

	float y = pTransform->GetY();

	if (ImGui::InputFloat("y", &y))
	{
		pTransform->SetY(y);
	}

	float z = pTransform->GetZ();

	if (ImGui::InputFloat("z", &z))
	{
		pTransform->SetZ(z);
	}

	float rx = pTransform->GetRX();

	if (ImGui::SliderFloat("rx", &rx, -PI, PI))
	{
		pTransform->SetRX(rx);
	}

	float ry = pTransform->GetRY();

	if (ImGui::SliderFloat("ry", &ry, -PI, PI))
	{
		pTransform->SetRY(ry);
	}

	float rz = pTransform->GetRZ();

	if (ImGui::SliderFloat("rz", &rz, -PI, PI))
	{
		pTransform->SetRZ(rz);
	}

	Engine::Vector3 vScale = pTransform->GetScale();

	if (ImGui::InputFloat("sx", &vScale.x))
	{
		pTransform->SetScale(vScale);
	}

	if (ImGui::InputFloat("sy", &vScale.y))
	{
		pTransform->SetScale(vScale);
	}

	if (ImGui::InputFloat("sz", &vScale.z))
	{
		pTransform->SetScale(vScale);
	}
}

//void Animation_ImGuiWindow()
//{
//	__super::ImGuiWindow();

//	if (m_pCurrentSequence)
//	{
//		m_pCurrentSequence->ImGuiWindow();
//	}
//}

//void CInput_ShowImGuiWindow()
//{
//	if (ImGui::Begin("Input"))
//	{
//		ImGui::Text("X: %d, Y: %d", m_tMousePos.x, m_tMousePos.y);
//	}

//	ImGui::End();
//}

void ImguiManager::MRT_ShowImGuiImage(std::shared_ptr<Engine::MRT> pMRT, const std::string& _name)
{
	std::string name = _name;

	name += pMRT->GetTag();

	if (ImGui::Begin(name.c_str()))
	{
		const std::vector<Engine::CPtr<ID3D11ShaderResourceView>>& vecSRV = pMRT->GetSRVs();

		int iCol = static_cast<int>(sqrtf(static_cast<float>(vecSRV.size())));

		for (size_t i = 0; i < vecSRV.size(); ++i)
		{
			ImGui::Image((void*)(*vecSRV[i]), { 640, 360 });

			if ((i + 1) % iCol)
			{
				ImGui::SameLine();
			}
		}

		ImGui::Image((void*)*pMRT->GetDepthSRV(), { 640, 360 });
	}

	ImGui::End();
}

void ImguiManager::Particle_ShowImGuiImage(std::shared_ptr<Engine::Particle> pParticle)
{
	static float fEmit = 0.f;

	if (ImGui::InputFloat("emit frequency", &fEmit))
	{
		pParticle->SetEmitTime(fEmit);
	}

	static Engine::Vector4 vStartColor = {};

	if (ImGui::ColorPicker4("Start Color", &vStartColor.x))
	{
		pParticle->SetStartColor(vStartColor);
	}

	static Engine::Vector4 vEndColor = {};

	if (ImGui::ColorPicker4("End Color", &vEndColor.x))
	{
		pParticle->SetEndColor(vEndColor);
	}

	static Engine::Vector3 vSpeed = {};

	if (ImGui::InputFloat3("speed", &vSpeed.x))
	{
		pParticle->SetVelocity(vSpeed);
	}

	static Engine::Vector3 vAccel = {};

	if (ImGui::InputFloat3("accel", &vAccel.x))
	{
		pParticle->SetAccelaration(vAccel);
	}
}

void ImguiManager::Cloth_ShowImguiWindow(std::shared_ptr<Engine::Cloth> pCloth)
{
	float fWind = pCloth->GetWindHeavyness();

	if (ImGui::SliderFloat("Wind Heavyness", &fWind, 0.f, 50.f))
	{
		pCloth->SetWindHeavyness(fWind);
	}
}

//void ImguiManager::RenderManager_ShowImGuiWindow()
//{
//	if (ImGui::Begin("RenderManager"))
//	{
//		ImGui::Text("RenderList Size: %d", m_RenderList[0].size());
//		for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
//		{
//			ImGui::Text("LightList Type: %d, Size: %d", i, m_LightList[i].size());
//		}

//		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[0].begin();
//		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterEnd = m_mapInstance[0].end();

//		for (; iter != iterEnd; ++iter)
//		{
//			ImGui::Text("Instance: %s, Size: %d", iter->second->GetTag().c_str(), iter->second->GetCount());
//		}

//		if (ImGui::Button("Reload Multi Shader"))
//		{
//			pMultiVertexShader->LoadShader();

//			pMultiPixelShader->LoadShader();
//		}
//	}

//	ImGui::End();
//}

void ImguiManager::Layer_DrawListImgui(std::shared_ptr<Engine::Layer> pLayer)
{
	if(!pLayer || !ImGui::Begin(pLayer->GetTag().c_str()))
	{
		return;
	}

	static int iCurrent = -1;

	const std::list<std::shared_ptr<Engine::Bindable>>& DrawList = pLayer->GetDrawList();

	std::vector<const char*> vecName(DrawList.size());

	std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iter = DrawList.begin();
	std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iterEnd = DrawList.end();

	for (int i = 0; iter != iterEnd; ++iter, ++i)
	{
		vecName[i] = (*iter)->GetTag().c_str();
	}

	if (vecName.size())
	{
		ImGui::ListBox("Draw List", &iCurrent, &vecName[0], static_cast<int>(vecName.size()));
	}

	if (iCurrent >= 0 && iCurrent < DrawList.size())
	{
		std::list<std::shared_ptr<Engine::Bindable>>::const_iterator iter = DrawList.begin();

		std::advance(iter, iCurrent);

		Drawable_ShowImGuiWindow(*iter);
	}

	static char strName[MAX_PATH] = {};

	ImGui::InputText("name", strName, MAX_PATH);

	ImGui::SameLine();

	if (ImGui::Button("Load Drawable"))
	{
		TCHAR strFile[MAX_PATH] = {};
		OPENFILENAME tName = {};
		tName.lStructSize = sizeof(OPENFILENAME);
		tName.hwndOwner = nullptr;
		tName.lpstrFilter = TEXT(".FBX;.OBJ;*.*");
		tName.nMaxFile = 2048;
		tName.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath();
		tName.Flags = 0;
		tName.lpstrFile = strFile;

		if (GetOpenFileName(&tName))
		{
			pLayer->CreateLoadingThread(tName.lpstrFile);

			memset(strName, 0, MAX_PATH);
		}
	}

	if (ImGui::Button("Load And Build Nav Mesh"))
	{
		TCHAR strFullPath[MAX_PATH] = {};

		OPENFILENAME tOFN = {};

		tOFN.lStructSize = sizeof(OPENFILENAME);
		tOFN.lpstrFilter = TEXT(".FBX;.OBJ;*.*");
		tOFN.lpstrFile = strFullPath;
		tOFN.nMaxFile = MAX_PATH;
		tOFN.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath(MESH_PATH);

		if (GetOpenFileName(&tOFN))
		{
			LoadNavMesh(tOFN.lpstrFile, Engine::SceneManager::GetInst()->GetScene());
		}
	}

	ImGui::Checkbox("CreateAgentMode", &m_bMode);

	if (pLayer->GetLoadingThread())
	{
		ImGui::Text("Loading ...");
	}

	ImGui::End();
}
