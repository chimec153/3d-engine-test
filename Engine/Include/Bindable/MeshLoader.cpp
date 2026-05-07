#include "MeshLoader.h"
#include "Drawable.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"

namespace Engine
{
    MeshLoader::Result MeshLoader::Load(const TCHAR* pFileName, const std::string& strPathKey)
    {
        Drawable::LoadedMeshResources r = Drawable::LoadObjResources(pFileName, strPathKey);

        Result out;
        out.pMesh      = r.pMesh;
        out.pMaterial  = r.pMaterial;
        out.vecTexture = r.vecTexture;
        return out;
    }

    void MeshLoader::LoadInto(const TCHAR* pFileName,
                              const std::string& strPathKey,
                              const std::shared_ptr<MeshRendererComponent>& pTarget)
    {
        Drawable::LoadIntoMeshRenderer(pFileName, strPathKey, pTarget);
    }
}
