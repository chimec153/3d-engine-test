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
        bool m_bFadeStart;

    public:
        void SetMaxFadeTime(float fMax);
        void SetFadeStartTime(float fStart);
        void StartFade();

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}