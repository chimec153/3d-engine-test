#include "TowerPlacementController.h"
#include "Tower.h"
#include "HealTower.h"
#include "TowerManager.h"
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

        Engine::CInput::GetInst()->AddKey(DIK_1);   // attack tower
        Engine::CInput::GetInst()->AddKey(DIK_2);   // heal tower

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

        // Preview meshes — cube for the attack tower, cylinder for the heal
        // tower (matches the real towers).
        m_pGhostMesh = Engine::MeshPresets::AxisBox(
            Engine::Vector3(-0.3f, 0.0f, -0.3f),
            Engine::Vector3( 0.3f, 1.6f,  0.3f));
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

        // Key 1 = attack tower, key 2 = heal tower. Pressing the active type's
        // key exits; pressing the other switches type. Entering needs budget
        // (placed < bought) for that type.
        if (pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_1))
        {
            if (m_bPlacing && m_ePlaceType == PlaceType::Attack) m_bPlacing = false;
            else if (HasBudgetFor(PlaceType::Attack)) { m_bPlacing = true; m_ePlaceType = PlaceType::Attack; }
            m_bHasCell = false;
        }
        if (pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_2))
        {
            if (m_bPlacing && m_ePlaceType == PlaceType::Heal) m_bPlacing = false;
            else if (HasBudgetFor(PlaceType::Heal)) { m_bPlacing = true; m_ePlaceType = PlaceType::Heal; }
            m_bHasCell = false;
        }

        if (!m_bPlacing)
            return;

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

        if (m_bHasCell && m_bValidCell &&
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
                    if (auto pHeal = pScene->CreateGameObject<HealTower>("HealTower", pLayer))
                        pHeal->SetCell(m_iCellX, m_iCellZ);
                }
                else
                {
                    if (auto pTower = pScene->CreateGameObject<Tower>("Tower", pLayer))
                    {
                        pTower->SetVoxelWorld(m_pVoxelWorld);
                        pTower->SetCell(m_iCellX, m_iCellZ);
                        // Consume the next reserve slot's configured weapon
                        // (shop-assigned, FIFO) instead of the global default.
                        pTower->SetWeaponId(TowerManager::GetInst().ConsumeReserveWeapon());
                    }
                }
            }
            // One tower per placement: exit the mode after dropping it.
            m_bPlacing = false;
            m_bHasCell = false;
        }
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

    int TowerPlacementController::CountByTag(const char* pTag) const
    {
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (!pLayer) return 0;
        int iCount = 0;
        for (const auto& p : pLayer->GetGameObjectList())
            if (p && p->IsActive() && p->GetTag() == pTag) ++iCount;
        return iCount;
    }

    bool TowerPlacementController::HasBudgetFor(PlaceType eType) const
    {
        if (eType == PlaceType::Heal)
            return CountByTag("HealTower") < TowerManager::GetInst().HealTowersOwned();
        return CountByTag("Tower") < TowerManager::GetInst().TowersOwned();
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
