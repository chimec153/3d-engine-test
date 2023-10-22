#pragma once

#include "../Core/Ref.h"

namespace Engine
{
    template <typename T>
    class std::shared_ptr;
    class VertexBuffer;
    template <typename T>
    class ComputeCBufffer;

    class ENGINE_DLL RenderInstancing :
        public CRef
    {
    public:
        RenderInstancing(const std::shared_ptr<class Mesh>& pVertexBuffer,
            const std::shared_ptr<class InputLayout>& pInputLayout, const std::shared_ptr<class VertexShader>& pVertexShader, const std::shared_ptr<class VertexShader>& pVertexShadowShader, const std::shared_ptr<class PixelShader>& pPixelShader, int iInstSize,
            const std::vector<std::shared_ptr<class Texture>>& vecTexture);
    public:
        virtual ~RenderInstancing() override = default;

    private:
        std::list<class std::shared_ptr<class Drawable>>   m_RenderList;
        class CPtr<ID3D11Buffer>  m_pInstBuffer;
        int m_iMaxCount;
        std::shared_ptr<Mesh>  m_pMesh;
        int m_iInstSize;
        std::shared_ptr<class InputLayout> m_pInputLayout;
        std::shared_ptr<class VertexShader>    m_pVertexShader;
        std::shared_ptr<class PixelShader> m_pPixelShader;
        std::vector<std::shared_ptr<class Texture>> m_vecTexture;
        std::shared_ptr<class VertexShader>    m_pVertexShadowShader;
        std::shared_ptr<class ComputeShader>    m_pAnimationComputeShader;
        std::shared_ptr<class StructuredBuffer> m_pAnimPaletteBuffer;
        std::shared_ptr<class StructuredBuffer> m_pBoneDataBuffer;
        std::shared_ptr<class StructuredBuffer> m_pFinalBuffer;
        BONECBUFFER m_tBoneCBuffer;
        std::shared_ptr<class ComputeCBuffer<BONECBUFFER>> m_pBoneConstBuffer;
        std::shared_ptr<class VertexCBuffer<BONECBUFFER>> m_pBoneVertexBuffer;
        std::shared_ptr<class Skeleton> m_pSkeleton;
        std::shared_ptr<class StructuredBuffer> m_pSkeletonBuffer;
        std::shared_ptr<class StructuredBuffer> m_pJointSocketBuffer;

    public:
        int GetCount()  const;
        void Clear();
        const std::list<class std::shared_ptr<class Drawable>>& GetRenderList()    const;
        void CreateBoneBuffer(const std::unordered_map<std::string, std::shared_ptr<class Sequence>>& mapSequence);
        void SetSkeleton(std::shared_ptr<class Skeleton> pBuffer);
        void SetJointSocketBuffer(std::shared_ptr<StructuredBuffer> pBuffer);

    private:
        void CreateInstBuffer();

    public:
        void AddDrawable(const class std::shared_ptr<Drawable>& pDrawable);
        void Update();
        void PreRender();
        void Draw();
        void Render();
        void RenderShadow();


    };

}