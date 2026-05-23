#pragma once

#include "UI/UIControl.h"
#include "../GameDefs.h"
#include <memory>

namespace Engine
{
    class Transform;
    class UIRenderer;
}

namespace Client
{
    class Player;

    // HUD experience gauge — mirror of HPBar (UIControl-derived
    // Component with two child UIRenderers). Lives just above the HP
    // bar in the bottom-left, slim, yellow fill on grey track.
    //
    // Reads Player::GetExp / GetXpToNext each Update; the fill quad's
    // child Transform is rescaled to the current XP ratio. Like HPBar
    // it has no Render method — UIRenderer children self-register
    // with RenderManager via their own PreDraw.
    class GAME_DLL XPBar : public Engine::UIControl
    {
    public:
        XPBar();
        virtual ~XPBar() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        std::weak_ptr<Player> m_pTarget;

        std::shared_ptr<Engine::Transform>  m_pTransformBG;
        std::shared_ptr<Engine::Transform>  m_pTransformFill;
        std::shared_ptr<Engine::UIRenderer> m_pRendererBG;
        std::shared_ptr<Engine::UIRenderer> m_pRendererFill;
    };
}
