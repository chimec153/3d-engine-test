#include "FragmentShard.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::FragmentShard, FragmentShard)

#include "../EnemyMeshRenderer.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/PaperBurn.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"

#include <algorithm>
#include <cmath>

namespace Client
{
    namespace
    {
        // Feel constants — mirror the GPU version that preceded this.
        constexpr float kGravity     = 22.f;
        constexpr float kDrag        = 0.6f;
        constexpr float kRestitution = 0.35f;
    }

    FragmentShard::FragmentShard() = default;

    bool FragmentShard::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform = AddComponent<Engine::Transform>("transform");
        m_pRenderer  = AddComponent<EnemyMeshRenderer>("mesh_renderer");

        // Idempotent; ensures EnemyVS/PS(+Inst) exist before we look them up.
        EnemyMeshRenderer::RegisterShaders();

        if (m_pRenderer)
        {
            m_pRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(EnemyMeshRenderer::kVSTag));
            m_pRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (EnemyMeshRenderer::kPSTag));
            m_pRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
            // Intentionally no SetCustomStencil → shards skip the outline pass
            // (cheap; debris reads fine without it).
        }

        // PaperBurn drives the dissolve through the b10 cbuffer (read by the
        // solo EnemyPS, gated on the material's UsePaperBurn flag). Having the
        // component also forces the shard out of the DrawInstanced bucket, so
        // every shard takes the solo path and dissolves consistently — instead
        // of only when 2+ shards happen to batch into EnemyPSInst.
        // The texture is dereferenced unconditionally in PaperBurn::Bind, so
        // only attach the component when it's available (GameScene loads it).
        if (auto pBurnTex = Engine::StaticFindBindable<Engine::Texture>("PaperBurn"))
            m_pPaperBurn = AddComponent<Engine::PaperBurn>("paperburn", pBurnTex);

        return true;
    }

    void FragmentShard::Launch(const std::shared_ptr<Engine::Mesh>& pMesh,
                               const std::shared_ptr<Engine::Material>& pMaterial,
                               const Engine::Vector3& vWorldPos,
                               const Engine::Vector3& vVelocity,
                               const Engine::Vector3& vAngularVel,
                               float fScale, float fMaxAge, float fGroundY)
    {
        m_vVelocity   = vVelocity;
        m_vAngularVel = vAngularVel;
        m_vRotation   = Engine::Vector3(0.f, 0.f, 0.f);
        m_fAge        = 0.f;
        m_fMaxAge     = fMaxAge;
        m_fGroundY    = fGroundY;

        if (m_pTransform)
        {
            m_pTransform->SetPosition(vWorldPos);
            m_pTransform->SetScale(fScale, fScale, fScale);
            m_pTransform->SetRotation(0.f, 0.f, 0.f);
        }
        if (m_pRenderer)
        {
            m_pRenderer->SetMesh(pMesh);
            if (pMaterial)
            {
                m_pRenderer->SetMaterial(pMaterial);
                m_pRenderer->SetOverrideMaterial(0, 0, pMaterial);
            }
            m_pRenderer->SetBurnRim(0.f);
        }

        // GetPaperBurnColor/ApplyDissolveInst fully clips at ratio≈0.67, so size
        // the burn timer to hit that exactly at death → the shard flies intact,
        // then burns away over the back half of its life with nothing to spare.
        if (m_pPaperBurn)
        {
            m_pPaperBurn->SetMaxTime(fMaxAge / 0.67f);
            m_pPaperBurn->StartPaperBurn();
        }
    }

    void FragmentShard::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        m_fAge += fDeltaTime;
        if (m_fAge >= m_fMaxAge)
        {
            InActivate();
            return;
        }

        m_vVelocity.y -= kGravity * fDeltaTime;
        m_vVelocity = m_vVelocity * std::max(0.f, 1.f - kDrag * fDeltaTime);

        Engine::Vector3 pos = m_pTransform ? m_pTransform->GetPosition() : Engine::Vector3();
        pos = pos + m_vVelocity * fDeltaTime;

        if (pos.y < m_fGroundY)
        {
            pos.y = m_fGroundY;
            m_vVelocity.y = -m_vVelocity.y * kRestitution;
            m_vVelocity.x *= 0.6f;
            m_vVelocity.z *= 0.6f;
            m_vAngularVel = m_vAngularVel * 0.5f;
            if (std::fabs(m_vVelocity.y) < 0.3f) m_vVelocity.y = 0.f;
        }

        m_vRotation = m_vRotation + m_vAngularVel * fDeltaTime;

        if (m_pTransform)
        {
            m_pTransform->SetPosition(pos);
            m_pTransform->SetRotation(m_vRotation);
        }
        // Dissolve is driven by the PaperBurn component (b10) + the solo EnemyPS,
        // not a manual ramp — see Launch().
    }
}
