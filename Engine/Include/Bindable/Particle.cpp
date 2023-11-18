#include "Particle.h"
#include "ComputeShader.h"
#include "ConstantBuffer.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "../Shader/StructuredBuffer.h"
#include "VertexShader.h"
#include "GeometryShader.h"
#include "PixelShader.h"
#include "Topology.h"
#include "BlendState.h"
#include "DepthStencilState.h"

namespace Engine
{
	Particle::Particle(int iMaxCount)	:
		Drawable()
		, m_pVS(StaticFindBindable<VertexShader>("ParticleVS"))
		, m_pGS(StaticFindBindable<GeometryShader>("ParticleGS"))
		, m_pPS(StaticFindBindable<PixelShader>("ParticlePS"))
		, m_pCS(StaticFindBindable<ComputeShader>("ParticleCS"))
		, m_tCBuffer(iMaxCount)
		, m_pBuffer(std::make_shared<StructuredBuffer>(iMaxCount, static_cast<int>(sizeof(PARTICLE))))
		, m_pSystemBuffer(std::make_shared<StructuredBuffer>(1, 4, nullptr, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS))
		, m_pTransformGSCBuffer(StaticFindBindable<ConstantBuffer<TRANSFORMBUFFER>>("Transform"))
		, m_pParticleGSCBuffer(StaticFindBindable<ConstantBuffer<PARTICLECBUFFER>>("Particle"))
		, m_pParticleCSCBuffer(StaticFindBindable<ConstantBuffer<PARTICLECBUFFER>>("Particle"))
		, m_fElapsedTime(0.f)
		, m_fEmitMaxTime(1.f)
		, m_pBlendState(StaticFindBindable<BlendState>("AlphaBlend"))
	{
		SetRenderLayer(RENDER_LAYER::ALPHA);
		SetBindableType(BINDABLE_TYPE::PARTICLE);
		FindAndAddBind<Topology>("PointList");
	}

	void Particle::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		m_fElapsedTime += fDeltaTime;

		int iCreateCount = static_cast<int>(m_fElapsedTime / m_fEmitMaxTime);

		m_fElapsedTime -= iCreateCount * m_fEmitMaxTime;

		m_tCBuffer.iCreateCount = iCreateCount;

		m_pParticleCSCBuffer->UpdateBuffer(m_tCBuffer);

		m_pBuffer->SetUAV(2);

		m_pSystemBuffer->WriteData(&iCreateCount, 0, 1);

		m_pSystemBuffer->SetUAV(3);

		m_pCS->Bind();

		m_pParticleCSCBuffer->Bind();

		m_pCS->Dispatch(m_tCBuffer.iMaxParticleCount / 64 + static_cast<bool>(m_tCBuffer.iMaxParticleCount % 64));

		m_pCS->PostBind();

		m_pSystemBuffer->ResetUAV(3);

		m_pBuffer->ResetUAV(2);

#ifdef _DEBUG
		std::vector<PARTICLE> vecParticle(m_tCBuffer.iMaxParticleCount);

		m_pBuffer->ReadBuffer(&vecParticle[0], 0, sizeof(PARTICLE) * m_tCBuffer.iMaxParticleCount);

		m_pSystemBuffer->ReadBuffer(&iCreateCount, 0, 4);
#endif
	}

	void Particle::Bind()
	{
		__super::Bind();

		m_pBuffer->SetSRV(40);

		m_pVS->Bind();

		m_pGS->Bind();

		m_pPS->Bind();

		m_pTransformGSCBuffer->Bind();

		m_pBlendState->Bind();

		Graphics::GetInst()->GetDeviceContext()->DrawInstanced(1, m_tCBuffer.iMaxParticleCount, 0, 0);

		m_pBlendState->PostBind();

		m_pVS->PostBind();

		m_pGS->PostBind();

		m_pPS->PostBind();

		m_pBuffer->ResetSRV(40);
	}

	void Particle::SetStartColor(const Vector4& vColor)
	{
		m_tCBuffer.vStartColor = vColor;
	}

	void Particle::SetEndColor(const Vector4& vColor)
	{
		m_tCBuffer.vEndColor = vColor;
	}

	void Particle::SetVelocity(const Vector3& vVelocity)
	{
		m_tCBuffer.vVelocity = vVelocity;
	}

	void Particle::SetAccelaration(const Vector3& vAccel)
	{
		m_tCBuffer.vAccelation = vAccel;
	}

	void Particle::SetMaxLifeTime(float fMaxTime)
	{
		m_tCBuffer.fMaxLifeTime = fMaxTime;
	}

	void Particle::SetMaxParticleCount(int iMaxCount)
	{
		m_tCBuffer.iMaxParticleCount = iMaxCount;
	}

	void Particle::SetMaxCreatePosition(const Vector3& vEnd)
	{
		m_tCBuffer.vMaximumPosition = vEnd;
	}

	void Particle::SetMinCreatePosition(const Vector3& vStart)
	{
		m_tCBuffer.vMinimumPosition = vStart;
	}
	void Particle::SetEmitTime(float fEmitTime)
	{
		m_fEmitMaxTime = fEmitTime;
	}
	void Particle::SetStartSize(const Vector2& vSize)
	{
		m_tCBuffer.vStartSize = vSize;
	}
	void Particle::SetEndSize(const Vector2& vSize)
	{
		m_tCBuffer.vEndSize = vSize;
	}
	void Particle::SetMaxFrame(int iFrame)
	{
		m_tCBuffer.iMaxFrame = iFrame;
	}
	void Particle::SetFrameWidth(int iWidth)
	{
		m_tCBuffer.iFrameWidth = iWidth;
	}
	void Particle::SetFrameHeight(int iHeight)
	{
		m_tCBuffer.iFrameHeight = iHeight;
	}
}