#include "WeaponComboScene.h"
#include "Core/ObjectFactory.h"
REGISTER_SCENE(Client::WeaponComboScene, WeaponComboScene)
#include "../UI/WeaponCombiner.h"
#include "../Object/WeaponDatabase.h"
#include "../Object/Vfx/TrailRenderManager.h"
#include "Bindable/Camera.h"
#include "Bindable/PointLight.h"
#include "Core/Graphics.h"
#include "Render/RenderManager.h"

namespace Client
{
    WeaponComboScene::WeaponComboScene()
    {
    }

    WeaponComboScene::~WeaponComboScene()
    {
    }

    bool WeaponComboScene::Init()
    {
        // Load the weapon catalogue so the editor's list is populated even when
        // entering straight from the start menu (without going through the game
        // scene). Re-loading here also picks up edits saved on a prior visit.
        WeaponDatabase::GetInst().LoadFromCSV("/Game/Data/Weapons/weapons_v2.csv");

        if (!FindLayer(DEFAULT_LAYER)) AddLayer(DEFAULT_LAYER);

        std::shared_ptr<Engine::Camera> pCamera;
        if (auto pCameraObj = CreateGameObject("camera", FindLayer(DEFAULT_LAYER)))
        {
            pCamera = pCameraObj->AddComponent<Engine::Camera>("camera");
            if (pCamera)
            {
                // Same locked top-down framing as the game scene (60 deg pitch,
                // facing -Z) so the live weapon preview reads at gameplay scale.
                // WeaponCombiner refreshes the camera *position* each frame to
                // track the fixed muzzle lane; the rotation is set once here.
                pCamera->SetProjectType(Engine::Camera::PROJECT_TYPE::PERSPECTIVE);
                constexpr float kPI = 3.14159265f;
                pCamera->GetTransform()->SetRelativeRotation(kPI / 3.f, kPI, 0.f);
            }
        }
        Engine::Graphics::GetInst()->SetCamera(pCamera);

        // Directional light so the preview bullets' meshes are lit instead of
        // rendering as flat black spheres in the deferred pass.
        if (auto pLightObj = CreateGameObject("light", FindLayer(DEFAULT_LAYER)))
        {
            if (auto pLight = pLightObj->AddComponent<Engine::PointLight>("light"))
            {
                pLight->SetLightType(Engine::LIGHT_TYPE::DIRECTIONAL);
                pLight->GetTransform()->SetRX(1.f);
                Engine::Graphics::GetInst()->SetLight(pLight);
            }
        }

        // Bullet tracer trails are additive ribbons drawn from an ALPHA-pass
        // custom-render callback (each Bullet Submit()s its history every
        // Update). Mirror the game scene's wiring so the preview shots show
        // their trails; without this Submit() would queue ribbons nothing ever
        // drains.
        TrailRenderManager::GetInst()->Init();
        Engine::RenderManager::GetInst()->AddCustomRender(
            Engine::RENDER_LAYER::ALPHA,
            []() { TrailRenderManager::GetInst()->Render(); });

        // Weapon-combination UI: six type-fixed attribute slots, an
        // inventory of attribute icons, and a craft button. The component
        // owns all the widgets + combination logic (back-to-menu button
        // included).
        if (auto pComboObj = CreateGameObject("weapon_combiner", FindLayer(DEFAULT_LAYER)))
        {
            pComboObj->AddComponent<WeaponCombiner>("combiner");
        }

        return true;
    }
}
