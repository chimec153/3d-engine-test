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
        std::shared_ptr<VertexCBuffer<TERRAINCBUFFER>> m_pVSTerrainBuffer;
        std::shared_ptr<PixelCBuffer<TERRAINCBUFFER>> m_pPSTerrainBuffer;
        TERRAINCBUFFER m_tTerrainBuffer;
        std::shared_ptr<Texture> m_pHeightMap;

    public:
        void CreateTerrain(int iWidth, int iHeight);
        void CreateTerrainTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainNormalTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainSpecularTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainEmissiveTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateBlendTerrainTexture(const std::vector<const TCHAR*>& vecFullPath);
        void CreateHeightMap(const TCHAR* pFilePath);

    public:
        virtual void Bind() override;
    };

}