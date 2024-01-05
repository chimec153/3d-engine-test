#include "Notify.h"

Engine::Notify::Notify()	:
	m_iFrame(-1)
	, m_fTime(-1.f)
	, m_pFunc(nullptr)
	, m_pOwner(nullptr)
	, m_bCalled(false)
{
}

void Engine::Notify::SetFrame(int iFrame)
{
	m_iFrame = iFrame;
}

void Engine::Notify::SetTime(float fTime)
{
	m_fTime = fTime;
}

void Engine::Notify::SetTag(const std::string& strTag)
{
	m_strTag = strTag;
}

void Engine::Notify::SetCallBack(std::function<void(int, float, Bindable*)> pFunc)
{
	m_pFunc = pFunc;
}

void Engine::Notify::SetCallBack(void(*pFunc)(int, float, Bindable*))
{
	m_pFunc = std::bind(pFunc, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void Engine::Notify::SetOwner(Bindable* pOwner)
{
	m_pOwner = pOwner;
}

const std::string& Engine::Notify::GetTag() const
{
	return m_strTag;
}

void Engine::Notify::Clear()
{
	m_bCalled = false;
}

void Engine::Notify::Update(float fTime, int iFrame)
{
	if (m_bCalled)
	{
		return;
	}

	if ((m_fTime != -1 && m_fTime <= fTime) ||
		(m_iFrame != -1 && m_iFrame <= iFrame))
	{
		m_pFunc(iFrame, fTime, m_pOwner);

		m_bCalled = true;
	}
}
