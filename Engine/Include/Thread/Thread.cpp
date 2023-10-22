#include "Thread.h"
#include <thread>

namespace Engine
{
    Thread::Thread() :
        m_hThread(INVALID_HANDLE_VALUE)
        , m_iThreadID(0)
        , m_hEvent(INVALID_HANDLE_VALUE)
        , m_bLoop(false)
    {
    }

    Thread::~Thread()
    {
        m_bLoop = false;

        DestroyEvent();

        DestroyThread();
    }

    bool Thread::Init()
    {
        DestroyThread();

        m_hThread = (HANDLE)_beginthreadex(nullptr, 0, Thread::ThreadFunc, this, 0, &m_iThreadID);

        DestroyEvent();

        m_hEvent = CreateEvent(nullptr, false, false, nullptr);

        return true;
    }

    void Thread::Start()
    {
        SetEvent(m_hEvent);
    }

    bool Thread::IsFinish()
    {
        return !WaitForSingleObject(m_hThread, 0);
    }

    void Thread::Reset()
    {
        ResetEvent(m_hEvent);
    }

    void Thread::Loop()
    {
        m_bLoop = true;
    }

    void Thread::DestroyEvent()
    {
        if (m_hEvent != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hEvent);
            m_hEvent = INVALID_HANDLE_VALUE;
        }
    }

    void Thread::DestroyThread()
    {
        if (m_hThread != INVALID_HANDLE_VALUE)
        {
            WaitForSingleObject(m_hThread, INFINITE);
            CloseHandle(m_hThread);
            m_hThread = INVALID_HANDLE_VALUE;
        }
    }

    unsigned __stdcall Thread::ThreadFunc(void* pThread)
    {
        Thread* _pThread = static_cast<Thread*>(pThread);

        WaitForSingleObject(_pThread->m_hEvent, INFINITE);

        do
        {
            _pThread->Run();
        } while (_pThread->m_bLoop);

        return 0;
    }
}