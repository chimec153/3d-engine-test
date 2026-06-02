#include "Window.h"
#include "../resource.h"
#include "PathManager.h"
#include "../Input/Input.h"
#include "../Bindable/Box.h"
#include "../Bindable/Sphere.h"
#include "../Bindable/Cone.h"
#include "../Bindable/Quad.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/Cylinder.h"
#include "../Bindable/Transform.h"
#include "../Bindable/Camera.h"
#include "../Bindable/PointLight.h"
#include "../Bindable/Sampler.h"
#include "../Shader/ShaderManager.h"
#include "../Bindable/Topology.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/BindableRegistry.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/IndexBuffer.h"
#include "../Bindable/Texture.h"
#include "../Bindable/Material.h"
#include "../Bindable/BlendState.h"
#include "../Scene/SceneManager.h"
#include "../Scene/Scene.h"
#include "../Render/RenderManager.h"
#include "../Collision/CollisionManager.h"
#include "../Bindable/ColliderSphere.h"
#include "../Bindable/Animation.h"
#include "../Animation/Skeleton.h"
#include "../Resource/ResourceManager.h"
#include "../Bindable/Mesh.h"
#include "../Animation/Sequence.h"
#include "../Thread/ThreadManager.h"
#include "../Animation/JointSocket.h"
#include "../Bindable/Terrain.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/ColliderMesh.h"
#include "../Bindable/GeometryShader.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Resource/FontManager.h"

namespace Engine
{
	class DepthStencilState;
	class HullShader;
	class DomainShader;
	class ComputeShader;

	Window* Window::m_pInst = nullptr;

