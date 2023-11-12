#include "Fluid.h"
#include "ConstantBuffer.h"
#include "ComputeShader.h"
#include "../Shader/StructuredBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "BindableManager.h"

Engine::Fluid::Fluid(int n, int m, int d, float p, float mu, float c, float t)	:
	Drawable()
	, m_pBuffer()
	, m_iCurrentBuffer(0)
	, m_pCS(StaticFindBindable<ComputeShader>("FluidCS"))
	, m_pCBuffer(FindAndAddBind<ConstantBuffer<FLUIDCBUFFER>>("Fluid"))
	, m_iHeight(m)
{
	assert(c < d / 2 / t * sqrtf(mu * t + 2.f));
	assert(t < (mu + sqrtf(mu * mu + 32 * c * c / d / d)) / (8.f * c * c / d / d));

	m_tCBuffer.c1 = 4.f - 8.f * c * c * t * t / d / d;
	m_tCBuffer.c2 = (mu* t - 2.f) / (mu * t + 2.f);
	m_tCBuffer.c3 = 2 * c * c * t * t / d / d / (mu * t + 2.f);
	m_tCBuffer.iWidth = n;
	m_tCBuffer.dist = d;

	m_pBuffer[0] = std::make_shared<StructuredBuffer>(n * m, 4);
	m_pBuffer[1] = std::make_shared<StructuredBuffer>(n * m, 4);
	m_pBuffer[2] = std::make_shared<StructuredBuffer>(n * m, 4);

	CreateVertexBufferAndIndexBuffer(n, m);
}

void Engine::Fluid::CreateVertexBufferAndIndexBuffer(int n, int m)
{
	// 0 ~ n 
	// 0 ~ m

	std::vector<VertexStandard> vecVertex;

	for (int i = 0; i <= n; ++i)
	{
		for (int j = 0; j <= m; ++j)
		{
			VertexStandard vertex;

			vertex.pos.x = i;
			vertex.pos.z = m - j;

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
				vecIndex.push_back(rt);
				vecIndex.push_back(rb);

				vecIndex.push_back(lt);
				vecIndex.push_back(rb);
				vecIndex.push_back(lb);
			}
			else
			{
				vecIndex.push_back(lt);
				vecIndex.push_back(rt);
				vecIndex.push_back(lb);

				vecIndex.push_back(rt);
				vecIndex.push_back(rb);
				vecIndex.push_back(lb);
			}
		}
	}

	CreateBindable<VertexBuffer>("FluidVertex", vecVertex);
	CreateBindable<IndexBuffer>("FluidIndex", vecIndex);
}

void Engine::Fluid::FixedUpdate(float fDeltaTime)
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

	m_iCurrentBuffer = iNextBuffer;
}

void Engine::Fluid::Bind()
{
	m_pBuffer[m_iCurrentBuffer]->SetSRV(39);

	__super::Bind();
}
