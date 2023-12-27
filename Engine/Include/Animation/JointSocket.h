#pragma once
#include "../Core/Ref.h"

namespace Engine
{
    class ENGINE_DLL JointSocket :
        public CRef
    {
    public:
        JointSocket();
        virtual ~JointSocket() override = default;

    private:
        int m_iParentIndex;
        Vector3 m_vScale;
        Vector3 m_vPosition;
        Vector3 m_vRotation;
        Matrix m_matJoint;
        std::shared_ptr<class Drawable> m_pDrawable;


    public:
        void SetParentIndex(int iIndex);
        void UpdateJointMatrix();
        int GetParentIndex()    const;
        const Matrix& GetJoint()    const;
        void SetDrawable(std::shared_ptr<Drawable> pDrawable);
        void SetScale(const Vector3& vScale);
        void SetScale(float x, float y, float z);
        void SetPosition(const Vector3& vPos);
        void SetRotation(const Vector3& vQuter);
        void AddRX(float x);
        void AddRY(float y);
        void AddRZ(float z);
        void Update(std::shared_ptr<class StructuredBuffer> pBuffer, const Matrix& matParent);
        const Vector3& GetScale()  const;
        const Vector3& GetPosition()    const;
        const Vector3& GetRotation()   const;
        std::shared_ptr<class Drawable> GetDrawable()   const;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}