	Window::Window() :
		m_hWnd()
		, m_hInst()
		, m_bRun(true)
		, pTimer(nullptr)
		, m_iWidth(0)
		, m_iHeight(0)
		, bCursorEnable(false)
		, bLockRotate(false)
		, m_fFixedTime(0.f)
	{
#ifdef _DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	}

	Window::~Window()
	{
		Engine::BindableManager<class Engine::VertexBuffer>::DestroyInst();
		Engine::BindableManager<class Engine::IndexBuffer>::DestroyInst();
		Engine::BindableManager<class Engine::RasterizerState>::DestroyInst();
		Engine::BindableManager<class Engine::DepthStencilState>::DestroyInst();
		Engine::BindableManager<class Engine::BlendState>::DestroyInst();
		Engine::BindableManager<class Engine::VertexShader>::DestroyInst();
		Engine::BindableManager<class Engine::HullShader>::DestroyInst();
		Engine::BindableManager<class Engine::DomainShader>::DestroyInst();
		Engine::BindableManager<class Engine::GeometryShader>::DestroyInst();
		Engine::BindableManager<class Engine::PixelShader>::DestroyInst();
		Engine::BindableManager<class Engine::ComputeShader>::DestroyInst();
		Engine::BindableManager<class Engine::Sampler>::DestroyInst();
		Engine::BindableManager<class Engine::Topology>::DestroyInst();
		Engine::BindableManager<class Engine::InputLayout>::DestroyInst();
		Engine::BindableManager<class Engine::Material>::DestroyInst();
		Engine::BindableManager<class Engine::Mesh>::DestroyInst();
		Engine::BindableManager<class Engine::Transform>::DestroyInst();
		Engine::BindableManager<class Engine::Texture>::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagTransformBuffer>>::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagBoneCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagUICBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagTerrainCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPointLight> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagMaterial> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPerspectiveBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagColor> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagIKCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagParticleCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagGlobalCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagDecalCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagPaperBurnCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ConstantBuffer<struct Engine::_tagFluidCBuffer> >::DestroyInst();

		ThreadManager::DestroyInst();

		Scene::Clear();

		SceneManager::DestroyInst();

		FontManager::DestroyInst();

		ResourceManager::DestroyInst();

		RenderManager::DestroyInst();

		ShaderManager::DestroyInst();

		CInput::DestroyInst();

		// Run every registered shutdown callback before the device dies.
		// Catches non-Bindable D3D resource holders (e.g. DamageTextManager)
		// that piggy-back on this registry so Client/Editor mains don't
		// need per-app wiring.
		BindableRegistry::DestroyAll();

		Graphics::DestroyInst();

		CollisionManager::DestroyInst();

		CPathManager::DestoryInst();

		if (m_hWnd)
		{
			CloseWindow(m_hWnd);
		}
	}

	std::shared_ptr<Timer> Window::GetTimer() const
	{
		return pTimer;
	}

	// Window::Stop / Resume removed — pause state moved to Timer. Use
	// Window::GetInst()->GetTimer()->Stop() / Resume() instead.

	void Window::CursorEnable()
	{
		//while (ShowCursor(TRUE) < 0);

		ClipCursor(nullptr);

		m_pInst->bCursorEnable = true;
	}

	void Window::CursorDisable()
	{
		//while (ShowCursor(FALSE) >= 0);

		POINT pt = {};

		ClientToScreen(m_hWnd, &pt);

		RECT rc = { pt.x, pt.y, m_iWidth + pt.x, m_iHeight + pt.y };

		ClipCursor(&rc);

		m_pInst->bCursorEnable = false;
	}

	bool Window::IsLockRotation() const
	{
		return bLockRotate;
	}

	int Window::GetWidth() const
	{
		return m_iWidth;
	}

	int Window::GetHeight() const
	{
		return m_iHeight;
	}

	bool Window::IsCursorEnabled() const
	{
		return bCursorEnable;
	}

	bool Window::IsRun() const
	{
		return m_bRun;
	}

	HWND Window::GetWinHandle() const
	{
		return m_hWnd;
	}

	void Window::StopRunning()
	{
		m_bRun = false;
	}

	int Window::RegisterResizeCallback(std::function<void(int, int)> cb)
	{
		const int iToken = m_iNextResizeToken++;
		m_mapResizeCallbacks.emplace(iToken, std::move(cb));
		return iToken;
	}

	void Window::UnregisterResizeCallback(int iToken)
	{
		m_mapResizeCallbacks.erase(iToken);
	}

	bool Window::Init(const TCHAR* pTitle, const TCHAR* pClass, HINSTANCE hInst, WNDPROC proc, int iWidth, int iHeight)
	{
		m_hInst = hInst;

		m_iWidth = iWidth;
		m_iHeight = iHeight;

		Register(pClass, hInst, proc);

		if (!Create(pTitle, pClass, hInst))
		{
			return false;
		}

		// ��� ������ �ʱ�ȭ
		if (!CPathManager::GetInst()->Init())
		{
			return false;
		}

		if (!Graphics::GetInst()->Init(m_hWnd, iWidth, iHeight))
		{
			return false;
		}

		pTimer = std::make_shared<Timer>();

		if (!pTimer->Init())
		{
			return false;
		}

		// �Է� �ʱ�ȭ
		if (!CInput::GetInst()->Init(hInst, m_hWnd))
		{
			return false;
		}

		// ���ҽ� ������ �ʱ�ȭ (����)
		if (!ResourceManager::GetInst()->Init())
		{
			return false;
		}

		// ���̴� ������ �ʱ�ȭ
		if (!ShaderManager::GetInst()->Init())
		{
			return false;
		}

		if (!BindableManager<VertexShader>::GetInst()->Init())
		{
			return false;
		}

		BindableManager<Sampler>::GetInst();

		if (!RenderManager::GetInst()->Init())
		{
			return false;
		}

		// Material assets live in Resource/Material/*.mat. Scan after the
		// shader/cbuffer infrastructure is up so each Material can resolve
		// its "Material" ConstantBuffer at construction. Empty/missing
		// folder is a no-op — fresh projects start with zero .mat assets.
		ResourceManager::GetInst()->LoadAllMaterials();

		CInput::GetInst()->AddKey(DIK_ESCAPE);

		Scene* pCurrentScene = SceneManager::GetInst()->GetScene(SCENE_TYPE::CURRENT);
		(void)pCurrentScene;

		// Phase E7 — DEBUG "Line" prototype removed. The only consumer was
		// CollisionManager's space/portal debug block (CollisionManager.h),
		// which has been commented out since the Drawable migration began.
		// Debug colliders today create their own Drawable inline (see
		// ColliderLine.cpp), bypassing this prototype path entirely.

		return true;
	}

	bool Window::Create(const TCHAR* pTitle, const TCHAR* pClass, HINSTANCE hInst, int iWidth, int iHeight)
	{
		RECT rc = { 0,0,iWidth,iHeight };
		DWORD iStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU;

		m_hWnd = CreateWindowW(pClass, pTitle, iStyle,
			rc.left, rc.top, rc.right, rc.bottom, nullptr, nullptr, hInst, nullptr);

		if (!m_hWnd)
		{
			return false;
		}

		AdjustWindowRect(&rc, iStyle, false);

		MoveWindow(m_hWnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, true);

		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);

		return true;
	}

	int Window::Register(const TCHAR* pClass, HINSTANCE hInst, WNDPROC proc)
	{
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInst;
		wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = pClass;
		wcex.hIconSm = nullptr;// LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		return RegisterClassExW(&wcex);
	}

	bool Window::Input(float fDeltaTime)
	{
		return SceneManager::GetInst()->Input(fDeltaTime);
	}

	bool Window::Update(float fDeltaTime)
	{
		if (CInput::GetInst()->IsKey(CInput::KEY_STATE::UP, DIK_ESCAPE))
		{
			bLockRotate ^= true;

			if (bLockRotate)
			{
				CursorEnable();
			}
			else
			{
				CursorDisable();
			}
		}

		float fTime = pTimer->GetElapsedTime();
#ifdef _DEBUG
		TCHAR strFPS[MAX_PATH] = {};

		swprintf_s(strFPS, TEXT("FPS: %d, Elapsed Time: %.4f"), static_cast<int>(pTimer->GetFPS()), fDeltaTime);

		SetWindowText(m_hWnd, strFPS);
#endif
		// fDeltaTime already comes from Timer::GetDeltTime which folds
		// the Stop() gate in directly (returns 0 while paused) — no
		// per-callsite `* !bStop` masking needed anymore.
		CInput::GetInst()->Update(fDeltaTime);

		ShaderManager::GetInst()->Update(fDeltaTime, fTime);

		ResourceManager::GetInst()->Update(fDeltaTime);

		return SceneManager::GetInst()->Update(fDeltaTime);
	}

