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
#include "../Bindable/TransformBuffer.h"
#include "../Bindable/Camera.h"
#include "../Bindable/PointLight.h"
#include "../Bindable/Sampler.h"
#include "../Shader/ShaderManager.h"
#include "../Bindable/Topology.h"
#include "../Bindable/BindableManager.h"
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
#include "../Bindable/ComputeCBuffer.h"

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
		, bStop(false)
		, m_iWidth(0)
		, m_iHeight(0)
		, bCursorEnable(false)
		, bLockRotate(false)
	{
#ifdef _DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	}

	Window::~Window()
	{
		Engine::BindableManager<class Engine::RasterizerState>::DestroyInst();
		Engine::BindableManager<class Engine::DepthStencilState>::DestroyInst();
		Engine::BindableManager<class Engine::BlendState>::DestroyInst();
		Engine::BindableManager<class Engine::VertexShader>::DestroyInst();
		Engine::BindableManager<class Engine::HullShader>::DestroyInst();
		Engine::BindableManager<class Engine::DomainShader>::DestroyInst();
		Engine::BindableManager<class Engine::PixelShader>::DestroyInst();
		Engine::BindableManager<class Engine::ComputeShader>::DestroyInst();
		Engine::BindableManager<class Engine::Sampler>::DestroyInst();
		Engine::BindableManager<class Engine::Topology>::DestroyInst();
		Engine::BindableManager<class Engine::InputLayout>::DestroyInst();
		Engine::BindableManager<class Engine::Material>::DestroyInst();
		Engine::BindableManager<class Engine::Mesh>::DestroyInst();
		Engine::BindableManager<class Engine::TransformBuffer>::DestroyInst();
		Engine::BindableManager<class Engine::Texture>::DestroyInst();
		Engine::BindableManager<class Engine::VertexCBuffer<struct Engine::_tagTransformBuffer>>::DestroyInst();
		Engine::BindableManager<class Engine::VertexCBuffer<struct Engine::_tagBoneCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::VertexCBuffer<struct Engine::_tagTerrainCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::VertexCBuffer<struct Engine::_tagPointLight> >::DestroyInst();
		Engine::BindableManager<class Engine::VertexCBuffer<struct Engine::_tagMaterial> >::DestroyInst();
		Engine::BindableManager<class Engine::DomainCBuffer<struct Engine::_tagTransformBuffer>>::DestroyInst();
		Engine::BindableManager<class Engine::PixelCBuffer<struct Engine::_tagPerspectiveBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::PixelCBuffer<struct Engine::_tagColor> >::DestroyInst();
		Engine::BindableManager<class Engine::PixelCBuffer<struct Engine::_tagMaterial> >::DestroyInst();
		Engine::BindableManager<class Engine::PixelCBuffer<struct Engine::_tagTerrainCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::PixelCBuffer<struct Engine::_tagPointLight> >::DestroyInst();
		Engine::BindableManager<class Engine::ComputeCBuffer<struct Engine::_tagBoneCBuffer> >::DestroyInst();
		Engine::BindableManager<class Engine::ComputeCBuffer<struct Engine::_tagIKCBuffer> >::DestroyInst();

		ThreadManager::DestroyInst();

		ResourceManager::DestroyInst();

		CollisionManager::DestroyInst();

		Scene::Clear();

		SceneManager::DestroyInst();

		RenderManager::DestroyInst();

		ShaderManager::DestroyInst();

		CInput::DestroyInst();

		Graphics::DestroyInst();

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

	void Window::Stop()
	{
		bStop = true;
	}

	void Window::Resume()
	{
		bStop = false;
	}

	void Window::CursorEnable()
	{
		while (ShowCursor(TRUE) < 0);

		ClipCursor(nullptr);

		m_pInst->bCursorEnable = true;
	}

	void Window::CursorDisable()
	{
		while (ShowCursor(FALSE) >= 0);

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

		// 경로 관리자 초기화
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

		// 입력 초기화
		if (!CInput::GetInst()->Init(hInst, m_hWnd))
		{
			return false;
		}

		// 셰이더 관리자 초기화
		if (!ShaderManager::GetInst()->Init())
		{
			return false;
		}

		BindableManager<Sampler>::GetInst();

		if (!RenderManager::GetInst()->Init())
		{
			return false;
		}

		StaticCreateBindable<VertexCBuffer<POINTLIGHT>>("PointLight", 1);

		StaticCreateBindable<PixelCBuffer<POINTLIGHT>>("PointLight", 1);

		CInput::GetInst()->AddKey(DIK_ESCAPE);

		Scene* pCurrentScene = SceneManager::GetInst()->GetScene(SCENE_TYPE::CURRENT);

		std::vector<const TCHAR*> vecTexture =
		{
			TEXT("LandScape\\Terrain_Cliff_15_Large.dds"),
			TEXT("LandScape\\BD_Terrain_Cliff05.dds"),
		};

		std::vector<const TCHAR*> vecNormalTexture =
		{
			TEXT("LandScape\\Terrain_Cliff_15_Large_NRM.bmp"),
			TEXT("LandScape\\BD_Terrain_Cliff05_NRM.bmp"),
		};

		std::vector<const TCHAR*> vecSpecularTexture =
		{
			TEXT("LandScape\\Terrain_Cliff_15_Large_SPEC.bmp"),
			TEXT("LandScape\\BD_Terrain_Cliff05_SPEC.bmp"),
		};

		std::vector<const TCHAR*> vecBlendTexture =
		{
			TEXT("LandScape\\baseAlpha.bmp"),
			TEXT("LandScape\\RoadAlpha.bmp"),
		};

		std::shared_ptr<Terrain> pTerrain = pCurrentScene->CreateDrawable<Terrain>("Terrain", pCurrentScene->FindLayer(DEFAULT_LAYER));

		pTerrain->CreateTerrainTexture(vecTexture);
		pTerrain->CreateTerrainNormalTexture(vecNormalTexture);
		pTerrain->CreateTerrainSpecularTexture(vecSpecularTexture);
		pTerrain->CreateBlendTerrainTexture(vecBlendTexture);
		pTerrain->CreateHeightMap(TEXT("LandScape\\height2.bmp"));

#ifdef _DEBUG
		const std::shared_ptr<Drawable>& pLine = pCurrentScene->CreateProtoType<Drawable>("Line", SCENE_TYPE::CURRENT);

		pLine->NotUseInstance();
		pLine->FindAndAddBind<VertexShader>("anisotropic_microfacet VS");
		pLine->FindAndAddBind<PixelShader>("DebugPS");
		pLine->FindAndAddBind<Topology>("LineStrip");
		pLine->FindAndAddBind<DepthStencilState>("DepthAlways");
		pLine->FindAndAddBind<InputLayout>("TPNT");
		pLine->FindAndAddBind<Material>("Brick");
#endif

		return true;
	}

	bool Window::Create(const TCHAR* pTitle, const TCHAR* pClass, HINSTANCE hInst, int iWidth, int iHeight)
	{
		RECT rc = { 0,0,iWidth,iHeight };
		DWORD iStyle = WS_OVERLAPPEDWINDOW;

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

		TCHAR strFPS[MAX_PATH] = {};

		swprintf_s(strFPS, TEXT("Google SoftwareEngineer FPS: %d, Elapsed Time: %.4f"), static_cast<int>(pTimer->GetFPS()), fDeltaTime);

		SetWindowText(m_hWnd, strFPS);

		CInput::GetInst()->Update(fDeltaTime * !bStop);

		Graphics::GetInst()->Update(fDeltaTime);

		return SceneManager::GetInst()->Update(fDeltaTime * !bStop);
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
		Graphics::GetInst()->Clear(0.f, 0.f, 0.f);

		RenderManager::GetInst()->Render();

		SceneManager::GetInst()->Draw();
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

		if (!Input(fDeltaTime))
		{
			return;
		}

		if (!Update(fDeltaTime))
		{
			return;
		}

		Collision(fDeltaTime);

		PreDraw(fDeltaTime);

		Draw(fDeltaTime);
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