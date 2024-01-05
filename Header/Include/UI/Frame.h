#pragma once
#include "UIControl.h"
namespace Engine
{
    class ENGINE_DLL Frame :
        public UIControl
    {
    public:
        Frame(const std::string& strTexture);
        virtual ~Frame() override = default;

    private:
        int m_iXStart;
        int m_iXEnd;
        int m_iYStart;
        int m_iYEnd;

    public:
        void SetXStart(int fX);
        void SetXEnd(int fX);
        void SetYStart(int fY);
        void SetYEnd(int fY);

    public:
        virtual void Bind() override;
    };
}