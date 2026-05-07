#include "Scene.h"
#include "Layer.h"
#include "../Bindable/Drawable.h"
#include "SceneManager.h"
#include "../Core/PathManager.h"
#include "../Core/Graphics.h"
#include "../Bindable/Camera.h"
#include "../Render/RenderManager.h"
#include "../RenderV2/Drawable.h"
#include "../RenderV2/RenderQueue.h"
#include "../Matrix.h"

#include <DirectXMath.h>

namespace Engine
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	void Scene::AddLayer(const std::string& strTag, int iZOrder)
	{
		std::shared_ptr<Layer> pLayer = std::make_shared<Layer>();

		pLayer->SetTag(strTag);

		pLayer->SetZOrder(iZOrder);

		AddLayer(pLayer);
	}

	void Scene::AddLayer(std::shared_ptr<Layer> pLayer)
	{
		pLayer->SetScene(this);

		m_LayerList.push_back(pLayer);

		m_LayerList.sort([](const std::shared_ptr<Layer>& src, const std::shared_ptr<Layer>& dest)->bool
			{
				return src->GetZOrder() < dest->GetZOrder();
			}
		);
	}

	std::shared_ptr<Layer> Scene::FindLayer(const std::string& strTag) const
	{
		std::list<std::shared_ptr<Layer>>::const_iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::const_iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetTag() == strTag)
			{
				return *iter;
			}
		}

		return std::shared_ptr<Layer>();
	}

	void Scene::Clear()
	{
		// Phase E7 — m_mapProtoType registry removed; nothing scene-static
		// to clear here today. Per-scene state lives on m_LayerList +
		// m_v2DrawableList and is destroyed with the Scene instance.
	}

	bool Scene::Init()
	{
		AddLayer(DEFAULT_LAYER);
		AddLayer(ALPHA_LAYER, 1);
		AddLayer(UI_LAYER, 2);

		return true;
	}

	void Scene::Input(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Input(fDeltaTime);
			++iter;
		}
	}

	void Scene::Update(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Update(fDeltaTime);
			++iter;
		}

		// RenderV2 drawables advance alongside legacy ones.
		for (auto& d : m_v2DrawableList)
		{
			if (d) d->Update(fDeltaTime);
		}
	}

	void Scene::AddV2Drawable(std::shared_ptr<RenderV2::Drawable> pDrawable)
	{
		if (pDrawable) m_v2DrawableList.push_back(pDrawable);
	}

	void Scene::FixedUpdate(float fDelatTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->FixedUpdate(fDelatTime);
			++iter;
		}
	}

	void Scene::PostUpdate(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->PostUpdate(fDeltaTime);
			++iter;
		}
	}

	void Scene::Collision(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Collision(fDeltaTime);
			++iter;
		}
	}

	void Scene::PreDraw(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->PreDraw(fDeltaTime);
			++iter;
		}

		// Submit V2 drawables to RenderManager's render-V2 queue. Camera info
		// comes from the active camera (Graphics::GetCamera). Falls back to a
		// fixed view if no camera is set, mirroring the Demo path.
		if (m_v2DrawableList.empty()) return;

		RenderManager* rm = RenderManager::GetInst();
		RenderV2::RenderQueue* queue = rm ? rm->GetV2Queue() : nullptr;
		if (!queue) return;

		auto toXM = [](const Matrix& m) {
			DirectX::XMFLOAT4X4 f;
			memcpy(&f, m.f, sizeof(float) * 16);
			return DirectX::XMLoadFloat4x4(&f);
		};

		RenderV2::FrameInfo frame;
		std::shared_ptr<Camera> cam = Graphics::GetInst()->GetCamera(CAMERA_TYPE::NORMAL);
		if (cam)
		{
			frame.view     = toXM(cam->GetView());
			frame.proj     = toXM(cam->GetProjectMatrix());
			frame.viewProj = toXM(cam->GetViewProject());
		}
		else
		{
			using namespace DirectX;
			frame.view     = XMMatrixLookAtLH({0, 0, -3, 1}, {0, 0, 0, 1}, {0, 1, 0, 0});
			frame.proj     = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
			frame.viewProj = frame.view * frame.proj;
		}

		for (auto& d : m_v2DrawableList)
		{
			if (d) d->Submit(*queue, frame);
		}
	}

	void Scene::Draw()
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Draw();
			++iter;
		}
	}

	void Scene::Save(FILE* pFile)
	{
		int iLayerCount =static_cast<int>(m_LayerList.size());

		fwrite(&iLayerCount, 4, 1, pFile);

		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->Save(pFile);
		}
	}

	void Scene::Load(FILE* pFile)
	{
		int iLayerCount = 0;

		fread(&iLayerCount, 4, 1, pFile);

		for (int i = 0; i < iLayerCount; ++i)
		{
			std::shared_ptr<Layer> pLayer = std::make_shared<Layer>();

			AddLayer(pLayer);

			pLayer->Load(pFile);

			m_LayerList.sort([](const std::shared_ptr<Layer>& src, const std::shared_ptr<Layer>& dest)->bool
				{
					return src->GetZOrder() < dest->GetZOrder();
				}
			);
		}
	}

	void Scene::Save(const char* pFilePath, const std::string& strPath)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPath);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		SaveFromFullPath(strFullPath);
	}

	void Scene::Load(const char* pFilePath, const std::string& strPath)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPath);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		LoadFromFullPath(strFullPath);
	}

	void Scene::SaveFromFullPath(const char* pFullPath)
	{
		FILE* pFile = nullptr;

		fopen_s(&pFile, pFullPath, "wb");

		if (pFile)
		{
			Save(pFile);

			fclose(pFile);
		}
	}

	void Scene::LoadFromFullPath(const char* pFullPath)
	{
		FILE* pFile = nullptr;

		fopen_s(&pFile, pFullPath, "rb");

		if (pFile)
		{
			Load(pFile);

			fclose(pFile);
		}
	}

	void Scene::SaveFromFullPath(const TCHAR* pFullPath)
	{
		char strFullPath[MAX_PATH] = {};

		WideCharToMultiByte(CP_ACP, 0, pFullPath, -1, strFullPath, MAX_PATH, nullptr, nullptr);

		SaveFromFullPath(strFullPath);
	}

	void Scene::LoadFromFullPath(const TCHAR* pFullPath)
	{
		char strFullPath[MAX_PATH] = {};

		WideCharToMultiByte(CP_ACP, 0, pFullPath, -1, strFullPath, MAX_PATH, nullptr, nullptr);

		LoadFromFullPath(strFullPath);
	}

	// Phase E7 — Scene::FindBindable removed. Its only implementation walked
	// each Layer's m_DrawList via Layer::FindDrawable, both of which are
	// gone now. No live callers existed; bindable lookups today go through
	// the BindableManager (StaticFindBindable<T>) instead.


}