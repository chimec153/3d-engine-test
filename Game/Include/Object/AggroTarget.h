#pragma once
#include "Component/Component.h"
#include <memory>

namespace Client
{
    // Marks a GameObject as something enemies can target, carrying an aggro
    // weight. Enemies path to and attack the single highest-aggro active
    // target (shared FlowField → one global goal). The Player and each Tower
    // attach one; towers use a higher weight so they soak hits first.
    class AggroTarget : public Engine::Component
    {
    public:
        explicit AggroTarget(int iAggro = 1) : m_iAggro(iAggro)
        {
            SetComponentType(Engine::COMPONENT_TYPE::NONE);
        }

        int  GetAggro() const { return m_iAggro; }
        void SetAggro(int iAggro) { m_iAggro = iAggro; }

        virtual std::shared_ptr<Engine::Component> Clone() override
        {
            return std::make_shared<AggroTarget>(*this);
        }

    private:
        int m_iAggro = 1;
    };
}
