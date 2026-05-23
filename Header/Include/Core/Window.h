#pragma once

#include "Macro.h"
#include "Graphics.h"
#include "Timer.h"
#include <functional>

namespace Engine
{
	class ENGINE_DLL Window
	{
	private:
		static Window* m_pInst;

	public:
		static Window* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new Window;
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

	public:
		Window();
		~Window();

	private:
		HWND m_hWnd;
		HINSTANCE m_hInst;
		bool m_bRun;
		std::shared_ptr<Timer> pTimer;
		// bStop moved to Timer — game-time pause is owned by the same
		// object that produces deltas. Use Window::GetInst()->GetTimer()
		// ->Stop() / Resume() from callers.
		int m_iWidth;
		int m_iHeight;
		bool bCursorEnable;
		bool bLockRotate;
		float m_fFixedTime;
		std::function<void()> m_PrePresentCb;

	public:
		void SetPrePresentCallback(std::function<void()> cb);
		std::shared_ptr<Timer> GetTimer()	const;
		void CursorEnable();
		void CursorDisable();
		bool IsLockRotation()	const;
		int GetWidth()	const;
		int GetHeight()	const;
		bool IsCursorEnabled()	const;
		bool IsRun()	const;
		HWND GetWinHandle()	const;
		void StopRunning();
	public:
		bool Init(const TCHAR* pTitle, const TCHAR* pClass, HINSTANCE hInst, WNDPROC proc, int iWidth = 1280, int iHeight = 720);
		int Run();
		void Logic();

	private:
		bool Create(const TCHAR* pTitle, const TCHAR* pClass, HINSTANCE hInst, int iWidth = 1280, int iHeight = 720);
		int Register(const TCHAR* pClass, HINSTANCE hInst, WNDPROC proc);
		bool Input(float fDeltaTime);
		bool Update(float fDeltaTime);
		void FixedUpdate(float fDeltaTime);
		void Collision(float fDeltaTime);
		bool PostUpdate(float fDeltaTime);
		void PreDraw(float fDeltaTime);
		void Draw(float fDeltaTime);

	public:
		static LRESULT __stdcall WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static void Pick(class Collider* pSrc, class Collider* pDest, float fDeltaTime);
	};

}