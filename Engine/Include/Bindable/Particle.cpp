#include "Particle.h"
#include "ComputeShader.h"
#include "ConstantBuffer.h"
#include "BindableManager.h"
#include "../Shader/StructuredBuffer.h"
#include "VertexShader.h"
#include "GeometryShader.h"
#include "PixelShader.h"
#include "Topology.h"
#include "InputLayout.h"
#include "BlendState.h"
#include "DepthStencilState.h"
#include "Transform.h"
#include "Texture.h"
#include "../Core/Graphics.h"
#include "../Render/RenderManager.h"

namespace Engine
{
	Particle::Particle()	:
		m_tCBuffer(0)
		, m_fElapsedTime(0.f)
		, m_fEmitMaxTime(1.f)
		, m_bStopEmit(false)
		, m_iPrevCreateGroupOffset(0)
		, m_iEmitCount(-1)
		, m_eRenderLayer(RENDER_LAYER::ALPHA)
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Particle::Particle(int iMaxCount)	:
		Component()
		, m_pVS(StaticFindBindable<VertexShader>("ParticleVS"))
		, m_pGS(StaticFindBindable<GeometryShader>("ParticleGS"))
		, m_pPS(StaticFindBindable<PixelShader>("ParticlePS"))
		, m_pCS(StaticFindBindable<ComputeShader>("ParticleCS"))
		, m_tCBuffer(iMaxCount)
		, m_pBuffer(std::make_shared<StructuredBuffer>(iMaxCount, static_cast<int>(sizeof(PARTICLE))))
		, m_pSystemBuffer(std::make_shared<StructuredBuffer>(static_cast<int>(ceil(iMaxCount / 64.f)), 4, nullptr, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS))
		, m_pParticleCBuffer(StaticFindBindable<ConstantBuffer<PARTICLECBUFFER>>("Particle"))
		, m_fElapsedTime(0.f)
		, m_fEmitMaxTime(1.f)
		, m_pBlendState(StaticFindBindable<BlendState>("AlphaBlend"))
		, m_bStopEmit(false)
		, m_iPrevCreateGroupOffset(0)
		, m_iEmitCount(-1)
		, m_eRenderLayer(RENDER_LAYER::ALPHA)
	{
		SetComponentType(COMPONENT_TYPE::NONE);
#ifdef _DEBUG
		m_vecPrevAlive.resize(m_tCBuffer.iMaxParticleCount);
#endif
	}

	Particle::Particle(const Particle& other) :
		Component(other)
		, m_pVS(other.m_pVS)
		, m_pGS(other.m_pGS)
		, m_pPS(other.m_pPS)
		, m_pCS(other.m_pCS)
		, m_tCBuffer(other.m_tCBuffer)
		, m_pBuffer(other.m_pBuffer)
		, m_pSystemBuffer(other.m_pSystemBuffer)
		, m_pParticleCBuffer(other.m_pParticleCBuffer)
		, m_fElapsedTime(other.m_fElapsedTime)
		, m_fEmitMaxTime(other.m_fEmitMaxTime)
		, m_pBlendState(other.m_pBlendState)
		, m_pTexture(other.m_pTexture)
		, m_pTransform(other.m_pTransform)
		, m_bStopEmit(other.m_bStopEmit)
		, m_iPrevCreateGroupOffset(other.m_iPrevCreateGroupOffset)
		, m_iEmitCount(other.m_iEmitCount)
		, m_eRenderLayer(other.m_eRenderLayer)
	{
	}

	bool Particle::Init()
	{
		if (!__super::Init()) return false;

		// Per-instance Transform — Particle owns its emitter anchor.
		if (!m_pTransform) m_pTransform = std::make_shared<Transform>();

		return true;
	}

	void Particle::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (m_pTransform) m_pTransform->Update(fDeltaTime);

		m_fElapsedTime += fDeltaTime;

		int iCreateCount = static_cast<int>(m_fElapsedTime / m_fEmitMaxTime);

		m_fElapsedTime -= iCreateCount * m_fEmitMaxTime;

		if (m_bStopEmit)
		{
			iCreateCount = 0;
		}
		else if(m_iEmitCount != -1)
		{
			if (iCreateCount > m_iEmitCount)
			{
				iCreateCount = m_iEmitCount;
			}

			m_iEmitCount -= iCreateCount;
		}

		if (m_pTransform) m_pTransform->Bind();

		m_pParticleCBuffer->UpdateBuffer(m_tCBuffer);

		m_pBuffer->SetUAV(2);

		int iGroupCount = static_cast<int>(ceil(m_tCBuffer.iMaxParticleCount / 64.f));

		std::vector<int> vecCreateCount(iGroupCount);

		for (int i = 0; i < iGroupCount; ++i)
		{
			vecCreateCount[i] = iCreateCount / iGroupCount;
		}

		for (int i = m_iPrevCreateGroupOffset; i < m_iPrevCreateGroupOffset + iCreateCount % iGroupCount; ++i)
		{
			++vecCreateCount[i % iGroupCount];
		}

		m_iPrevCreateGroupOffset = (m_iPrevCreateGroupOffset + iCreateCount % iGroupCount) % iGroupCount;

