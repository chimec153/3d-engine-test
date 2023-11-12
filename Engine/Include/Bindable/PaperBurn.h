#pragma once
#include "Bindable.h"
namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    class ENGINE_DLL PaperBurn :
        public Bindable
    {
    public:
        PaperBurn(std::shared_ptr<class Texture> pTexture);
        PaperBurn(const PaperBurn& paper);
        virtual ~PaperBurn() override = default;

    private:
        std::shared_ptr<class Texture>    m_pPaperBurnTexture;
        PAPERBURNCBUFFER    m_tCBuffer;
        std::shared_ptr<ConstantBuffer<PAPERBURNCBUFFER>>   m_pCBuffer;
        bool m_bStart;

    public:
        void SetPaperBurnTexture(std::shared_ptr<Texture> pTexture);
        void SetMaxTime(float fMax);
        void StartPaperBurn();
        void SetStartColor(const Vector4& vColor);
        void SetMidColor(const Vector4& vColor);
        void SetFinalColor(const Vector4& vColor);
        void SetStartRate(float fRate);
        void SetMidRate(float fRate);
        void SetFinalRate(float fRate);
        void SetEndRate(float fRate);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}