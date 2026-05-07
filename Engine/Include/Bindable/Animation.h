#pragma once
#include "../Component/Component.h"
#include "FbxLoader.h"
#include "../Animation/Notify.h"

namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    class Drawable;

    // Phase E3 — Animation migrated from Bindable to Component. Animation
    // is fundamentally a CPU sequencer that drives a compute-shader skinning
    // pass. Bind/PostBind are NOT virtual overrides anymore (Component has
    // no Bind interface) — they're regular methods invoked by the owning
    // Drawable's render path.
    class ENGINE_DLL Animation :
        public Component
    {
    public:
        typedef struct _tagSequenceInfo
        {
            std::shared_ptr<class Sequence> pSequence;
            float fTime;
            std::list<std::shared_ptr<class Notify>> NotifyList;

            std::shared_ptr<class Notify> FindNotify(const std::string& strNotify)  const
            {
                std::list<std::shared_ptr<Notify>>::const_iterator iter = NotifyList.begin();
                std::list<std::shared_ptr<Notify>>::const_iterator iterEnd = NotifyList.end();

                for (; iter != iterEnd; ++iter)
                {
                    if ((*iter)->GetTag() == strNotify)
                    {
                        return *iter;
                    }
                }

                return std::shared_ptr<class Notify>();
            }

            _tagSequenceInfo() :
                pSequence()
                , fTime()
            {
            }
        }SEQUENCEINFO, *PSEQUENCEINFO;
    public:
        Animation();
        Animation(const Animation& animation);
        virtual ~Animation() override;

    private:
        std::unordered_map<std::string, PSEQUENCEINFO> m_mapSequence;
        PSEQUENCEINFO    m_pCurrentSequence;
        std::list<std::shared_ptr<class JointSocket>> m_SocketList;
        class std::shared_ptr<class Skeleton> m_pSkeleton;
        std::shared_ptr<class ComputeShader> m_pComputeShader;
        std::shared_ptr<class ComputeShader> m_pPostProcessShader;
        std::shared_ptr<class StructuredBuffer> m_pMidBuffer;
        std::shared_ptr<class StructuredBuffer> m_pFinalBuffer;
        std::shared_ptr<class StructuredBuffer> m_pPoseBuffer;
        std::vector<IKINFO>  m_vecIKInfo;
        std::shared_ptr<class ConstantBuffer<IKCBUFFER>> m_pIKCBuffer;
        // Phase E5 — Animation::m_pOwner (Drawable*) removed. Animation
        // is a Component now and reaches its host Transform via the
        // Component-side GetGameObjectOwner() in Update(). Drawable
        // hosts no longer exist live.
        float   m_fRate;
        PSEQUENCEINFO m_pAdditiveSequence;
        std::shared_ptr<class ConstantBuffer<BONECBUFFER>> m_pBoneCBuffer;
        BONECBUFFER m_tBoneCBuffer;
        bool m_bStop;

    public:
        PSEQUENCEINFO AddSequance(const std::string& strTag, const std::shared_ptr<Sequence>& pSequence);
        void AddSequance(const std::string& strTag, const std::vector<FbxLoader::FBXBONEKEYFRAME>& vecPose);
        std::shared_ptr<Sequence> FindAndAddSequence(const std::string& strTag);
        void ChangeSequence(const std::string& strTag);
        // Phase E5 — Drawable overload removed (no more live Drawable
        // instances). GameObject is the only target type now; AddSocket
        // extracts its Transform Component into the JointSocket.
        std::shared_ptr<class JointSocket> AddSocket(int iJointIndex, const std::shared_ptr<class GameObject>& pGameObject);
        std::shared_ptr<class Sequence> GetCurrentSequence()    const;
        std::shared_ptr<class Sequence> GetAdditiveSequence()    const;
        const std::list<std::shared_ptr<class JointSocket>>& GetSocketList()  const;
        void AddSocket(const std::string& strJoint, const std::shared_ptr<JointSocket>& pSocket);
        void AddSocket(int iJoint, const std::shared_ptr<JointSocket>& pSocket);
        void DeleteSocket(std::shared_ptr<JointSocket> pSocket);
        void SetSkeleton(const std::string& strTag);
        void SetSkeleton(std::shared_ptr<Skeleton> pSkeleton);
        class std::shared_ptr<class Skeleton> GetSkeleton() const;
        void UpdateMatrix();
        void MatrixPostProcess();
        const std::unordered_map<std::string, PSEQUENCEINFO>& GetSequences()  const;
        float GetTime() const;
        int GetCurrentAnimID()  const;
        std::shared_ptr<StructuredBuffer> GetFinalBuffer()  const;
        void AddIkInfo(int iJointIndex, int iRootIndex);
        void SetIkPosition(int iIndex, const Vector3& vPos);
        // Phase E5 — SetOwner removed (Drawable hosts gone).
        void SetTime(float fTime);
        float GetRate() const;
        void SetRate(float fRate);
        void SetLoop(const std::string& strSeq);
        void SetNextSequence(const std::string& strSeq, const std::string& strNext);
        std::shared_ptr<class Notify> AddNotify(const std::string& strSeq, const std::string& strNotify, int iFrame);
        std::shared_ptr<class Notify> AddNotify(const std::string& strSeq, const std::string& strNotify, float fTime);
        void SetAdditiveSequence(const std::string& strSequence);
        const PSEQUENCEINFO FindSeuqence(const std::string& strSeq) const;
        void SetFinalBuffer();

    private:
        std::shared_ptr<class Notify> AddNotify(const std::string& strSeq, const std::string& strNotify);
        PSEQUENCEINFO FindSequence(const std::string& strTag);
        PIKINFO FindIkInfo(int iIndex);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind/PostBind are NOT virtual overrides anymore — Drawable's
        // render path calls these directly via its m_pAnimation reference
        // (was previously called via Bindable child-list iteration).
        void Bind();
        void PostBind();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}