#pragma once
#include "Component/Component.h"
#include <memory>

namespace Engine
{
    class Transform;
    class Mesh;
    class Material;
    class VertexShader;
    class PixelShader;
    class InputLayout;
    class Topology;
    class DepthStencilState;
    class VoxelWorld;
}

namespace Client
{
    // Drives tower placement. Press 1 to toggle placement mode: a translucent
    // ghost of the tower follows the cursor, snapped to the voxel grid cell
    // under the mouse; left-click drops a real Tower on that cell and exits.
    //
    // The ghost can't go through the normal mesh path — RenderManager's alpha
    // pass only runs particles + custom callbacks (mesh buckets are opaque/
    // deferred only). So RenderGhost() hand-binds the cube through the forward
    // alpha shader and is invoked from a RENDER_LAYER::ALPHA custom render
    // callback that GameScene registers each frame (same hook as Footsteps).
    class TowerPlacementController :
        public Engine::Component
    {
    public:
        TowerPlacementController();
        virtual ~TowerPlacementController() override;

        // Borrowed scene voxel world — used to reject wall cells and handed
        // to each spawned Tower so its bullets can reflect off walls.
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override { return nullptr; }

        // Draw the placement ghost. Call only from a RENDER_LAYER::ALPHA
        // custom render callback (AlphaBlend + a light cbuffer are already
        // bound there). No-op unless placement mode has a valid cell.
        void RenderGhost();

        // Enter attack-tower placement targeting a SPECIFIC reserve slot — the
        // on-screen tower HUD picks WHICH unplaced tower to deploy. Re-clicking
        // the already-selected slot cancels (mirrors the key toggle).
        // iReserveIndex < 0 = "next placeable". No-op while the world is frozen (a
        // modal is up) or without a placeable reserve. Suppresses the triggering
        // left-click from also committing this frame.
        void BeginAttackPlacement(int iReserveIndex);
        // Enter heal-tower placement (heal towers are fungible — the front-most
        // ready heal reserve is consumed at commit). Re-selecting cancels. Used by
        // the tower HUD when a heal slot is clicked.
        void BeginHealPlacement();

    private:
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;

        // Which tower the current placement drops (key 1 = attack, 2 = heal).
        enum class PlaceType { Attack, Heal };
        PlaceType m_ePlaceType = PlaceType::Attack;

        bool m_bPlacing = false;
        // Reserve slot the HUD chose to deploy; -1 = next placeable (key-1).
        // Read by the commit path via TowerManager::ConsumePlaceableSlotAt.
        int  m_iSelectedReserve = -1;
        // Tower type the attack ghost mesh currently reflects (-2 = none built
        // yet), so RefreshAttackGhostMesh only rebuilds on a type change.
        int  m_iGhostTowerId = -2;
        bool m_bHasCell = false;   // cursor is over a valid grid cell
        bool m_bValidCell = false; // ...and that cell is free to build on
        // Set when the tower button opens placement, so the same left-click that
        // pressed the button doesn't immediately commit a tower under it. Cleared
        // after one Update of the commit check.
        bool m_bIgnoreCommitClick = false;
        int  m_iCellX   = 0;
        int  m_iCellZ   = 0;

        // Shared ghost render resources (resolved once in Init).
        std::shared_ptr<Engine::Transform>    m_pGhostTr;     // standalone, NORMAL cam
        std::shared_ptr<Engine::Mesh>         m_pGhostMesh;     // attack tower (cube)
        std::shared_ptr<Engine::Mesh>         m_pGhostMeshHeal; // heal tower (cylinder)
        std::shared_ptr<Engine::Material>     m_pGhostMat;
        std::shared_ptr<Engine::VertexShader> m_pVS;
        std::shared_ptr<Engine::PixelShader>  m_pPS;
        std::shared_ptr<Engine::InputLayout>  m_pInputLayout;
        std::shared_ptr<Engine::Topology>     m_pTopology;
        // Depth states for the ghost draw: normal (test on, occluded by walls)
        // for a valid cell; test-off for an invalid cell so the red ghost
        // isn't hidden behind the identical opaque tower already on that cell.
        std::shared_ptr<Engine::DepthStencilState> m_pDepthTest;
        std::shared_ptr<Engine::DepthStencilState> m_pDepthNone;

        // Cast the cursor onto the floor plane and floor() to a cell. Returns
        // false for off-map or wall cells (no placement there).
        bool MouseToCell(int& cx, int& cz) const;
        // True if any tower (attack or heal) already occupies cell (cx,cz).
        bool IsCellOccupied(int cx, int cz) const;
        // Budget check for the current place type (placed < bought).
        bool HasBudgetFor(PlaceType eType) const;
        // Rebuild m_pGhostMesh to match the tower that will actually be deployed
        // (its type's N-gon prism), so the attack preview isn't a generic box.
        void RefreshAttackGhostMesh();
        // Number-key 1..kMaxTowers handler: deploy the tower in HUD slot iSlotIndex
        // (acquisition order, see BuildTowerSlots). No-op when that slot isn't a
        // deployable reserve (empty / placed / cooldown / weaponless).
        void BeginPlacementForSlot(int iSlotIndex);
    };
}
