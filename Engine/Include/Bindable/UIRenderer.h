#pragma once
#include "Drawable.h"

namespace Engine
{
    class ENGINE_DLL UIRenderer :
        public Drawable
    {
    public:
        UIRenderer();
        virtual ~UIRenderer() override = default;

    private:
        std::shared_ptr<class Camera> m_pCamera;
        std::shared_ptr<class Transform> m_pParentTransform;
        std::shared_ptr<class Mesh> m_pParentMesh;
        std::shared_ptr<class Animation> m_pParentAnimation;
        std::shared_ptr<class Light> m_pLight;

    public:
        void SetCamera(std::shared_ptr<class Camera> pCamera);
        void SetTarget(std::shared_ptr<class Drawable> pTarget);

    public:
        virtual bool Init() override;
        virtual void Bind() override;
    };

}