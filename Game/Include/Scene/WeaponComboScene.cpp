#include "WeaponComboScene.h"
#include "Core/ObjectFactory.h"
REGISTER_SCENE(Client::WeaponComboScene, WeaponComboScene)
#include "../UI/WeaponCombiner.h"
#include "../Object/WeaponDatabase.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"

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
        // Restore crafted weapons saved from previous runs (load-once) so
        // the loadout panel shows them straight away.
        //WeaponDatabase::GetInst().LoadCrafted("/Game/Data/Weapons/crafted.csv");

        if (!FindLayer(DEFAULT_LAYER)) AddLayer(DEFAULT_LAYER);

        std::shared_ptr<Engine::Camera> pCamera;
        if (auto pCameraObj = CreateGameObject("camera", FindLayer(DEFAULT_LAYER)))
        {
            pCamera = pCameraObj->AddComponent<Engine::Camera>("camera");
        }
        Engine::Graphics::GetInst()->SetCamera(pCamera);

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
