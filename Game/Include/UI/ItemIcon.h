#pragma once
#include "UI/Image.h"
#include "../GameDefs.h"

namespace Client
{
    class ItemIcon :
        public Engine::Image
    {
    public:
        ItemIcon(const std::string& strTexture);
        virtual ~ItemIcon() override = default;

    private:
        bool m_bDrag;
        std::weak_ptr<Engine::UIControl> m_pOwner;

    public:
        void SetOwner(std::weak_ptr<Engine::UIControl> pOwner);
        bool IsDrag()   const
        {
            return m_bDrag;
        }

    public:
        virtual bool Init() override; 
        virtual void Update(float fDeltaTime) override;

    public:
        void CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };
}

