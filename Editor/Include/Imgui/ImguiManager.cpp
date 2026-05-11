#include "ImguiManager.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
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

namespace Editor
{
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

		RenderManager_ShowImGuiWindow();
		Scene_ImGuiWindow(Engine::SceneManager::GetInst()->GetScene());
		WorldOutliner_ImGuiWindow(Engine::SceneManager::GetInst()->GetScene());

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

		int iContainerCount = pMesh->GetMeshCount();

		for (int i = 0; i < iContainerCount; ++i)
		{
			int iSubCount = pMesh->GetMeshSubCount(i);

			for (int j = 0; j < iSubCount; ++j)
			{
				ImGui::Text("Container: %d, Sub: %d Material", i, j);

				std::shared_ptr<Engine::Material> pMaterial = pMesh->GetMaterial(i, j);

				if (pMaterial)
				{
					Material_ImGuiWindow(pMaterial);
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
		ImGui::Text("==============Animation============");

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
	}

	void ImguiManager::Scene_ImGuiWindow(Engine::Scene* pScene)
	{
		if (ImGui::Begin("scene"))
		{
			if (ImGui::Button("Save Scene"))
			{
				TCHAR pFilePath[MAX_PATH] = {};

				OPENFILENAME tOFN = {};

				tOFN.lStructSize = sizeof(OPENFILENAME);
				tOFN.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
				tOFN.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath();
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
			OPENFILENAME tName = {};
			tName.lStructSize = sizeof(OPENFILENAME);
			tName.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
			tName.lpstrFilter = TEXT("Mesh\0*.msh;*.mesh\0All\0*.*\0");
			tName.nMaxFile = MAX_PATH;
			tName.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath(MESH_PATH);
			tName.lpstrFile = strFile;
			tName.lpstrDefExt = TEXT("msh");

			if (GetSaveFileName(&tName))
			{
				pMesh->SaveFromFullPath(strFile);
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
				auto pMaterial = pMesh->GetMaterial(i, j);
				if (!pMaterial)
					continue;

				std::string strMatLabel = "Material " + std::to_string(j) +
					" (" + pMaterial->GetTag() + ")##mat" + std::to_string(i) + "_" + std::to_string(j);
				if (ImGui::TreeNode(strMatLabel.c_str()))
				{
					Material_ImGuiWindow(pMaterial);
					ImGui::TreePop();
				}
			}

			// Named texture slots. Slot indices match register(tN) in
			// shared.hlsl: 0=Diffuse, 1=Normal, 2=Specular, 3=Emissive,
			// 6=Roughness, 8=AO, 9=Metalness. Roughness/AO/Metalness are
			// optional in the main PS (detected via GetDimensions) and
			// fall back cleanly when unbound — the engine's specular
			// workflow stays intact for assets without these textures.
			struct SlotDesc { int iSlot; const char* pName; };
			static const SlotDesc kSlots[] = {
				{ 0, "Diffuse"   },
				{ 1, "Normal"    },
				{ 2, "Specular"  },
				{ 3, "Emissive"  },
				{ 6, "Roughness" },
				{ 8, "AO"        },
				{ 9, "Metalness" },
			};

			const int iTexCount = pMesh->GetTextureCount(i);

			for (const SlotDesc& slot : kSlots)
			{
				// Locate the texture currently occupying this GPU slot in
				// this container's vector (linear scan; vector is tiny).
				std::shared_ptr<Engine::Texture> pTex;
				int iVecIdx = -1;
				for (int j = 0; j < iTexCount; ++j)
				{
					auto pCandidate = pMesh->GetTexture(i, j);
					if (pCandidate && pCandidate->GetSlot() == slot.iSlot)
					{
						pTex = pCandidate;
						iVecIdx = j;
						break;
					}
				}

				ImGui::Text("%s: %s", slot.pName, pTex ? pTex->GetTag().c_str() : "(empty)");

				ImGui::SameLine();

				std::string strBtnLabel = "Set##slot" + std::to_string(i) + "_" + std::to_string(slot.iSlot);
				if (ImGui::Button(strBtnLabel.c_str()))
				{
					TCHAR strFile[MAX_PATH] = {};
					OPENFILENAME tName = {};
					tName.lStructSize = sizeof(OPENFILENAME);
					tName.hwndOwner = Engine::Window::GetInst()->GetWinHandle();
					tName.lpstrFilter = TEXT("Texture\0*.png;*.dds;*.tga;*.jpg;*.bmp\0All\0*.*\0");
					tName.nMaxFile = MAX_PATH;
					tName.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath(TEXTURE_PATH);
					tName.lpstrFile = strFile;

					if (GetOpenFileName(&tName))
					{
						char szTag[MAX_PATH] = {};
						WideCharToMultiByte(CP_ACP, 0, strFile, -1, szTag, MAX_PATH, nullptr, nullptr);
						std::string strTag(szTag);
						auto pNewTex = Engine::StaticCreateBindable<Engine::Texture>(strTag, strFile, slot.iSlot);
						if (pNewTex)
						{
							// Replace if a texture exists at this slot,
							// otherwise append to the vector.
							const int iWriteIdx = (iVecIdx >= 0) ? iVecIdx : pMesh->GetTextureCount(i);
							pMesh->SetTexture(i, iWriteIdx, pNewTex);
						}
					}
				}

			}

			// Surface any textures that don't match a named slot (legacy
			// data, custom slots) so they remain visible to the user.
			auto isNamedSlot = [](int s) {
				for (const SlotDesc& d : kSlots) if (d.iSlot == s) return true;
				return false;
			};
			bool bAnyOther = false;
			for (int j = 0; j < pMesh->GetTextureCount(i); ++j)
			{
				auto pT = pMesh->GetTexture(i, j);
				if (!pT) continue;
				int iSlot = pT->GetSlot();
				if (isNamedSlot(iSlot)) continue;
				if (!bAnyOther)
				{
					ImGui::Separator();
					ImGui::TextDisabled("Other textures:");
					bAnyOther = true;
				}
				ImGui::Text("  slot t%d: %s", iSlot, pT->GetTag().c_str());
			}

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

				OPENFILENAME tOFN = {};

				tOFN.lpstrInitialDir = Engine::CPathManager::GetInst()->FindPath(MESH_PATH);
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
}