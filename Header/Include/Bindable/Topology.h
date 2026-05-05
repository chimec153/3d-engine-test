#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL Topology :
        public Bindable
    {
        friend class BindableManager<Topology>;
    public:
        Topology(D3D_PRIMITIVE_TOPOLOGY topology);
        virtual ~Topology() noexcept override;

    private:
        D3D_PRIMITIVE_TOPOLOGY m_eTopology;
        D3D_PRIMITIVE_TOPOLOGY m_ePrevTopology;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
        void GetAndBind();
        void BindEnd();

        // Sort-by-state cache — see VertexShader::ResetBoundCache.
        static void ResetBoundCache();

    private:
        static D3D_PRIMITIVE_TOPOLOGY s_eBound;
    };

}