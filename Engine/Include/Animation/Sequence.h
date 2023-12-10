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
            std::vector<POSE>   vecPose;

            _tagSequenceInfo()
            {
            }

            _tagSequenceInfo(const _tagSequenceInfo& info)  :
                vecPose(info.vecPose)
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
        BONECBUFFER m_tCBuffer;

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

    public:
        void Update(float fDeltaTime);
        void ResetResource();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    public:
        std::shared_ptr<Sequence> Clone();
    };

}