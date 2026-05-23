#include "ImguiManager.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include "../Project/ProjectModule.h"
#include "../Scene/InGameScene.h"
#include "Core/ObjectFactory.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Input/Input.h"
#include "Core/Window.h"
// Phase E7 — BindableManager.h must come before headers that use the
// `friend class BindableManager<X>` non-template-form declaration
// (InputLayout, Topology, RasterizerState, etc.). Engine + Client builds
// happen to satisfy this transitively; Editor's unity build doesn't.
#include "Bindable/BindableManager.h"
#include "Scene/Layer.h"
#include "Core/PathManager.h"
#include <commdlg.h>
#include "Bindable/Mesh.h"
#include "Bindable/VertexShader.h"
#include "Bindable/HullShader.h"
#include "Bindable/DomainShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Texture.h"
#include "Bindable/Material.h"
#include "Bindable/Transform.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/ConstantBuffer.h"
#include "Bindable/Bindable.h"
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
#include "GameObject/GameObject.h"
#include "Component/MeshRendererComponent.h"
#include "Resource/ResourceManager.h"
#include "Animation/Sequence.h"
#include "Animation/Skeleton.h"
#include "Bindable/PointLight.h"
#include "Bindable/Sphere.h"
#include "Render/MRT.h"
#include "Bindable/Particle.h"
#include "Bindable/Cloth.h"
#include "Bindable/Terrain.h"
#include "Render/RenderManager.h"
#include "Bindable/BlendState.h"
#include <shlobj.h>
#include <algorithm>

namespace Editor
{
	namespace
	{
		// Editor preferences live alongside the editor executable. Win32's
		// GetPrivateProfile* / WritePrivateProfile* APIs only treat fully-
		// qualified paths as INI files — relative paths fall back to the
		// Windows directory, which silently breaks persistence — so we
		// build the absolute path from CPathManager's ROOT_PATH.
		void BuildEditorIniPath(TCHAR* pOut, size_t iLen)
		{
			pOut[0] = 0;
			const TCHAR* pRoot = Engine::CPathManager::GetInst()->FindPath();
			if (pRoot)
			{
				_tcscpy_s(pOut, iLen, pRoot);
			}
			_tcscat_s(pOut, iLen, TEXT("Editor.ini"));
		}

		// Default for m_strClientResourcePath: climb out of Editor\Bin\ and
		// over to Client\Bin\Resource\. Relies on the standard repo layout
		// (Editor and Client are sibling top-level folders); user can override
		// via Editor.ini if their layout differs.
		void BuildDefaultClientResourcePath(TCHAR* pOut, size_t iLen)
		{
			pOut[0] = 0;
			const TCHAR* pRoot = Engine::CPathManager::GetInst()->FindPath();
			if (!pRoot) return;

			_tcscpy_s(pOut, iLen, pRoot);

			size_t cur = _tcslen(pOut);
			if (cur > 0 && (pOut[cur - 1] == TEXT('\\') || pOut[cur - 1] == TEXT('/')))
			{
				pOut[--cur] = 0;
			}
			for (int strip = 0; strip < 2 && cur > 0; ++strip)
			{
				while (cur > 0 && pOut[cur - 1] != TEXT('\\') && pOut[cur - 1] != TEXT('/'))
				{
					pOut[--cur] = 0;
				}
				if (cur > 0)
				{
					pOut[--cur] = 0;
				}
			}
			_tcscat_s(pOut, iLen, TEXT("\\Client\\Bin\\Resource\\"));
		}

		// Forward decl — the definition lives further down with the rest of
		// the MRT debug-view helpers (alpha-blend override for ImGui::Image).
		// ImguiManager's destructor calls this to release the cached opaque
		// blend state before Graphics::DestroyInst runs in main.cpp.
		void ReleaseDebugViewStatics();
	}

	ImguiManager* ImguiManager::m_pInst = nullptr;

	ImguiManager::ImguiManager() :
		m_bDemoWindow(false)
		, m_pHeightField(nullptr)
		, m_pCompactHeightField(nullptr)
		, m_pContourSet(nullptr)
		, m_pPolyMesh(nullptr)
		, m_pPolyMeshDetail(nullptr)
		, m_bMode(true)
		, m_iOutlinedContainerIdx(-1)
	{
		m_strTextureDefaultPath[0] = 0;
		m_strClientResourcePath[0] = 0;

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

		LoadEditorSettings();
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

		// Release MRT debug-view's cached opaque blend state before the
		// ImGui DX11 backend tears down — both share the device, and we
		// want this gone before Graphics::DestroyInst runs in main.cpp.
		ReleaseDebugViewStatics();

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	bool ImguiManager::Init(HWND hwnd)
	{
		m_hWnd = hwnd;
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

		m_vecBrushTexture.push_back(Engine::StaticCreateBindable<Engine::Texture>("brush1", TEXT("brush\\circle.png"), TEXTURE_PATH));
		m_vecBrushTexture.push_back(Engine::StaticCreateBindable<Engine::Texture>("brush2", TEXT("brush\\star.png"), TEXTURE_PATH));
		m_vecBrushTexture.push_back(Engine::StaticCreateBindable<Engine::Texture>("brush3", TEXT("brush\\brush.png"), TEXTURE_PATH));

		InitSelectionOutline();

		return true;
	}

	void ImguiManager::InitSelectionOutline()
	{
		m_pOutlineMaskMRT = std::make_shared<Engine::MRT>(
			std::vector<DXGI_FORMAT>({ DXGI_FORMAT_R8_UNORM }), 30);

		m_pOutlineMaskVS = std::make_shared<Engine::VertexShader>(TEXT("SelectionOutline.fx"), "MaskVS");
		m_pOutlineMaskPS = std::make_shared<Engine::PixelShader>(TEXT("SelectionOutline.fx"), "MaskPS");
		m_pOutlineFullScreenVS = std::make_shared<Engine::VertexShader>(TEXT("SelectionOutline.fx"), "OutlineFullScreenVS");
		m_pOutlineCompositePS = std::make_shared<Engine::PixelShader>(TEXT("SelectionOutline.fx"), "OutlineCompositePS");

		m_pOutlineCB = std::make_shared<Engine::ConstantBuffer<OUTLINECBUFFER>>(0);
		m_pOutlineCB->CreateBuffer();

		// Standard SrcAlpha / InvSrcAlpha. Don't reuse BindableManager's
		// "AlphaBlend" because Init() runs before Scene-side bindables are
		// registered.
		m_pOutlineBlend = std::make_shared<Engine::BlendState>();
	}

	void ImguiManager::RenderSelectionOutline()
	{
		auto pObj = m_pOutlinedObject.lock();
		if (!pObj || m_iOutlinedContainerIdx < 0)
			return;

		auto pMR = pObj->GetComponent<Engine::MeshRendererComponent>();
		if (!pMR)
			return;

		auto pMesh = pMR->GetMesh();
		if (!pMesh || m_iOutlinedContainerIdx >= pMesh->GetMeshCount())
			return;

		auto pTransform = pObj->GetComponent<Engine::Transform>();
		if (!pTransform)
			return;

		// ---- Pass 1: render selected container into mask RT ----
		m_pOutlineMaskMRT->SetTargets();
		m_pOutlineMaskMRT->Clear();

		pTransform->Bind();

		auto pStandardIL = Engine::StaticFindBindable<Engine::InputLayout>(STANDARD_INPUT_LAYOUT);
		if (pStandardIL) pStandardIL->Bind();

		Engine::Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pOutlineMaskVS->Bind();
		m_pOutlineMaskPS->Bind();

		pMesh->DrawContainer(m_iOutlinedContainerIdx);

		m_pOutlineMaskMRT->ResetTargets();

		// ---- Pass 2: full-screen edge-detect composite onto back buffer ----
		OUTLINECBUFFER tCB = {};
		tCB.vColor[0] = 1.f; tCB.vColor[1] = 0.5f; tCB.vColor[2] = 0.f; tCB.vColor[3] = 1.f;
		tCB.vTexelSize[0] = 1.f / static_cast<float>(Engine::Window::GetInst()->GetWidth());
		tCB.vTexelSize[1] = 1.f / static_cast<float>(Engine::Window::GetInst()->GetHeight());
		tCB.iThickness = 2;
		m_pOutlineCB->UpdateBuffer(tCB);
		m_pOutlineCB->Bind();

		m_pOutlineMaskMRT->SetSRV(0, 0);

		m_pOutlineFullScreenVS->Bind();
		m_pOutlineCompositePS->Bind();
		m_pOutlineBlend->Bind();

		auto pCtx = Engine::Graphics::GetInst()->GetDeviceContext();
		pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		pCtx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		pCtx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		pCtx->IASetInputLayout(nullptr);
		pCtx->Draw(4, 0);

		// Unbind mask SRV from slot 0 so subsequent passes don't sample it.
		ID3D11ShaderResourceView* pNullSRV = nullptr;
		pCtx->PSSetShaderResources(0, 1, &pNullSRV);

		m_pOutlineBlend->PostBind();

		// Invalidate VS/PS cache so the next pass rebinds (we left our
		// outline shaders bound).
		Engine::Graphics::GetInst()->ResetBindCache();
	}

	void ImguiManager::Update(float fDeltaTime)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		// ImGuizmo needs its own frame init AFTER ImGui::NewFrame so it
		// can pull mouse/keyboard state through ImGui's IO.
		ImGuizmo::BeginFrame();

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
			Engine::Window::GetInst()->GetTimer()->Stop();
		}

		ImGui::SameLine();

		if (ImGui::Button("resume"))
		{
			Engine::Window::GetInst()->GetTimer()->Resume();
		}

		// NavMesh wireframe overlay toggle. The "NavMesh_Debug" GameObject
		// only exists after LoadNavMesh has been invoked; before that the
		// FindGameObject call below returns null and the checkbox is just
		// a stored preference applied as soon as the navmesh is built.
		ImGui::Checkbox("Show NavMesh Debug", &m_bShowNavMeshDebug);
		if (auto pScene = Engine::SceneManager::GetInst()->GetScene())
		{
			if (auto pLayer = pScene->FindLayer(DEFAULT_LAYER))
			{
				if (auto pNavDebug = pLayer->FindGameObject("NavMesh_Debug"))
				{
					if (m_bShowNavMeshDebug) pNavDebug->Enable();
					else                     pNavDebug->Disable();
				}
			}
		}

		// Collider wireframe overlay. Every ColliderOBB/Sphere/Line pushes
		// its edges to RenderManager's debug-line buffer each frame when
		// this flag is on; the flush pass at the end of Render() draws
		// them on top of the final image. Only available in _DEBUG (the
		// engine-side toggle and flush path are themselves _DEBUG-only).
#ifdef _DEBUG
		{
			bool bShowColliders = Engine::RenderManager::GetInst()->IsDebugDrawColliders();
			if (ImGui::Checkbox("Show Colliders", &bShowColliders))
			{
				Engine::RenderManager::GetInst()->SetDebugDrawColliders(bShowColliders);
			}
		}
#endif

		ImGui::End();

		Project_ImGuiWindow();
		RenderManager_ShowImGuiWindow();
		Scene_ImGuiWindow(Engine::SceneManager::GetInst()->GetScene());
		WorldOutliner_ImGuiWindow(Engine::SceneManager::GetInst()->GetScene());
		EditorSettings_ImGuiWindow();
		MaterialBrowser_ImGuiWindow();

