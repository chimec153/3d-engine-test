#pragma once

#include "../Types.h"

namespace Engine
{
	class ENGINE_DLL Thread
	{
		friend class ThreadManager;

	protected:
		Thread();
	public:
		virtual ~Thread();

	private:
		HANDLE m_hThread;
		unsigned m_iThreadID;
		HANDLE m_hEvent;
		bool m_bLoop;

	public:
		bool Init();
		void Start();
		bool IsFinish();
		virtual void Run() = 0;
		void Reset();
		void Loop();
		void DestroyEvent();
		void DestroyThread();

	public:
		static unsigned __stdcall ThreadFunc(void*);
	};

}