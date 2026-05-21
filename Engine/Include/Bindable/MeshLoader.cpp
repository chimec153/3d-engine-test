// Phase E7 — physical home of the FBX/OBJ parser. The bodies that
// previously lived as Drawable::LoadOBJ / LoadFBX / LoadOBJMaterial /
// LoadOBJMaterialFromFullPath / SaveMesh / LoadMesh have been migrated
// onto a Drawable-free MeshLoadContext below. The MeshLoader public
// facade (Load / LoadInto) drives the parsers via MeshLoadContext and
// hands the resulting Bindables to the caller (a result struct or a
// MeshRendererComponent). Drawable is no longer involved in mesh
// ingestion — this TU is what unblocks deleting Drawable entirely.
#include "MeshLoader.h"
#include "Bindable.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "Transform.h"
#include "FbxLoader.h"
#include "PixelShader.h"
#include "VertexShader.h"
#include "InputLayout.h"
#include "Topology.h"
#include "Animation.h"
#include "MeshUtils.h"
#include "../Animation/Sequence.h"
#include "../Animation/Skeleton.h"
#include "../Component/MeshRendererComponent.h"
#include "../Shader/ShaderManager.h"
#include "../Core/PathManager.h"
#include "BindableManager.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Engine
{
    namespace
    {
        // ------------------------------------------------------------------
        // MaterialInfo — Drawable::MATERIALINFO replacement, scoped to this
        // TU. Used only by the OBJ .mtl parser to carry a Material + its
        // Textures across the parse.
        // ------------------------------------------------------------------
        struct MaterialInfo
        {
            std::shared_ptr<Material>                   pMaterial;
            std::vector<std::shared_ptr<class Texture>> vecTexture;
        };

        // ------------------------------------------------------------------
        // MeshLoadContext — accumulates parser output as the OBJ/FBX
        // parsers run. Mirrors the slice of Drawable's surface that the
        // parser bodies actually used (AddChild routing, FindAndAddBind,
        // SetBoundingSphereInfo / GetBoundingSphere, m_pAnimation +
        // CreateComponent<Animation>, AddSeqeunces, GetTag/SetTag) so the
        // bodies can swap `this` from Drawable to MeshLoadContext with
        // minimal changes. After parsing, MeshLoader::Load /
        // MeshLoader::LoadInto drains the accumulated Bindables out into
        // the caller's storage.
        // ------------------------------------------------------------------
        class MeshLoadContext
        {
        public:
            // Tag assigned to the parsed Mesh (OBJ uses it via GetTag()).
            std::string                            m_strTag;
            std::shared_ptr<Mesh>                  m_pMesh;
            std::shared_ptr<Material>              m_pMaterial;
            std::vector<std::shared_ptr<Texture>>  m_vecTexture;
            std::shared_ptr<Animation>             m_pAnimation;
            // Everything else AddChild would have routed somewhere on
            // Drawable (VS / PS / IL / Topology / RS / etc.) lands here.
            std::vector<std::shared_ptr<Bindable>> m_vecOtherBindings;

            Vector4                                m_tSphereInfo{};
            BOUNDING_VOLUME_TYPE                   m_eBoundingVolumeType{};

            const std::string& GetTag() const { return m_strTag; }
            void SetTag(const std::string& s) { m_strTag = s; }

            void SetBoundingSphereInfo(const Vector4& v) { m_tSphereInfo = v; }
            const Vector4& GetSphereInfo() const { return m_tSphereInfo; }

            // Drawable::AddChild dispatch, simplified — we only care about
            // the routing slots the parsers populate. Anything else (IL,
            // Topology, RS, ...) ends up in m_vecOtherBindings and gets
            // forwarded to MeshRendererComponent::AddBindable later, which
            // will drop unrouted types into its own m_OtherBindables.
            void AddChild(const std::shared_ptr<Bindable>& pChild)
            {
                if (!pChild) return;

                switch (pChild->GetBindableType())
                {
                case BINDABLE_TYPE::MESH:
                    m_pMesh = std::static_pointer_cast<Mesh>(pChild);
                    break;
                case BINDABLE_TYPE::MATERIAL:
                    m_pMaterial = std::static_pointer_cast<Material>(pChild);
                    break;
                case BINDABLE_TYPE::TEXTURE:
                    m_vecTexture.push_back(std::static_pointer_cast<Texture>(pChild));
                    break;
                default:
                    m_vecOtherBindings.push_back(pChild);
                    break;
                }
            }

            void AddChild(const std::vector<std::shared_ptr<Bindable>>& vec)
            {
                for (const auto& p : vec)
                {
                    AddChild(p);
                }
            }

            template <typename T>
            std::shared_ptr<T> FindAndAddBind(const std::string& strTag)
            {
                std::shared_ptr<T> pBindable = StaticFindBindable<T>(strTag);

                if (pBindable == nullptr)
                {
                    assert(false);
                    return nullptr;
                }

                AddChild(pBindable);

                return pBindable;
            }

            // Mirror of Drawable's non-static GetBoundingSphere<T>(vector<T>) —
            // sets bounding volume type, computes via MeshUtils, records into
            // m_tSphereInfo, and returns it. Same semantics, no side-effects
            // on a Drawable instance.
            template <typename T>
            Vector4 GetBoundingSphere(const std::vector<T>& vecVertex)
            {
                m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::SPHERE;
                SetBoundingSphereInfo(MeshUtils::ComputeBoundingSphere(vecVertex));
                return m_tSphereInfo;
            }

            template <typename T>
            Vector4 GetBoundingSphere(const std::vector<std::vector<T>>& vecVertex)
            {
                m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::SPHERE;
                SetBoundingSphereInfo(MeshUtils::ComputeBoundingSphere(vecVertex));
                return m_tSphereInfo;
            }

            // Drawable::CreateComponent template, narrowed to the only path
            // the FBX parser exercises — Animation. The parsers use this to
            // establish m_pAnimation (used by AddSeqeunces and SetSkeleton).
            template <typename T, typename... Args>
            std::shared_ptr<T> CreateComponent(const std::string& strTag, Args... args)
            {
                std::shared_ptr<T> pComp = std::make_shared<T>(args...);
                if (!pComp) return nullptr;
                pComp->SetTag(strTag);
                if (!pComp->Init()) return nullptr;

                if constexpr (std::is_same_v<T, Animation>)
                {
                    m_pAnimation = std::static_pointer_cast<Animation>(pComp);
                }
                return pComp;
            }

            // Lifted verbatim from Drawable::AddSeqeunces — copies sequence
            // data out of FbxLoader and adds each into m_pAnimation.
            void AddSeqeunces(const std::vector<FbxLoader::SEQUENCE>& vecSequance, const std::string& strSeq = "")
            {
                for (size_t j = 0; j < vecSequance.size(); ++j)
                {
                    std::shared_ptr<Sequence> pSequence = std::make_shared<Sequence>();

                    std::vector<FbxLoader::FBXBONEKEYFRAME> vecKeyFrame;

                    pSequence->SetTag(vecSequance[j].strTag + strSeq);

                    if (vecKeyFrame.size() < vecSequance[j].vecBoneKeyFrame.size())
                    {
                        vecKeyFrame.resize(vecSequance[j].vecBoneKeyFrame.size());
                    }

                    for (size_t k = 0; k < vecSequance[j].vecBoneKeyFrame.size(); ++k)
                    {
                        for (size_t m = 0; m < vecSequance[j].vecBoneKeyFrame[k].vecKeyFrame.size(); ++m)
                        {
                            vecKeyFrame[k].vecKeyFrame.push_back(vecSequance[j].vecBoneKeyFrame[k].vecKeyFrame[m]);
                        }
                    }

                    if (vecKeyFrame.empty())
                    {
                        continue;
                    }

                    if (!pSequence->SetSequance(vecKeyFrame))
                    {
                        continue;
                    }

                    m_pAnimation->AddSequance(pSequence->GetTag(), pSequence);
                }
            }

            // Parser entry points (formerly Drawable::LoadOBJ / LoadFBX).
            void ParseOBJ(const TCHAR* pFileName, const std::string& strPathKey);
            void ParseFBX(const TCHAR* pFileName, const std::string& strPathKey);
        };

        // ------------------------------------------------------------------
        // OBJ .mtl parser. Kept as free helpers (they don't touch any
        // Drawable / MeshLoadContext state) — `LoadOBJMaterialFromFullPath`
        // is invoked from ParseOBJ by name, mirroring the original Drawable
        // member-call relationship.
        // ------------------------------------------------------------------
        std::vector<MaterialInfo> LoadOBJMaterialFromFullPath(const char* strFullPath)
        {
            std::vector<MaterialInfo> vecMaterial;

            MaterialInfo* pCurrentMaterial = nullptr;

            // Returns a pointer into `path` starting at the immediate
            // parent directory name (one separator back from the filename).
            // Example: "C:\Resource\Texture\Decal\brick.png" → "Decal\brick.png".
            // Two files with the same filename in different folders no
            // longer collide as long as the parent dir differs. The full
            // path is still used as the file-load argument.
            auto FileNameWithParent = [](const char* path) -> const char*
            {
                const char* last = strrchr(path, '\\');
                if (!last) last = strrchr(path, '/');
                if (!last) return path;

                const char* prev = nullptr;
                for (const char* p = last - 1; p >= path; --p)
                {
                    if (*p == '\\' || *p == '/') { prev = p; break; }
                }
                return prev ? (prev + 1) : path;
            };

            FILE* pFile = nullptr;

            fopen_s(&pFile, strFullPath, "rt");

            if (pFile)
            {
                while (true)
                {
                    char _strLine[MAX_PATH] = {};

                    fgets(_strLine, MAX_PATH, pFile);

                    char* strLine = _strLine;

                    if (!strcmp(strLine, ""))
                    {
                        break;
                    }

                    if (strLine[0] == '#')
                    {
                        continue;
                    }

                    else if (strLine[0] == '\t')
                    {
                        ++strLine;
                    }

                    char* pContext = nullptr;

                    char* pResult = strtok_s(strLine, " ", &pContext);

                    if (!strcmp(pResult, "newmtl"))
                    {
                        vecMaterial.emplace_back();

                        pCurrentMaterial = &vecMaterial.back();

                        if (pContext)
                        {
                            pContext[strlen(pContext) - 1] = 0;

                            pCurrentMaterial->pMaterial = StaticCreateBindable<Material>(pContext);

                            if (!pCurrentMaterial->pMaterial)
                            {
                                pCurrentMaterial->pMaterial = StaticFindBindable<Material>(pContext);
                            }
                        }
                    }

                    else if (!strcmp(pResult, "Ns"))
                    {
                        pCurrentMaterial->pMaterial->SetShininess(static_cast<float>(atof(pContext)));
                    }

                    else if (!strcmp(pResult, "Ka"))
                    {
                        Vector3 vColor;

                        for (int i = 0; i < 3; ++i)
                        {
                            char* pResult = strtok_s(nullptr, " ", &pContext);

                            vColor[i] = static_cast<float>(atof(pResult));
                        }

                        pCurrentMaterial->pMaterial->SetAmbientColor(vColor.x, vColor.y, vColor.z, 1.f);
                    }

                    else if (!strcmp(pResult, "Kd"))
                    {
                        Vector3 vColor;

                        for (int i = 0; i < 3; ++i)
                        {
                            char* pResult = strtok_s(nullptr, " ", &pContext);

                            vColor[i] = static_cast<float>(atof(pResult));
                        }

                        pCurrentMaterial->pMaterial->SetDiffuseColor(vColor.x, vColor.y, vColor.z, 1.f);
                    }

                    else if (!strcmp(pResult, "Ks"))
                    {
                        Vector3 vColor;

                        for (int i = 0; i < 3; ++i)
                        {
                            char* pResult = strtok_s(nullptr, " ", &pContext);

                            vColor[i] = static_cast<float>(atof(pResult));
                        }

                        // Skip when the file states no specular at all. Most
                        // metalness-workflow assets ship `Ks 0 0 0`; honouring
                        // that would zero out the engine's dielectric F0
                        // default (0.04) and remove specular highlights.
                        if (vColor.x != 0.f || vColor.y != 0.f || vColor.z != 0.f)
                        {
                            pCurrentMaterial->pMaterial->SetSpecularColor(vColor.x, vColor.y, vColor.z, 1.f);
                        }
                    }

                    else if (!strcmp(pResult, "Ke"))
                    {
                        Vector4 vColor = { 0.f, 0.f, 0.f, 1.f };

                        for (int i = 0; i < 3; ++i)
                        {
                            char* pResult = strtok_s(nullptr, " ", &pContext);

                            vColor[i] = static_cast<float>(atof(pResult));
                        }

                        pCurrentMaterial->pMaterial->SetEmissiveColor(vColor);
                    }

                    else if (!strcmp(pResult, "map_Kd"))
                    {
                        pResult = strtok_s(nullptr, " ", &pContext);

                        char pFullPath[MAX_PATH] = {};

                        strcpy_s(pFullPath, strFullPath);

                        int iLength = static_cast<int>(strlen(pFullPath));

                        for (int i = iLength - 1; i >= 0; --i)
                        {
                            if (pFullPath[i] == '/' || pFullPath[i] == '\\')
                            {
                                memset(pFullPath + i + 1, 0, iLength - i);
                                break;
                            }
                        }

                        strcat_s(pFullPath, pResult);

                        pFullPath[strlen(pFullPath) - 1] = 0;

                        const char* pTag = FileNameWithParent(pFullPath);
                        std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(pTag);

                        if (pTexture == nullptr)
                        {
                            pTexture = StaticCreateBindable<Texture>(pTag, pFullPath);
                        }

                        pCurrentMaterial->vecTexture.push_back(pTexture);
                    }

                    else if (!strcmp(pResult, "map_Kn"))
                    {
                        pResult = strtok_s(nullptr, " ", &pContext);

                        char pFullPath[MAX_PATH] = {};

                        strcpy_s(pFullPath, strFullPath);

                        int iLength = static_cast<int>(strlen(pFullPath));

                        for (int i = iLength - 1; i >= 0; --i)
                        {
                            if (pFullPath[i] == '/' || pFullPath[i] == '\\')
                            {
                                memset(pFullPath + i + 1, 0, iLength - i);
                                break;
                            }
                        }

                        strcat_s(pFullPath, pResult);

                        pFullPath[strlen(pFullPath) - 1] = 0;

                        const char* pTag = FileNameWithParent(pFullPath);
                        std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(pTag);

                        if (pTexture == nullptr)
                        {
                            pTexture = StaticCreateBindable<Texture>(pTag, pFullPath, 1);
                        }

                        pCurrentMaterial->vecTexture.push_back(pTexture);
                    }

                    else if (!strcmp(pResult, "map_Ks"))
                    {
                        pResult = strtok_s(nullptr, " ", &pContext);

                        char pFullPath[MAX_PATH] = {};

                        strcpy_s(pFullPath, strFullPath);

                        int iLength = static_cast<int>(strlen(pFullPath));

                        for (int i = iLength - 1; i >= 0; --i)
                        {
                            if (pFullPath[i] == '/' || pFullPath[i] == '\\')
                            {
                                memset(pFullPath + i + 1, 0, iLength - i);
                                break;
                            }
                        }

                        strcat_s(pFullPath, pResult);

                        pFullPath[strlen(pFullPath) - 1] = 0;

                        const char* pTag = FileNameWithParent(pFullPath);
                        std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(pTag);

                        if (pTexture == nullptr)
                        {
                            pTexture = StaticCreateBindable<Texture>(pTag, pFullPath, 2);
                        }

                        pCurrentMaterial->vecTexture.push_back(pTexture);
                    }
                }

                fclose(pFile);
            }

            return vecMaterial;
        }

        // ------------------------------------------------------------------
        // SaveMesh / LoadMesh — disk persistence helpers. They never read
        // or write a Drawable / MeshLoadContext field; pure free helpers.
        // ------------------------------------------------------------------
        void SaveMesh(const std::vector<std::vector<VertexStandard>>& vecVertex, const std::vector<std::vector<std::vector<unsigned int>>>& vecIndex,
            const std::vector<std::vector<std::shared_ptr<Texture>>>& vecTexture, const std::vector<std::vector<std::shared_ptr<Material>>>& vecMaterial,
            const char* pFilePath, const std::string& strPathKey = MESH_PATH)
        {
            char strFullPath[MAX_PATH] = {};

            CPathManager::GetInst()->ResolveMB(pFilePath, strPathKey, strFullPath);

            FILE* pFile = nullptr;

            fopen_s(&pFile, strFullPath, "wb");

            if (pFile)
            {
                // New format — magic + version header, no per-container
                // textures (each Material::Save embeds its own). MeshLoader's
                // FBX import path distributed `vecTexture` into the materials
                // earlier, so `vecMaterial` already carries the textures.
                // The `vecTexture` argument here is preserved only to keep
                // the call signature compatible.
                static constexpr uint32_t kMeshMagic   = 0x4853454D;  // 'MESH'
                static constexpr uint32_t kMeshVersion = 1;
                fwrite(&kMeshMagic,   4, 1, pFile);
                fwrite(&kMeshVersion, 4, 1, pFile);
                (void)vecTexture;

                int iContainerSize = static_cast<int>(vecVertex.size());

                fwrite(&iContainerSize, 4, 1, pFile);

                for (int i = 0; i < iContainerSize; ++i)
                {
                    int iVertexSize = static_cast<int>(vecVertex[i].size());

                    fwrite(&iVertexSize, 4, 1, pFile);
                    fwrite(&vecVertex[i][0], sizeof(VertexStandard), vecVertex[i].size(), pFile);

                    short iIndexSize = static_cast<short>(vecIndex[i].size());

                    fwrite(&iIndexSize, 2, 1, pFile);

                    for (int j = 0; j < iIndexSize; ++j)
                    {
                        int iIndexCount = static_cast<int>(vecIndex[i][j].size());

                        fwrite(&iIndexCount, 4, 1, pFile);

                        if (iIndexCount)
                        {
                            fwrite(&vecIndex[i][j][0], 4, vecIndex[i][j].size(), pFile);
                        }
                    }

                    int iMaterial = static_cast<int>(vecMaterial[i].size());

                    fwrite(&iMaterial, 1, 4, pFile);

                    for (int j = 0; j < iMaterial; ++j)
                    {
                        if (vecMaterial[i][j])
                        {
                            vecMaterial[i][j]->Save(pFile);
                        }
                        else
                        {
                            assert(false);
                        }
                    }
                }

                fclose(pFile);
            }
        }

        // ------------------------------------------------------------------
        // OBJ parser. Reads vertices / uvs / normals / faces from an .obj
        // file and an associated .mtl file referenced via mtllib. Builds a
        // Mesh with one or more sub-groups (one per usemtl block); attaches
        // a default shader chain via ShaderManager based on the texture
        // bitset (diffuse / normal / spec). The parsed Mesh / Material /
        // Texture all land in BindableManager.
        // ------------------------------------------------------------------
        void MeshLoadContext::ParseOBJ(const TCHAR* pFileName, const std::string& strPathKey)
        {
            TCHAR strFullPath[MAX_PATH] = {};

            CPathManager::GetInst()->Resolve(pFileName, strPathKey, strFullPath);

            char strFull[MAX_PATH] = {};

#ifdef UNICODE
            WideCharToMultiByte(CP_ACP, 0, strFullPath, -1, strFull, MAX_PATH, 0, 0);
#else
            strcpy_s(strFull, strFullPath);
#endif

            FILE* pFile = nullptr;

            fopen_s(&pFile, strFull, "rt");

            if (pFile)
            {
                bool bSame = true;

                std::vector<VertexStandard> vecVertex;
                std::vector<std::vector<VertexStandard>> vecTotalVertex;
                std::vector<Vector3> vecPos;
                std::vector<unsigned int> vecSubIndex;
                std::vector<std::vector<unsigned int>> vecIndex;
                std::vector<std::vector<std::vector<unsigned int>>> vecTotalIndex;

                std::vector<std::shared_ptr<Texture>> vecTexture;
                std::vector<std::vector<std::shared_ptr<Texture>>> vecTotalTexture;

                std::vector<DirectX::XMFLOAT2> vecUV;
                std::vector<Vector3> vecNormal;

                bool bHasNormal = false;
                bool bHasUV = false;
                int iNormalCount = 0;

                std::vector<MaterialInfo> vecMaterial;

                std::vector<MaterialInfo> vecUseMaterial;

                int iPrevPos = 0;

                int iPrevUV = 0;

                int iPrevNormal = 0;

                bool bPath = true;

                int iTexture = 0;

                while (bPath)
                {
                    char strLine[MAX_PATH] = {};

                    char* pResult = fgets(strLine, MAX_PATH, pFile);

                    if (!pResult || !strcmp(pResult, ""))
                    {
                        break;
                    }

                    switch (pResult[0])
                    {
                    case '#':
                        break;
                    case 'n':
                    case 'o':
                    case 'g':
                    {
                        char* pContext = nullptr;

                        char* _pResult = strtok_s(pResult, " ", &pContext);

                        if (pContext)
                        {
                            pContext[strlen(pContext) - 1] = 0;

                            if (vecSubIndex.size())
                            {
                                iPrevPos = static_cast<int>(vecPos.size());
                                iPrevUV = static_cast<int>(vecUV.size());
                                iPrevNormal = static_cast<int>(vecNormal.size());

                                vecIndex.push_back(vecSubIndex);

                                std::vector<std::vector<unsigned int>> _vecSubIndex;

                                _vecSubIndex.push_back(vecSubIndex);

                                vecTotalIndex.push_back(_vecSubIndex);

                                vecTotalTexture.push_back(vecTexture);

                                vecTotalVertex.push_back(vecVertex);

                                vecVertex.clear();

                                vecSubIndex.clear();

                                vecTexture.clear();
                            }
                        }
                    }
                    break;
                    case 'v':

                        switch (pResult[1])
                        {
                        case 't':
                        {
                            bHasUV = true;

                            char* pContext = nullptr;
                            char* _pResult = strtok_s(pResult, " ", &pContext);

                            _pResult = strtok_s(nullptr, " ", &pContext);

                            float fU = (float)atof(_pResult);

                            _pResult = strtok_s(nullptr, " ", &pContext);

                            float fV = (float)atof(_pResult);

                            vecUV.push_back({ fU, fV });
                        }
                        break;
                        case 'n':
                        {
                            bHasNormal = true;

                            char* pContext = nullptr;
                            char* _pResult = strtok_s(pResult, " ", &pContext);

                            Vector3 vNormal;

                            for (int i = 0; i < 3; ++i)
                            {
                                _pResult = strtok_s(nullptr, " ", &pContext);

                                vNormal[i] = (float)atof(_pResult);
                            }

                            vecNormal.push_back(vNormal);

                            ++iNormalCount;
                        }
                        break;
                        default:
                        {
                            char* pContext = nullptr;
                            char* _pResult = strtok_s(pResult, " ", &pContext);

                            Vector3 position;

                            for (int i = 0; i < 3; ++i)
                            {
                                _pResult = strtok_s(nullptr, " ", &pContext);

                                position[i] = (float)atof(_pResult);
                            }

                            vecPos.push_back(position);
                        }
                        break;
                        }
                        break;
                    case 'f':
                    {
                        char* pContext = nullptr;
                        char* _pResult = strtok_s(pResult, " ", &pContext);

                        std::vector<int> vecVertexSub;

                        while (true)
                        {
                            _pResult = strtok_s(nullptr, " ", &pContext);

                            if (!_pResult)
                            {
                                break;
                            }

                            char* _pContext = nullptr;

                            char* __pResult = strtok_s(_pResult, "/", &_pContext);

                            char* __pResult2 = strtok_s(nullptr, "/", &_pContext);

                            unsigned int iVertex = atoi(__pResult);

                            if (!iVertex)
                            {
                                break;
                            }

                            unsigned int iIndex = 0;

                            if (__pResult2)
                            {
                                iIndex = atoi(__pResult2);
                            }

                            unsigned int iNormalIndex = 0;

                            if (_pContext)
                            {
                                iNormalIndex = atoi(_pContext);
                            }

                            if (bSame &&
                                vecPos.size() - iPrevPos == vecUV.size() - iPrevUV &&
                                vecNormal.size() - iPrevNormal == vecUV.size() - iPrevUV)
                            {
                                if (vecVertex.size() < iVertex)
                                {
                                    vecVertex.resize(iVertex);
                                }

                                vecVertex[iVertex - 1].pos = vecPos[iVertex - 1];

                                if (bHasUV && iIndex)
                                {
                                    vecVertex[iVertex - 1].uv.x = vecUV[iIndex - 1].x;
                                    vecVertex[iVertex - 1].uv.y = 1.f - vecUV[iIndex - 1].y;
                                }

                                if (bHasNormal && iNormalIndex)
                                {
                                    vecVertex[iVertex - 1].normal = vecNormal[iNormalIndex - 1];
                                }

                                vecVertexSub.push_back(iVertex - 1);
                            }
                            else
                            {
                                bSame = false;

                                VertexStandard vVertex;

                                vVertex.pos = vecPos[iVertex - 1];

                                if (bHasUV)
                                {
                                    vVertex.uv.x = vecUV[iIndex - 1].x;
                                    vVertex.uv.y = 1.f - vecUV[iIndex - 1].y;
                                }

                                if (bHasNormal)
                                {
                                    vVertex.normal = vecNormal[iNormalIndex - 1];
                                }

                                vecVertexSub.push_back(static_cast<int>(vecVertex.size()));

                                vecVertex.push_back(vVertex);
                            }
                        }

                        // 0 1 2
                        // 0 1 2 0 2 3
                        // 0 1 2 0 2 3 0 3 4

                        for (int i = 0; i < vecVertexSub.size() - 2; ++i)
                        {
                            vecSubIndex.push_back(vecVertexSub[0]);
                            vecSubIndex.push_back(vecVertexSub[i + 1]);
                            vecSubIndex.push_back(vecVertexSub[i + 2]);
                        }
                    }
                    break;
                    case 'm':
                    {
                        char* pContext = nullptr;

                        char* _pResult = strtok_s(pResult, " ", &pContext);

                        if (!strcmp(_pResult, "mtllib"))
                        {
                            int iLength = static_cast<int>(strlen(strFull));

                            for (int i = iLength - 1; i >= 0; --i)
                            {
                                if (strFull[i] == '/' || strFull[i] == '\\')
                                {
                                    memset(strFull + i + 1, 0, iLength - i);
                                    break;
                                }
                            }

                            if (pContext[0] == '.' && pContext[1] == '/')
                            {
                                strcat_s(strFull, &pContext[2]);
                            }
                            else
                            {
                                strcat_s(strFull, pContext);
                            }

                            strFull[strlen(strFull) - 1] = 0;

                            vecMaterial = LoadOBJMaterialFromFullPath(strFull);
                        }
                    }
                    break;
                    case 'u':
                    {
                        char* pContext = nullptr;

                        char* _pResult = strtok_s(pResult, " ", &pContext);

                        if (!strcmp(_pResult, "usemtl"))
                        {
                            if (vecSubIndex.size())
                            {
                                iPrevPos = static_cast<int>(vecPos.size());
                                iPrevUV = static_cast<int>(vecUV.size());
                                iPrevNormal = static_cast<int>(vecNormal.size());

                                vecIndex.push_back(vecSubIndex);

                                std::vector<std::vector<unsigned int>> _vecSubIndex;

                                _vecSubIndex.push_back(vecSubIndex);

                                vecTotalIndex.push_back(_vecSubIndex);

                                vecTotalTexture.push_back(vecTexture);

                                vecTotalVertex.push_back(vecVertex);

                                vecVertex.clear();

                                vecSubIndex.clear();

                                vecTexture.clear();
                            }

                            bool bFind = false;

                            pContext[strlen(pContext) - 1] = 0;

                            for (size_t i = 0; i < vecMaterial.size(); ++i)
                            {
                                if (vecMaterial[i].pMaterial->GetTag() == pContext)
                                {
                                    bFind = true;

                                    vecUseMaterial.push_back(vecMaterial[i]);

                                    const std::shared_ptr<Material>& pMaterial = std::static_pointer_cast<Material>(vecMaterial[i].pMaterial->Clone());

                                    for (size_t j = 0; j < vecMaterial[i].vecTexture.size(); ++j)
                                    {
                                        iTexture |= 1 << static_cast<std::shared_ptr<Texture>>(vecMaterial[i].vecTexture[j])->GetSlot();

                                        vecTexture.push_back(vecMaterial[i].vecTexture[j]);
                                    }

                                    break;
                                }
                            }

                            assert(bFind);
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }

                if (vecVertex.size())
                {
                    vecTotalVertex.push_back(vecVertex);
                }

                if (vecSubIndex.size())
                {
                    vecIndex.push_back(vecSubIndex);

                    std::vector<std::vector<unsigned int>> _vecSubIndex;

                    _vecSubIndex.push_back(vecSubIndex);

                    vecTotalIndex.push_back(_vecSubIndex);
                }

                std::vector<unsigned int> _vecIndex;

                for (size_t i = 0; i < vecIndex.size(); ++i)
                {
                    size_t iSize = _vecIndex.size();
                    size_t iAddSize = vecIndex[i].size();

                    _vecIndex.resize(iSize + iAddSize);

                    memcpy_s(&_vecIndex[iSize], 4 * iAddSize, &vecIndex[i][0], 4 * iAddSize);
                }

                if (!bHasNormal)
                {
                    MeshUtils::SetNormals<VertexStandard>(vecTotalVertex, vecIndex);
                }

                MeshUtils::SetTangent<VertexStandard>(vecTotalVertex, vecIndex);

                std::shared_ptr<Mesh> pMesh = StaticCreateBindable<Mesh>(GetTag(), vecTotalVertex, vecTotalIndex);

                if (!pMesh)
                {
                    pMesh = StaticFindBindable<Mesh>(GetTag());
                }

                AddChild(pMesh);

                switch (iTexture)
                {
                case 0:
                {
                    //const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong_NoDiffuseNoNormalNoSpec");
                    const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoDiffuseNoNormalNoSpec");

                    if (pvecBindable)
                    {
                        AddChild(*pvecBindable);
                    }
                }
                break;
                case 1:
                {
                    const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoSpecNoNormal");
                    //const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong_NoNormalNoSpec");

                    if (pvecBindable)
                    {
                        AddChild(*pvecBindable);
                    }
                }
                break;
                case 1 | 2:
                {
                    const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoSpec");

                    if (pvecBindable)
                    {
                        AddChild(*pvecBindable);
                    }
                }
                break;
                case 1 | 4:
                {
                    const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet_NoNormal");
                    //const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong_NoNormal");

                    if (pvecBindable)
                    {
                        AddChild(*pvecBindable);
                    }
                }
                break;
                case 1 | 2 | 4:
                {
                    const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("anisotropic_microfacet");
                    //const std::vector<std::shared_ptr<Bindable>>* pvecBindable = ShaderManager::GetInst()->FindShader("Phong");

                    if (pvecBindable)
                    {
                        AddChild(*pvecBindable);
                    }
                }
                break;
                default:
                    assert(false);
                    break;
                }

                const Vector4& vSphereInfo = GetBoundingSphere(vecVertex);

                SetBoundingSphereInfo(vSphereInfo);

                FindAndAddBind<Topology>("TriangleList");

                for (int i = 0; i < static_cast<int>(vecUseMaterial.size()); ++i)
                {
                    vecUseMaterial[i].pMaterial->SetReflectivity(1.f);

                    pMesh->AddMaterial(i, vecUseMaterial[i].pMaterial);
                }

                fclose(pFile);
            }
        }

        // ------------------------------------------------------------------
        // FBX parser. Drives FbxLoader, builds Skeleton (saved to .skel),
        // creates one Mesh per LOD (saved to .mesh), populates Animation
        // (with sequences saved to .seq), wires textures + materials, and
        // chooses default shader/IL/Topology based on whether the file
        // carried skeletal data.
        // ------------------------------------------------------------------
        void MeshLoadContext::ParseFBX(const TCHAR* pFileName, const std::string& strPathKey)
        {
            FbxLoader loader;

            if (!loader.Init())
            {
                return;
            }

            char strFilePath[MAX_PATH] = {};

#ifdef UNICODE
            WideCharToMultiByte(CP_ACP, 0, pFileName, -1, strFilePath, MAX_PATH, nullptr, nullptr);
#else
            strcpy_s(strFilePath, pFileName);
#endif

            char strFileName[_MAX_FNAME] = {};

            char _strExt[_MAX_EXT] = {};

            _splitpath_s(strFilePath, nullptr, 0, nullptr, 0, strFileName, _MAX_FNAME, _strExt, _MAX_EXT);

            int iLength = static_cast<int>(strlen(strFilePath));

            for (int i = iLength - 1; i >= 0; --i)
            {
                if (strFilePath[i] == '/' || strFilePath[i] == '\\')
                {
                    // 11
                    // asdf/ad.wzt
                    // 01234567890
                    memset(strFilePath + i + 1, 0, iLength - i - 1);
                    break;
                }
            }

            loader.LoadFile(pFileName, strPathKey);

            const FbxLoader::SKELETON& tSkeleton = loader.GetSkeleton();

            std::shared_ptr<Skeleton> pSkeleton = nullptr;

            if (tSkeleton.vecBone.size())
            {
                pSkeleton = std::make_shared<Skeleton>();

                pSkeleton->SetTag(strFileName);

                pSkeleton->SetBone(tSkeleton.vecBone);

                char strSkelPath[MAX_PATH] = {};

                strcat_s(strSkelPath, strFileName);

                strcat_s(strSkelPath, ".skel");

                pSkeleton->SaveFromPath(strSkelPath, MESH_PATH);
            }

            if (!loader.GetLODCount())
            {
                assert(false);
                return;
            }

            std::vector<std::vector<VertexStandard>> vecVertex;
            std::vector<std::vector<std::vector<unsigned int>>> vecIndex;

            for (int i = 0; i < loader.GetLODCount(); ++i)
            {
                vecVertex.push_back(loader.GetVertexData(i));
                vecIndex.push_back(loader.GetIndexData(i));
            }

            std::shared_ptr<Mesh> pMesh = StaticCreateBindable<Mesh>(strFileName, vecVertex, vecIndex);

            if (!pMesh)
            {
                pMesh = StaticFindBindable<Mesh>(strFileName);
            }

            AddChild(pMesh);

            GetBoundingSphere<VertexStandard>(vecVertex);

            if (tSkeleton.vecBone.size())
            {
                CreateComponent<class Animation>("Animation");

                int iLodCount = loader.GetLODCount();

                for (int i = 0; i < iLodCount; ++i)
                {
                    const std::vector<FbxLoader::SEQUENCE>& vecSequance = loader.GetSequences(i);

                    AddSeqeunces(vecSequance);
                }

                const std::unordered_map<std::string, Animation::PSEQUENCEINFO>& mapSequence = m_pAnimation->GetSequences();

                std::unordered_map<std::string, Animation::PSEQUENCEINFO>::const_iterator iter = mapSequence.begin();
                std::unordered_map<std::string, Animation::PSEQUENCEINFO>::const_iterator iterEnd = mapSequence.end();

                for (; iter != iterEnd; ++iter)
                {
                    char strSeqPath[MAX_PATH] = {};

                    strcat_s(strSeqPath, strFileName);

                    strcat_s(strSeqPath, iter->second->pSequence->GetTag().c_str());

                    strcat_s(strSeqPath, ".seq");

                    char* pPos = strstr(strSeqPath, "|");

                    if (pPos)
                    {
                        *pPos = '_';
                    }

                    iter->second->pSequence->SaveFromPath(strSeqPath, MESH_PATH);
                }

                //for (int i = 0; i < loader.GetLODCount(); ++i)
                //{
                //	AddSeqeunces(loader.GetSequences(i), "_test");
                //}

                m_pAnimation->SetSkeleton(pSkeleton);

                FindAndAddBind<VertexShader>("anisotropic_microfacet VSSkin");
                FindAndAddBind<InputLayout>("Standard");
            }
            else
            {
                FindAndAddBind<VertexShader>("anisotropic_microfacet VSNoSkin");
                FindAndAddBind<InputLayout>("Standard");
            }
            FindAndAddBind<Topology>("TriangleList");

            std::vector<std::vector<std::shared_ptr<Texture>>> _vecTexture;

            std::vector<std::vector<std::shared_ptr<Material>>> _vecMaterial(loader.GetLODCount());

            int iTextureCount = 0;

            for (int i = 0; i < loader.GetLODCount(); ++i)
            {
                const std::vector<FbxLoader::TEXTUREINFO>& vecTextureInfo = loader.GetTextures(i);

                std::vector<std::shared_ptr<Texture>> vecTexture;

                for (size_t j = 0; j < vecTextureInfo.size(); ++j)
                {
                    TCHAR strFullPath[MAX_PATH] = {};

#ifdef UNICODE
                    MultiByteToWideChar(CP_ACP, 0, vecTextureInfo[j].strFullPath.c_str(), -1, strFullPath, MAX_PATH);
#else
                    strcpy_s(strFullPath, vecTextureInfo[i].strFullPath.c_str());
#endif

                    std::shared_ptr<Texture> pTexture = StaticFindBindable<Texture>(vecTextureInfo[j].strFullPath);

                    if (pTexture != nullptr)
                    {
                        vecTexture.push_back(pTexture);
                        continue;
                    }

                    ++iTextureCount;

                    switch (vecTextureInfo[j].type)
                    {
                    case fbxsdk::FbxLayerElement::EType::eTextureDiffuse:
                        vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 0));
                        break;
                    case fbxsdk::FbxLayerElement::EType::eTextureEmissive:
                        vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 3));
                        break;
                    case fbxsdk::FbxLayerElement::EType::eTextureAmbient:
                        assert(false);
                        break;
                    case fbxsdk::FbxLayerElement::EType::eTextureSpecular:
                        vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 2));
                        break;
                    case fbxsdk::FbxLayerElement::EType::eTextureNormalMap:
                    case fbxsdk::FbxLayerElement::EType::eTextureBump:
                        vecTexture.push_back(StaticCreateBindable<Texture>(vecTextureInfo[j].strFullPath, strFullPath, 1));
                        break;
                    default:
                        break;
                    }
                }

                // war.fbx ships with no texture refs; map mesh-name prefix
                // (Maya namespace stripped) to a texture group in
                // Warrior/additional-files/textures_extracted/TEXTURES/.
                if (vecTexture.empty() && _stricmp(strFileName, "war") == 0)
                {
                    std::string meshName = loader.GetMeshName(i);

                    auto colon = meshName.find(':');
                    if (colon != std::string::npos)
                    {
                        meshName = meshName.substr(colon + 1);
                    }

                    size_t end = meshName.size();
                    while (end > 0 && isdigit(static_cast<unsigned char>(meshName[end - 1])))
                    {
                        --end;
                    }
                    meshName.resize(end);

                    std::string lower = meshName;
                    std::transform(lower.begin(), lower.end(), lower.begin(),
                        [](unsigned char c) { return static_cast<char>(::tolower(c)); });

                    const char* group = "1_Armor";
                    if      (lower == "body" || lower == "cherep" ||
                             lower == "koza" || lower == "ruka"   ||
                             lower == "zubi" || lower == "serdce" ||
                             lower == "brov" || lower == "chep")    group = "1_body";
                    else if (lower == "eye"  || lower == "glaz" ||
                             lower == "glazelectro")                 group = "1_eye";
                    else if (lower == "gun"  || lower == "pulemet" ||
                             lower == "patron" || lower == "molot")  group = "1_gun";
                    else if (lower == "hair")                        group = "1_hair";
                    else if (lower == "portal")                      group = "1_portal23";
                    else if (lower == "stand" || lower == "stani")   group = "1_stand";

                    {
                        char dbg[512];
                        sprintf_s(dbg,
                            "[war.fbx] mesh[%d] raw=\"%s\" stripped=\"%s\" -> group=\"%s\"\n",
                            i, loader.GetMeshName(i).c_str(), lower.c_str(), group);
                        ::OutputDebugStringA(dbg);
                    }

                    auto LoadFallback = [&](const char* suffix, int slot)
                    {
                        char relPath[MAX_PATH];
                        sprintf_s(relPath,
                            "Warrior\\additional-files\\textures_extracted\\TEXTURES\\%s_%s.tga",
                            group, suffix);

                        TCHAR wRelPath[MAX_PATH] = {};
                        MultiByteToWideChar(CP_ACP, 0, relPath, -1, wRelPath, MAX_PATH);

                        std::string tag = std::string(group) + "_" + suffix;
                        std::shared_ptr<Texture> pTex = StaticFindBindable<Texture>(tag);
                        if (!pTex)
                        {
                            pTex = StaticCreateBindable<Texture>(tag, wRelPath, std::string(MESH_PATH), slot);
                        }

                        char dbg[512];
                        sprintf_s(dbg,
                            "[war.fbx]   slot=%d suffix=%s file=%s -> %s\n",
                            slot, suffix, relPath, pTex ? "OK" : "FAIL");
                        ::OutputDebugStringA(dbg);

                        if (pTex)
                        {
                            vecTexture.push_back(pTex);
                            ++iTextureCount;
                        }
                    };

                    LoadFallback("diffuze", 0);
                    LoadFallback("Normal",  1);
                }

                _vecTexture.push_back(vecTexture);

                const std::vector<FbxLoader::MATERIALINFO>& vecMaterial = loader.GetMaterials(i);

                for (int k = 0; k < vecMaterial.size(); ++k)
                {
                    // Materials are shared assets now (BindableManager<Material>
                    // SSoT). Same-name materials across containers / meshes /
                    // imports collapse to one instance — editing it updates
                    // every reference at once. Per-instance variation lives on
                    // MeshRendererComponent::OverrideMaterials.
                    std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>(vecMaterial[k].name);

                    if (pMaterial == nullptr)
                    {
                        pMaterial = StaticCreateBindable<Material>(vecMaterial[k].name);

                        pMaterial->SetDiffuseColor(vecMaterial[k].tMaterial.diffuseColor);
                        pMaterial->SetAmbientColor(vecMaterial[k].tMaterial.ambientColor);
                        // Same rationale as the OBJ MTL `Ks` branch above:
                        // metalness-workflow FBX exports almost always carry
                        // specular=(0,0,0). Skip the overwrite so the engine's
                        // dielectric F0 default (0.04) survives.
                        const Vector4& vSpec = vecMaterial[k].tMaterial.specularColor;
                        if (vSpec.x != 0.f || vSpec.y != 0.f || vSpec.z != 0.f)
                        {
                            pMaterial->SetSpecularColor(vSpec);
                        }
                        pMaterial->SetEmissiveColor(vecMaterial[k].tMaterial.emissiveColor);
                        pMaterial->SetShininess(vecMaterial[k].tMaterial.fSpecPower);
                        //pMaterial->SetReflectivity(vecMaterial[k].tMaterial.fFraction);
                        pMaterial->SetReflectivity(1.f);
                    }

                    pMesh->AddMaterial(i, pMaterial);

                    _vecMaterial[i].push_back(pMaterial);
                }

                // Distribute the collected textures to every Material in
                // this container, mapping each texture's t-register slot
                // to the Material's slot index. Sub-meshes in the same
                // container all share the same texture set, matching the
                // previous container-level texture model. Per-sub-mesh
                // overrides can be set later (editor or game code).
                for (const auto& pTex : vecTexture)
                {
                    if (!pTex) continue;
                    int iSlotIdx = Material::SlotRegisterToIndex(pTex->GetSlot());
                    if (iSlotIdx < 0) continue;
                    for (const auto& pMat : _vecMaterial[i])
                    {
                        if (pMat) pMat->SetTexture(iSlotIdx, pTex);
                    }
                }
            }

            if (iTextureCount > 0)
            {
                FindAndAddBind<PixelShader>("anisotropic_microfacet PS");
            }
            else
            {
                FindAndAddBind<PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
            }

            char strFullPath[MAX_PATH] = {};

            strcat_s(strFullPath, strFileName);

            strcat_s(strFullPath, ".mesh");

            SaveMesh(vecVertex, vecIndex, _vecTexture, _vecMaterial, strFullPath);
        }

        // ------------------------------------------------------------------
        // Dispatch on file extension. Mirrors the old Drawable::Load(TCHAR*)
        // routing — .OBJ/.obj → ParseOBJ, .FBX/.fbx → ParseFBX, anything
        // else asserts.
        // ------------------------------------------------------------------
        void DriveParser(MeshLoadContext& ctx, const TCHAR* pFileName, const std::string& strPathKey)
        {
            TCHAR strExt[_MAX_EXT] = {};

            _wsplitpath_s(pFileName, nullptr, 0, nullptr, 0, nullptr, 0, strExt, _MAX_EXT);

            _wcsupr_s(strExt);

            if (!wcscmp(strExt, TEXT(".OBJ")))
            {
                ctx.ParseOBJ(pFileName, strPathKey);
            }
            else if (!wcscmp(strExt, TEXT(".FBX")))
            {
                ctx.ParseFBX(pFileName, strPathKey);
            }
            else
            {
                assert(false);
            }
        }
    } // anonymous namespace

    // ------------------------------------------------------------------
    // MeshLoader public facade. Drives a transient MeshLoadContext to
    // run the parser; the resources (Mesh / Material / Texture) live in
    // the BindableManager registry so they outlive the context. Load()
    // returns just the headline resources; LoadInto() forwards every
    // accumulated Bindable into a MeshRendererComponent so it gets the
    // shaders / IL / Topology / etc. it needs to actually draw.
    // ------------------------------------------------------------------
    MeshLoader::Result MeshLoader::Load(const TCHAR* pFileName, const std::string& strPathKey)
    {
        MeshLoadContext ctx;
        DriveParser(ctx, pFileName, strPathKey);

        Result out;
        out.pMesh      = ctx.m_pMesh;
        out.pMaterial  = ctx.m_pMaterial;
        out.vecTexture = ctx.m_vecTexture;
        return out;
    }

    void MeshLoader::LoadInto(const TCHAR* pFileName,
                              const std::string& strPathKey,
                              const std::shared_ptr<MeshRendererComponent>& pTarget)
    {
        if (!pTarget) return;

        MeshLoadContext ctx;
        DriveParser(ctx, pFileName, strPathKey);

        // Route each parsed Bindable through the MeshRenderer's AddBindable
        // dispatcher (which slots Mesh / VS / PS / Material / Texture into
        // named members and drops the rest into m_OtherBindables for IL /
        // Topology / RS). Mirrors the behaviour of the old
        // Drawable::LoadIntoMeshRenderer bridge.
        if (ctx.m_pMesh) pTarget->AddBindable(ctx.m_pMesh);
        if (ctx.m_pMaterial) pTarget->AddBindable(ctx.m_pMaterial);
        for (const auto& p : ctx.m_vecTexture)
        {
            if (p) pTarget->AddBindable(p);
        }
        for (const auto& p : ctx.m_vecOtherBindings)
        {
            if (p) pTarget->AddBindable(p);
        }
        if (ctx.m_pAnimation) pTarget->SetAnimation(ctx.m_pAnimation);
    }
}
