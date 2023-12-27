#pragma once

#include "../Core/Ref.h"
#include "../Bindable/FbxLoader.h"
#include "../Shader/StructuredBuffer.h"

namespace Engine
{
    template <typename T>
    class ConstantBuffer;
    template <typename T>
    class ConstantBuffer;
    template <typename T>
    class std::shared_ptr;

    class ENGINE_DLL Sequence :
        public CRef
    {
    public:
        typedef struct _tagSequenceInfo
        {
            std::vector<JOINT>   vecJoint;

            _tagSequenceInfo()
            {
            }

            _tagSequenceInfo(const _tagSequenceInfo& info)  :
                vecJoint(info.vecJoint)
            {
            }
        }SEQUENCEINFO, * PSEQUENCEINFO;

    public:
        Sequence();
        Sequence(const Sequence& seq);
        virtual ~Sequence() override;

    private:
        class std::shared_ptr<class ConstantBuffer<BONECBUFFER>>    m_pBoneConstantBuffer;
        std::vector<PSEQUENCEINFO>    m_vecInfo;
        bool m_bRootMotion;
        std::shared_ptr<class StructuredBuffer> m_pBuffer;
        int m_iMaxFrame;
        float               fTime;
        float               fMaxTime;
        BONEINFO m_tCBuffer;
        bool m_bLoop;
        std::string m_strNextSequence;
        std::vector<float> m_vecBlendPalette;

    public:
        bool SetSequance(const std::vector<FbxLoader::FBXBONEKEYFRAME>& vecPose);
        void AddSequenceInfo(PSEQUENCEINFO);
        void UseRootMotion();
        float GetMaxTime()  const;
        int GetMaxFrame()   const;
        PSEQUENCEINFO GetSequenceInfo(int iIndex = 0) const;
        bool IsRootMotion() const;
        int GetFrame()  const;
        void SetFramePosition(int iBone, int iFrame, const Vector3& vPos);
        void SetFrameRotation(int iBone, int iFrame, const Vector4& vQuternion);
        void SetFrameScale(int iBone, int iFrame, const Vector3& vScale);
        void CreateSequenceBuffer();
        void Loop();
        bool IsLoop()   const;
        void SetNextSequence(const std::string& strSeq);
        const std::string& GetNextSequence()    const;
        const std::vector<float>& GetBlendPalette() const;
        void SetBlendFactor(int iJoint, float fBlendFactor);
        const BONEINFO& GetBoneInfo()   const;

    public:
        void Update(float fDeltaTime, int iSlot = 31, int iIndex = 0);
        void ResetResource();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    public:
        std::shared_ptr<Sequence> Clone();
    };

}