#include "UIRenderer.h"
#include "Transform.h"
#include "Mesh.h"
#include "Animation.h"
#include "PointLight.h"
#include "Camera.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Texture.h"
#include "Topology.h"
#include "ConstantBuffer.h"
#include "BindableManager.h"
#include "RasterizerState.h"
#include "../Core/Graphics.h"
#include "../Render/RenderManager.h"

namespace Engine
{
	namespace
	{
		// Scissor-enabled rasterizer state for clipping a UI element's draw to
		// a SetClipRect rect (e.g. ScrollView viewport). CULL_NONE so the UI
		// quad draws regardless of winding. Cached in the RasterizerState
		// BindableManager (tag "UIScissor"), so its lifetime is released by
		// BindableRegistry::DestroyAll before the device dies — no raw leak.
		std::shared_ptr<RasterizerState> GetScissorRasterizerState()
		{
			auto* pMgr = BindableManager<RasterizerState>::GetInst();
			std::shared_ptr<RasterizerState> pRS = pMgr->FindBindable("UIScissor");
			if (!pRS)
				pRS = pMgr->CreateBindable("UIScissor", true, D3D11_CULL_NONE, D3D11_FILL_SOLID, 0.f, 0.f, true);
			return pRS;
		}
	}

	void UIRenderer::SetClipRect(float fX, float fY, float fW, float fH)
	{
		m_bClip   = true;
		m_fClip[0] = fX; m_fClip[1] = fY; m_fClip[2] = fW; m_fClip[3] = fH;
	}

	UIRenderer::UIRenderer() :
		m_eRenderLayer(RENDER_LAYER::UI)
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	UIRenderer::UIRenderer(const UIRenderer& other) :
		Component(other)
		, m_pCamera(other.m_pCamera)
		, m_pParentTransform(other.m_pParentTransform)
		, m_pParentMesh(other.m_pParentMesh)
		, m_pParentAnimation(other.m_pParentAnimation)
		, m_pLight(other.m_pLight)
		, m_eRenderLayer(other.m_eRenderLayer)
		, m_vTint(other.m_vTint)
	{
	}

	void UIRenderer::SetTint(unsigned int uRGBA)
	{
		// 0xRRGGBBAA → (r, g, b, a). Master alpha in .w; PS_UITint uses it.
		m_vTint = Vector4(
			((uRGBA >> 24) & 0xFF) / 255.f,
			((uRGBA >> 16) & 0xFF) / 255.f,
			((uRGBA >>  8) & 0xFF) / 255.f,
			((uRGBA      ) & 0xFF) / 255.f);
	}

	void UIRenderer::SetCamera(std::shared_ptr<Camera> pCamera)
	{
		m_pCamera = pCamera;
	}

	void UIRenderer::SetTarget(std::shared_ptr<Transform> pTransform,
	                           std::shared_ptr<Mesh> pMesh,
	                           std::shared_ptr<Animation> pAnimation)
	{
		m_pParentTransform = pTransform;
		m_pParentMesh = pMesh;
		m_pParentAnimation = pAnimation;
	}

	bool UIRenderer::Init()
	{
		if (!__super::Init())
			return false;
		return true;
	}

