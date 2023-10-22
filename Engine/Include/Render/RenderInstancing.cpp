#include "RenderInstancing.h"
#include "../Core/Graphics.h"
#include "../Bindable/Drawable.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/IndexBuffer.h"
#include "../Bindable/Texture.h"
#include "../Bindable/Mesh.h"
#include "../Bindable/BindableManager.h"
#include "../Animation/Sequence.h"
#include "../Bindable/Animation.h"
#include "../Bindable/ComputeShader.h"
#include "../Bindable/ComputeCBuffer.h"
#include "../Animation/Skeleton.h"
#include "../Animation/JointSocket.h"

namespace Engine
{
	RenderInstancing::RenderInstancing(const std::shared_ptr<class Mesh>& pMesh,
		const std::shared_ptr<class InputLayout>& pInputLayout, const std::shared_ptr<class VertexShader>& pVertexShader, const std::shared_ptr<class VertexShader>& pVertexShadowShader,
		const std::shared_ptr<class PixelShader>& pPixelShader, int iInstSize,
		const std::vector<std::shared_ptr<class Texture>>& vecTexture) :
		m_pInstBuffer(nullptr)
		, m_iMaxCount(500)
		, m_pMesh(pMesh)
		, m_iInstSize(iInstSize)
		, m_pInputLayout(pInputLayout)
		, m_pVertexShader(pVertexShader)
		, m_pPixelShader(pPixelShader)
		, m_vecTexture(vecTexture)
		, m_pVertexShadowShader(pVertexShadowShader)
		, m_pAnimationComputeShader(StaticFindBindable<ComputeShader>("SequenceInst"))
		, m_pAnimPaletteBuffer(nullptr)
		, m_pBoneConstBuffer(StaticFindBindable<ComputeCBuffer<BONECBUFFER>>("Bone"))
		, m_pBoneVertexBuffer(StaticFindBindable<VertexCBuffer<BONECBUFFER>>("Bone"))
		, m_pSkeleton(nullptr)
		, m_pSkeletonBuffer(nullptr)
		, m_pJointSocketBuffer(nullptr)
	{
		CreateInstBuffer();
	}

	int RenderInstancing::GetCount() const
	{
		return static_cast<int>(m_RenderList.size());
	}

	void RenderInstancing::Clear()
	{
		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList.begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList.end();

		for (; iter != iterEnd; )
		{
			(*iter)->SetInstancing(nullptr);
			iter = m_RenderList.erase(iter);
			iterEnd = m_RenderList.end();
		}

		m_RenderList.clear();
	}

	const std::list<class std::shared_ptr<class Drawable>>& RenderInstancing::GetRenderList()	const
	{
		return m_RenderList;
	}

	void RenderInstancing::CreateBoneBuffer(const std::unordered_map<std::string, std::shared_ptr<class Sequence>>& mapSequence)
	{
		m_tBoneCBuffer.iMaxFrame = INT_MIN;
		m_tBoneCBuffer.iMaxJoint = 0;

		std::unordered_map<std::string, std::shared_ptr<Sequence>>::const_iterator iter = mapSequence.begin();
		std::unordered_map<std::string, std::shared_ptr<Sequence>>::const_iterator iterEnd = mapSequence.end();

		for (; iter != iterEnd; ++iter)
		{
			Sequence::PSEQUENCEINFO pInfo = iter->second->GetSequenceInfo();

			m_tBoneCBuffer.iMaxJoint = static_cast<int>(pInfo->vecPose.size());

			if (m_tBoneCBuffer.iMaxFrame < iter->second->GetMaxFrame())
			{
				m_tBoneCBuffer.iMaxFrame = iter->second->GetMaxFrame();
			}
		}

		if (m_tBoneCBuffer.iMaxFrame < 0)
		{
			assert(false);
			return;
		}

		iter = mapSequence.begin();
		iterEnd = mapSequence.end();

		std::vector<TRANSFORM>  vecTransform(m_tBoneCBuffer.iMaxFrame * m_tBoneCBuffer.iMaxJoint * mapSequence.size());

		for (int k=0; iter != iterEnd; ++iter, ++k)
		{
			Sequence::PSEQUENCEINFO pInfo = iter->second->GetSequenceInfo();

			for (size_t i = 0; i < pInfo->vecPose.size(); ++i)
			{
				for (size_t j = 0; j < pInfo->vecPose[i].vecJoint.size(); ++j)
				{
					int iIndex = (k * m_tBoneCBuffer.iMaxJoint + static_cast<int>(i)) * m_tBoneCBuffer.iMaxFrame + static_cast<int>(j);

					vecTransform[iIndex].vPos = pInfo->vecPose[i].vecJoint[j].vPos;
					vecTransform[iIndex].vQueternion = pInfo->vecPose[i].vecJoint[j].vQueternion;
					vecTransform[iIndex].vScale = pInfo->vecPose[i].vecJoint[j].vScale;
				}
			}
		}

		m_pAnimPaletteBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(vecTransform.size()), static_cast<int>(sizeof(TRANSFORM)), &vecTransform.front(), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE);

