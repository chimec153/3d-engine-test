#pragma once
#include "UIControl.h"
namespace Engine
{
    // Phase E5 — Frame is a UIControl-derived Component shell. The
    // 9-slice tiling logic in the previous Drawable-era Bind() relied
    // on Drawable methods (BindChild / GetTransform / GetTextures) and
    // has been stripped for the shell migration.
    class ENGINE_DLL Frame :
        public UIControl
    {
    public:
        Frame();
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
        virtual std::shared_ptr<Component> Clone() override;
    };
}