		if (auto pAnim = m_pSelectedAnimation.lock())
		{
			Animation_ImGuiWindow(pAnim);
		}
		DrawSelectionGizmo();
		RenderLightBillboards();

		// Register the selection-outline pass with RenderManager every frame
		// (Clear() wipes the list each frame). Runs at UI layer so it draws
		// on top of the final HDR-tone-mapped back buffer.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::UI,
			[this]() { RenderSelectionOutline(); });
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

		// Phase E7 — dead `if (false)` Drawable cast block removed.
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

		// Phase E7 — Navigation migrated from Drawable to GameObject. Mesh /
		// shaders / material now live on a MeshRendererComponent; ColliderMesh
		// and NavMesh are sibling Components on the same GameObject.
		std::shared_ptr<Engine::GameObject> pNavigation =
			pScene->CreateGameObject<Engine::GameObject>("Navigation", pScene->FindLayer(DEFAULT_LAYER));

		if (!pNavigation)
		{
			return;
		}

		pNavigation->AddComponent<Engine::Transform>("transform");

		std::shared_ptr<Engine::MeshRendererComponent> pMR =
			pNavigation->AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

		std::shared_ptr<Engine::ColliderMesh> pColliderMesh =
			pNavigation->AddComponent<Engine::ColliderMesh>("ColliderMesh");
		pColliderMesh->SetInfo(vecPoint, vecTris);

		std::shared_ptr<Engine::Mesh> pNavMeshGeometry =
			Engine::StaticCreateBindable<Engine::Mesh>("NavMesh", vecVertexAll, vecIndexAll);

		if (pMR)
		{
			pMR->SetMesh(pNavMeshGeometry);
			pMR->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>("anisotropic_microfacet VSNoSkin"));
			pMR->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>("anisotropic_microfacet PS_NoTexture"));
			pMR->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
			pMR->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));

			std::shared_ptr<Engine::Material> pMaterial =
				Engine::StaticFindBindable<Engine::Material>("Material");
			if (pMaterial)
				pMR->SetMaterial(std::static_pointer_cast<Engine::Material>(pMaterial->Clone()));
		}

		m_pNavMesh = CreateNavMesh(vecPoint, vecTris, vMax, vMin);
		m_pNavMesh->SetTag("NavigationMesh");
		pNavigation->AddComponent(std::static_pointer_cast<Engine::Component>(m_pNavMesh));

		// NavMesh wireframe debug overlay — extracts the Detour polygon
		// data as a renderable mesh and parks it on a sibling GameObject
		// rendered with the WIREFRAME rasterizer state. Toggled via
		// m_bShowNavMeshDebug (default off; ImGui Checkbox below sets it).
		if (auto pDebugMesh = m_pNavMesh->CreateDebugMesh())
		{
			auto pNavDebug = pScene->CreateGameObject<Engine::GameObject>(
				"NavMesh_Debug", pScene->FindLayer(DEFAULT_LAYER));
			if (pNavDebug)
			{
				pNavDebug->AddComponent<Engine::Transform>("transform");
				auto pDebugMR = pNavDebug->AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
				if (pDebugMR)
				{
					pDebugMR->SetMesh(pDebugMesh);
					pDebugMR->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(
						"anisotropic_microfacet VSNoSkin"));
					pDebugMR->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>(
						"anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal"));
					pDebugMR->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
					pDebugMR->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
					// WIREFRAME rasterizer = fill mode wireframe + cull none.
					// Already registered in BindableManager.cpp:95 under the
					// "WireFrame" tag (= the WIREFRAME macro).
					pDebugMR->AddBindable(Engine::StaticFindBindable<Engine::RasterizerState>(WIREFRAME));

					if (auto pMat = Engine::StaticFindBindable<Engine::Material>("Material"))
						pDebugMR->SetMaterial(std::static_pointer_cast<Engine::Material>(pMat->Clone()));
				}
				if (m_bShowNavMeshDebug) pNavDebug->Enable();
				else                     pNavDebug->Disable();
			}
		}

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
		// Thin wrapper around Engine::NavMesh::Build now that the Recast
		// pipeline lives in the engine and can be invoked from Client
		// game code too. Editor UI sliders feed the config struct here so
		// changes in the inspector still tune the build per-bake.
		Engine::NavMeshConfig cfg;
		cfg.fCellSize             = m_fCellSize;
		cfg.fCellHeight           = m_fCellHeight;
		cfg.fAgentSlopeAngle      = m_fAgentSlopeAngle;
		cfg.fAgentHeight          = m_fAgentHeight;
		cfg.fAgentRadius          = m_fAgentRadius;
		cfg.fAgentClimb           = m_fAgentClimb;
		cfg.fMaxEdgeLen           = m_fMaxEdgeLen;
		cfg.fMaxEdgeError         = m_fMaxEdgeError;
		cfg.fRegionMinSize        = m_fRegionMinSize;
		cfg.fRegionMergeSize      = m_fRegionMergeSize;
		cfg.fVertsPerPoly         = m_fVertsPerPoly;
		cfg.fDetailSampleDist     = m_fDetailSampleDist;
		cfg.fDetailSampleMaxError = m_fDetailSampleMaxError;

		return Engine::NavMesh::Build(vecPoint, vecTris, vMax, vMin, cfg);
	}

	void ImguiManager::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::LEFT))
		{
			Engine::Collider* pNavOwner = nullptr;

			if ((pSrc->GetComponentType() == Engine::COMPONENT_TYPE::COLLIDER_LINE && pSrc->GetTag() == "MouseLine"))
			{
				pNavOwner = pDest;
			}

			else if (pDest->GetComponentType() == Engine::COMPONENT_TYPE::COLLIDER_LINE && pDest->GetTag() == "MouseLine")
			{
				pNavOwner = pSrc;
			}

			if (pNavOwner)
			{
				if (m_bMode)
				{
					Engine::Scene* pScene = Engine::SceneManager::GetInst()->GetScene();

					std::shared_ptr<Engine::Layer> pLayer = pScene->FindLayer(DEFAULT_LAYER);

					char strPlayer[TEXT_LEN] = {};

					sprintf_s(strPlayer, "Player_%d", static_cast<int>(m_PlayerList.size()));

					// Phase E7 — Player is a GameObject; spawn directly via
					// CreateGameObject. The old CreateCloneDrawable path
					// depended on a "Player" prototype that was never
					// registered, so this is the first time the editor-side
					// agent spawn actually creates a live entity.
					std::shared_ptr<Player> pPlayer = pScene->CreateGameObject<Player>(strPlayer, pLayer);

					if (pPlayer && m_pNavMesh)
					{
						// Phase E7 — NavMesh is the editor's own member (set
						// in CreateNavMesh / LoadNavMesh); resolve directly
						// instead of walking back through the Collider's
						// parent. Sidesteps Component::GetParent vs Bindable
						// type mismatch and works regardless of whether the
						// nav-host entity is still a Drawable or a GameObject.
						pPlayer->SetAgent(m_pNavMesh->CreateAgent(strPlayer, pPlayer->GetTransform(), pSrc->GetCross()));
					}

					m_PlayerList.push_back(pPlayer);
				}
				else
				{
					for (const auto& pPlayer : m_PlayerList)
					{
						if (pPlayer) pPlayer->Move(pSrc->GetCross());
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
		case Engine::BINDABLE_TYPE::MESH:
			Mesh_ImGuiWindow(std::static_pointer_cast<Engine::Mesh>(pDrawable));
			break;
		case Engine::BINDABLE_TYPE::TERRAIN:
			// Phase E7 — Terrain migrated out of the Bindable hierarchy; the
			// downcast no longer compiles. The TERRAIN enum entry stays for
			// legacy serialized scenes but the editor inspector hook is dead.
			break;
		case Engine::BINDABLE_TYPE::MATERIAL:
			Material_ImGuiWindow(std::static_pointer_cast<Engine::Material>(pDrawable));
			break;
		case Engine::BINDABLE_TYPE::TRANSFORM:
		case Engine::BINDABLE_TYPE::LIGHT:
		case Engine::BINDABLE_TYPE::ANIMATION:
		case Engine::BINDABLE_TYPE::PARTICLE:
		case Engine::BINDABLE_TYPE::CLOTH:
			// Phase E7 — these types migrated from Bindable to Component (or
			// to GameObject). pDrawable here is shared_ptr<Bindable>, so
			// downcasting to the new Component-derived class no longer
			// compiles. The editor inspector for these needs to be rewritten
			// against the GameObject's component list; until then, fall
			// through to the generic CRef inspector.
			CRef_ImGuiWindow(pDrawable);
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
					// Phase E7 — Transform migrated to Component; the
					// "add a Transform Bindable" UI no longer applies.
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

		// F0 (specular color) presets — common PBR reference values used as
		// the Fresnel base reflectance in PS_Multi. Dielectric covers ~all
		// non-metals (plastic, wood, fabric, skin, ceramic). The metals are
		// measured F0 values widely cited in Disney / Substance references.
		struct F0Preset
		{
			const char* name;
			float r, g, b;
		};
		static const F0Preset kPresets[] =
		{
			{ "Dielectric", 0.04f, 0.04f, 0.04f },
			{ "Gold",       1.00f, 0.86f, 0.57f },
			{ "Silver",     0.95f, 0.93f, 0.88f },
			{ "Copper",     0.95f, 0.64f, 0.54f },
			{ "Iron",       0.56f, 0.57f, 0.58f },
			{ "Aluminum",   0.91f, 0.92f, 0.92f },
		};
		ImGui::PushID("F0Presets");
		ImGui::TextUnformatted("F0 preset:");
		for (const F0Preset& preset : kPresets)
		{
			ImGui::SameLine();
			if (ImGui::Button(preset.name))
			{
				pMaterial->SetSpecularColor(preset.r, preset.g, preset.b, 1.f);
			}
		}
		ImGui::PopID();

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

		// Texture slots — owned by the Material itself. Slot indices on the
		// labels match the register(tN) in shared.hlsl. The "Clear" button
		// drops the slot back to nullptr so Material::Bind pushes a null SRV
		// and the shader's GetDimensions guard falls back to uniforms.
		ImGui::Separator();
		ImGui::TextUnformatted("Textures");

		struct SlotLabel { int iSlotIdx; const char* pName; };
		static const SlotLabel kSlotLabels[Engine::Material::kMaterialSlotCount] = {
			{ 0, "Diffuse"   },
			{ 1, "Normal"    },
			{ 2, "Specular"  },
			{ 3, "Emissive"  },
			{ 4, "Roughness" },
			{ 5, "AO"        },
			{ 6, "Metalness" },
		};

		ImGui::PushID(static_cast<const void*>(pMaterial.get()));
		for (const SlotLabel& slot : kSlotLabels)
		{
			auto pTex = pMaterial->GetTexture(slot.iSlotIdx);

			ImGui::PushID(slot.iSlotIdx);
			ImGui::Text("%s: %s", slot.pName, pTex ? pTex->GetTag().c_str() : "(empty)");

			// Buttons on the line below — texture tags are full file paths
			// (set by StaticCreateBindable<Texture> using the absolute path
			// as the tag), so SameLine would push them off-screen.
			if (ImGui::Button("Set"))
			{
				TCHAR strFile[MAX_PATH] = {};
				OPENFILENAME tName = {};
				tName.lStructSize = sizeof(OPENFILENAME);
				tName.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
				tName.lpstrFilter = TEXT("Texture\0*.png;*.dds;*.tga;*.jpg;*.bmp\0All\0*.*\0");
				tName.nMaxFile = MAX_PATH;
				tName.lpstrInitialDir = m_strTextureDefaultPath;
				tName.lpstrFile = strFile;

				if (GetOpenFileName(&tName))
				{
					char szPath[MAX_PATH] = {};
					WideCharToMultiByte(CP_ACP, 0, strFile, -1, szPath, MAX_PATH, nullptr, nullptr);
					// Tag = "parent_dir\filename" (e.g. "Decal\brick.png").
					// Short enough for the inspector, disambiguates same-named
					// files in different folders. Full path still used for
					// the actual file load below.
					const char* pLast = strrchr(szPath, '\\');
					if (!pLast) pLast = strrchr(szPath, '/');
					const char* pTagStart = szPath;
					if (pLast)
					{
						const char* pPrev = nullptr;
						for (const char* p = pLast - 1; p >= szPath; --p)
						{
							if (*p == '\\' || *p == '/') { pPrev = p; break; }
						}
						pTagStart = pPrev ? (pPrev + 1) : szPath;
					}
					std::string strTag(pTagStart);

					const int iRegister = Engine::Material::kMaterialSlotRegisters[slot.iSlotIdx];
					auto pNewTex = Engine::StaticCreateBindable<Engine::Texture>(strTag, strFile, iRegister);
					if (!pNewTex)
					{
						pNewTex = Engine::StaticFindBindable<Engine::Texture>(strTag);
					}
					if (pNewTex)
					{
						pMaterial->SetTexture(slot.iSlotIdx, pNewTex);
					}
				}
			}

			if (pTex)
			{
				ImGui::SameLine();
				if (ImGui::Button("Clear"))
				{
					pMaterial->SetTexture(slot.iSlotIdx, nullptr);
				}
			}
			ImGui::PopID();
		}
		ImGui::PopID();

		// Persist this material to disk as a .mat asset under Resource/Material.
		// On reload the asset auto-registers via ResourceManager::LoadAllMaterials,
		// so any mesh that references it by tag picks it up next session.
		ImGui::Separator();
		ImGui::PushID(static_cast<const void*>(pMaterial.get()));
		if (ImGui::Button("Save as .mat Asset"))
		{
			if (!pMaterial->GetTag().empty())
			{
				std::string strFile = pMaterial->GetTag() + ".mat";
				pMaterial->SaveFromPath(strFile.c_str(), MATERIAL_PATH);
			}
		}
		if (pMaterial->GetTag().empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(set tag first)");
		}
		ImGui::PopID();
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

			if (bEnable)
			{
				int iSub = pMesh->GetMeshSubCount(i);

				for (int j = 0; j < iSub; ++j)
				{
					char strSub[MAX_PATH] = {};

					sprintf_s(strSub, "Mesh: %d, Sub: %d Offset", i, j);

					int iOffset = pMesh->GetMeshSubOffset(i, j);

					if (ImGui::InputInt(strSub, &iOffset))
					{
						pMesh->SetMeshSubOffset(i, j, iOffset);
					}
				}
			}
		}
#endif

		// Mesh default material slots — affects every instance that uses
		// this Mesh. Per-instance overrides live on MeshRendererComponent
		// (see MeshRenderer_ImGuiWindow's "[override]" picker). Material
		// editing itself happens in Material Browser; this UI only chooses
		// which material is bound to each slot.
		int iContainerCount = pMesh->GetMeshCount();
		const auto& mapMaterials = Engine::ResourceManager::GetInst()->GetAllMaterials();

		ImGui::Separator();
		ImGui::TextUnformatted("Default Material Slots");

		for (int i = 0; i < iContainerCount; ++i)
		{
			int iSubCount = pMesh->GetMeshSubCount(i);

			for (int j = 0; j < iSubCount; ++j)
			{
				auto pCurrent = pMesh->GetMaterial(i, j);
				const char* pCurrentLabel = pCurrent ? pCurrent->GetTag().c_str() : "(empty)";

				std::string strComboId =
					"Container " + std::to_string(i) + " / Sub " + std::to_string(j) +
					"##mslot" + std::to_string(i) + "_" + std::to_string(j);

				if (ImGui::BeginCombo(strComboId.c_str(), pCurrentLabel))
				{
					if (ImGui::Selectable("(empty)", !pCurrent))
					{
						pMesh->SetMaterial(i, j, nullptr);
					}
					for (const auto& entry : mapMaterials)
					{
						const std::string& strTag = entry.first;
						const std::shared_ptr<Engine::Material>& pMat = entry.second;
						if (!pMat) continue;
						const bool bSel = (pCurrent == pMat);
						if (ImGui::Selectable(strTag.c_str(), bSel))
						{
							pMesh->SetMaterial(i, j, pMat);
						}
					}
					ImGui::EndCombo();
				}
			}
		}
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

		// Cone exponent — only meaningful for SPOT. Hidden for POINT/DIRECTIONAL
		// since the shader's spot branch is the only consumer. Range 1~128 covers
		// "very wide cone" → "laser-like" practically.
		if (pLight->GetLightType() == Engine::LIGHT_TYPE::SPOT)
		{
			float fConeExp = pLight->GetSpotConeExponent();
			if (ImGui::SliderFloat("Cone Exponent", &fConeExp, 1.f, 128.f, "%.1f", ImGuiSliderFlags_Logarithmic))
			{
				pLight->SetSpotConeExponent(fConeExp);
			}
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

	namespace
	{
		// GBuffer slots pack non-color data into .w (roughness, fraction, etc).
		// ImGui's default alpha-blended Image draws those targets as fully
		// transparent when .w == 0 — looks like "nothing was output". Wrap each
		// debug Image with a callback that disables blending, then ask ImGui
		// to restore its default render state afterwards.
		// Single owner of the static so both Get/Release access the same
		// pointer. Wrapped in a function-local static to avoid initialisation
		// order issues across translation units.
		ID3D11BlendState*& OpaqueBlendStateRef()
		{
			static ID3D11BlendState* s_pOpaque = nullptr;
			return s_pOpaque;
		}

		ID3D11BlendState* GetOpaqueBlendState()
		{
			ID3D11BlendState*& s_pOpaque = OpaqueBlendStateRef();
			if (!s_pOpaque)
			{
				D3D11_BLEND_DESC desc = {};
				desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				desc.RenderTarget[0].BlendEnable = FALSE;
				Engine::Graphics::GetInst()->GetDevice()->CreateBlendState(&desc, &s_pOpaque);
			}
			return s_pOpaque;
		}

		// Called by ImguiManager destructor — releases the cached blend
		// state explicitly so it doesn't outlive the D3D11 device. (A
		// function-local static would otherwise destruct AFTER our main
		// shutdown, after Graphics::DestroyInst, leading to a release on
		// a stale device.)
		void ReleaseDebugViewStatics()
		{
			ID3D11BlendState*& s_pOpaque = OpaqueBlendStateRef();
			if (s_pOpaque)
			{
				s_pOpaque->Release();
				s_pOpaque = nullptr;
			}
		}

		void DisableImguiAlphaBlend(const ImDrawList*, const ImDrawCmd*)
		{
			const float bf[4] = { 0.f, 0.f, 0.f, 0.f };
			Engine::Graphics::GetInst()->GetDeviceContext()->OMSetBlendState(
				GetOpaqueBlendState(), bf, 0xffffffff);
		}

		void OpaqueImguiImage(ID3D11ShaderResourceView* pSRV, const ImVec2& size)
		{
			ImDrawList* pList = ImGui::GetWindowDrawList();
			pList->AddCallback(DisableImguiAlphaBlend, nullptr);
			ImGui::Image((void*)pSRV, size);
			pList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
		}
	}

	void ImguiManager::MRT_ShowImGuiImage(std::shared_ptr<Engine::MRT> pMRT, const std::string& _name)
	{
		std::string name = _name;

		name += pMRT->GetTag();

		if (ImGui::Begin(name.c_str()))
		{
			const std::vector<Engine::CPtr<ID3D11ShaderResourceView>>& vecSRV = pMRT->GetSRVs();

			static const char* kSlotNames[] = {
				"albedo", "normal", "specular map", "specular color", "emissive"
			};
			constexpr int kSlotNameCount = sizeof(kSlotNames) / sizeof(kSlotNames[0]);

			// 2-column grid: each cell is [name] over [image]. 5 GBuffer SRVs
			// plus 1 depth SRV fit exactly 3 rows × 2 columns.
			if (ImGui::BeginTable("MRTGrid", 2))
			{
				for (size_t i = 0; i < vecSRV.size(); ++i)
				{
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(i < kSlotNameCount ? kSlotNames[i] : "unknown");
					OpaqueImguiImage(*vecSRV[i], { 640, 360 });
				}

				ImGui::TableNextColumn();
				ImGui::TextUnformatted("depth");
				OpaqueImguiImage(*pMRT->GetDepthSRV(), { 640, 360 });

				ImGui::EndTable();
			}
		}

		ImGui::End();
	}

	void ImguiManager::LoadEditorSettings()
	{
		// Default to the engine's TEXTURE_PATH (Resource\Texture\) so existing
		// users get the same behavior as before when no Editor.ini exists yet.
		const TCHAR* pDefault = Engine::CPathManager::GetInst()->FindPath(TEXTURE_PATH);
		if (pDefault)
		{
			_tcscpy_s(m_strTextureDefaultPath, MAX_PATH, pDefault);
		}
		else
		{
			m_strTextureDefaultPath[0] = 0;
		}

		BuildDefaultClientResourcePath(m_strClientResourcePath, MAX_PATH);

		TCHAR strIni[MAX_PATH] = {};
		BuildEditorIniPath(strIni, MAX_PATH);

		TCHAR strBuf[MAX_PATH] = {};
		// 4th arg = fallback returned when the key is missing.
		GetPrivateProfileString(TEXT("Paths"), TEXT("TextureDefault"),
			m_strTextureDefaultPath, strBuf, MAX_PATH, strIni);

		if (strBuf[0])
		{
			_tcscpy_s(m_strTextureDefaultPath, MAX_PATH, strBuf);
		}

		TCHAR strClientBuf[MAX_PATH] = {};
		GetPrivateProfileString(TEXT("Paths"), TEXT("ClientResourceRoot"),
			m_strClientResourcePath, strClientBuf, MAX_PATH, strIni);

		if (strClientBuf[0])
		{
			_tcscpy_s(m_strClientResourcePath, MAX_PATH, strClientBuf);
		}

		m_RecentProjects.clear();
		for (int i = 0; i < kMaxRecentProjects; ++i)
		{
			TCHAR strKey[32] = {};
			_stprintf_s(strKey, _countof(strKey), TEXT("Project%d"), i);

			TCHAR strRecent[MAX_PATH] = {};
			GetPrivateProfileString(TEXT("Recent"), strKey, TEXT(""),
				strRecent, MAX_PATH, strIni);

			if (strRecent[0])
			{
				m_RecentProjects.emplace_back(strRecent);
			}
		}
	}

	void ImguiManager::SaveEditorSettings() const
	{
		TCHAR strIni[MAX_PATH] = {};
		BuildEditorIniPath(strIni, MAX_PATH);

		WritePrivateProfileString(TEXT("Paths"), TEXT("TextureDefault"),
			m_strTextureDefaultPath, strIni);
		WritePrivateProfileString(TEXT("Paths"), TEXT("ClientResourceRoot"),
			m_strClientResourcePath, strIni);

		// Write current entries; clear any tail slots from a previously
		// longer list by writing NULL (deletes the key).
		for (int i = 0; i < kMaxRecentProjects; ++i)
		{
			TCHAR strKey[32] = {};
			_stprintf_s(strKey, _countof(strKey), TEXT("Project%d"), i);

			if (i < (int)m_RecentProjects.size())
			{
				WritePrivateProfileString(TEXT("Recent"), strKey,
					m_RecentProjects[i].c_str(), strIni);
			}
			else
			{
				WritePrivateProfileString(TEXT("Recent"), strKey,
					nullptr, strIni);
			}
		}
	}

	void ImguiManager::AddRecentProject(const std::wstring& dllPath)
	{
		if (dllPath.empty()) return;

		auto it = std::find(m_RecentProjects.begin(), m_RecentProjects.end(), dllPath);
		if (it != m_RecentProjects.end())
		{
			m_RecentProjects.erase(it);
		}
		m_RecentProjects.insert(m_RecentProjects.begin(), dllPath);

		if ((int)m_RecentProjects.size() > kMaxRecentProjects)
		{
			m_RecentProjects.resize(kMaxRecentProjects);
		}

		SaveEditorSettings();
	}

	void ImguiManager::RemoveRecentProject(const std::wstring& dllPath)
	{
		auto it = std::find(m_RecentProjects.begin(), m_RecentProjects.end(), dllPath);
		if (it == m_RecentProjects.end()) return;

		m_RecentProjects.erase(it);
		SaveEditorSettings();
	}

	void ImguiManager::BuildClientSubPath(TCHAR* pOut, size_t iLen, const TCHAR* pSubFolder, const std::string& strFallbackKey) const
	{
		pOut[0] = 0;

		if (m_strClientResourcePath[0] == 0)
		{
			const TCHAR* pFallback = Engine::CPathManager::GetInst()->FindPath(strFallbackKey);
			if (pFallback)
			{
				_tcscpy_s(pOut, iLen, pFallback);
			}
			return;
		}

		_tcscpy_s(pOut, iLen, m_strClientResourcePath);
		size_t cur = _tcslen(pOut);
		if (cur > 0 && pOut[cur - 1] != TEXT('\\') && pOut[cur - 1] != TEXT('/') && cur + 1 < iLen)
		{
			pOut[cur] = TEXT('\\');
			pOut[cur + 1] = 0;
		}
		_tcscat_s(pOut, iLen, pSubFolder);
		cur = _tcslen(pOut);
		if (cur > 0 && pOut[cur - 1] != TEXT('\\') && pOut[cur - 1] != TEXT('/') && cur + 1 < iLen)
		{
			pOut[cur] = TEXT('\\');
			pOut[cur + 1] = 0;
		}
	}

	void ImguiManager::EditorSettings_ImGuiWindow()
	{
		if (!ImGui::Begin("Editor Settings"))
		{
			ImGui::End();
			return;
		}

		ImGui::TextUnformatted("Texture default path");
		ImGui::TextDisabled("Initial directory for the texture picker dialog.");

		// ImGui::InputText uses UTF-8 char buffers; the underlying storage
		// is TCHAR (wide on UNICODE builds). Bridge with a static edit buffer
		// that syncs from the TCHAR field when the field changes externally.
		static char  s_szEditBuf[MAX_PATH] = {};
		static TCHAR s_szLastSeen[MAX_PATH] = {};

		if (_tcscmp(s_szLastSeen, m_strTextureDefaultPath) != 0)
		{
			_tcscpy_s(s_szLastSeen, MAX_PATH, m_strTextureDefaultPath);
			WideCharToMultiByte(CP_UTF8, 0, m_strTextureDefaultPath, -1,
				s_szEditBuf, MAX_PATH, nullptr, nullptr);
		}

		if (ImGui::InputText("##TexturePath", s_szEditBuf, MAX_PATH))
		{
			MultiByteToWideChar(CP_UTF8, 0, s_szEditBuf, -1,
				m_strTextureDefaultPath, MAX_PATH);
			_tcscpy_s(s_szLastSeen, MAX_PATH, m_strTextureDefaultPath);
		}

		ImGui::SameLine();
		if (ImGui::Button("Browse..."))
		{
			BROWSEINFO tBI = {};
			tBI.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
			tBI.lpszTitle = TEXT("Pick the default texture directory");
			tBI.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

			LPITEMIDLIST pPidl = SHBrowseForFolder(&tBI);
			if (pPidl)
			{
				TCHAR strPicked[MAX_PATH] = {};
				if (SHGetPathFromIDList(pPidl, strPicked))
				{
					// Trailing slash so it concatenates cleanly with file
					// names downstream (matches CPathManager convention).
					size_t iLen = _tcslen(strPicked);
					if (iLen > 0 && iLen + 1 < MAX_PATH &&
						strPicked[iLen - 1] != TEXT('\\') &&
						strPicked[iLen - 1] != TEXT('/'))
					{
						strPicked[iLen]     = TEXT('\\');
						strPicked[iLen + 1] = 0;
					}
					_tcscpy_s(m_strTextureDefaultPath, MAX_PATH, strPicked);
				}
				CoTaskMemFree(pPidl);
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextUnformatted("Client resource root");
		ImGui::TextDisabled("Initial directory for Scene/Mesh/Sequence save dialogs.");

		static char  s_szClientEditBuf[MAX_PATH] = {};
		static TCHAR s_szClientLastSeen[MAX_PATH] = {};

		if (_tcscmp(s_szClientLastSeen, m_strClientResourcePath) != 0)
		{
			_tcscpy_s(s_szClientLastSeen, MAX_PATH, m_strClientResourcePath);
			WideCharToMultiByte(CP_UTF8, 0, m_strClientResourcePath, -1,
				s_szClientEditBuf, MAX_PATH, nullptr, nullptr);
		}

		if (ImGui::InputText("##ClientResourcePath", s_szClientEditBuf, MAX_PATH))
		{
			MultiByteToWideChar(CP_UTF8, 0, s_szClientEditBuf, -1,
				m_strClientResourcePath, MAX_PATH);
			_tcscpy_s(s_szClientLastSeen, MAX_PATH, m_strClientResourcePath);
		}

		ImGui::SameLine();
		if (ImGui::Button("Browse...##Client"))
		{
			BROWSEINFO tBI = {};
			tBI.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
			tBI.lpszTitle = TEXT("Pick the client resource root");
			tBI.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

			LPITEMIDLIST pPidl = SHBrowseForFolder(&tBI);
			if (pPidl)
			{
				TCHAR strPicked[MAX_PATH] = {};
				if (SHGetPathFromIDList(pPidl, strPicked))
				{
					size_t iLen = _tcslen(strPicked);
					if (iLen > 0 && iLen + 1 < MAX_PATH &&
						strPicked[iLen - 1] != TEXT('\\') &&
						strPicked[iLen - 1] != TEXT('/'))
					{
						strPicked[iLen]     = TEXT('\\');
						strPicked[iLen + 1] = 0;
					}
					_tcscpy_s(m_strClientResourcePath, MAX_PATH, strPicked);
				}
				CoTaskMemFree(pPidl);
			}
		}

		ImGui::Spacing();

		if (ImGui::Button("Save"))
		{
			SaveEditorSettings();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset to engine default"))
		{
			const TCHAR* pDefault = Engine::CPathManager::GetInst()->FindPath(TEXTURE_PATH);
			if (pDefault)
			{
				_tcscpy_s(m_strTextureDefaultPath, MAX_PATH, pDefault);
			}
			BuildDefaultClientResourcePath(m_strClientResourcePath, MAX_PATH);
		}

		ImGui::End();
	}

	void ImguiManager::DrawSelectionGizmo()
	{
		// Nothing selected → no gizmo. Cheap early-out so we don't burn a
		// fullscreen draw list on every frame in empty selection state.
		auto pSel = m_pSelectedObject.lock();
		if (!pSel) return;

		auto pTr = pSel->GetComponent<Engine::Transform>();
		if (!pTr)
		{
			// PointLight / Camera own their Transform internally instead of
			// registering one on the GameObject's m_Components list. Fall
			// back to Component::GetTransform() (virtual) on each component
			// so the gizmo still attaches to those.
			for (const auto& pComp : pSel->GetComponentList())
			{
				if (!pComp) continue;
				auto pInner = pComp->GetTransform();
				if (pInner) { pTr = pInner; break; }
			}
		}
		if (!pTr) return;

		auto pCam = Engine::Graphics::GetInst()->GetCamera();
		if (!pCam) return;

		// W/E/R cycle TRS modes; X toggles world ↔ local. Skipped when an
		// ImGui text field has focus so typing tag/name doesn't fight the
		// shortcuts.
		if (!ImGui::GetIO().WantTextInput)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W)) m_iGizmoOp = 0;
			if (ImGui::IsKeyPressed(ImGuiKey_E)) m_iGizmoOp = 1;
			if (ImGui::IsKeyPressed(ImGuiKey_R)) m_iGizmoOp = 2;
			if (ImGui::IsKeyPressed(ImGuiKey_X)) m_iGizmoMode ^= 1;
		}

		static const ImGuizmo::OPERATION kOps[] = {
			ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE,
		};
		static const ImGuizmo::MODE kModes[] = {
			ImGuizmo::LOCAL, ImGuizmo::WORLD,
		};
		ImGuizmo::OPERATION eOp = kOps[m_iGizmoOp];
		ImGuizmo::MODE      eMd = kModes[m_iGizmoMode];

		// Overlay the whole viewport; ImGuizmo draws on the foreground draw
		// list when SetDrawlist isn't called. Perspective camera → not ortho.
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetRect(0.f, 0.f, io.DisplaySize.x, io.DisplaySize.y);

		const Engine::Matrix& matView = pCam->GetView();
		const Engine::Matrix& matProj = pCam->GetProjectMatrix();

		// Build the GameObject's world matrix from Transform state. We can't
		// just feed GetTransformMatrix() back into the setters after Manipulate
		// because Transform recomposes from position/rotation/scale every
		// frame — round-tripping through DecomposeMatrixToComponents keeps
		// the three component arrays as the source of truth.
		Engine::Vector3 vPos   = pTr->GetPosition();
		Engine::Vector3 vRot   = pTr->GetRotation();          // radians
		Engine::Vector3 vScale = pTr->GetScale();

		float aTranslation[3] = { vPos.x, vPos.y, vPos.z };
		float aRotationDeg[3] = {
			DirectX::XMConvertToDegrees(vRot.x),
			DirectX::XMConvertToDegrees(vRot.y),
			DirectX::XMConvertToDegrees(vRot.z),
		};
		float aScale[3]       = { vScale.x, vScale.y, vScale.z };

		float aWorld[16] = {};
		ImGuizmo::RecomposeMatrixFromComponents(aTranslation, aRotationDeg, aScale, aWorld);

		const bool bChanged = ImGuizmo::Manipulate(
			reinterpret_cast<const float*>(&matView),
			reinterpret_cast<const float*>(&matProj),
			eOp, eMd, aWorld);

		if (bChanged)
		{
			ImGuizmo::DecomposeMatrixToComponents(aWorld, aTranslation, aRotationDeg, aScale);
			pTr->SetPosition(aTranslation[0], aTranslation[1], aTranslation[2]);
			pTr->SetRelativeRotation(
				DirectX::XMConvertToRadians(aRotationDeg[0]),
				DirectX::XMConvertToRadians(aRotationDeg[1]),
				DirectX::XMConvertToRadians(aRotationDeg[2]));
			pTr->SetRelativeScale(aScale[0], aScale[1], aScale[2]);
		}

		// Small mode-indicator overlay (no separate panel — keeps the
		// gizmo discoverable). Click the labels to switch modes too.
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f);
		if (ImGui::Begin("##GizmoOverlay", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav))
		{
			ImGui::Text("Gizmo (W/E/R, X=mode)");
			if (ImGui::RadioButton("Translate", m_iGizmoOp == 0)) m_iGizmoOp = 0;
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate",    m_iGizmoOp == 1)) m_iGizmoOp = 1;
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale",     m_iGizmoOp == 2)) m_iGizmoOp = 2;
			if (ImGui::RadioButton("Local",     m_iGizmoMode == 0)) m_iGizmoMode = 0;
			ImGui::SameLine();
			if (ImGui::RadioButton("World",     m_iGizmoMode == 1)) m_iGizmoMode = 1;
		}
		ImGui::End();
	}

	void ImguiManager::RenderLightBillboards()
	{
		Engine::Scene* pScene = Engine::SceneManager::GetInst()->GetScene();
		if (!pScene) return;

		auto pCam = Engine::Graphics::GetInst()->GetCamera();
		if (!pCam) return;

		// Fullscreen invisible overlay window glued to the main viewport.
		// Without SetNextWindowViewport, multi-viewport mode breaks this
		// window out into its own OS window that ends up fully covering
		// the game viewport. SetNextWindowBgAlpha(0) + NoBackground
		// double-belt-and-braces transparency so the window itself never
		// renders a fill.
		ImGuiViewport* pMainVP = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(pMainVP->Pos);
		ImGui::SetNextWindowSize(pMainVP->Size);
		ImGui::SetNextWindowViewport(pMainVP->ID);
		ImGui::SetNextWindowBgAlpha(0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
		const ImGuiWindowFlags eFlags =
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoDocking;
		ImGui::Begin("##LightBillboardOverlay", nullptr, eFlags);
		ImDrawList* pOverlayDraw = ImGui::GetWindowDrawList();
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);

		const Engine::Matrix& matVP = pCam->GetViewProject();
		// Engine::Matrix is a custom union (float f[16] row-major) — convert
		// to XMMATRIX via its float* constructor so XMVector4Transform's
		// FXMMATRIX argument is happy. ImGuizmo accepts row-major float*
		// directly so the same VP can stay in Engine::Matrix elsewhere.
		DirectX::XMMATRIX xmVP(reinterpret_cast<const float*>(&matVP));
		ImDrawList* pDraw = pOverlayDraw;

		// Skip click-picking if ImGui is consuming the click (modal/window)
		// or ImGuizmo is in active drag — otherwise a light click would also
		// kick off a re-selection mid-manipulation.
		ImGuiIO& io = ImGui::GetIO();
		const bool bCanPick = !io.WantCaptureMouse && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver();

		auto ProjectToScreen = [&](const Engine::Vector3& v, ImVec2& out) -> bool
		{
			DirectX::XMVECTOR p = DirectX::XMVectorSet(v.x, v.y, v.z, 1.f);
			DirectX::XMVECTOR clip = DirectX::XMVector4Transform(p, xmVP);
			float w = DirectX::XMVectorGetW(clip);
			if (w <= 0.f) return false; // behind camera or on plane
			float ndcX = DirectX::XMVectorGetX(clip) / w;
			float ndcY = DirectX::XMVectorGetY(clip) / w;
			out.x = (ndcX * 0.5f + 0.5f) * io.DisplaySize.x;
			out.y = (1.f - (ndcY * 0.5f + 0.5f)) * io.DisplaySize.y;
			return true;
		};

		for (const auto& pLayer : pScene->GetLayerList())
		{
			if (!pLayer) continue;
			for (const auto& pObj : pLayer->GetGameObjectList())
			{
				if (!pObj) continue;
				auto pLight = pObj->GetComponent<Engine::PointLight>();
				if (!pLight) continue;
				// PointLight owns its Transform internally — pull it via the
				// Component::GetTransform() virtual rather than searching
				// GameObject's m_Components list (where it isn't registered).
				auto pTr = pLight->GetTransform();
				if (!pTr) continue;

				const Engine::Vector3& vPos = pTr->GetPosition();
				ImVec2 vScreen;
				if (!ProjectToScreen(vPos, vScreen)) continue;

				Engine::LIGHT_TYPE eType = pLight->GetLightType();
				ImU32 uColor = IM_COL32(255, 220, 80, 255);
				const char* pTypeMark = "P";
				switch (eType)
				{
				case Engine::LIGHT_TYPE::POINT:       uColor = IM_COL32(255, 220, 80, 255); pTypeMark = "P"; break;
				case Engine::LIGHT_TYPE::SPOT:        uColor = IM_COL32(255, 160, 60, 255); pTypeMark = "S"; break;
				case Engine::LIGHT_TYPE::DIRECTIONAL: uColor = IM_COL32(200, 220, 255, 255); pTypeMark = "D"; break;
				default: break;
				}

				const float fRadius = 9.f;

				// Selected light gets a brighter outer ring so it pops in
				// crowded scenes. m_pSelectedObject is the same selection
				// the Details panel and gizmo follow.
				const bool bSelected = (m_pSelectedObject.lock() == pObj);
				if (bSelected)
				{
					pDraw->AddCircle(vScreen, fRadius + 4.f, IM_COL32(255, 255, 255, 255), 16, 2.f);
				}

				pDraw->AddCircleFilled(vScreen, fRadius, uColor);
				pDraw->AddCircle(vScreen, fRadius, IM_COL32(0, 0, 0, 255), 16, 2.f);
				pDraw->AddText(ImVec2(vScreen.x - 3, vScreen.y - 7), IM_COL32(0, 0, 0, 255), pTypeMark);

				// Direction line for spot/directional — uses Transform's
				// +Z axis as light forward (matches the conventional engine
				// forward direction used elsewhere).
				if (eType == Engine::LIGHT_TYPE::SPOT || eType == Engine::LIGHT_TYPE::DIRECTIONAL)
				{
					const Engine::Vector3& vForward = pTr->GetAxis(Engine::AXIS_TYPE::Z);
					Engine::Vector3 vEnd = vPos + vForward * 2.f;
					ImVec2 vScreenEnd;
					if (ProjectToScreen(vEnd, vScreenEnd))
					{
						pDraw->AddLine(vScreen, vScreenEnd, uColor, 2.f);
					}
				}

				// Tag label to the right of the icon for quick identification
				// in scenes with many lights.
				pDraw->AddText(ImVec2(vScreen.x + fRadius + 4, vScreen.y - 8),
					IM_COL32(0, 0, 0, 200), pObj->GetTag().c_str());
				pDraw->AddText(ImVec2(vScreen.x + fRadius + 5, vScreen.y - 9),
					uColor, pObj->GetTag().c_str());

				// Click pick — circular hit test on icon. ImGui's
				// IsMouseClicked is global, so the bCanPick guard above
				// makes sure UI clicks and gizmo drags don't trigger it.
				if (bCanPick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					ImVec2 vMouse = ImGui::GetMousePos();
					float fDx = vMouse.x - vScreen.x;
					float fDy = vMouse.y - vScreen.y;
					if (fDx * fDx + fDy * fDy <= fRadius * fRadius)
					{
						m_pSelectedObject = pObj;
					}
				}
			}
		}

	}

	void ImguiManager::MaterialBrowser_ImGuiWindow()
	{
		if (!ImGui::Begin("Material Browser"))
		{
			ImGui::End();
			return;
		}

		const auto& mapMaterials = Engine::ResourceManager::GetInst()->GetAllMaterials();

		// ── Toolbar ───────────────────────────────────────────────────
		if (ImGui::Button("+ New Material"))
		{
			ImGui::OpenPopup("NewMaterialPopup");
		}
		ImGui::SameLine();
		if (ImGui::Button("Load .mat..."))
		{
			TCHAR strFile[MAX_PATH] = {};
			OPENFILENAME tName = {};
			tName.lStructSize = sizeof(OPENFILENAME);
			tName.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
			tName.lpstrFilter = TEXT("Material\0*.mat\0All\0*.*\0");
			tName.nMaxFile = MAX_PATH;
			tName.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath(MATERIAL_PATH);
			tName.lpstrFile = strFile;
			tName.lpstrDefExt = TEXT("mat");

			if (GetOpenFileName(&tName))
			{
				char szPath[MAX_PATH] = {};
				WideCharToMultiByte(CP_ACP, 0, strFile, -1, szPath, MAX_PATH, nullptr, nullptr);

				// CRef has no LoadFromFullPath, only LoadFromPath(pathKey-relative).
				// Open by absolute path directly so the picker can pull a .mat
				// from any folder, not just Resource/Material.
				auto pNew = std::make_shared<Engine::Material>();
				FILE* pFile = nullptr;
				fopen_s(&pFile, szPath, "rb");
				if (pFile)
				{
					pNew->Load(pFile);
					fclose(pFile);
				}

				// Fall back to filename stem if the .mat had no tag baked in.
				if (pNew->GetTag().empty())
				{
					char strStem[_MAX_FNAME] = {};
					_splitpath_s(szPath, nullptr, 0, nullptr, 0, strStem, _MAX_FNAME, nullptr, 0);
					pNew->SetTag(strStem);
				}

				// Dedup against the cache — same tag wins (live edits stay
				// in flight). New tag → register so the picker dropdown sees it.
				auto* mgr = Engine::BindableManager<Engine::Material>::GetInst();
				if (auto pPrev = mgr->FindBindable(pNew->GetTag()))
					pNew = pPrev;
				else
					mgr->AddBindable(pNew);

				m_pSelectedMaterial = pNew;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save All"))
		{
			// Persist every cached material with a non-empty tag to
			// Resource/Material/{tag}.mat. Untagged anonymous materials
			// (rare — only legacy untagged FBX imports) are skipped.
			for (const auto& entry : mapMaterials)
			{
				if (entry.first.empty() || !entry.second) continue;
				std::string strFile = entry.first + ".mat";
				entry.second->SaveFromPath(strFile.c_str(), MATERIAL_PATH);
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(%d materials)", static_cast<int>(mapMaterials.size()));

		// ── New Material popup ───────────────────────────────────────
		if (ImGui::BeginPopup("NewMaterialPopup"))
		{
			static char s_szNameBuf[128] = {};
			ImGui::TextUnformatted("Tag (must be unique):");
			ImGui::InputText("##NewMatName", s_szNameBuf, sizeof(s_szNameBuf));

			const bool bNameValid = (s_szNameBuf[0] != 0);
			const bool bCollision = bNameValid &&
				Engine::StaticFindBindable<Engine::Material>(s_szNameBuf) != nullptr;

			if (bCollision)
			{
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Tag already in use.");
			}

			ImGui::BeginDisabled(!bNameValid || bCollision);
			if (ImGui::Button("Create"))
			{
				auto pNew = Engine::StaticCreateBindable<Engine::Material>(s_szNameBuf);
				if (pNew) m_pSelectedMaterial = pNew;
				s_szNameBuf[0] = 0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				s_szNameBuf[0] = 0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::Separator();

		// ── Left: material list ──────────────────────────────────────
		ImGui::BeginChild("MatList", ImVec2(220, 0), true);
		auto pSelected = m_pSelectedMaterial.lock();
		for (const auto& entry : mapMaterials)
		{
			const std::string& strTag = entry.first;
			const std::shared_ptr<Engine::Material>& pMat = entry.second;
			if (!pMat) continue;
			const bool bSel = (pSelected == pMat);
			if (ImGui::Selectable(strTag.c_str(), bSel))
			{
				m_pSelectedMaterial = pMat;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// ── Right: selected material inspector ───────────────────────
		ImGui::BeginChild("MatInspector", ImVec2(0, 0), true);
		if (pSelected)
		{
			Material_ImGuiWindow(pSelected);
		}
		else
		{
			ImGui::TextDisabled("(Select a material from the list)");
		}
		ImGui::EndChild();

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

	void ImguiManager::Terrain_ShowImguiWindow(std::shared_ptr<Engine::Terrain> pTerrain)
	{
		CRef_ImGuiWindow(pTerrain);

		ImGui::Text("=======Terrain======");

		for (int i = 0; i < m_vecBrushTexture.size(); ++i)
		{
			if (ImGui::ImageButton(*m_vecBrushTexture[i]->GetSRV(), ImVec2(32.f, 32.f)))
			{
				pTerrain->SetBrushTexture(m_vecBrushTexture[i]);
			}

			if (i + 1 != m_vecBrushTexture.size()) {
				ImGui::SameLine();
			}
		}

		bool bEraseMode = pTerrain->IsEraseMode();

		if (ImGui::Checkbox("EraseMode", &bEraseMode))
		{
			if (bEraseMode)
			{
				pTerrain->SetEraseMode();
			}
			else
			{
				pTerrain->SetAddMode();
			}

		}
	}

	void ImguiManager::Animation_ImGuiWindow(std::shared_ptr<Engine::Animation> pAnimation)
	{
		if (!pAnimation)
		{
			return;
		}

		bool bOpen = true;
		if (!ImGui::Begin("Animation Editor", &bOpen))
		{
			ImGui::End();
			if (!bOpen)
			{
				m_pSelectedAnimation.reset();
			}
			return;
		}

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

			const std::unordered_map<std::string, Engine::Animation::PSEQUENCEINFO>& mapSequence = pAnimation->GetSequences();

			std::vector<const char*> vecSequenceName;

			std::unordered_map<std::string, Engine::Animation::PSEQUENCEINFO>::const_iterator iterS = mapSequence.begin();
			std::unordered_map<std::string, Engine::Animation::PSEQUENCEINFO>::const_iterator iterSEnd = mapSequence.end();

			for (; iterS != iterSEnd; ++iterS)
			{
				vecSequenceName.push_back(iterS->first.c_str());
			}

			if (vecSequenceName.size())
			{
				static int iSequence = -1;

				ImGui::ListBox("Sequences", &iSequence, &vecSequenceName[0], vecSequenceName.size());
				
				if (iSequence >= 0 && iSequence < vecSequenceName.size())
				{
					const Engine::Animation::PSEQUENCEINFO pInfo = pAnimation->FindSeuqence(vecSequenceName[iSequence]);

					Sequence_ImGuiWindow(pInfo->pSequence);
				}

				if (ImGui::Button("Change Sequence"))
				{
					if (vecSequenceName.size() > iSequence && iSequence >= 0)
					{
						pAnimation->ChangeSequence(vecSequenceName[iSequence]);
					}
				}

				if (ImGui::Button("Add Sequence"))
				{
					if (vecSequenceName.size() > iSequence && iSequence >= 0)
					{
						pAnimation->SetAdditiveSequence(vecSequenceName[iSequence]);
					}
				}

				std::shared_ptr<Engine::Sequence> pSequence = pAnimation->GetCurrentSequence();

				if (pSequence)
				{
					Sequence_ImGuiWindow(pSequence);
				}

				float fTime = pAnimation->GetTime();

				if (ImGui::SliderFloat("time", &fTime, 0.f, pSequence->GetMaxTime()))
				{
					pAnimation->SetTime(fTime);
				}

				float fRate = pAnimation->GetRate();

				if (ImGui::SliderFloat("rate", &fRate, 0.f, 2.f))
				{
					pAnimation->SetRate(fRate);
				}
			}
		}

		ImGui::End();

		if (!bOpen)
		{
			m_pSelectedAnimation.reset();
		}
	}

	void ImguiManager::Scene_ImGuiWindow(Engine::Scene* pScene)
	{
		if (ImGui::Begin("scene"))
		{
			if (ImGui::Button("Save Scene"))
			{
				TCHAR pFilePath[MAX_PATH] = {};
				TCHAR strInitDir[MAX_PATH] = {};
				BuildClientSubPath(strInitDir, MAX_PATH, TEXT("Scene"), ROOT_PATH);

				OPENFILENAME tOFN = {};

				tOFN.lStructSize = sizeof(OPENFILENAME);
				tOFN.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
				tOFN.lpstrInitialDir = strInitDir;
				tOFN.nMaxFile = MAX_PATH;
				tOFN.lpstrFilter = TEXT(".scn");
				tOFN.lpstrFile = pFilePath;

				if (GetOpenFileName(&tOFN))
				{
					if (_tcsstr(tOFN.lpstrFile, TEXT(".")))
					{
						if (_tcsstr(tOFN.lpstrFile, TEXT(".scn")))
						{
							pScene->SaveFromFullPath(pFilePath);
						}
						else
						{
							MessageBox(0, TEXT("Ȯ���� ���� �ùٸ��� �ʽ��ϴ�."), TEXT("����"), MB_OK);
						}
					}
					else
					{
						_tcscat_s(pFilePath, tOFN.lpstrFilter);

						pScene->SaveFromFullPath(pFilePath);
					}
				}
			}
		}

		ImGui::End();
	}

	void ImguiManager::WorldOutliner_ImGuiWindow(Engine::Scene* pScene)
	{
		if (!pScene)
			return;

		if (ImGui::Begin("World Outliner"))
		{
			// ── Add Light toolbar ────────────────────────────────────────
			// Creates a GameObject + PointLight component on DEFAULT_LAYER.
			// PointLight covers all three engine light types via SetLightType
			// (POINT / SPOT / DIRECTIONAL — shader branches in
			// shared.hlsl::GetLightDirAndColor). Auto-selects the new object
			// so Details panel + Transform are immediately editable.
			if (ImGui::Button("+ Add Light"))
			{
				ImGui::OpenPopup("AddLightPopup");
			}

			if (ImGui::BeginPopup("AddLightPopup"))
			{
				static char s_szLightName[64] = "NewLight";
				static int  s_iLightType     = static_cast<int>(Engine::LIGHT_TYPE::POINT);

				ImGui::InputText("Name", s_szLightName, sizeof(s_szLightName));

				const char* aLightTypeNames[] = { "Point", "Spot", "Directional" };
				ImGui::Combo("Type", &s_iLightType, aLightTypeNames, IM_ARRAYSIZE(aLightTypeNames));

				const bool bValid = (s_szLightName[0] != 0);
				ImGui::BeginDisabled(!bValid);
				if (ImGui::Button("Create"))
				{
					auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
					if (pLayer)
					{
						auto pNewObj = pScene->CreateGameObject(s_szLightName, pLayer);
						if (pNewObj)
						{
							auto pLight = pNewObj->AddComponent<Engine::PointLight>(s_szLightName);
							if (pLight)
							{
								pLight->SetLightType(static_cast<Engine::LIGHT_TYPE>(s_iLightType));
							}
							m_pSelectedObject = pNewObj;
						}
					}
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			const auto& layerList = pScene->GetLayerList();
			int iLayerIdx = 0;
			for (const auto& pLayer : layerList)
			{
				if (!pLayer)
					continue;

				std::string strLayerLabel = pLayer->GetTag() + "##l" + std::to_string(iLayerIdx++);
				if (ImGui::TreeNode(strLayerLabel.c_str()))
				{
					const auto& objList = pLayer->GetGameObjectList();
					int iObjIdx = 0;
					auto pSelected = m_pSelectedObject.lock();
					for (const auto& pObj : objList)
					{
						if (!pObj)
							continue;

						std::string strObjLabel = pObj->GetTag() + "##o" + std::to_string(iObjIdx++);
						bool bSelected = (pSelected == pObj);
						if (ImGui::Selectable(strObjLabel.c_str(), bSelected))
						{
							m_pSelectedObject = pObj;
						}
					}
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();

		if (ImGui::Begin("Details"))
		{
			if (auto pSel = m_pSelectedObject.lock())
			{
				GameObject_ImGuiWindow(pSel);
			}
			else
			{
				ImGui::Text("(No GameObject selected)");
			}
		}
		ImGui::End();
	}

	void ImguiManager::GameObject_ImGuiWindow(std::shared_ptr<Engine::GameObject> pObject)
	{
		if (!pObject)
			return;

		CRef_ImGuiWindow(pObject);

		if (auto pParent = pObject->GetParent())
			ImGui::Text("Parent: %s", pParent->GetTag().c_str());
		else
			ImGui::Text("Parent: (none)");

		if (auto pLayer = pObject->GetLayer())
			ImGui::Text("Layer: %s", pLayer->GetTag().c_str());

		ImGui::Separator();

		const auto& compList = pObject->GetComponentList();
		ImGui::Text("Components (%d)", static_cast<int>(compList.size()));

		int iCompIdx = 0;
		for (const auto& pComp : compList)
		{
			if (!pComp)
				continue;

			std::string strLabel = pComp->GetTag() + "##c" + std::to_string(iCompIdx++);
			if (ImGui::TreeNode(strLabel.c_str()))
			{
				Component_ImGuiWindow(pComp);
				ImGui::TreePop();
			}
		}
	}

	void ImguiManager::Component_ImGuiWindow(std::shared_ptr<Engine::Component> pComponent)
	{
		if (!pComponent)
			return;

		ImGui::Text("Type: %d", static_cast<int>(pComponent->GetComponentType()));
		CRef_ImGuiWindow(pComponent);

		if (auto pTrans = pComponent->GetTransform())
		{
			TransformBuffer_ImGuiWindow(pTrans);
		}

		if (auto pMR = std::dynamic_pointer_cast<Engine::MeshRendererComponent>(pComponent))
		{
			MeshRenderer_ImGuiWindow(pMR);
		}

		if (auto pLight = std::dynamic_pointer_cast<Engine::PointLight>(pComponent))
		{
			PointLight_ImGuiWindow(pLight);
		}

		if (auto pAnim = std::dynamic_pointer_cast<Engine::Animation>(pComponent))
		{
			if (ImGui::Button("Open Animation Editor"))
			{
				m_pSelectedAnimation = pAnim;
			}
		}
	}

	void ImguiManager::MeshRenderer_ImGuiWindow(std::shared_ptr<Engine::MeshRendererComponent> pRenderer)
	{
		if (!pRenderer)
			return;

		auto pMesh = pRenderer->GetMesh();
		if (!pMesh)
		{
			ImGui::Text("(No Mesh)");
			return;
		}

		ImGui::Text("Mesh: %s", pMesh->GetTag().c_str());

		ImGui::SameLine();

		if (ImGui::Button("Save Mesh"))
		{
			TCHAR strFile[MAX_PATH] = {};
			TCHAR strInitDir[MAX_PATH] = {};
			BuildClientSubPath(strInitDir, MAX_PATH, TEXT("Mesh"), MESH_PATH);
			OPENFILENAME tName = {};
			tName.lStructSize = sizeof(OPENFILENAME);
			tName.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
			tName.lpstrFilter = TEXT("Mesh\0*.msh;*.mesh\0All\0*.*\0");
			tName.nMaxFile = MAX_PATH;
			tName.lpstrInitialDir = strInitDir;
			tName.lpstrFile = strFile;
			tName.lpstrDefExt = TEXT("msh");

			if (GetSaveFileName(&tName))
			{
				pMesh->SaveFromFullPath(strFile);
			}
		}

		// Primary material shortcut — surfaces the MeshRenderer's
		// renderer-level material (the one assigned via SetMaterial). Voxel
		// chunks, Orbs, and other entities that only call SetMaterial
		// would otherwise be invisible in this panel, since the per-
		// container loop below only walks mesh-slot defaults + per-slot
		// overrides. Clicking the row promotes the material into the
		// Material Browser's selection so its editor pane jumps to it.
		{
			auto pPrimary = pRenderer->GetMaterial();
			const std::string strLabel = pPrimary
				? pPrimary->GetTag()
				: std::string("(none)");
			const std::string strRow = "Material: " + strLabel + "##primarymat";
			if (ImGui::Selectable(strRow.c_str()))
			{
				if (pPrimary)
				{
					m_pSelectedMaterial = pPrimary;
					// Surface the Material Browser too — if the user has it
					// closed or hidden behind other windows, the selection
					// change would otherwise be invisible. Matches the
					// "jump to inspector" expectation of clicking a name.
					ImGui::SetWindowFocus("Material Browser");
				}
			}
		}

		// TreeNode open state IS the outline-selection state: exactly one
		// container can be open at a time, and "open" == "outlined". Opening
		// another node closes the previous one. Closing the open one clears
		// the outline.
		auto pSelected = m_pSelectedObject.lock();
		const int iCurOutlined = (pSelected && m_pOutlinedObject.lock() == pSelected) ? m_iOutlinedContainerIdx : -1;

		const int iContainerCount = pMesh->GetMeshCount();

		// Arrow-key navigation between containers when one is already open.
		// Skipped if a text input field has focus (so arrows still move the
		// caret normally). Drives m_iOutlinedContainerIdx; the SetNextItemOpen
		// loop below picks up the new index on this same frame.
		if (iCurOutlined >= 0 && iContainerCount > 0 && !ImGui::GetIO().WantTextInput)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			{
				m_iOutlinedContainerIdx = (iCurOutlined + 1 < iContainerCount) ? iCurOutlined + 1 : iCurOutlined;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			{
				m_iOutlinedContainerIdx = (iCurOutlined > 0) ? iCurOutlined - 1 : 0;
			}
		}

		const int iEffectiveOutlined = (m_pOutlinedObject.lock() == pSelected) ? m_iOutlinedContainerIdx : iCurOutlined;

		for (int i = 0; i < iContainerCount; ++i)
		{
			const bool bShouldBeOpen = (iEffectiveOutlined == i);
			ImGui::SetNextItemOpen(bShouldBeOpen, ImGuiCond_Always);

			std::string strContainerLabel = "Container " + std::to_string(i) + "##mc" + std::to_string(i);
			bool bOpen = ImGui::TreeNode(strContainerLabel.c_str());

			if (ImGui::IsItemClicked())
			{
				if (bShouldBeOpen)
				{
					m_pOutlinedObject.reset();
					m_iOutlinedContainerIdx = -1;
				}
				else
				{
					m_pOutlinedObject = pSelected;
					m_iOutlinedContainerIdx = i;
				}
			}

			if (!bOpen)
				continue;

			const int iSubCount = pMesh->GetMeshSubCount(i);
			for (int j = 0; j < iSubCount; ++j)
			{
				// Three-layer resolution:
				//   override on this MR → mesh slot default
				// Editing the *effective* material is the user's intent
				// (they see what's currently in-flight). The picker below
				// switches which material the slot points at.
				auto pMeshDefault = pMesh->GetMaterial(i, j);
				auto pOverride    = pRenderer->GetOverrideMaterial(i, j);
				auto pEffective   = pOverride ? pOverride : pMeshDefault;
				if (!pEffective)
					continue;

				std::string strMatLabel = "Material " + std::to_string(j) +
					" (" + pEffective->GetTag() + (pOverride ? " [override]" : "") +
					")##mat" + std::to_string(i) + "_" + std::to_string(j);
				if (ImGui::TreeNode(strMatLabel.c_str()))
				{
					// Override picker — every loaded .mat asset is selectable,
					// plus "(Mesh Default)" to clear the override and fall
					// back to the slot material baked into the .mesh.
					std::string strComboId = "##matpick" + std::to_string(i) + "_" + std::to_string(j);
					const char* pCurrentLabel = pOverride ? pOverride->GetTag().c_str() : "(Mesh Default)";
					if (ImGui::BeginCombo(strComboId.c_str(), pCurrentLabel))
					{
						if (ImGui::Selectable("(Mesh Default)", !pOverride))
						{
							pRenderer->SetOverrideMaterial(i, j, nullptr);
						}
						for (const auto& entry : Engine::ResourceManager::GetInst()->GetAllMaterials())
						{
							const std::string& strTag = entry.first;
							const std::shared_ptr<Engine::Material>& pMat = entry.second;
							const bool bSelected = pOverride && pOverride->GetTag() == strTag;
							if (ImGui::Selectable(strTag.c_str(), bSelected))
							{
								pRenderer->SetOverrideMaterial(i, j, pMat);
							}
						}
						ImGui::EndCombo();
					}

					Material_ImGuiWindow(pEffective);
					ImGui::TreePop();
				}
			}

			// Texture slot picker used to live here (per-container). It
			// moved into Material_ImGuiWindow above when textures became
			// per-Material — every Material tree node now shows its own
			// slot list with Set/Clear buttons.

			ImGui::TreePop();
		}
	}

	void ImguiManager::Sequence_ImGuiWindow(std::shared_ptr<Engine::Sequence> pSequence)
	{
		std::string strTitle = "Sequence";

		strTitle += pSequence->GetTag();

		if (ImGui::Begin(strTitle.c_str()))
		{
			bool bLoop = pSequence->IsLoop();

			if (ImGui::Checkbox("Loop", &bLoop))
			{
				if (bLoop)
				{
					pSequence->Loop();
				}
			}

			int iMaxFrame = pSequence->GetMaxFrame();
			int iFrameLimit = pSequence->GetFrameDataLimit();
			ImGui::Text("Frame Data Limit: %d", iFrameLimit);
			ImGui::Text("Max Time: %.3f", pSequence->GetMaxTime());
			if (iFrameLimit > 0)
			{
				if (ImGui::SliderInt("Max Frame", &iMaxFrame, 1, iFrameLimit))
				{
					pSequence->SetMaxFrame(iMaxFrame);
				}
			}

			int iFrame = pSequence->GetFrame();

			Engine::Sequence::PSEQUENCEINFO pInfo = pSequence->GetSequenceInfo();

			if (pInfo)
			{
				std::string s = "s";
				std::string r = "r";
				std::string t = "t";

				for (int i = 0; i < static_cast<int>(pInfo->vecJoint.size()); ++i)
				{
					if (pInfo->vecJoint[i].vecFrame.size() > iFrame)
					{
						std::string bone = std::to_string(i);
						ImGui::Text("bone: %d", i);
						ImGui::SameLine();
						Engine::Vector3 vScale = pInfo->vecJoint[i].vecFrame[iFrame].vScale;
						if (ImGui::InputFloat3((s + bone).c_str(), &vScale.x))
						{
							pSequence->SetFrameScale(i, iFrame, vScale);
						}
						Engine::Vector4 vQuternion = pInfo->vecJoint[i].vecFrame[iFrame].vQueternion;
						if (ImGui::InputFloat4((r + bone).c_str(), &vQuternion.x))
						{
							pSequence->SetFrameRotation(i, iFrame, vQuternion);
						}
						Engine::Vector3 vPos = pInfo->vecJoint[i].vecFrame[iFrame].vPos;
						if (ImGui::InputFloat3((t + bone).c_str(), &vPos.x))
						{
							pSequence->SetFramePosition(i, iFrame, vPos);
						}
					}
				}
			}

			const std::vector<float>& vecBlendPalette = pSequence->GetBlendPalette();

			for (int i = 0; i < vecBlendPalette.size(); ++i)
			{
				std::string strBlend = "joint_blend";

				strBlend += std::to_string(i);

				float fBlendFactor = vecBlendPalette[i];

				if (ImGui::SliderFloat(strBlend.c_str(), &fBlendFactor, 0.f, 1.f))
				{
					pSequence->SetBlendFactor(i, fBlendFactor);
				}
			}

			float fBlend = 0.f;

			if (ImGui::SliderFloat("joint_blend_all", &fBlend, 0.f, 1.f))
			{
				for (int i = 0; i < vecBlendPalette.size(); ++i)
				{
					pSequence->SetBlendFactor(i, fBlend);
				}
			}

			if (ImGui::Button("Save Sequence"))
			{
				TCHAR strFullPath[MAX_PATH] = {};
				TCHAR strInitDir[MAX_PATH] = {};
				BuildClientSubPath(strInitDir, MAX_PATH, TEXT("Mesh"), MESH_PATH);

				OPENFILENAME tOFN = {};

				tOFN.lpstrInitialDir = strInitDir;
				tOFN.lpstrFile = strFullPath;
				tOFN.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
				tOFN.lStructSize = sizeof(OPENFILENAME);
				tOFN.nMaxFile = MAX_PATH;
				tOFN.lpstrFilter = TEXT("*.SEQ");

				if (GetOpenFileName(&tOFN))
				{
					pSequence->SaveFromFullPath(strFullPath);
				}
			}
		}

		ImGui::End();
	}

	void ImguiManager::RenderManager_ShowImGuiWindow()
	{
		if (ImGui::Begin("RenderManager"))
		{
			/*ImGui::Text("RenderList Size: %d", m_RenderList[0].size());
			for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
			{
				ImGui::Text("LightList Type: %d, Size: %d", i, m_LightList[i].size());
			}

			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[0].begin();
			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterEnd = m_mapInstance[0].end();

			for (; iter != iterEnd; ++iter)
			{
				ImGui::Text("Instance: %s, Size: %d", iter->second->GetTag().c_str(), iter->second->GetCount());
			}

			if (ImGui::Button("Reload Multi Shader"))
			{
				pMultiVertexShader->LoadShader();

				pMultiPixelShader->LoadShader();
			}*/

			float fMidGray = Engine::RenderManager::GetInst()->GetHDRMidGray();

			if (ImGui::SliderFloat("Middle Gray", &fMidGray, 0.f, 1.f))
			{
				Engine::RenderManager::GetInst()->SetHDRMidGray(fMidGray);
			}

			float fWhite = sqrtf(Engine::RenderManager::GetInst()->GetHDRWhiteSqr());

			if (ImGui::SliderFloat("White", &fWhite, 0.f, 1.f))
			{
				Engine::RenderManager::GetInst()->SetHDRWhiteSqr(fWhite * fWhite);
			}

			if (ImGui::Checkbox("Enable Bloom", &m_bEnableBloom))
			{
				if (m_bEnableBloom)
				{
					Engine::RenderManager::GetInst()->SetBloomScale(m_fSavedBloomScale);
				}
				else
				{
					m_fSavedBloomScale = Engine::RenderManager::GetInst()->GetBloomScale();
					Engine::RenderManager::GetInst()->SetBloomScale(0.f);
				}
			}

			ImGui::BeginDisabled(!m_bEnableBloom);
			float fBloomScale = Engine::RenderManager::GetInst()->GetBloomScale();

			if (ImGui::SliderFloat("Bloom Scale", &fBloomScale, 0.f, 5.f))
			{
				Engine::RenderManager::GetInst()->SetBloomScale(fBloomScale);
			}

			float fThreshold = Engine::RenderManager::GetInst()->GetBloomThreshold();

			if (ImGui::SliderFloat("Bloom Threshold", &fThreshold, 0.f, 100.f))
			{
				Engine::RenderManager::GetInst()->SetBloomThreshold(fThreshold);
			}
			ImGui::EndDisabled();

			float fAdaptSpeed = Engine::RenderManager::GetInst()->GetAdaptationSpeed();

			if (ImGui::SliderFloat("Adaptation Speed", &fAdaptSpeed, 0.1f, 20.f))
			{
				Engine::RenderManager::GetInst()->SetAdaptationSpeed(fAdaptSpeed);
			}

			if (ImGui::Checkbox("Enable DOF (FOV)", &m_bEnableDOF))
			{
				if (m_bEnableDOF)
				{
					Engine::RenderManager::GetInst()->SetFOVValueY(m_fSavedFOVValueY);
				}
				else
				{
					m_fSavedFOVValueY = Engine::RenderManager::GetInst()->GetFOVValueY();
					Engine::RenderManager::GetInst()->SetFOVValueY(0.f);
				}
			}

			ImGui::BeginDisabled(!m_bEnableDOF);
			float fDOFVAlueX = Engine::RenderManager::GetInst()->GetFOVValueX();

			if (ImGui::SliderFloat("FOV Value X", &fDOFVAlueX, 0.f, 1000.f))
			{
				Engine::RenderManager::GetInst()->SetFOVValueX(fDOFVAlueX);
			}

			float fDOFVAlueY = Engine::RenderManager::GetInst()->GetFOVValueY();

			if (ImGui::SliderFloat("FOV Value Y", &fDOFVAlueY, 0.f, 5.f))
			{
				Engine::RenderManager::GetInst()->SetFOVValueY(fDOFVAlueY);
			}
			ImGui::EndDisabled();
		}

		ImGui::End();

		if (ImGui::Begin("Debug HDR"))
		{
			std::shared_ptr<Engine::Texture> pHDRDownScaleTexture = Engine::RenderManager::GetInst()->GetHDRDownScaleTexture();

			ImGui::Image(*pHDRDownScaleTexture->GetSRV(), ImVec2(128.f, 128.f));

			std::shared_ptr<Engine::Texture> pBloomTexture = Engine::RenderManager::GetInst()->GetBloomTexture();

			ImGui::Image(*pBloomTexture->GetSRV(), ImVec2(128.f, 128.f));

			std::shared_ptr<Engine::Texture> pBloomFinalTexture = Engine::RenderManager::GetInst()->GetBloomFinalTexture();

			ImGui::Image(*pBloomFinalTexture->GetSRV(), ImVec2(128.f, 128.f));
		}

		ImGui::End();

		if (ImGui::Begin("Fog Setting"))
		{
			if (ImGui::Checkbox("Enable Fog", &m_bEnableFog))
			{
				if (m_bEnableFog)
				{
					Engine::RenderManager::GetInst()->SetFogDensity(m_fSavedFogDensity);
				}
				else
				{
					m_fSavedFogDensity = Engine::RenderManager::GetInst()->GetFogDensity();
					Engine::RenderManager::GetInst()->SetFogDensity(0.f);
				}
			}

			ImGui::BeginDisabled(!m_bEnableFog);
			Engine::Vector3 vColor = Engine::RenderManager::GetInst()->GetFogColor();

			if (ImGui::ColorPicker3("color", &vColor.x))
			{
				Engine::RenderManager::GetInst()->SetFogColor(vColor);
			}

			Engine::Vector3 vHighlightColor = Engine::RenderManager::GetInst()->GetFogHighlightColor();

			if (ImGui::ColorPicker3("highlightcolor", &vHighlightColor.x))
			{
				Engine::RenderManager::GetInst()->SetFogHighlightColor(vHighlightColor);
			}

			float fDepth = Engine::RenderManager::GetInst()->GetFogStartDepth();

			if (ImGui::InputFloat("depth", &fDepth))
			{
				Engine::RenderManager::GetInst()->SetFogStartDepth(fDepth);
			}

			float fDensity = Engine::RenderManager::GetInst()->GetFogDensity();

			if (ImGui::InputFloat("density", &fDensity))
			{
				Engine::RenderManager::GetInst()->SetFogDensity(fDensity);
			}

			float fHeightFallOff = Engine::RenderManager::GetInst()->GetFogHeightFallOff();

			if (ImGui::InputFloat("height fall off", &fHeightFallOff))
			{
				Engine::RenderManager::GetInst()->SetFogHeightFallOff(fHeightFallOff);
			}
			ImGui::EndDisabled();
		}

		ImGui::End();
	}

	void ImguiManager::Layer_DrawListImgui(std::shared_ptr<Engine::Layer> pLayer)
	{
		if (!pLayer || !ImGui::Begin(pLayer->GetTag().c_str()))
		{
			return;
		}

		// Phase E7 — Layer's m_DrawList of Bindable-typed entities is gone;
		// the per-drawable list-and-inspect UI moves into a future
		// GameObject inspector (the Layer's m_GameObjectList walks would
		// produce roughly the same shape). For now this panel keeps the
		// FBX/OBJ import + NavMesh build buttons that still apply.

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

	void ImguiManager::Project_ImGuiWindow()
	{
		auto& project = ProjectModule::Get();

		if (!ImGui::Begin("Project"))
		{
			ImGui::End();
			return;
		}

		if (!project.IsLoaded())
		{
			ImGui::TextUnformatted("No game module loaded.");
			if (ImGui::Button("Open Project..."))
			{
				std::wstring path = ProjectModule::OpenDialog(m_hWnd);
				if (!path.empty())
				{
					if (project.Load(path))
					{
						AddRecentProject(path);
					}
					else
					{
						ImGui::OpenPopup("LoadFailed");
					}
				}
			}

			// Recent projects — one click reopens. On failure we drop the
			// entry (DLL likely moved/deleted) and surface the same popup.
			if (!m_RecentProjects.empty())
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::TextUnformatted("Recent projects:");

				std::wstring wstrToLoad;
				std::wstring wstrToRemove;

				for (size_t i = 0; i < m_RecentProjects.size(); ++i)
				{
					const std::wstring& wstrPath = m_RecentProjects[i];

					char strUtf8[MAX_PATH * 2] = {};
					WideCharToMultiByte(CP_UTF8, 0, wstrPath.c_str(), -1,
						strUtf8, sizeof(strUtf8), nullptr, nullptr);

					std::string strLabel = strUtf8;
					strLabel += "##recent";
					strLabel += std::to_string(i);

					if (ImGui::Selectable(strLabel.c_str()))
					{
						wstrToLoad = wstrPath;
					}
				}

				if (!wstrToLoad.empty())
				{
					if (project.Load(wstrToLoad))
					{
						AddRecentProject(wstrToLoad);
					}
					else
					{
						wstrToRemove = wstrToLoad;
						ImGui::OpenPopup("LoadFailed");
					}
				}

				if (!wstrToRemove.empty())
				{
					RemoveRecentProject(wstrToRemove);
				}
			}

			if (ImGui::BeginPopup("LoadFailed"))
			{
				ImGui::TextUnformatted("LoadLibrary failed. Is the DLL path valid?");
				if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}
			ImGui::End();
			return;
		}

		ImGui::Text("Loaded: %ls", project.Path().c_str());
		ImGui::Separator();

		// --- Play / Stop ---
		auto scenes = Engine::SceneFactory::ListAll();
		static int selectedScene = 0;
		if (selectedScene >= (int)scenes.size()) selectedScene = 0;

		ImGui::TextUnformatted("Startup scene:");
		if (!scenes.empty())
		{
			if (ImGui::BeginCombo("##scene", scenes[selectedScene].c_str()))
			{
				for (int i = 0; i < (int)scenes.size(); ++i)
				{
					bool sel = (selectedScene == i);
					if (ImGui::Selectable(scenes[i].c_str(), sel)) selectedScene = i;
				}
				ImGui::EndCombo();
			}

			if (ImGui::Button("> Play"))
			{
				Engine::Scene* s = Engine::SceneFactory::Create(scenes[selectedScene]);
				Engine::SceneManager::GetInst()->SetScene(s, Engine::SCENE_TYPE::NEXT);
			}
			ImGui::SameLine();
			if (ImGui::Button("# Stop"))
			{
				Engine::SceneManager::GetInst()->SetScene(
					new Editor::InGameScene(), Engine::SCENE_TYPE::NEXT);
			}
		}
		else
		{
			ImGui::TextDisabled("(no scenes registered by this module)");
		}

		ImGui::Separator();

		// --- Actor palette ---
		auto actors = Engine::GameObjectFactory::ListAll();
		ImGui::Text("Registered actors (%d):", (int)actors.size());
		for (auto& name : actors)
		{
			ImGui::BulletText("%s", name.c_str());
			ImGui::SameLine();
			std::string btn = "Spawn##" + name;
			if (ImGui::SmallButton(btn.c_str()))
			{
				if (auto* pScene = Engine::SceneManager::GetInst()->GetScene())
				{
					if (auto pLayer = pScene->FindLayer(DEFAULT_LAYER))
					{
						if (auto* pGO = Engine::GameObjectFactory::Create(name))
						{
							std::shared_ptr<Engine::GameObject> sp(pGO);
							sp->Init();
							pLayer->AddGameObject(sp);
						}
					}
				}
			}
		}

		ImGui::End();
	}
}