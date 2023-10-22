#pragma once
#include "Drawable.h"
#include "PixelCBuffer.h"
#include "VertexCBuffer.h"

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

    class ENGINE_DLL PointLight :
        public Drawable
    {
        friend class Scene;

    public:
        PointLight();
    public:
        virtual ~PointLight() override = default;

    private:
        std::shared_ptr<PixelCBuffer<POINTLIGHT>> pPointCBuffer;
        std::shared_ptr<VertexCBuffer<POINTLIGHT>> pVSPointCBuffer;
        POINTLIGHT  tPointLight;
        Matrix  matView;
        Matrix  matViewProject;
        ORTHOINFO m_tOrthoInfo;

    public:
        void SetLightType(LIGHT_TYPE eType);
        LIGHT_TYPE GetLightType()   const;
        void SetIntensity(float fIntensity);
        const std::shared_ptr<PixelCBuffer<POINTLIGHT>>& GetLightCBuffer()    const;
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

    public:
        virtual void Reset() override;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}