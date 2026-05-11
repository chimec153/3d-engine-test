#pragma once
#include "../GameObject/GameObject.h"
#include "../Component/Component.h"
#include "../Types.h"

namespace Engine
{
    class StructuredBuffer;
    template <typename T> class ConstantBuffer;
    class Texture;
    class Mesh;
    class Transform;
    class MeshRendererComponent;
    class Decal;
    class Collider;
    class Terrain;

    // Phase E5 — small Component that bridges a Terrain GameObject's
    // per-frame SRV / CB bind into the MeshRenderer pass via the
    // RenderBind / RenderUnbind hooks. Sits as a sibling of the
    // MeshRendererComponent on the Terrain GameObject.
    class ENGINE_DLL TerrainBindHook : public Component
    {
    public:
        TerrainBindHook() = default;
        virtual ~TerrainBindHook() override = default;

    public:
        virtual void RenderBind() override;
        virtual void RenderUnbind() override;
        virtual std::shared_ptr<Component> Clone() override
        {
            return std::make_shared<TerrainBindHook>(*this);
        }
    };

    class ENGINE_DLL Terrain :
        public GameObject
    {
    public:
        Terrain();
        virtual ~Terrain() override = default;

    private:
        // Phase E5 — Components on this GameObject.
        std::shared_ptr<Transform>             m_pTransform;
        std::shared_ptr<MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<TerrainBindHook>       m_pBindHook;

        std::shared_ptr<Mesh>   m_pMesh;
        std::vector<std::shared_ptr<Texture>> m_vecTexture;
        std::shared_ptr<ConstantBuffer<TERRAINCBUFFER>> m_pTerrainCBuffer;
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
        std::shared_ptr<StructuredBuffer> m_pTileTypeBuffer;
        std::vector<int> m_vecTileType;

    public:
        void CreateTerrain(int iWidth, int iHeight);
        void CreateTileTypeBuffer(int iWidth, int iHeight);
        void CreateTerrainTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainNormalTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainSpecularTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath);
        void CreateTerrainEmissiveTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath);
        void CreateHeightMap(const std::string& strTag, const TCHAR* pFilePath);

        // Phase E7 — wire an already-registered texture into the MeshRenderer
        // without creating a new one. Use these when GameScene (or another
        // owner) has already called StaticCreateBindable<Texture>(...) for
        // its own resource bookkeeping. The Create* counterparts above are
        // thin wrappers that delegate to these after creating-or-finding.
        void SetTerrainTexture(const std::shared_ptr<class Texture>& pTex);
        void SetTerrainNormalTexture(const std::shared_ptr<class Texture>& pTex);
        void SetTerrainSpecularTexture(const std::shared_ptr<class Texture>& pTex);
        void SetTerrainEmissiveTexture(const std::shared_ptr<class Texture>& pTex);
        void SetHeightMap(const std::shared_ptr<class Texture>& pTex);
        void SaveHeightMap(const TCHAR* pFilePath, const std::string& strPathKey = TEXTURE_PATH);
        void CreateMeshCollider();
        void GetPoints(std::vector<float>& vecPoints);
        void GetTris(std::vector<int>& vecTris);
        void SetEraseMode();
        void SetAddMode();
        bool IsEraseMode()  const;
        float GetTerrainHeight(const Vector3& vPos);
        void AddTerrainHeight(const Vector3& vPos, int iHeight = 1);
        int GetTerrainIndex(const Vector3& vLocalPos)   const;
        Vector3 GetTerrainLocalPos(const Vector3& vWorldPos)    const;
        void SetTileType(const Vector3& vWorldPos, int iTileType);
        int GetTileType(const Vector3& vWorldPos)   const;

        // Convenience accessor for callers (Player, Inventory, etc.) that
        // used to reach in via Drawable::GetTransform.
        std::shared_ptr<Transform> GetTransform() const { return m_pTransform; }

        // Phase E5 — invoked by TerrainBindHook (sibling Component) right
        // before / after the MeshRenderer's draw. SRV bind then unbind
        // around Mesh::Draw.
        void RenderBind();
        void RenderUnbind();

    private:
        void CreateVertexAndIndex(std::vector<VertexStandard>& vecVertex, std::vector<unsigned int>& vecIndex, int iWidth, int iHeight);

    public:
        virtual bool Init() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    public:
        void CollisionStay(Collider* pSrc, Collider* pDest, float fDeltaTime);
        void CollisionEnd(Collider* pSrc, Collider* pDest, float fDeltaTime);

    public:
        void EditHeightMapWithTexture(const Vector3& vCross);
        void SetBrushTexture(std::shared_ptr<Texture> pBrushTexture);
    };

}
