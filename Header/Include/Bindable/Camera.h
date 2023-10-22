#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Camera :
        public Drawable
    {
        friend class Scene;

    public:
        Camera();
    public:
        virtual ~Camera() override = default;

    private:
        Matrix matView;
        float   m_fSpeed;
#ifdef _DEBUG
        std::shared_ptr<class Drawable>    m_pDebugDrawable;
#endif
        bool m_bControl;

    public:
        const Matrix& GetView()  const;
        virtual void Reset() override;
        void UpdateView();

    public:
        virtual bool Init() override;
        virtual void Input(float fDeltaTime) override;
        virtual void Update(float fDeltaTime) override;
        virtual void Collision(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        void CameraMoveFront(float fDeltaTime);
        void CameraMoveBack(float fDeltaTime);
        void CameraMoveLeft(float fDeltaTime);
        void CameraMoveRight(float fDeltaTime);
        void CameraMoveUp(float fDeltaTime);
        void CameraMoveDown(float fDeltaTime);
    };

}