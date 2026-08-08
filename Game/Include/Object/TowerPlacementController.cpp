#include "TowerPlacementController.h"
#include "Tower.h"
#include "HealTower.h"
#include "TowerManager.h"
#include "TowerData.h"
#include "TowerSlots.h"
#include "GameStateManager.h"
#include "../GameDefs.h"
#include "../Scene/GameWorldBuilder.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Input/Input.h"
#include "Bindable/Camera.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Material.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/DepthStencilState.h"
#include "Bindable/BindableManager.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Types.h"
#include <cmath>

namespace Client
{
    TowerPlacementController::TowerPlacementController()  = default;
    TowerPlacementController::~TowerPlacementController() = default;

    bool TowerPlacementController::Init()
    {
        if (!__super::Init())
            return false;

        // Number keys 1..5 each deploy the tower in the matching tower-HUD slot.
        Engine::CInput::GetInst()->AddKey(DIK_1);
        Engine::CInput::GetInst()->AddKey(DIK_2);
        Engine::CInput::GetInst()->AddKey(DIK_3);
        Engine::CInput::GetInst()->AddKey(DIK_4);
        Engine::CInput::GetInst()->AddKey(DIK_5);

        // Shared ghost resources. Forward alpha PS (AlphaNoUVNoShadowPS) is
        // registered unconditionally and outputs the material diffuse with
        // its .w as alpha — exactly the ghost look we want.
        m_pVS          = Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS);
        m_pPS          = Engine::StaticFindBindable<Engine::PixelShader> ("AlphaNoUVNoShadowPS");
        m_pInputLayout = Engine::StaticFindBindable<Engine::InputLayout> ("Standard");
        m_pTopology    = Engine::StaticFindBindable<Engine::Topology>    ("TriangleList");
        // "NoDepthWrite" = depth test on, no write (normal alpha occlusion).
        // "NoDepth" = depth test off (draw on top — used for the red/blocked
        // ghost so it shows over the identical tower already on that cell).
        m_pDepthTest   = Engine::StaticFindBindable<Engine::DepthStencilState>("NoDepthWrite");
        m_pDepthNone   = Engine::StaticFindBindable<Engine::DepthStencilState>("NoDepth");

        // Preview meshes — the attack ghost is rebuilt per selected tower TYPE
        // (RefreshAttackGhostMesh) so it matches the real tower's N-gon prism;
        // seed it with the default-type body. The heal ghost is the heal
        // cylinder (matches the real heal tower).
        m_pGhostMesh     = Tower::BuildBodyMesh(nullptr);
        m_pGhostMeshHeal = HealTower::BuildCylinderMesh();

        if (auto pSrc = Engine::StaticFindBindable<Engine::Material>("Material"))
            m_pGhostMat = std::static_pointer_cast<Engine::Material>(pSrc->Clone());
        else
            m_pGhostMat = std::make_shared<Engine::Material>();
        if (m_pGhostMat)
        {
            // Translucent cyan-blue. Emissive keeps it readable regardless of
            // scene lighting so it always looks like a hologram.
            m_pGhostMat->SetDiffuseColor (0.3f, 0.6f, 1.0f, 0.4f);
            m_pGhostMat->SetEmissiveColor({ 0.2f, 0.4f, 0.7f, 1.f });
            m_pGhostMat->SetTag("TowerGhostMat");
        }

        // Standalone transform (not a child component); NORMAL camera type so
        // Bind() uploads world*VP, matching the FootstepManager ghost path.
        m_pGhostTr = std::make_shared<Engine::Transform>();
        m_pGhostTr->SetCameraType(Engine::CAMERA_TYPE::NORMAL);

