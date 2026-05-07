#include "Fluid.h"
#include "ConstantBuffer.h"
#include "ComputeShader.h"
#include "../Shader/StructuredBuffer.h"
#include "Mesh.h"
#include "BindableManager.h"
#include "../Input/Input.h"

namespace Engine
{
	Fluid::Fluid()	:
		m_pBuffer()
		, m_iCurrentBuffer(0)
		, m_tCBuffer()
		, m_iHeight()
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	Fluid::Fluid(int n, int m, float d, float mu, float c, float t)	:
		Component()
		, m_pBuffer()
		, m_iCurrentBuffer(0)
		, m_pCS(StaticFindBindable<ComputeShader>("FluidCS"))
		, m_pCBuffer(StaticFindBindable<ConstantBuffer<FLUIDCBUFFER>>("FluidCBuffer"))
		, m_iHeight(m + 1)
	{
		SetComponentType(COMPONENT_TYPE::NONE);

		assert(c < d / 2 / t * sqrtf(mu * t + 2.f));
		assert(t < (mu + sqrtf(mu * mu + 32 * c * c / d / d)) / (8.f * c * c / d / d));

		m_tCBuffer.c1 = (4.f - 8.f * c * c * t * t / d / d) / (mu * t + 2.f);
		m_tCBuffer.c2 = (mu * t - 2.f) / (mu * t + 2.f);
		m_tCBuffer.c3 = 2 * c * c * t * t / d / d / (mu * t + 2.f);
		m_tCBuffer.iWidth = n + 1;
		m_tCBuffer.dist = d;

		// Phase E5 — GPU rendering resources (VS / PS / IL / Topology /
		// Material) and Transform setup used to be wired here through
		// Drawable's child API; they belong on a paired MeshRenderer-style
		// Component in any future GameObject-hosted fluid setup.

		Ready();
	}

	void Fluid::CreateVertexBufferAndIndexBuffer(int n, int m)
	{
		// 0 ~ n
		// 0 ~ m

		std::vector<VertexStandard> vecVertex;

		for (int i = 0; i <= n; ++i)
		{
			for (int j = 0; j <= m; ++j)
			{
				VertexStandard vertex;

				vertex.pos.x = static_cast<float>(i);
				vertex.pos.z = static_cast<float>(m - j);

				vecVertex.push_back(vertex);
			}
		}

		std::vector<unsigned int> vecIndex;

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < m; ++j)
			{
				int lt = i + j * (n + 1);
				int rt = i + 1 + j * (n + 1);
				int lb = i + (j + 1) * (n + 1);
				int rb = i + 1 + (j + 1) * (n + 1);

				if ((i + j) % 2 == 0)
				{
					vecIndex.push_back(lt);
					vecIndex.push_back(rb);
					vecIndex.push_back(rt);

					vecIndex.push_back(lt);
					vecIndex.push_back(lb);
					vecIndex.push_back(rb);
				}
				else
				{
					vecIndex.push_back(lt);
					vecIndex.push_back(lb);
					vecIndex.push_back(rt);

					vecIndex.push_back(rt);
					vecIndex.push_back(lb);
					vecIndex.push_back(rb);
				}
			}
		}

		m_pMesh = StaticCreateBindable<Mesh>("FluidMesh", vecVertex, vecIndex);
	}

	void Fluid::Input(float fDeltaTime)
	{
		if (CInput::GetInst()->IsKey(CInput::KEY_STATE::UP, DIK_SPACE))
		{
			int x = rand() % m_tCBuffer.iWidth;
			int z = rand() % m_iHeight;

			float fHeight = 15.f;

			m_pBuffer[m_iCurrentBuffer]->WriteData(&fHeight, 4 * (x + z * m_tCBuffer.iWidth), 4);
		}
	}

	void Fluid::FixedUpdate(float fDeltaTime)
	{
		m_pCBuffer->UpdateBuffer(m_tCBuffer);

		m_pCBuffer->Bind();

		int iNextBuffer = (m_iCurrentBuffer + 1) % FLUID_BUFFER_COUNT;

		m_pBuffer[(m_iCurrentBuffer + FLUID_BUFFER_COUNT - 1) % FLUID_BUFFER_COUNT]->SetSRV(38);

		m_pBuffer[m_iCurrentBuffer]->SetSRV(39);

		m_pBuffer[iNextBuffer]->SetUAV(4);

		m_pCS->Bind();

		m_pCS->Dispatch(m_tCBuffer.iWidth / 32 + static_cast<bool>(m_tCBuffer.iWidth % 32), m_iHeight / 32 + static_cast<bool>(m_iHeight % 32));

		m_pCS->PostBind();

		m_pBuffer[iNextBuffer]->ResetUAV(4);

		m_pBuffer[m_iCurrentBuffer]->ResetSRV(39);

		m_pBuffer[(m_iCurrentBuffer + FLUID_BUFFER_COUNT - 1) % FLUID_BUFFER_COUNT]->ResetSRV(38);

		m_iCurrentBuffer = iNextBuffer;
	}

	void Fluid::Bind()
	{
		// Phase E5 — Bind used to chain through Drawable::Bind to render
		// the displacement mesh. Now exposes only the displacement-buffer
		// SRV binding; mesh draw is the responsibility of the paired
		// renderer component (or render pass) that hosts this Fluid.
		if (m_pBuffer[m_iCurrentBuffer])
			m_pBuffer[m_iCurrentBuffer]->SetSRV(39);
	}

	std::shared_ptr<Component> Fluid::Clone()
	{
		return std::make_shared<Fluid>(*this);
	}

	void Fluid::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_tCBuffer, sizeof(FLUIDCBUFFER), 1, pFile);
		fwrite(&m_iHeight, 4, 1, pFile);
	}

	void Fluid::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_tCBuffer, sizeof(FLUIDCBUFFER), 1, pFile);
		fread(&m_iHeight, 4, 1, pFile);

		// Phase E5 — Drawable child-list lookup is gone; resources are
		// fetched from BindableManager by their well-known tags.
		m_pCS = StaticFindBindable<ComputeShader>("FluidCS");
		m_pCBuffer = StaticFindBindable<ConstantBuffer<FLUIDCBUFFER>>("FluidCBuffer");

		Ready();
	}

	void Fluid::Ready()
	{
		for (int i = 0; i < FLUID_BUFFER_COUNT; ++i)
		{
			m_pBuffer[i] = std::make_shared<StructuredBuffer>(m_iHeight * m_tCBuffer.iWidth, 4);
		}

		CreateVertexBufferAndIndexBuffer(m_tCBuffer.iWidth - 1, m_iHeight - 1);
	}
}
