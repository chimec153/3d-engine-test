#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Decal :
        public Drawable
    {
    public:
        Decal();
        virtual ~Decal() override = default;
    private:
        DECALCBUFFER m_tCBuffer;
        std::shared_ptr<ConstantBuffer<DECALCBUFFER>>   m_pCBuffer;

    public:
        void SetMaxFadeTime(float fMax);
        void SetFadeStartTime(float fStart);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
    };
}