		m_pBoneDataBuffer = std::make_shared<StructuredBuffer>(m_iMaxCount, static_cast<int>(sizeof(BONEINSTDATA)), nullptr, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE);

		m_pFinalBuffer = std::make_shared<StructuredBuffer>(m_iMaxCount * m_tBoneCBuffer.iMaxJoint, static_cast<int>(sizeof(Matrix)));
	}

	void RenderInstancing::SetSkeleton(std::shared_ptr<class Skeleton> pBuffer)
	{
		m_pSkeleton = pBuffer;

		if (m_pSkeleton)
		{
			std::vector<Matrix> matBone;

			const std::vector<PBONE>& vecJoints = m_pSkeleton->GetJoints();

			for (size_t i = 0; i < vecJoints.size() * m_iMaxCount; ++i)
			{
				matBone.push_back(vecJoints[i % vecJoints.size()]->matInvBindPose);

				matBone.back().Transpose();
			}

			m_pSkeletonBuffer = std::make_shared<StructuredBuffer>(m_iMaxCount * static_cast<int>(vecJoints.size()), static_cast<int>(sizeof(Matrix)), &matBone.front());
		}
	}

	void RenderInstancing::SetJointSocketBuffer(std::shared_ptr<StructuredBuffer> pBuffer)
	{
		m_pJointSocketBuffer = pBuffer;
	}

	void RenderInstancing::CreateInstBuffer()
	{
		D3D11_BUFFER_DESC desc = {};

		desc.ByteWidth = m_iMaxCount * m_iInstSize;

		if (desc.ByteWidth)
		{
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&desc, nullptr, &m_pInstBuffer)))
			{
				assert(false);
				return;
			}
		}
	}

	void RenderInstancing::AddDrawable(const std::shared_ptr<Drawable>& pDrawable)
	{
		pDrawable->SetInstancing(this);

		m_RenderList.push_back(pDrawable);

		if (m_RenderList.size() > m_iMaxCount)
		{
			m_iMaxCount *= 2;

			CreateInstBuffer();
		}
	}

	void RenderInstancing::Update()
	{
		if (!m_pAnimPaletteBuffer)
		{
			return;
		}

		std::vector<BONEINSTDATA> vecBoneData(m_RenderList.size());

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList.begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList.end();

		for (int i = 0; iter != iterEnd; ++iter, ++i)
		{
			std::shared_ptr<Animation> pAnimation = (*iter)->GetAnimation();
			std::shared_ptr<Sequence> pSequence = pAnimation->GetCurrentSequence();

			vecBoneData[i].iAnimationID = pAnimation->GetCurrentAnimID();
			vecBoneData[i].iMaxFrame = pSequence->GetMaxFrame();
			vecBoneData[i].iFrame = static_cast<int>(pAnimation->GetTime() / pSequence->GetMaxTime() * vecBoneData[i].iMaxFrame);
			vecBoneData[i].iNextFrame = (vecBoneData[i].iFrame + 1) % vecBoneData[i].iMaxFrame;
			vecBoneData[i].fTime = pAnimation->GetTime() / pSequence->GetMaxTime() * vecBoneData[i].iMaxFrame - static_cast<float>(vecBoneData[i].iFrame);
			vecBoneData[i].iRootPos = pSequence->IsRootMotion();

			const std::list<std::shared_ptr<JointSocket>>& SocketList = pAnimation->GetSocketList();

			std::list<std::shared_ptr<JointSocket>>::const_iterator iterS = SocketList.begin();
			std::list<std::shared_ptr<JointSocket>>::const_iterator iterSEnd = SocketList.end();

			for (; iterS != iterSEnd; ++iterS)
			{
				std::shared_ptr<Drawable> pDrawable = (*iterS)->GetDrawable();

				if (!pDrawable) {
					continue;
				}

				pDrawable->SetInstID(i);
				pDrawable->SetParentJointCount(m_tBoneCBuffer.iMaxJoint);

				RenderInstancing* pInstancing = pDrawable->GetInstancing();

				if (!pInstancing) {
					continue;
				}

				pInstancing->SetJointSocketBuffer(m_pFinalBuffer);
			}

		}

		m_pBoneDataBuffer->WriteData(&vecBoneData.front(), static_cast<int>(vecBoneData.size()));

		m_pAnimPaletteBuffer->SetSRV(33);

		m_pBoneDataBuffer->SetSRV(34);

		m_pFinalBuffer->SetUAV(0);

		m_pBoneConstBuffer->UpdateBuffer(m_tBoneCBuffer);
		m_pBoneConstBuffer->Bind();

		m_pSkeletonBuffer->SetSRV(30);

		m_pAnimationComputeShader->Bind();

		m_pAnimationComputeShader->Dispatch(m_tBoneCBuffer.iMaxJoint / 32 + static_cast<bool>(m_tBoneCBuffer.iMaxJoint % 32), static_cast<int>(vecBoneData.size()) / 32 + static_cast<bool>(vecBoneData.size() % 32));

		m_pSkeletonBuffer->ResetSRV(30);
		m_pAnimPaletteBuffer->ResetSRV(33);

		m_pBoneDataBuffer->ResetSRV(34);

		m_pFinalBuffer->ResetUAV(0);

#ifdef _DEBUG
		//std::vector<TRANSFORM> vecPaletteBuffer(m_tBoneCBuffer.iMaxFrame * m_tBoneCBuffer.iMaxJoint);
		//m_pAnimPaletteBuffer->DebugBuffer(&vecPaletteBuffer.front(),0, m_tBoneCBuffer.iMaxFrame * m_tBoneCBuffer.iMaxJoint * sizeof(TRANSFORM));

		//Matrix vecSkel;

		//m_pSkeletonBuffer->DebugBuffer(&vecSkel, 0, sizeof(Matrix));

		//std::vector<Matrix> matFinal(m_tBoneCBuffer.iMaxJoint);
		//m_pFinalBuffer->DebugBuffer(&matFinal.front(), 0, m_tBoneCBuffer.iMaxJoint * 64);
#endif
	}

	void RenderInstancing::PreRender()
	{
		D3D11_MAPPED_SUBRESOURCE sub = {};

		Graphics::GetInst()->GetDeviceContext()->Map(*m_pInstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub);

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList.begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList.end();

		int iIndex = 0;

		char* pData = reinterpret_cast<char*>(sub.pData);

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->GetInstData(pData, m_iInstSize);

			pData += m_iInstSize;
		}

		Graphics::GetInst()->GetDeviceContext()->Unmap(*m_pInstBuffer, 0);

	}

	void RenderInstancing::Draw()
	{
		if (!m_RenderList.size())
		{
			return;
		}

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pInputLayout->Bind();

		if (m_pJointSocketBuffer)
		{
			m_pJointSocketBuffer->SetSRV(32);
		}

		if (m_pFinalBuffer)
		{
			m_pBoneVertexBuffer->UpdateBuffer(m_tBoneCBuffer);
			m_pBoneVertexBuffer->Bind();
			m_pFinalBuffer->SetSRV(30);
		}

		m_pMesh->DrawInst(static_cast<int>(m_RenderList.size()), m_iInstSize, m_pInstBuffer);

		if (m_pFinalBuffer)
		{
			m_pFinalBuffer->ResetSRV(30);
		}

		if (m_pJointSocketBuffer)
		{
			m_pJointSocketBuffer->ResetSRV(32);
		}
	}

	void RenderInstancing::Render()
	{
		m_pVertexShader->Bind();

		m_pPixelShader->Bind();

		for (size_t i = 0; i < m_vecTexture.size(); ++i)
		{
			m_vecTexture[i]->Bind();
		}

		Draw();
	}

	void RenderInstancing::RenderShadow()
	{
		if (m_pVertexShadowShader)
		{
			m_pVertexShadowShader->Bind();
		}

		Draw();
	}
}