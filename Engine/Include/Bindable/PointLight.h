#pragma once
#include "../Component/Component.h"
#include "ConstantBuffer.h"

namespace Engine
{
    typedef struct _tagOrthoInfo
    {
        float   fLeft;
        float   fRight;
        float   fTop;
        float   fBottom;
        float   fNear;
        float   fFar;
    }ORTHOINFO, * PORTHOINFO;

    // Phase B.7 — PointLight migrated from Drawable to Component. PointLight
    // doesn't render itself; deferred lighting reads its CB during the
    // RenderLight pass (Multi VS/PS fullscreen quad). Bind/PostBind kept as
    // regular methods (not Component overrides) since RenderManager calls
    // them directly when iterating m_LightList.
    class ENGINE_DLL PointLight :
        public Component
    {
        friend class Scene;

    public:
        PointLight();
    public:
        virtual ~PointLight() override = default;

    private:
        std::shared_ptr<class Transform> m_pTransform;
        std::shared_ptr<ConstantBuffer<POINTLIGHT>> pPointCBuffer;
        POINTLIGHT  tPointLight;
        Matrix  matView;
        Matrix  matViewProject;
        ORTHOINFO m_tOrthoInfo;

    public:
        void SetLightType(LIGHT_TYPE eType);
        LIGHT_TYPE GetLightType()   const;
        void SetIntensity(float fIntensity);
        const std::shared_ptr<ConstantBuffer<POINTLIGHT>>& GetLightCBuffer()    const;
        const Matrix& GetView() const;
        const Matrix& GetViewProject() const;
        const ORTHOINFO& GetOrthoInfo() const;
        void SetOrthoInfo(const ORTHOINFO& tInfo);
        float GetIntensity()    const;
        Vector4 GetLightColor() const;
        Vector4 GetAmbientColor()   const;
        void SetLightColor(const Vector4& vColor);
        void SetAmbientColor(const Vector4& vColor);
        float GetConstantAttenuation()  const;
        float GetLinearAttenuation()   const;
        float GetQuadraticAttenuation() const;
        void SetConstantAttenuation(float fAttenuation);
        void SetLinearAttenuation(float fAttenuation);
        void SetQuadraticAttenuation(float fAttenuation);
        // Spot-only cone falloff exponent. Higher = narrower/sharper cone.
        // Range typically 1.0 (very wide, almost no cone) to 128.0 (laser).
        // Ignored for POINT/DIRECTIONAL light types.
        float GetSpotConeExponent() const;
        void  SetSpotConeExponent(float fExponent);

    public:
        virtual void Reset() override;

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;
        virtual std::shared_ptr<class Transform> GetTransform() const override { return m_pTransform; }

        // Bind/PostBind are NOT virtual overrides anymore (Component has
        // no Bind interface). RenderManager::RenderLight invokes these
        // directly when iterating m_LightList — they update + bind the
        // light's POINTLIGHT cbuffer for the deferred shading pass.
        void Bind();
        void PostBind();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}