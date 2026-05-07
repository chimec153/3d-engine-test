#pragma once
#include "../Component/Component.h"
namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    // Phase E5 — PaperBurn migrated from Bindable to Component. The owner
    // (e.g. Attackable) is responsible for calling Bind() during its own
    // Bind path (Component has no virtual Bind interface).
    class ENGINE_DLL PaperBurn :
        public Component
    {
    public:
        enum class PAPER_BURN_STAGE
        {
            READY,
            START,
            MID,
            FINAL,
            OUT_STAGE,
            END
        };
    public:
        PaperBurn();
        PaperBurn(std::shared_ptr<class Texture> pTexture);
        PaperBurn(const PaperBurn& paper);
        virtual ~PaperBurn() override = default;

    private:
        std::shared_ptr<class Texture>    m_pPaperBurnTexture;
        PAPERBURNCBUFFER    m_tCBuffer;
        std::shared_ptr<ConstantBuffer<PAPERBURNCBUFFER>>   m_pCBuffer;
        bool m_bStart;
        std::vector<std::function<void(float)>> m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::END)];
        bool m_bCalled[static_cast<int>(PAPER_BURN_STAGE::END)];

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
        template <typename T>
        void AddCallBack(PAPER_BURN_STAGE eStage, T* pObj, void(T::* pFunc)(float))
        {
            m_vecCallBack[static_cast<int>(eStage)].push_back(std::bind(pFunc, pObj, std::placeholders::_1));
        }
        void AddCallBack(PAPER_BURN_STAGE eStage, std::function<void(float)> pFunc);
        void AddCallBack(PAPER_BURN_STAGE eStage, void(*pFunc)(float));

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Called explicitly by the owning Drawable's render path (legacy)
        // or via RenderBind() below for GameObject-hosted MeshRenderers.
        void Bind();

        // Phase E5 — RenderBind override forwards to the existing Bind so
        // GameObject-hosted Attackable/Player/etc. get paper-burn applied
        // automatically during their MeshRenderer pass (between Transform
        // bind and Mesh::Draw).
        virtual void RenderBind() override { Bind(); }

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}