        return true;
    }

    void TowerPlacementController::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        // Placement is a play-phase action: during a modal (intermission /
        // level-up / pause) the world is frozen and clicks belong to the UI,
        // so ignore the toggle key and hide the ghost.
        if (!GameStateManager::GetInst().IsPlaying())
        {
            m_bHasCell = false;
            return;
        }

        auto* pInput = Engine::CInput::GetInst();

        // Number keys 1..kMaxTowers deploy the tower in the matching tower-HUD
        // slot (slots are in acquisition order — see BuildTowerSlots). Re-pressing
        // the key of the tower already selected cancels (Begin*Placement toggles).
        static const int kSlotKeys[] = { DIK_1, DIK_2, DIK_3, DIK_4, DIK_5 };
        for (int k = 0; k < static_cast<int>(sizeof(kSlotKeys) / sizeof(kSlotKeys[0])); ++k)
            if (pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, kSlotKeys[k]))
                BeginPlacementForSlot(k);

        if (!m_bPlacing)
            return;

        // Keep the attack ghost shaped like the tower that will actually drop.
        if (m_ePlaceType == PlaceType::Attack)
            RefreshAttackGhostMesh();

        // Right-click cancels the in-progress placement without building (the
        // 1/2 toggle still works too). Left-click below commits; this is the
        // standard "right-click to back out" of the build cursor.
        if (pInput->IsMouseButtonDown(Engine::CInput::MOUSE_TYPE::RIGHT))
        {
            m_bPlacing = false;
            m_bHasCell = false;
            return;
        }

        m_bHasCell = MouseToCell(m_iCellX, m_iCellZ);
        // A cell is buildable only if no tower already occupies it (the ghost
        // turns red on occupied cells; walls/off-map already hide the ghost).
        m_bValidCell = m_bHasCell && !IsCellOccupied(m_iCellX, m_iCellZ);

        if (m_bHasCell && m_bValidCell && !m_bIgnoreCommitClick &&
            pInput->IsMouseButtonDown(Engine::CInput::MOUSE_TYPE::LEFT))
        {
            auto* pOwner = GetGameObjectOwner();
            Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
            std::shared_ptr<Engine::Layer> pLayer =
                pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
            if (pScene && pLayer)
            {
                if (m_ePlaceType == PlaceType::Heal)
                {
                    if (TowerManager::GetInst().PlaceableHealCount() > 0)
                    {
                        if (auto pHeal = pScene->CreateGameObject<HealTower>("HealTower", pLayer))
                        {
                            pHeal->SetCell(m_iCellX, m_iCellZ);
                            // Consume a ready heal reserve and carry its seq + level
                            // so the placed heal tower keeps its slot and merged level.
                            const auto hs = TowerManager::GetInst().ConsumePlaceableHeal();
                            pHeal->SetSlotSeq(hs.iSeq);
                            pHeal->SetLevel(hs.iLevel);
                        }
                    }
                }
                // A tower can only be DEPLOYED with a weapon equipped. A freshly
                // bought tower starts weaponless and must be equipped in the shop
                // first; PlaceableTowerCount counts only weaponed (non-down) slots.
                else if (TowerManager::GetInst().PlaceableTowerCount() > 0)
                {
                    if (auto pTower = pScene->CreateGameObject<Tower>("Tower", pLayer))
                    {
                        pTower->SetVoxelWorld(m_pVoxelWorld);
                        pTower->SetCell(m_iCellX, m_iCellZ);
                        // Consume the reserve slot the HUD selected (or the next
                        // placeable when key-1 / generic): it carries the tower's
                        // weapon AND level (a re-placed destroyed tower keeps both;
                        // a freshly bought one is level 1).
                        auto slot = TowerManager::GetInst()
                            .ConsumePlaceableSlotAt(m_iSelectedReserve);
                        // Apply the bought tower TYPE first (seeds per-level
                        // deltas + base HP) so SetLevel's HP bonus is correct.
                        pTower->SetTowerDefId(slot.iTowerId);
                        pTower->SetLevel(slot.iLevel);   // tower level
                        pTower->SetSlotSeq(slot.iSeq);   // numbered HUD/key slot
                        // Hand over the weapon OBJECT (carries its own level).
                        pTower->SetWeapon(slot.pWeapon);
                    }
                }
            }
            // One tower per placement: exit the mode after dropping it.
            m_bPlacing = false;
            m_bHasCell = false;
            m_iSelectedReserve = -1;
        }

        // The button-press click is consumed for exactly this frame's commit
        // check; allow normal left-click commits from the next frame on.
        m_bIgnoreCommitClick = false;
    }

    void TowerPlacementController::BeginAttackPlacement(int iReserveIndex)
    {
        // Mirror the key-1 DOWN-edge logic, including the play-phase gate.
        if (!GameStateManager::GetInst().IsPlaying()) return;
        // Re-clicking the tower that's already selected toggles placement off.
        if (m_bPlacing && m_ePlaceType == PlaceType::Attack &&
            m_iSelectedReserve == iReserveIndex)
        {
            m_bPlacing = false;
        }
        else if (HasBudgetFor(PlaceType::Attack))
        {
            m_bPlacing         = true;
            m_ePlaceType       = PlaceType::Attack;
            m_iSelectedReserve = iReserveIndex;   // which reserve slot to deploy
            // Don't let the click that pressed the button also drop a tower
            // at the cell under the button this same frame.
            m_bIgnoreCommitClick = true;
        }
        m_bHasCell = false;
    }

    void TowerPlacementController::BeginHealPlacement()
    {
        if (!GameStateManager::GetInst().IsPlaying()) return;
        // Re-selecting heal placement cancels (heal towers are fungible, so there
        // is no per-slot selection to switch between).
        if (m_bPlacing && m_ePlaceType == PlaceType::Heal)
        {
            m_bPlacing = false;
        }
        else if (HasBudgetFor(PlaceType::Heal))
        {
            m_bPlacing   = true;
            m_ePlaceType = PlaceType::Heal;
            m_bIgnoreCommitClick = true;
        }
        m_bHasCell = false;
    }

    void TowerPlacementController::BeginPlacementForSlot(int iSlotIndex)
    {
        if (!GameStateManager::GetInst().IsPlaying()) return;
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        const std::vector<TowerSlotView> slots = BuildTowerSlots(pLayer.get());
        if (iSlotIndex < 0 || iSlotIndex >= static_cast<int>(slots.size())) return;
        const TowerSlotView& s = slots[iSlotIndex];
        if (!s.Deployable()) return;   // placed / cooldown / weaponless → no-op
        if (s.bHeal) BeginHealPlacement();
        else         BeginAttackPlacement(s.iReserveIdx);
    }

    void TowerPlacementController::RefreshAttackGhostMesh()
    {
        // The id ConsumePlaceableSlotAt(m_iSelectedReserve) would deploy. Resolve
        // it to the same TowerDef SetTowerDefId will use, then build the ghost
        // with the shared Tower::BuildBodyMesh so preview == placed tower.
        const int iTowerId = TowerManager::GetInst().PeekPlaceableTowerId(m_iSelectedReserve);
        if (iTowerId == m_iGhostTowerId) return;   // unchanged — keep cached mesh
        m_iGhostTowerId = iTowerId;
        const TowerDef* pDef = (iTowerId >= 0)
            ? TowerDatabase::GetInst().Get(iTowerId)
            : TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
        m_pGhostMesh = Tower::BuildBodyMesh(pDef);
    }

    bool TowerPlacementController::MouseToCell(int& cx, int& cz) const
    {
        const std::shared_ptr<Engine::Camera>& pCamera =
            Engine::Graphics::GetInst()->GetCamera();
        if (!pCamera) return false;

        auto* pInput = Engine::CInput::GetInst();
        const Engine::Vector2 vMouseScreen{
            static_cast<float>(pInput->GetMouseX()),
            static_cast<float>(pInput->GetMouseY()) };

        // Same unprojection Player::ComputeAimYaw uses; the Y flip matches the
        // ScreenPosToClipPos / CameraPosToWorldPos convention there.
        const Engine::Vector3 vClip  = pCamera->ScreenPosToClipPos(vMouseScreen);
        const Engine::Vector3 vWorld = pCamera->CameraPosToWorldPos({ vClip.x, -vClip.y });
        const Engine::Vector3 vCamPos = pCamera->GetTransform()->GetPosition();
        const Engine::Vector3 vRayDir = vWorld - vCamPos;

        // Intersect the floor-top plane (y = kWallY) where towers sit.
        const float planeY = static_cast<float>(kWallY);
        if (std::abs(vRayDir.y) <= 1e-4f) return false;
        const float t = (planeY - vCamPos.y) / vRayDir.y;
        if (t <= 0.f) return false;

        const float hx = vCamPos.x + vRayDir.x * t;
        const float hz = vCamPos.z + vRayDir.z * t;
        const int ix = static_cast<int>(std::floor(hx));
        const int iz = static_cast<int>(std::floor(hz));

        // Reject the perimeter wall ring and anything outside the floor.
        const int iMax = GameWorldBuilder::kFloorSize;
        if (ix < 1 || ix > iMax - 2 || iz < 1 || iz > iMax - 2) return false;
        // Reject occupied wall cells.
        if (m_pVoxelWorld && Engine::IsSolid(m_pVoxelWorld->GetBlock(ix, kWallY, iz)))
            return false;

        cx = ix;
        cz = iz;
        return true;
    }

    bool TowerPlacementController::HasBudgetFor(PlaceType eType) const
    {
        // Placeable = owned but not currently placed and not on destroy-cooldown.
        // Both PlaceableHealCount and PlaceableTowerCount already exclude placed
        // (they count unplaced reserves) and benched (destroy-cooldown) towers.
        if (eType == PlaceType::Heal)
            return TowerManager::GetInst().PlaceableHealCount() > 0;
        return TowerManager::GetInst().PlaceableTowerCount() > 0;
    }

    bool TowerPlacementController::IsCellOccupied(int cx, int cz) const
    {
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (!pLayer) return false;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive()) continue;
            // Any tower kind blocks the cell.
            if (p->GetTag() != "Tower" && p->GetTag() != "HealTower") continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;
            const Engine::Vector3 v = pTr->GetPosition();
            if (static_cast<int>(std::floor(v.x)) == cx &&
                static_cast<int>(std::floor(v.z)) == cz)
                return true;
        }
        return false;
    }

    void TowerPlacementController::RenderGhost()
    {
        if (!m_bPlacing || !m_bHasCell) return;
        std::shared_ptr<Engine::Mesh> pMesh =
            (m_ePlaceType == PlaceType::Heal) ? m_pGhostMeshHeal : m_pGhostMesh;
        if (!pMesh || !m_pVS || !m_pPS || !m_pGhostTr) return;

        auto* pGraphics = Engine::Graphics::GetInst();
        auto* pDC = pGraphics->GetDeviceContext();

        // Bind the standard mesh input layout (unlike the footstep quad, the
        // cube VS reads real vertex-buffer inputs, so the IL must be set).
        if (m_pInputLayout) m_pInputLayout->Bind();
        if (m_pTopology)    m_pTopology->Bind();
        // Depth: a valid ghost occludes normally (test on); a blocked ghost
        // draws with depth test OFF so the red shows over the opaque tower
        // already sitting on the cell (same size/position would otherwise
        // z-fight and lose to the tower).
        if (m_bValidCell) { if (m_pDepthTest) m_pDepthTest->Bind(); }
        else              { if (m_pDepthNone) m_pDepthNone->Bind(); }
        m_pVS->Bind();
        m_pPS->Bind();
        if (m_pGhostMat)
        {
            // Placeable = blue (attack) / green (heal); blocked = saturated red
            // so it reads clearly even over the opaque tower beneath it.
            if (!m_bValidCell)
                m_pGhostMat->SetDiffuseColor(1.0f, 0.0f, 0.0f, 0.75f);
            else if (m_ePlaceType == PlaceType::Heal)
                m_pGhostMat->SetDiffuseColor(0.3f, 1.0f, 0.5f, 0.4f);
            else
                m_pGhostMat->SetDiffuseColor(0.3f, 0.6f, 1.0f, 0.4f);
            m_pGhostMat->Bind();   // g_vDiffuseColor (incl. alpha)
        }

        m_pGhostTr->SetPosition(
            static_cast<float>(m_iCellX) + 0.5f,
            static_cast<float>(kWallY),
            static_cast<float>(m_iCellZ) + 0.5f);
        m_pGhostTr->PostUpdate(0.f);   // compute world*VP into g_matTransform
        m_pGhostTr->Bind();
        pMesh->Draw();

        // Restore a normal (test-on) depth state so the test-off case doesn't
        // leak into later passes, and leave shader stages clean.
        if (m_pDepthTest) m_pDepthTest->Bind();
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        pGraphics->ResetBindCache();
    }
}