	void UIRenderer::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		// First-class UI registration — no std::function indirection,
		// RenderUI iterates m_UIList and calls Bind directly. m_eRenderLayer
		// stays for informational use; the UI pass is the only consumer.
		RenderManager::GetInst()->AddUIRenderer(
			std::dynamic_pointer_cast<UIRenderer>(shared_from_this()));
	}

	void UIRenderer::Bind()
	{
		// Per-instance UI state bound right before this renderer's own draw
		// (inside the m_UIList pass): the tint colour, and a full-quad UV
		// reset on the shared b5 cbuffer. Both cbuffers are shared, so they
		// must be set per-draw — pushing them in a later pass let a sibling
		// element clobber them first. Lazy lookup keeps clones working.
		if (!m_pTintCBuffer)
			m_pTintCBuffer = StaticFindBindable<ConstantBuffer<UITINTBUFFER>>("UITint");
		if (!m_pUICBuffer)
			m_pUICBuffer = StaticFindBindable<ConstantBuffer<UICBUFFER>>("UI");
		if (m_pTintCBuffer)
		{
			UITINTBUFFER tint{};
			tint.vTint = m_vTint;
			m_pTintCBuffer->UpdateBuffer(tint);
			m_pTintCBuffer->Bind();
		}
		if (m_pUICBuffer)
		{
			UICBUFFER ui{};
			ui.vStartUV = Vector2(0.f, 0.f);
			ui.vEndUV   = Vector2(1.f, 1.f);
			m_pUICBuffer->UpdateBuffer(ui);
			m_pUICBuffer->Bind();
		}

		// Phase E5 — Drawable::BindChild used to bind the UIRenderer's own
		// Bindable child list (shaders/topology/etc.). The Component-shell
		// migration moved those slots up to UIRenderer itself (SetVS /
		// SetPS / SetTexture / SetTopology) so a Component-only UI
		// element can drive its draw through here without a paired
		// MeshRendererComponent.

		if (m_pCamera)
		{
			m_pCamera->PostUpdate(0.f);
			m_pCamera->Update(0.f);

			if (m_pParentTransform)
			{
				m_pParentTransform->UpdateCameraRelateMatrix(m_pCamera);
				m_pParentTransform->Bind();
			}
		}
		else if (m_pParentTransform)
		{
			m_pParentTransform->Bind();
		}

		if (m_pVS)       m_pVS->Bind();
		if (m_pPS)       m_pPS->Bind();
		if (m_pTexture)  m_pTexture->Bind();
		if (m_pTopology) m_pTopology->Bind();

		if (m_pParentAnimation)
		{
			m_pParentAnimation->SetFinalBuffer();
		}

		if (m_pParentMesh)
		{
			// Clear the input layout: the UI VS pipeline reads only
			// SV_VertexID and never consumes VB inputs. A leftover IL
			// from the previous (opaque) pass would mismatch the bound
			// VB or VS signature; null IL keeps the IA passive while
			// Mesh::Draw still binds a VB.
			Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(nullptr);
			Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;

			// Optional scissor clip (ScrollView trims items to its viewport).
			// Swap in a scissor-enabled rasterizer state + rect for this draw,
			// then restore whatever state was bound so other UI is unaffected.
			if (m_bClip)
			{
				auto* pDC = Graphics::GetInst()->GetDeviceContext();
				const D3D11_RECT rc = {
					static_cast<LONG>(m_fClip[0]),
					static_cast<LONG>(m_fClip[1]),
					static_cast<LONG>(m_fClip[0] + m_fClip[2]),
					static_cast<LONG>(m_fClip[1] + m_fClip[3]) };
				pDC->RSSetScissorRects(1, &rc);
				// Lazy fetch like the cbuffers above; keeps clones working.
				if (!m_pScissorRS) m_pScissorRS = GetScissorRasterizerState();
				if (m_pScissorRS) m_pScissorRS->Bind();
			}

			m_pParentMesh->Draw();

			if (m_bClip && m_pScissorRS)
			{
				m_pScissorRS->PostBind();   // restores the previously-bound state
			}
		}

		// Tear down the shader / texture state we set so a subsequent
		// pass starts from a clean slot 0 / VS / PS. Mirrors the manual
		// cleanup HPBar used to do at the end of its render — moved here
		// once UIRenderer owns the shader binding.
		if (m_pVS || m_pPS || m_pTexture)
		{
			auto* pDC = Graphics::GetInst()->GetDeviceContext();
			ID3D11ShaderResourceView* pNullSRV[1] = { nullptr };
			pDC->PSSetShaderResources(0, 1, pNullSRV);
			pDC->VSSetShaderResources(0, 1, pNullSRV);
			if (m_pVS) pDC->VSSetShader(nullptr, nullptr, 0);
			if (m_pPS) pDC->PSSetShader(nullptr, nullptr, 0);
			Graphics::GetInst()->ResetBindCache();
		}
	}

	std::shared_ptr<Component> UIRenderer::Clone()
	{
		return std::make_shared<UIRenderer>(*this);
	}
}
