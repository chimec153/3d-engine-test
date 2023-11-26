#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Terrain :
        public Drawable
    {
    public:
        Terrain();
        virtual ~Terrain() override = default;

    private:
        std::shared_ptr<Mesh>   m_pMesh;
        std::vector<std::shared_ptr<Texture>> m_vecTexture;
        std::shared_ptr<ConstantBuffer<TERRAINCBUFFER>> m_pVSTerrainBuffer;
        std::shared_ptr<ConstantBuffer<TERRAINCBUFFER>> m_pPSTerrainBuffer;
        TERRAINCBUFFER m_tTerrainBuffer;
        std::shared_ptr<Texture> m_pHeightMap;
        std::vector<int>    m_vecHeight;
        bool    m_bEditting;
        float   m_fEditRange;
        std::vector<VertexStandard> m_vecVertex;
        std::vector<unsigned int> m_vecIndex;
        std::shared_ptr<Texture>    m_pBrushTexture;
        std::shared_ptr<class Decal> m_pDecal;
        bool m_bEraseMode;

    public:
        void CreateTerrain(int iWidth, int iHeight);
        void CreateTerrainTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainNormalTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainSpecularTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainEmissiveTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateBlendTerrainTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateHeightMap(const TCHAR* pFilePath);
        void SaveHeightMap(const TCHAR* pFilePath, const std::string& strPathKey = TEXTURE_PATH);
        void CreateMeshCollider();
        void GetPoints(std::vector<float>& vecPoints);
        void GetTris(std::vector<int>& vecTris);
        void SetEraseMode();
        void SetAddMode();
        bool IsEraseMode()  const;

    private:
        void CreateVertexAndIndex(std::vector<VertexStandard>& vecVertex, std::vector<unsigned int>& vecIndex, int iWidth, int iHeight);

    public:
        virtual void Bind() override;

    public:
        void CollisionStay(Collider* pSrc, Collider* pDest, float fDeltaTime);
        void CollisionEnd(Collider* pSrc, Collider* pDest, float fDeltaTime);

    public:
        void EditHeightMapWithTexture(const Vector3& vCross);
        void SetBrushTexture(std::shared_ptr<Texture> pBrushTexture);
    };

}