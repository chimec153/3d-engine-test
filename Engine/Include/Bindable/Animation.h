#pragma once
#include "Drawable.h"
#include "FbxLoader.h"

namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    class ENGINE_DLL Animation :
        public Bindable
    {
    public:
        Animation();
        Animation(const Animation& animation);
        virtual ~Animation() override = default;

    private:
        std::unordered_map<std::string, std::shared_ptr<class Sequence>> m_mapSequence;
        std::shared_ptr<class Sequence>    m_pCurrentSequence;
        std::list<std::shared_ptr<class JointSocket>> m_SocketList;
        class std::shared_ptr<class Skeleton> m_pSkeleton;
        std::shared_ptr<class ComputeShader> m_pComputeShader;
        std::shared_ptr<class ComputeShader> m_pPostProcessShader;
        std::shared_ptr<class StructuredBuffer> m_pMidBuffer;
        std::shared_ptr<class StructuredBuffer> m_pFinalBuffer;
        std::shared_ptr<class StructuredBuffer> m_pPoseBuffer;
        float   m_fTime;
        std::vector<IKINFO>  m_vecIKInfo;
        std::shared_ptr<class ConstantBuffer<IKCBUFFER>> m_pIKCBuffer;
        Drawable* m_pOwner;

    public:
        void AddSequance(const std::string& strTag, const std::shared_ptr<Sequence>& pSequence);
        void AddSequance(const std::string& strTag, const std::vector<FbxLoader::FBXBONEKEYFRAME>& vecPose);
        void ChangeSequence(const std::string& strTag);
        std::shared_ptr<class JointSocket> AddSocket(int iJointIndex, const std::shared_ptr<Drawable>& pDrawable);
        std::shared_ptr<class Sequence> GetCurrentSequence()    const;
        const std::list<std::shared_ptr<class JointSocket>>& GetSocketList()  const;
        void AddSocket(const std::string& strJoint, const std::shared_ptr<JointSocket>& pSocket);
        void AddSocket(int iJoint, const std::shared_ptr<JointSocket>& pSocket);
        void SetSkeleton(const std::shared_ptr<Skeleton>& pSkeleton);
        class std::shared_ptr<class Skeleton> GetSkeleton() const;
        void UpdateMatrix();
        void MatrixPostProcess();
        const std::unordered_map<std::string, std::shared_ptr<class Sequence>>& GetSequences()  const;
        float GetTime() const;
        int GetCurrentAnimID()  const;
        std::shared_ptr<StructuredBuffer> GetFinalBuffer()  const;
        void AddIkInfo(int iJointIndex, int iRootIndex);
        void SetIkPosition(int iIndex, const Vector3& vPos);
        void SetOwner(Drawable* pOwner);

    private:
        std::shared_ptr<Sequence> FindSequence(const std::string& strTag)   const;
        PIKINFO FindIkInfo(int iIndex);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}