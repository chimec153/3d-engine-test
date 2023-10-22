#pragma once
#include "Scene\Scene.h"

class InGameScene :
    public Engine::Scene
{
public:
    InGameScene();
    virtual ~InGameScene() = default;

public:
    virtual bool Init() override;
};

