#pragma once

namespace Engine
{
    class Mesh;
    class Material;
    class Texture;
    class MeshRendererComponent;

    // Phase E7 — MeshLoader is a stateless facade for FBX/OBJ ingestion.
    // Consumers (LoadingThread, GameObject-based game classes, etc.) get a
    // load API that doesn't expose Drawable. The actual parser still lives
    // in Drawable::LoadOBJ / Drawable::LoadFBX for the time being; Phase C
    // will physically move the parser bodies here, after which Drawable.h
    // can be retired entirely.
    class ENGINE_DLL MeshLoader
    {
    public:
        struct Result
        {
            std::shared_ptr<Mesh>                  pMesh;
            std::shared_ptr<Material>              pMaterial;
            std::vector<std::shared_ptr<Texture>>  vecTexture;
        };

        // Load a .obj / .fbx file into a result struct. Mesh / Material /
        // Texture get registered in BindableManager during the load (so
        // they outlive the call); the returned shared_ptrs let the caller
        // wire them into a MeshRendererComponent or other consumer.
        static Result Load(const TCHAR* pFileName, const std::string& strPathKey = MESH_PATH);

        // Load a .obj / .fbx and install all Bindable children
        // (Mesh / Material / Texture / VS / PS / IL / Topology / etc.) onto
        // the target MeshRendererComponent via its AddBindable router.
        // Recommended path for game classes migrating off Drawable —
        // Drawable's load pipeline adds default shaders + pipeline state
        // that the MeshRenderer pass needs in order to actually draw.
        static void LoadInto(const TCHAR* pFileName,
                             const std::string& strPathKey,
                             const std::shared_ptr<MeshRendererComponent>& pTarget);
    };
}
