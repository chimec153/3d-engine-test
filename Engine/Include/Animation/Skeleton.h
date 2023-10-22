#pragma once

#include "../Core/Ref.h"
#include "../Core/Ptr.h"

namespace Engine
{
    class ENGINE_DLL Skeleton :
        public CRef
    {
    public:
        Skeleton();
        virtual ~Skeleton() override;

    private:
        std::vector<PBONE>  m_vecJoint;
        std::shared_ptr<class StructuredBuffer>   m_pBuffer;
        std::shared_ptr<class StructuredBuffer> m_pJointHierarchy;

    public:
        void SetBone(const std::vector<BONE>& vecBone);
        const BONE& GetBone(int iIndex) const;
        const std::vector<PBONE>& GetBones() const;
        int GetBoneCount()  const;
        int FindJointIndex(const std::string& strJoint)   const;
        void Update(int iJointIndex, std::shared_ptr<StructuredBuffer> pBuffer);
        const std::vector<PBONE> GetJoints()    const;
        void SetSRV();
        void ResetSRV();
        std::shared_ptr<class StructuredBuffer> GetBuffer() const;
        void SetHierarchySRV();
        void ResetHierarchySRV();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}