#pragma once
#include "Thread.h"
namespace Engine
{
    class ENGINE_DLL LoadingThread :
        public Thread
    {
    public:
        LoadingThread();
        virtual ~LoadingThread() override;

    private:
        TCHAR m_strFullPath[MAX_PATH];
        std::shared_ptr<class Drawable> m_pDrawable;

    public:
        std::shared_ptr<class Drawable> GetDrawable()   const;
        void SetFullPath(const TCHAR* pFullPath);
        void SetDrawable(std::shared_ptr<class Drawable> pDrawable);

    public:
        virtual void Run() override;
    };

}