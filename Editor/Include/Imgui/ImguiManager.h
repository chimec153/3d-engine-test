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
	class GameObject;
	class Component;
	class MeshRendererComponent;
	class VertexShader;
	class PixelShader;
	class BlendState;
	template <typename T> class ConstantBuffer;
}
namespace Editor
{
	// Constant buffer for the selection-outline composite pass. Layout
	// mirrors `cbuffer OutlineCB` in SelectionOutline.fx — keep them in
	// sync (16-byte aligned, 32-byte total).
	struct OUTLINECBUFFER
	{
		float vColor[4];
		float vTexelSize[2];
		int iThickness;
		float padding;
	};

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

		// Unreal-style debug windows for entity/component inspection.
		// WorldOutliner draws the Scene→Layer→GameObject tree and tracks
		// selection in m_pSelectedObject; GameObject_ImGuiWindow renders the
		// Details panel for one entity; Component_ImGuiWindow dispatches a
		// single Component's inline editor (Transform for now).
		void WorldOutliner_ImGuiWindow(Engine::Scene* pScene);
		void GameObject_ImGuiWindow(std::shared_ptr<Engine::GameObject> pObject);
		void Component_ImGuiWindow(std::shared_ptr<Engine::Component> pComponent);
		void MeshRenderer_ImGuiWindow(std::shared_ptr<Engine::MeshRendererComponent> pRenderer);

		// Selection-outline pass (Unreal-style post-process). Two stages:
		//   mask  — render the selected (GameObject, containerIdx)'s
		//           container geometry into m_pOutlineMaskMRT
		//   composite — full-screen edge-detect that samples the mask and
		//               writes outline color to whatever RT is currently
		//               bound (back buffer at UI render time).
		// Called from Update() via AddCustomRender on RENDER_LAYER::UI.
		void InitSelectionOutline();
		void RenderSelectionOutline();

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

		// Outliner selection; weak so a deleted GameObject auto-clears.
		std::weak_ptr<Engine::GameObject> m_pSelectedObject;

		// Selection-outline state. Single container highlighted at a time;
		// -1 means "no container selected". The owning GameObject is held
		// weakly so deleting it auto-clears the highlight.
		std::weak_ptr<Engine::GameObject> m_pOutlinedObject;
		int m_iOutlinedContainerIdx;

		std::shared_ptr<Engine::MRT>          m_pOutlineMaskMRT;
		std::shared_ptr<Engine::VertexShader> m_pOutlineMaskVS;
		std::shared_ptr<Engine::PixelShader>  m_pOutlineMaskPS;
		std::shared_ptr<Engine::VertexShader> m_pOutlineFullScreenVS;
		std::shared_ptr<Engine::PixelShader>  m_pOutlineCompositePS;
		std::shared_ptr<Engine::ConstantBuffer<OUTLINECBUFFER>> m_pOutlineCB;
		std::shared_ptr<Engine::BlendState>   m_pOutlineBlend;

		// Post-process toggles. When a flag flips off, the previous "live"
		// value is cached in the matching m_fSaved* field and the engine
		// param is pushed to a neutral value (no-op). Flipping back on
		// restores the cached value. Default-on so editor mirrors fresh
		// engine state.
		bool  m_bEnableBloom = true;
		bool  m_bEnableDOF   = true;
		bool  m_bEnableFog   = true;
		float m_fSavedBloomScale = 0.f;
		float m_fSavedFOVValueY  = 0.f;
		float m_fSavedFogDensity = 0.f;

	public:
		void CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
		void LoadNavMesh(const TCHAR* pFullPath, class Engine::Scene* pScene);
		void LoadNavMesh(class Engine::Scene* pScene, const TCHAR* pFilePath, const std::string& strPathKey);
		std::shared_ptr<Engine::NavMesh> CreateNavMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecTris, const Engine::Vector3& vMax, const Engine::Vector3& vMin);
	};
}