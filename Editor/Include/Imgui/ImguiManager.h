#pragma once

#include "../Editor.h"
#include "Animation/JointSocket.h"
#include "Core/Graphics.h"
#include "../Navigation/Recast/Recast.h"

namespace Engine
{
	class Layer;
	class Drawable;
	class Mesh;
	class Collider;
	class NavMesh;
	class Transform;
	class PointLight;
	class Sphere;
	class MRT;
	class Material;
	class Shader;
	class Scene;
	class Particle;
	class Cloth;
	class Terrain;
	class Texture;
	class Animation;
	class Sequence;
}
namespace Editor
{
	class ImguiManager
	{
	private:
		ImguiManager();
		~ImguiManager();

	private:
		static ImguiManager* m_pInst;

	public:
		static ImguiManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new ImguiManager;
			}

			return m_pInst;
		}
		static void DestroyInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		bool m_bDemoWindow;

	public:
		bool Init(HWND hWnd);
		void Update(float fDeltaTime);
		void Render(float fDeltaTime);

	public:
		void DisableMouse();
		void EnableMouse();

	public:
		void CRef_ImGuiWindow(std::shared_ptr<Engine::CRef> pRef);
		void JointSocket_ImGuiWindow(std::shared_ptr<class Engine::JointSocket> pRef, int iIndex);
		void SceneWindow(class Engine::Scene* pScene);
		void Layer_DrawListImgui(std::shared_ptr<Engine::Layer> pLayer);
		void Drawable_ShowImGuiWindow(std::shared_ptr<Engine::Bindable> pDrawable);
		void Drawable_ImGuiWindow(std::shared_ptr<Engine::Bindable> pDrawable);
		void Mesh_ImGuiWindow(std::shared_ptr<Engine::Mesh> pMesh);
		void Material_ImGuiWindow(std::shared_ptr<Engine::Material> pMaterial);
		std::shared_ptr<Engine::Bindable> Drawable_ShowImGuiTree(std::shared_ptr<Engine::Bindable>, bool& bSelect);
		void TransformBuffer_ImGuiWindow(std::shared_ptr<Engine::Transform> pTransform);
		void PointLight_ImGuiWindow(std::shared_ptr<Engine::PointLight> pLight);
		void Shader_ImGuiWindow(std::shared_ptr<Engine::Shader> pShader);
		void Sphere_ImGuiWindow(std::shared_ptr<Engine::Sphere> pSphere);
		void MRT_ShowImGuiImage(std::shared_ptr<Engine::MRT> pMRT, const std::string& name = "MRT: ");
		void Particle_ShowImGuiImage(std::shared_ptr<Engine::Particle> pParticle);
		void RenderManager_ShowImGuiWindow();
		void Cloth_ShowImguiWindow(std::shared_ptr<Engine::Cloth> pCloth);
		void Terrain_ShowImguiWindow(std::shared_ptr<Engine::Terrain> pTerrain);
		void Animation_ImGuiWindow(std::shared_ptr<Engine::Animation> pAnimation);
		void Scene_ImGuiWindow(Engine::Scene* pScene);
		void Sequence_ImGuiWindow(std::shared_ptr<Engine::Sequence> pSeq);

	private:
		float m_fCellSize;
		float m_fCellHeight;
		float m_fAgentSlopeAngle;
		float m_fAgentHeight;
		float m_fAgentRadius;
		float m_fAgentClimb;
		float m_fMaxEdgeLen;
		float m_fMaxEdgeError;
		float m_fRegionMinSize;
		float m_fRegionMergeSize;
		float m_fVertsPerPoly;
		float m_fDetailSampleDist;
		float m_fDetailSampleMaxError;

		rcHeightfield* m_pHeightField;
		rcContext m_tContext;

		std::unique_ptr<unsigned char[]> m_pTriAreas;
		rcCompactHeightfield* m_pCompactHeightField;
		rcContourSet* m_pContourSet;
		rcPolyMesh* m_pPolyMesh;
		rcPolyMeshDetail* m_pPolyMeshDetail;
		std::shared_ptr<Engine::NavMesh> m_pNavMesh;
		bool m_bMode;
		std::list<std::shared_ptr<class Player>> m_PlayerList;

	private:
		std::vector<std::shared_ptr<Engine::Texture>> m_vecBrushTexture;

	public:
		void CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
		void LoadNavMesh(const TCHAR* pFullPath, class Engine::Scene* pScene);
		void LoadNavMesh(class Engine::Scene* pScene, const TCHAR* pFilePath, const std::string& strPathKey);
		std::shared_ptr<Engine::NavMesh> CreateNavMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecTris, const Engine::Vector3& vMax, const Engine::Vector3& vMin);
	};
}