#include "Demo.h"
#include "D3D11Context.h"
#include "RenderQueue.h"
#include "Drawables/Box.h"
#include "Drawables/Quad.h"
#include "Drawables/Sphere.h"
#include "Drawables/Mesh.h"
#include "../Core/Graphics.h"
#include "../Render/RenderManager.h"
#include "../Bindable/Camera.h"
#include "../Matrix.h"

#include <DirectXMath.h>
#include <memory>

namespace Engine::RenderV2
{
	namespace
	{
		// Convert engine Matrix (row-major float[16]) to XMMATRIX. Engine uses
		// row-major; XMMATRIX is row-major too, so direct load works.
		DirectX::XMMATRIX ToXM(const Matrix& m)
		{
			DirectX::XMFLOAT4X4 f4x4;
			memcpy(&f4x4, m.f, sizeof(float) * 16);
			return DirectX::XMLoadFloat4x4(&f4x4);
		}
	}

	bool RunBoxDemo()
	{
		Graphics* gfx = Graphics::GetInst();
		ID3D11Device* device = gfx->GetDevice();
		ID3D11DeviceContext* ctx = gfx->GetDeviceContext();
		if (!device || !ctx)
			return false;

		Drawables::Box box;
		if (!box.Init(device))
			return false;

		using namespace DirectX;
		FrameInfo frame;
		frame.view     = XMMatrixLookAtLH({0, 0, -3, 1}, {0, 0, 0, 1}, {0, 1, 0, 0});
		frame.proj     = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
		frame.viewProj = frame.view * frame.proj;

		box.Update(0.0f);

		RenderQueue queue;
		box.Submit(queue, frame);

		D3D11Context renderCtx(device, ctx);
		queue.Flush(renderCtx);

		return true;
	}

	namespace
	{
		// Persistent demo state. Wrapped in a getter so ShutdownDemo() can
		// reset() it deterministically before the D3D11 device dies.
		struct DemoState
		{
			std::unique_ptr<Drawables::Box>    box;
			std::unique_ptr<Drawables::Quad>   quad;
			std::unique_ptr<Drawables::Sphere> sphere;
			std::unique_ptr<Drawables::Mesh>   mesh;
			bool initFailed = false;
		};

		DemoState& Demo()
		{
			static DemoState s_state;
			return s_state;
		}
	}

	bool SubmitBoxThisFrame(float deltaTime)
	{
		DemoState& st = Demo();

		if (st.initFailed)
			return false;

		if (!st.box)
		{
			Graphics* gfx = Graphics::GetInst();
			ID3D11Device* device = gfx->GetDevice();
			if (!device)
				return false;

			st.box = std::make_unique<Drawables::Box>();
			if (!st.box->Init(device))
			{
				st.box.reset();
				st.initFailed = true;
				return false;
			}

			st.quad = std::make_unique<Drawables::Quad>();
			if (!st.quad->Init(device))
			{
				st.box.reset();
				st.quad.reset();
				st.initFailed = true;
				return false;
			}
			// Place the quad slightly in front of (and overlapping) the cube
			// so alpha blending is observable — at +1.5x they never met.
			st.quad->SetPosition({0.4f, 0.0f, -0.7f});

			st.sphere = std::make_unique<Drawables::Sphere>(16, 24);
			if (!st.sphere->Init(device))
			{
				st.box.reset();
				st.quad.reset();
				st.sphere.reset();
				st.initFailed = true;
				return false;
			}
			st.sphere->SetPosition({-1.5f, 0.0f, 0.0f});

			// Phase 2.4c — lit FBX mesh via NormalShader.hlsl. Walking.fbx
			// is the test asset because its .fbm/ folder ships next to the
			// FBX so texture paths resolve. (MONHUN.FBX references textures
			// at hardcoded artist paths that don't exist on this machine.)
			st.mesh = std::make_unique<Drawables::Mesh>();
			if (st.mesh->Init(device, L"Walking.fbx"))
			{
				st.mesh->SetPosition({1.5f, -0.5f, 0.0f});
				st.mesh->SetScale(1.0f);
			}
			else
			{
				::OutputDebugStringW(L"[RenderV2] Walking.fbx Init failed\n");
				st.mesh.reset();
			}
		}

		RenderManager* rm = RenderManager::GetInst();
		RenderQueue* queue = rm ? rm->GetV2Queue() : nullptr;
		if (!queue)
			return false;

		// Build FrameInfo (view + proj kept separate; lit drawables need
		// matWorldView, primitives only viewProj). Falls back to a fixed
		// camera if no engine camera is present.
		FrameInfo frame;
		std::shared_ptr<Camera> cam = Graphics::GetInst()->GetCamera(CAMERA_TYPE::NORMAL);
		if (cam)
		{
			frame.view     = ToXM(cam->GetView());
			frame.proj     = ToXM(cam->GetProjectMatrix());
			frame.viewProj = ToXM(cam->GetViewProject());
		}
		else
		{
			using namespace DirectX;
			frame.view     = XMMatrixLookAtLH({0, 0, -3, 1}, {0, 0, 0, 1}, {0, 1, 0, 0});
			frame.proj     = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
			frame.viewProj = frame.view * frame.proj;
		}

		st.box->Update(deltaTime);
		st.box->Submit(*queue, frame);

		if (st.sphere)
		{
			st.sphere->Update(deltaTime);
			st.sphere->Submit(*queue, frame);
		}

		if (st.mesh)
		{
			st.mesh->Update(deltaTime);
			st.mesh->Submit(*queue, frame);
		}

		if (st.quad)
		{
			st.quad->Update(deltaTime);
			st.quad->Submit(*queue, frame);
		}
		return true;
	}

	void ShutdownDemo()
	{
		DemoState& st = Demo();
		st.box.reset();
		st.quad.reset();
		st.sphere.reset();
		st.mesh.reset();
		st.initFailed = false;
	}
}
