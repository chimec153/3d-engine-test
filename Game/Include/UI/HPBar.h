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

    // HUD health bar — UIControl-derived Component. Owns no Render
    // method; the actual draw is delegated to two child UIRenderer
    // Components (background + fill), each pointed at a shared 4-vertex
    // "UIQuad" Mesh and the BG/Fill 1x1 textures. Update each frame
    // re-scales the fill's child Transform to the player's HP ratio.
    //
    // Lifecycle:
    //   Init           — find shaders / textures / mesh, build child
    //                    Transforms (CAMERA_TYPE::UI, no camera → NDC
    //                    space via Transform's UI fallback) and child
    //                    UIRenderers wired to them.
    //   Update         — Component base ticks children (UIRenderer
    //                    PreDraw self-registers with RenderManager's UI
    //                    layer); HPBar updates the fill Transform scale
    //                    from the target Player's HP.
    class GAME_DLL HPBar : public Engine::UIControl
    {
    public:
        HPBar();
        virtual ~HPBar() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        std::weak_ptr<Player> m_pTarget;

        // Child Components built in Init. Transforms hold per-quad
        // scale + position; UIRenderers do the draw against the shared
        // UIQuad Mesh and the BG/Fill textures.
        std::shared_ptr<Engine::Transform>  m_pTransformBG;
        std::shared_ptr<Engine::Transform>  m_pTransformFill;
        std::shared_ptr<Engine::UIRenderer> m_pRendererBG;
        std::shared_ptr<Engine::UIRenderer> m_pRendererFill;
    };
}
