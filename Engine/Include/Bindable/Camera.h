#pragma once
#include "../Component/Component.h"
namespace Engine
{
    // Phase B.5 — Camera migrated from Drawable to Component. Camera doesn't
    // render itself (PreDraw was empty). It owns a Transform component for
    // its world placement and provides view/projection matrices to other
    // systems. Lifecycle is driven by the owning GameObject.
    class ENGINE_DLL Camera :
        public Component
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
        std::shared_ptr<class Transform> m_pTransform;
        Matrix matView;
        float   m_fSpeed;
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
        // World → screen pixels. Returns true if the point is in front of
        // the camera (clip w > 0); outPxX/outPxY are the back-buffer
        // pixel coords, outW is the clip-space w (useful for depth
        // attenuation / size-with-distance). When the function returns
        // false the world point is behind the camera and the outputs
        // are not meaningful.
        bool WorldToScreen(const Vector3& vWorld,
                           float& outPxX, float& outPxY, float& outW) const;
        void SetCameraType(CAMERA_TYPE eType);
        CAMERA_TYPE GetCameraType() const noexcept { return m_eCameraType; }

    public:
        virtual bool Init() override;
        virtual void Input(float fDeltaTime) override;
        virtual void Update(float fDeltaTime) override;
        virtual void Collision(float fDeltaTime) override;
        virtual void PostUpdate(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;
        virtual std::shared_ptr<class Transform> GetTransform() const override { return m_pTransform; }

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