#pragma once
#include "../Component/Component.h"
namespace Engine
{
    // Phase E5 — SkyBox migrated from Drawable to Component. Currently
    // unused at runtime (no live construction path); kept as a usable
    // shell for future GameObject-based skybox setup. Owner GameObject
    // is expected to also hold a Transform; Update follows the active
    // camera's position by writing to that Transform.
    class ENGINE_DLL SkyBox :
        public Component
    {
    public:
        SkyBox();
        SkyBox(const TCHAR* pTexturePath, const std::string& strKey = TEXTURE_PATH);
        SkyBox(const SkyBox& other);
        virtual ~SkyBox() override = default;

    private:
        std::shared_ptr<class Mesh>          m_pMesh;
        std::shared_ptr<class VertexShader>  m_pVS;
        std::shared_ptr<class PixelShader>   m_pPS;
        std::shared_ptr<class Topology>      m_pTopology;
        std::shared_ptr<class InputLayout>   m_pInputLayout;
        std::shared_ptr<class Texture>       m_pTexture;

    public:
        void SetMesh(const std::shared_ptr<class Mesh>& p);
        void SetVertexShader(const std::shared_ptr<class VertexShader>& p);
        void SetPixelShader(const std::shared_ptr<class PixelShader>& p);
        void SetTopology(const std::shared_ptr<class Topology>& p);
        void SetInputLayout(const std::shared_ptr<class InputLayout>& p);
        void SetTexture(const std::shared_ptr<class Texture>& p);

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Called explicitly by RenderManager::RenderSkyBox.
        void Bind();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}
