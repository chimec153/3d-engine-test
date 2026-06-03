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
	class LightComponent;
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
		HWND m_hWnd = nullptr;

	public:
		bool Init(HWND hWnd);
		void Update(float fDeltaTime);
		void Render(float fDeltaTime);

	public:
		void DisableMouse();
		void EnableMouse();

	public:
		// "Open Project..." + Play / Stop toolbar. Lists scenes from
		// Engine::SceneFactory and actors from Engine::GameObjectFactory once
		// a project DLL is loaded via ProjectModule.
		void Project_ImGuiWindow();

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
		void PointLight_ImGuiWindow(std::shared_ptr<Engine::LightComponent> pLight);
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

		// Asset browser for every registered Material. Left pane lists
		// BindableManager<Material>'s entries; right pane reuses
		// Material_ImGuiWindow for the currently-selected entry. Toolbar
		// supports New (create + register), Load (.mat from disk), and
		// Save All (bulk-write every tagged material to Resource/Material).
		void MaterialBrowser_ImGuiWindow();

		// Fragment Baker — bake-time tool that procedurally carves a convex
		// base volume into N low-poly shards (FragmentGenerator) and writes the
		// combined multi-container Mesh to a .mesh asset. "Generate" also
		// previews the shards on a throwaway __FragmentPreview object in the
		// current scene; "Save" opens a file dialog and serialises the Mesh.
		void FragmentBaker_ImGuiWindow();

		// Particle Editor — spawns a throwaway live preview emitter in the
		// current scene and tunes it in real time (no persistence). The editor
		// holds the authoritative parameter values; each widget change is pushed
		// to the live Particle via its setter, and the scene's normal per-frame
		// Update ticks the GPU simulation, so edits are visible immediately.
		void ParticleEditor_ImGuiWindow();
		void SpawnParticlePreview();   // (re)create the preview at the current Max Count
		void ApplyParticleParams(std::shared_ptr<Engine::Particle> pParticle);  // push every member value
		// (Re)load a texture from a path and bind it to the live preview. A
		// leading '/' is treated as a /Game/ mount path (path-key load),
		// otherwise an absolute file path. Remembers the path for save/respawn.
		void SetParticleTexture(const char* szPathUtf8);
		void SaveParticlePreset();     // write all tunables + texture path to a .particle file
		void LoadParticlePreset();     // read them back and apply to the live preview

		// Shader hot-reload — recompile every registered shader from its source
		// .fx/.hlsl without restarting. Compile errors keep the old shader and
		// are collected into m_strShaderReloadLog.
		void ShaderReload_ImGuiWindow();

		// 3D translate/rotate/scale gizmo for the currently-selected GameObject.
		// Backed by ImGuizmo — overlays directly on the main viewport, hooked
		// to the active camera's view/projection. W/E/R cycle operation,
		// X toggles world ↔ local mode.
		void DrawSelectionGizmo();

		// Per-light 2D billboard icons drawn over the viewport so lights are
		// visible/clickable even when they don't render geometry themselves.
		// Each light's world position is projected through the active camera's
		// view-projection; the icon is colored by LIGHT_TYPE and click-picks
		// into m_pSelectedObject.
		void RenderLightBillboards();

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

		// Material Browser selection. Weak so removing the material from
		// BindableManager (future feature) auto-clears the inspector.
		std::weak_ptr<Engine::Material> m_pSelectedMaterial;

		// Animation editor target. Set by the "Open Animation Editor" button
		// in Component_ImGuiWindow; Animation_ImGuiWindow is then called every
		// frame from Update with this pointer's lock(). Weak so the window
		// auto-closes when the GameObject (or its Animation component) is
		// removed.
		std::weak_ptr<Engine::Animation> m_pSelectedAnimation;

		// Gizmo state. Kept as ints so this header doesn't have to pull
		// in ImGuizmo.h (cpp maps them onto ImGuizmo::OPERATION / MODE).
		// 0=Translate, 1=Rotate, 2=Scale  /  0=Local, 1=World.
		int m_iGizmoOp   = 0;
		int m_iGizmoMode = 1;

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
		// NavMesh wireframe overlay — toggled from the debug UI. The
		// "NavMesh_Debug" GameObject is created right after navmesh build
		// (LoadNavMesh path) so flipping this updates its enable flag
		// next frame. Default off so freshly-built scenes don't show the
		// overlay until the user asks.
		bool  m_bShowNavMeshDebug = false;

		bool  m_bEnableBloom = true;
		bool  m_bEnableDOF   = true;
		bool  m_bEnableFog   = true;
		float m_fSavedBloomScale = 0.f;
		float m_fSavedFOVValueY  = 0.f;
		float m_fSavedFogDensity = 0.f;

		// Fragment Baker UI state + last bake result. The preview GameObject is
		// held weakly so a scene swap (Play/Stop, project reload) auto-clears
		// it and the next Generate rebuilds it.
		int   m_iFragShape       = 0;     // 0 = Box, 1 = Sphere, 2 = Capsule
		int   m_iFragCount       = 12;
		int   m_iFragSeed        = 1337;
		float m_fFragSize        = 1.f;
		float m_fFragBoxHeight   = 1.f;  // Box only: Y half-extent (= Size → cube)
		int   m_iFragSubdiv      = 1;
		float m_fFragCylHeight   = 0.4f;  // Capsule only: straight-section length
		int   m_iFragCapRings    = 4;     // Capsule only: rings per hemisphere
		int   m_iFragCapSectors  = 12;    // Capsule/Cylinder: longitude segments
		float m_fFragCylinderHeight = 2.f; // Cylinder only: full height (Y)
		int   m_iFragOutShards   = 0;
		int   m_iFragOutVerts    = 0;
		int   m_iFragOutTris     = 0;
		std::shared_ptr<Engine::Mesh>     m_pFragMesh;
		std::weak_ptr<Engine::GameObject> m_pFragPreview;

		// Particle Editor UI state. Float arrays feed ImGui widgets directly and
		// are the authoritative values (no getters on Particle needed). Defaults
		// roughly mirror the in-game hit-spark emitter (Attackable). The preview
		// GameObject is held weakly so a scene swap auto-clears it.
		float m_ptStartColor[4]  = { 1.f, 1.f, 0.f, 1.f };
		float m_ptEndColor[4]    = { 1.f, 1.f, 0.f, 0.f };
		float m_ptVelocity[3]    = { -0.1f, 0.f, -0.1f };
		float m_ptMaxVelocity[3] = {  0.1f, 0.f,  0.1f };
		float m_ptAccel[3]       = { 0.f, 1.f, 0.f };
		float m_ptMinPos[3]      = { -1.f, 0.f, -1.f };
		float m_ptMaxPos[3]      = {  1.f, 0.f,  1.f };
		float m_ptStartSize[2]   = { 0.05f, 0.05f };
		float m_ptEndSize[2]     = { 0.10f, 0.10f };
		float m_ptLifeTime       = 3.f;
		float m_ptEmitTime       = 0.01f;
		int   m_ptMaxCount       = 4096;
		int   m_ptMaxFrame       = 1;
		int   m_ptFrameW         = 1;
		int   m_ptFrameH         = 1;
		bool  m_ptStopEmit       = false;
		std::weak_ptr<Engine::GameObject> m_pParticlePreview;
		// Chosen emitter texture (slot 0, like the in-game atlas). Persists
		// across respawns; null until the first spawn seeds the default.
		std::shared_ptr<Engine::Texture>  m_pPtTexture;
		// Path m_pPtTexture was loaded from (UTF-8/ACP). Saved with the preset
		// so a load can re-bind the same image. "/..." = /Game/ mount path.
		char m_strPtTexturePath[MAX_PATH] = {};

		// Result text of the last shader hot-reload (per-shader compile errors,
		// or an all-OK message).
		std::string m_strShaderReloadLog;

	public:
		void CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
		void LoadNavMesh(const TCHAR* pFullPath, class Engine::Scene* pScene);
		void LoadNavMesh(class Engine::Scene* pScene, const TCHAR* pFilePath, const std::string& strPathKey);
		std::shared_ptr<Engine::NavMesh> CreateNavMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecTris, const Engine::Vector3& vMax, const Engine::Vector3& vMin);

	private:
		// Editor preferences persisted to Editor.ini next to the executable.
		// Today only the texture-picker default directory lives here; future
		// editor-only settings (mesh/sound default paths, recent-files list,
		// etc.) can be added by adding members and Get/Set in INI sections.
		TCHAR m_strTextureDefaultPath[MAX_PATH];

		// Root of the client's Resource folder — used as the initial directory
		// for Scene/Mesh/Sequence save dialogs so artists don't have to climb
		// out of the editor's mirrored Resource tree and over to the client's.
		// Always ends with a path separator. Default at first launch is
		// derived from the editor exe location (../../Client/Bin/Resource/).
		TCHAR m_strClientResourcePath[MAX_PATH];

		// Recent projects (DLL paths) shown on the empty Project window so the
		// user can re-open a project with one click instead of re-browsing.
		// Most-recently-used first. Capped at kMaxRecentProjects; persisted to
		// Editor.ini's [Recent] section as Project0..ProjectN-1.
		static constexpr int kMaxRecentProjects = 8;
		std::vector<std::wstring> m_RecentProjects;

		// Build "<m_strClientResourcePath><pSubFolder>\" into pOut. Used by
		// the save dialogs to default into Client\Bin\Resource\<subfolder>\.
		// Falls back to engine's FindPath(strFallbackKey) when the client
		// path is empty (user cleared it in settings).
		void BuildClientSubPath(TCHAR* pOut, size_t iLen, const TCHAR* pSubFolder, const std::string& strFallbackKey) const;

		// Move/insert path at the front of m_RecentProjects (dedupes, caps at
		// kMaxRecentProjects), then persists. Called after a successful Load.
		void AddRecentProject(const std::wstring& dllPath);
		// Drop an entry by exact path match and persist. Called when a click
		// on a recent entry fails to load (DLL missing/moved).
		void RemoveRecentProject(const std::wstring& dllPath);

	public:
		void EditorSettings_ImGuiWindow();
		void LoadEditorSettings();
		void SaveEditorSettings() const;
	};
}