	void Window::FixedUpdate(float fDeltaTime)
	{
		SceneManager::GetInst()->FixedUpdate(fDeltaTime);
	}

	bool Window::PostUpdate(float fDeltaTime)
	{
		return SceneManager::GetInst()->PostUpdate(fDeltaTime);
	}

	void Window::Collision(float fDeltaTime)
	{
		SceneManager::GetInst()->Collision(fDeltaTime);

		CollisionManager::GetInst()->Collision(fDeltaTime);

		CollisionManager::GetInst()->VisibleTest();
	}

	void Window::PreDraw(float fDeltaTime)
	{
		SceneManager::GetInst()->PreDraw(fDeltaTime);

		RenderManager::GetInst()->Update(fDeltaTime);

		RenderManager::GetInst()->PreRender();
	}

	void Window::Draw(float fDeltaTime)
	{
		Graphics::GetInst()->SetRenderTarget();

		Graphics::GetInst()->Clear(0.f, 0.f, 0.f);

		RenderManager::GetInst()->Render();

		SceneManager::GetInst()->Draw();

		if (m_PrePresentCb)
		{
			m_PrePresentCb();
		}

		Graphics::GetInst()->EndScene();
	}

	void Window::SetPrePresentCallback(std::function<void()> cb)
	{
		m_PrePresentCb = std::move(cb);
	}

	int Window::Run()
	{
		MSG msg = {};

		while (m_bRun)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Logic();
			}
		}

		return static_cast<int>(msg.wParam);
	}

	void Window::Logic()
	{
		pTimer->Update();

		float fDeltaTime = pTimer->GetDeltTime();

		bool bRanDraw = false;

		if (Input(fDeltaTime))
		{
			m_fFixedTime += fDeltaTime;

			while (m_fFixedTime >= FIXED_UPDATE_TIME)
			{
				FixedUpdate(FIXED_UPDATE_TIME);

				m_fFixedTime -= FIXED_UPDATE_TIME;
			}

			if (Update(fDeltaTime))
			{
				Collision(fDeltaTime);

				if (PostUpdate(fDeltaTime))
				{
					PreDraw(fDeltaTime);

					Draw(fDeltaTime);

					bRanDraw = true;
				}
			}
		}

		if (!bRanDraw)
		{
			// Scene short-circuited (e.g. transition). Still need to close the
			// ImGui frame and advance the swap chain so NewFrame stays balanced.
			Graphics::GetInst()->SetRenderTarget();
			Graphics::GetInst()->Clear(0.f, 0.f, 0.f);

			if (m_PrePresentCb)
			{
				m_PrePresentCb();
			}

			Graphics::GetInst()->EndScene();
		}
	}


	LRESULT __stdcall Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_MOUSEMOVE:
		{
			POINT pt;

			GetCursorPos(&pt);

			ScreenToClient(hWnd, &pt);

			bool bIn = pt.x >= 0 && pt.x <= m_pInst->m_iWidth &&
				pt.y >= 0 && pt.y <= m_pInst->m_iHeight;

			if (bIn == m_pInst->bCursorEnable && !m_pInst->bLockRotate)
			{
				m_pInst->CursorDisable();
			}
		}
		break;
		case WM_MOUSELEAVE:
			m_pInst->CursorEnable();
			break;
		case WM_ACTIVATE:
			if (wParam)
			{
				CInput::GetInst()->Enable();
			}
			else
			{
				CInput::GetInst()->Disable();
			}
			break;
		case WM_SIZE:
			// SIZE_MINIMIZED reports (0, 0) which would zero out every
			// anchor-driven UI rect — skip it. Real resize / restore /
			// maximize paths still notify.
			if (wParam != SIZE_MINIMIZED && m_pInst)
			{
				const int iW = LOWORD(lParam);
				const int iH = HIWORD(lParam);
				if (iW > 0 && iH > 0 && (iW != m_pInst->m_iWidth || iH != m_pInst->m_iHeight))
				{
					m_pInst->m_iWidth = iW;
					m_pInst->m_iHeight = iH;
					// Copy first so a listener that unregisters itself
					// inside the callback doesn't invalidate the iterator.
					auto callbacks = m_pInst->m_mapResizeCallbacks;
					for (auto& kv : callbacks)
					{
						if (kv.second) kv.second(iW, iH);
					}
				}
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			Window::GetInst()->m_bRun = false;
			return wParam;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	void Window::Pick(Collider* pSrc, Collider* pDest, float fDeltaTime)
	{
		MessageBox(0, nullptr, nullptr, MB_OK);
	}
}