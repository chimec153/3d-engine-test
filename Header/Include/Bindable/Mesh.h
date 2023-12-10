#pragma once
#include "Bindable.h"
#include "Texture.h"
#include "Material.h"

namespace Engine
{
    ENGINE_DLL typedef struct _tagIndexBuffer
    {
        class CPtr<ID3D11Buffer> pBuffer;
        int iCount;
        DXGI_FORMAT eFormat;
#ifdef _DEBUG
        int iOffset;
#endif

        _tagIndexBuffer(const CPtr<ID3D11Buffer>& pBuffer, int iCount, DXGI_FORMAT eFormat) :
            pBuffer(pBuffer)
            , iCount(iCount)
            , eFormat(eFormat)
#ifdef _DEBUG
            , iOffset(0)
#endif
        {
        }
    }INDEXBUFFER, * PINDEXBUFFER;

    ENGINE_DLL typedef struct _tagMeshContainer
    {
        CPtr<ID3D11Buffer> m_pVertexBuffer;
        std::vector<INDEXBUFFER> m_vecIndexBuffer;
        int m_iSize;
        int m_iCount;
        std::vector<std::shared_ptr<Texture>>   vecTexture;
        std::vector<std::shared_ptr<Material>> vecMaterial;
#ifdef _DEBUG
        bool bEnable;
#endif
        _tagMeshContainer() :
            m_iSize(0)
            , m_iCount(0)
#ifdef _DEBUG
            , bEnable(true)
#endif
        {
        }
    }MESHCONTAINER, * PMESHCONTAINER;
    class ENGINE_DLL Mesh :
        public Bindable
    {
    public:
        template <typename T, typename P>
        Mesh(const std::vector<T>& vecVertex, const std::vector<P>& vecIndex, D3D11_USAGE eUsage = D3D11_USAGE_IMMUTABLE) :
            Bindable()
        {
            SetBindableType(BINDABLE_TYPE::MESH);

            CreateMesh(vecVertex, vecIndex, eUsage);
        }
        template <typename T, typename P>
        Mesh(const std::vector<T>& vecVertex, const std::vector<std::vector<P>>& vecIndex, D3D11_USAGE eUsage = D3D11_USAGE_IMMUTABLE) :
            Bindable()
        {
            SetBindableType(BINDABLE_TYPE::MESH);

            CreateMesh(vecVertex, vecIndex, eUsage);
        }
        template <typename T, typename P>
        Mesh(const std::vector<std::vector<T>>& vecVertex, const std::vector<std::vector<std::vector<P>>>& vecIndex, D3D11_USAGE eUsage = D3D11_USAGE_IMMUTABLE) :
            Bindable()
        {
            SetBindableType(BINDABLE_TYPE::MESH);

            CreateMesh(vecVertex, vecIndex, eUsage);
        }
        Mesh(const char* pFileName, const std::string& strPathKey = MESH_PATH) :
            Bindable()
        {
            SetBindableType(BINDABLE_TYPE::MESH);

            LoadFromPath(pFileName, strPathKey);
        }
        Mesh(const Mesh& mesh) = delete;
        virtual ~Mesh() override = default;

    private:
        std::vector<MESHCONTAINER>  m_vecMeshContainer;
        Vector4 m_vBoundingSphereInfo;
        

    public:
        const Vector4& GetBoundingSphereInfo()  const;
        int GetMeshCount()  const;
        int GetMeshSubCount(int iIndex)   const;
#ifdef _DEBUG
        bool IsMeshEnabled(int iIndex)    const;
        void ToggleMesh(int iIndex);
        void SetMeshSubOffset(int iIndex, int iSubIndex, int iOffset);
        int GetMeshSubOffset(int iIndex, int iSubIndex) const;
#endif

    public:
        void SetTextures(const std::vector<std::vector<std::shared_ptr<Texture>>>& vecTexture);
        void SetTextures(int iIndex, const std::vector<std::shared_ptr<Texture>>& vecTexture);
        void SetMaterial(int iIndex, const std::shared_ptr<Material>& pMaterial);
        std::shared_ptr<Material> GetMaterial(int iIndex = 0, int iSubIndex = 0)   const;

        template <typename T, typename P>
        void CreateMesh(const std::vector<std::vector<T>>& vecVertex, const std::vector<std::vector<std::vector<P>>>& vecIndex, D3D11_USAGE eUsage = D3D11_USAGE_IMMUTABLE)
        {
            for (int i = 0; i < vecVertex.size(); ++i)
            {
                CreateMesh(vecVertex[i], vecIndex[i], eUsage);
            }
        }

        template <typename T, typename P>
        void CreateMesh(const std::vector<T>& vecVertex, const std::vector<P>& vecIndex, D3D11_USAGE eUsage = D3D11_USAGE_IMMUTABLE)
        {
            std::vector<std::vector<P>> _vecIndex;

            _vecIndex.push_back(vecIndex);

            CreateMesh(vecVertex, _vecIndex, eUsage);
        }

        template <typename T, typename P>
        void CreateMesh(const std::vector<T>& vecVertex, const std::vector<std::vector<P>>& vecIndex, D3D11_USAGE eUsage = D3D11_USAGE_IMMUTABLE)
        {
            m_vecMeshContainer.emplace_back();

            MESHCONTAINER& container = m_vecMeshContainer.back();

            D3D11_BUFFER_DESC tVertexDesc = {};

            tVertexDesc.ByteWidth = static_cast<unsigned int>(sizeof(T) * vecVertex.size());
            tVertexDesc.Usage = eUsage;
            tVertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            tVertexDesc.StructureByteStride = sizeof(T);

            switch (eUsage)
            {
            case D3D11_USAGE_DYNAMIC:
                tVertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                break;
            case D3D11_USAGE_STAGING:
                tVertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                break;
            }

            D3D11_SUBRESOURCE_DATA tVertexData = {};

            tVertexData.pSysMem = &vecVertex[0];

            if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tVertexDesc, &tVertexData, &container.m_pVertexBuffer)))
            {
                m_vecMeshContainer.pop_back();
                assert(false);
                return;
            }

            for (int i = 0; i < static_cast<int>(vecIndex.size()); ++i)
            {
                CPtr<ID3D11Buffer> pIndexBuffer = nullptr;

                if (vecIndex[i].size())
                {
                    D3D11_BUFFER_DESC tIndexDesc = {};

                    tIndexDesc.ByteWidth = static_cast<unsigned int>(4 * vecIndex[i].size());
                    tIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                    tIndexDesc.StructureByteStride = 4;
                    tIndexDesc.Usage = eUsage;

                    switch (eUsage)
                    {
                    case D3D11_USAGE_DYNAMIC:
                        tIndexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                        break;
                    case D3D11_USAGE_STAGING:
                        tIndexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                        break;
                    }

                    D3D11_SUBRESOURCE_DATA tIndexData = {};

                    tIndexData.pSysMem = &vecIndex[i][0];

                    if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tIndexDesc, &tIndexData, &pIndexBuffer)))
                    {
                        m_vecMeshContainer.pop_back();
                        assert(false);
                        return;
                    }

                    container.m_vecIndexBuffer.emplace_back(pIndexBuffer, static_cast<int>(vecIndex[i].size()), DXGI_FORMAT_R32_UINT);
                }
                else
                {
                    container.m_vecIndexBuffer.emplace_back(pIndexBuffer, static_cast<int>(vecIndex[i].size()), DXGI_FORMAT_UNKNOWN);
                }
            }

            container.m_iCount = static_cast<int>(vecVertex.size());
            container.m_iSize = sizeof(T);
        }

    public:
        bool SetVertexBuffer(int iIndex, const void* pData, int iSize);
        bool SetIndexBuffer(int iIndex, int iSubIndex, const void* pData, int iSize);

    public:
        virtual void Bind() override;
        void Draw();
        void DrawInst(int iCount, int iSize, const CPtr<ID3D11Buffer>& pInstBuffer);
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        virtual void Load(FILE* pFile) override;

    };

}