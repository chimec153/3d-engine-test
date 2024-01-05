#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Camera :
        public Drawable
    {
        friend class Scene;

    public:
        enum class PROJECT_TYPE
        {
            ORTHOGONAL,
            PERSPECTIVE,
            END
        };

    public:
        Camera();
        Camera(const Camera& cam);
    public:
        virtual ~Camera() override = default;

    private:
        Matrix matView;
        float   m_fSpeed;
#ifdef _DEBUG
        std::shared_ptr<class Drawable>    m_pDebugDrawable;
#endif
        bool m_bControl;
        Matrix m_matProj;
        Matrix m_matVP;
        PROJECT_TYPE m_eProjType;
        float m_fRatio;
        float m_fAngle;
        float m_fNear;
        CAMERA_TYPE m_eCameraType;

    public:
        const Matrix& GetView()  const noexcept;
        virtual void Reset() override;
        void UpdateView();
        const Matrix& GetInvView() const noexcept;
        void SetProjectType(PROJECT_TYPE eType);
        float GetAngle() const noexcept;
        float GetRatio() const noexcept;
        float GetNear() const noexcept;
        const Matrix& GetProjectMatrix()    const noexcept;
        const Matrix& GetViewProject()    const noexcept;
        const Vector3& CameraPosToWorldPos(const Vector2& vCameraPos)   const;
        const Vector3& ScreenPosToClipPos(const Vector2& vScreenPos)    const;
        void SetCameraType(CAMERA_TYPE eType);

    public:
        virtual bool Init() override;
        virtual void Input(float fDeltaTime) override;
        virtual void Update(float fDeltaTime) override;
        virtual void Collision(float fDeltaTime) override;
        virtual void PostUpdate(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    public:
        void CameraMoveFront(float fDeltaTime);
        void CameraMoveBack(float fDeltaTime);
        void CameraMoveLeft(float fDeltaTime);
        void CameraMoveRight(float fDeltaTime);
        void CameraMoveUp(float fDeltaTime);
        void CameraMoveDown(float fDeltaTime);
    };

}