		m_pSystemBuffer->WriteData(&vecCreateCount[0], 0, 4 * iGroupCount);

		m_pSystemBuffer->SetUAV(3);

		m_pCS->Bind();

		m_pParticleCBuffer->Bind();

		m_pCS->Dispatch(iGroupCount);

		m_pCS->PostBind();

		m_pSystemBuffer->ResetUAV(3);

		m_pBuffer->ResetUAV(2);
	}

	void Particle::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		// Phase E5 — self-register with RenderManager so the alpha (or blur)
		// pass renders this emitter alongside Drawables in m_RenderList.
		auto pSelf = std::dynamic_pointer_cast<Particle>(shared_from_this());
		if (pSelf) RenderManager::GetInst()->AddParticle(pSelf);
	}

	void Particle::Bind()
	{
		// Transform CB binding (mirrors the old Drawable-era BindChild path
		// that Drawable::BindChild ran for us).
		if (m_pTransform) m_pTransform->Bind();

		if (m_pTexture) m_pTexture->Bind();

		m_pBuffer->SetSRV(40);

		m_pVS->Bind();

		m_pGS->Bind();

		m_pPS->Bind();

		m_pBlendState->Bind();

		Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(nullptr);
		// Direct IL/Topology set bypasses the bound caches — invalidate
		// them so the next drawable's Bind actually re-issues the
		// IASet* call instead of skipping on a stale cache hit.
		// Without this, debug colliders / alpha drawables that share
		// "Standard" IL pointer w/ a previously-bound drawable end up
		// drawing with no IL → DEVICE_DRAW_INPUTLAYOUT_NOT_SET.
		Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		Graphics::GetInst()->GetBindCache().eBoundTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

		Graphics::GetInst()->GetDeviceContext()->DrawInstanced(1, m_tCBuffer.iMaxParticleCount, 0, 0);

		m_pBlendState->PostBind();

		m_pVS->PostBind();

		m_pGS->PostBind();

		m_pPS->PostBind();

		m_pBuffer->ResetSRV(40);

		if (m_pTransform) m_pTransform->PostBind();
	}

	std::shared_ptr<Component> Particle::Clone()
	{
		return std::make_shared<Particle>(*this);
	}

	void Particle::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_tCBuffer, sizeof(PARTICLECBUFFER), 1, pFile);
		fwrite(&m_fElapsedTime, 4, 1, pFile);
		fwrite(&m_fEmitMaxTime, 4, 1, pFile);
	}

	void Particle::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_tCBuffer, sizeof(PARTICLECBUFFER), 1, pFile);
		fread(&m_fElapsedTime, 4, 1, pFile);
		fread(&m_fEmitMaxTime, 4, 1, pFile);

		m_pBuffer = std::make_shared<StructuredBuffer>(m_tCBuffer.iMaxParticleCount, static_cast<int>(sizeof(PARTICLE)));
		m_pSystemBuffer = std::make_shared<StructuredBuffer>(1, 4, nullptr, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS);
	}

	void Particle::SetStartColor(const Vector4& vColor)         { m_tCBuffer.vStartColor = vColor; }
	void Particle::SetEndColor(const Vector4& vColor)           { m_tCBuffer.vEndColor = vColor; }
	void Particle::SetVelocity(const Vector3& vVelocity)        { m_tCBuffer.vVelocity = vVelocity; }
	void Particle::SetAccelaration(const Vector3& vAccel)       { m_tCBuffer.vAccelation = vAccel; }
	void Particle::SetMaxLifeTime(float fMaxTime)               { m_tCBuffer.fMaxLifeTime = fMaxTime; }
	void Particle::SetMaxParticleCount(int iMaxCount)           { m_tCBuffer.iMaxParticleCount = iMaxCount; }
	void Particle::SetMaxCreatePosition(const Vector3& vEnd)    { m_tCBuffer.vMaximumPosition = vEnd; }
	void Particle::SetMinCreatePosition(const Vector3& vStart)  { m_tCBuffer.vMinimumPosition = vStart; }
	void Particle::SetEmitTime(float fEmitTime)                 { m_fEmitMaxTime = fEmitTime; }
	void Particle::SetStartSize(const Vector2& vSize)           { m_tCBuffer.vStartSize = vSize; }
	void Particle::SetEndSize(const Vector2& vSize)             { m_tCBuffer.vEndSize = vSize; }
	void Particle::SetMaxFrame(int iFrame)                      { m_tCBuffer.iMaxFrame = iFrame; }
	void Particle::SetFrameWidth(int iWidth)                    { m_tCBuffer.iFrameWidth = iWidth; }
	void Particle::SetFrameHeight(int iHeight)                  { m_tCBuffer.iFrameHeight = iHeight; }
	void Particle::SetMaxVelocity(const Vector3& vMaxVelocity)  { m_tCBuffer.vMaxVelocity = vMaxVelocity; }
	void Particle::StopEmit()                                   { m_bStopEmit = true; }
	void Particle::ResumeEmit()                                 { m_bStopEmit = false; }
	void Particle::AddEmitCount(int iCount)                     { m_iEmitCount += iCount; }
}
