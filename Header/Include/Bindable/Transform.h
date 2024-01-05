#pragma once

#include "../Types.h"

namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    class ENGINE_DLL Transform :
        public Bindable
    {
        friend class Drawable;

    public:
        Transform();
        Transform(const Transform& buffer);
    public:
        virtual ~Transform() noexcept override = default;

    private:
        std::shared_ptr<ConstantBuffer<_tagTransformBuffer>> m_pConstantBuffer;

        Transform* m_pParentTrasnform;
        std::list<Transform*> m_ChildTransformList;
        _tagTransformBuffer m_tBuffer;
        std::shared_ptr<class StructuredBuffer> m_pJointSequenceBuffer;
        CAMERA_TYPE m_eCameraType;

    public:
        void SetParentTransform(Transform* pParent);
        void AddChildTransform(Transform* pChild);
        const std::shared_ptr<class ConstantBuffer<_tagTransformBuffer>>& GetConstantBuffer()    const;
        const _tagTransformBuffer& GetBuffer()  const;
        void SetCameraType(CAMERA_TYPE eType);
        CAMERA_TYPE GetCameraType() const;

    private:
        Vector3 m_vPosition;
        Vector3 m_vVelocity;
        Vector3 m_vRotation;
        Vector3 m_vRotationVelocity;
        Vector3 m_vScale;
        Vector3 m_vScaleVelocity;
        Vector3 m_vRelativePosition;
        Vector3 m_vRelativeRotation;
        Vector3 m_vRelativeScale;
        Matrix m_matTransform;
        Matrix m_matRotation;
        Matrix m_matRotationTranslation;
        Matrix m_matWV;
        Matrix m_matParent;
        Vector3 m_vAxis[static_cast<int>(AXIS_TYPE::END)];
        bool m_bUpdateRotation;
        bool m_bUpdatePosition;
        bool m_bUpdateScale;

    public:
        void SetX(float x);
        void SetY(float y);
        void SetZ(float z);
        void AddX(float x);
        void AddY(float y);
        void AddZ(float z);
        float GetX() const;
        float GetY() const;
        float GetZ() const;
        void  SetDX(float x);
        void  SetDY(float y);
        void  SetDZ(float z);
        void  AddDX(float x);
        void  AddDY(float y);
        void  AddDZ(float z);
        float GetDX() const;
        float GetDY() const;
        float GetDZ() const;
        void  SetRX(float x);
        void  SetRY(float y);
        void  SetRZ(float z);
        void  AddRX(float x);
        void  AddRY(float y);
        void  AddRZ(float z);
        float GetRX() const;
        float GetRY() const;
        float GetRZ() const;
        void  SetDRX(float x);
        void  SetDRY(float y);
        void  SetDRZ(float z);
        void  AddDRX(float x);
        void  AddDRY(float y);
        void  AddDRZ(float z);
        float GetDRX() const;
        float GetDRY() const;
        float GetDRZ() const;
        float GetRelativeRY()   const;
        void SetRandomPosAndRotation();
        const Vector3& GetAxis(AXIS_TYPE type)    const;
        const Vector3& GetPosition()  const;
        void SetPosition(const Vector3& pos);
        void SetPosition(float x, float y, float z);
        void AddPosition(const Vector3& pos);
        void SetRelativePosition(const Vector3& pos);
        void SetRelativePosition(float x, float y, float z);
        void SetRelativeScale(float x, float y, float z);
        void SetRelativeRotation(float x, float y, float z);
        void SetRelativeRotation(const Vector3& vRotation);
        void AddRelativeRX(float fX);
        void AddRelativeRY(float fY);
        void AddRelativeRZ(float fZ);
        const Matrix& GetTransformMatrix()  const;
        void SetScale(const Vector3& scale);
        void SetScale(float x, float y, float z);
        void SetRotation(const Vector3& rot);
        void SetRotation(float x, float y, float z);
        const Vector3& GetRotation()    const;
        const Vector3& GetScale()   const;
        const Vector3& GetVelocity()   const;
        const Matrix& GetRotationMatrix()  const;
        const Matrix& GetRotationTranslationMatrix()  const;
        const Matrix& GetWV()  const;
        void SetVelocity(const Vector3& vVelocity);
        void SetAxis(AXIS_TYPE eType, const Vector3& vAxisZ, const Vector3& vUp = {0.f, 1.f, 0.f});
        void SetParentMatrix(const Matrix& matParent, int iJointIndex, std::shared_ptr<class StructuredBuffer> pBuffer);
        void SetRotationTranslationMatrix(const Matrix& mat);
        void UpdateCameraRelateMatrix(std::shared_ptr<Camera> pCamera);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void PostUpdate(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
        virtual void PostBind() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    public:
        void UpdateRelativePosition();
        void UpdatePosition();
        void UpdateRelativeRotation();
        void UpdateRotation();
        void UpdateRelativeScale();
        void UpdateScale();
        void Reset();
    };

}