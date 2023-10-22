#pragma once
#include "Bindable\Drawable.h"
namespace Engine
{
    class Agent;
    class Animation;
    class ColliderLine;
}

class Player :
    public Engine::Drawable
{
public:
    Player();
    Player(const Player& player);
    virtual ~Player() = default;

private:
    std::shared_ptr<Engine::Agent>  m_pAgent;
    std::shared_ptr<Engine::Animation> m_pAnimation;
    std::shared_ptr<Engine::ColliderLine> m_pFootLineCollider[2];
#ifdef _DEBUG
    std::shared_ptr<Engine::Sphere> m_pSphere[2];
#endif
public:
    void CreateAgent(std::shared_ptr<Engine::NavMesh> pNavMesh, const Engine::Vector3& pos);
    void Move(const Engine::Vector3& pos);
    virtual void Update(float fDeltaTime) override;

public:
    void CollisionStay(class Engine::Collider* pSrc, class Engine::Collider* pDest, float fDeltaTime);

public:
    virtual std::shared_ptr<Bindable> Clone() override;
};

