#include "Fluid.h"
#include "ConstantBuffer.h"
#include "ComputeShader.h"
#include "../Shader/StructuredBuffer.h"
#include "Mesh.h"
#include "BindableManager.h"
#include "InputLayout.h"
#include "Topology.h"
#include "../Input/Input.h"
#include "TransformBuffer.h"

Engine::Fluid::Fluid()	:
	m_pBuffer()
	, m_iCurrentBuffer(0)
	, m_tCBuffer()
	, m_iHeight()
{
}

Engine::Fluid::Fluid(int n, int m, float d, float mu, float c, float t)	:
	Drawable()
	, m_pBuffer()
	, m_iCurrentBuffer(0)
	, m_pCS(StaticFindBindable<ComputeShader>("FluidCS"))
	, m_pCBuffer(FindAndAddBind<ConstantBuffer<FLUIDCBUFFER>>("FluidCBuffer"))
	, m_iHeight(m + 1)
{
	assert(c < d / 2 / t * sqrtf(mu * t + 2.f));
	assert(t < (mu + sqrtf(mu * mu + 32 * c * c / d / d)) / (8.f * c * c / d / d));

	m_tCBuffer.c1 = (4.f - 8.f * c * c * t * t / d / d) / (mu * t + 2.f);
	m_tCBuffer.c2 = (mu* t - 2.f) / (mu * t + 2.f);
	m_tCBuffer.c3 = 2 * c * c * t * t / d / d / (mu * t + 2.f);
	m_tCBuffer.iWidth = n + 1;
	m_tCBuffer.dist = d;

	FindAndAddBind<VertexShader>("FluidVS");
	FindAndAddBind<PixelShader>("AlphaNoUVPS");
	FindAndAddBind<InputLayout>(STANDARD_INPUT_LAYOUT);
	FindAndAddBind<Topology>("TriangleList");

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	AddChild(pMaterial->Clone());

	GetTransform()->SetScale(d, GetTransform()->GetScale().y, d);

	Ready();
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

	CreateBindable<Mesh>("FluidMesh", vecVertex, vecIndex);
}

void Engine::Fluid::Input(float fDeltaTime)
{
	if (CInput::GetInst()->IsKey(CInput::KEY_STATE::UP, DIK_SPACE))
	{
		int x = rand() % m_tCBuffer.iWidth;
		int z = rand() % m_iHeight;

		float fHeight = 15.f;

		m_pBuffer[m_iCurrentBuffer]->WriteData(&fHeight, 4 * (x + z * m_tCBuffer.iWidth), 4);
	}
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

	m_pBuffer[iNextBuffer]->ResetUAV(4);

	m_pBuffer[m_iCurrentBuffer]->ResetSRV(39);

	m_pBuffer[(m_iCurrentBuffer + FLUID_BUFFER_COUNT - 1) % FLUID_BUFFER_COUNT]->ResetSRV(38);

	m_iCurrentBuffer = iNextBuffer;
}

void Engine::Fluid::Bind()
{
	m_pBuffer[m_iCurrentBuffer]->SetSRV(39);

	__super::Bind();

	m_pBuffer[m_iCurrentBuffer]->ResetSRV(39);
}

void Engine::Fluid::Save(FILE* pFile)
{
	__super::Save(pFile);

	fwrite(&m_tCBuffer, sizeof(FLUIDCBUFFER), 1, pFile);
	fwrite(&m_iHeight, 4, 1, pFile);
}

void Engine::Fluid::Load(FILE* pFile)
{
	__super::Load(pFile);

	fread(&m_tCBuffer, sizeof(FLUIDCBUFFER), 1, pFile);
	fread(&m_iHeight, 4, 1, pFile);

	m_pCS = std::static_pointer_cast<ComputeShader>(FindChild("FluidCS"));
	m_pCBuffer = std::static_pointer_cast<ConstantBuffer<FLUIDCBUFFER>>(FindChild("FluidCBuffer"));

	Ready();
}

void Engine::Fluid::Ready()
{
	for (int i = 0; i < FLUID_BUFFER_COUNT; ++i)
	{
		m_pBuffer[i] = std::make_shared<StructuredBuffer>(m_iHeight * m_tCBuffer.iWidth, 4);
	}

	CreateVertexBufferAndIndexBuffer(m_tCBuffer.iWidth - 1, m_iHeight - 1);
}
