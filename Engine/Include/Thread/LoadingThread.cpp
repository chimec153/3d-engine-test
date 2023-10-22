#include "LoadingThread.h"
#include "../Core/Window.h"
#include "../Bindable/Drawable.h"

namespace Engine
{
	LoadingThread::LoadingThread() :
		m_strFullPath()
		, m_pDrawable(nullptr)
	{
	}

	LoadingThread::~LoadingThread()
	{
	}

	std::shared_ptr<class Drawable> LoadingThread::GetDrawable() const
	{
		return m_pDrawable;
	}

	void LoadingThread::SetFullPath(const TCHAR* pFullPath)
	{
		_tcscpy_s(m_strFullPath, pFullPath);
	}

	void LoadingThread::SetDrawable(std::shared_ptr<class Drawable> pDrawable)
	{
		m_pDrawable = pDrawable;
	}

	void LoadingThread::Run()
	{
		m_pDrawable = std::make_shared<Drawable>();

		m_pDrawable->Load(m_strFullPath, "");
	}
}