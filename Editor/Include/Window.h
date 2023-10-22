#pragma once
#include "Editor.h"
class Window
{
public:
	Window();
	~Window() = default;
public:
	bool Init();
	int Run();
	void Logic();

public:
	static LRESULT __stdcall WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
};
