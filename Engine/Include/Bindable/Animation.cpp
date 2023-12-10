#include "Animation.h"
#include "../Animation/Sequence.h"
#include "../Animation/Skeleton.h"
#include "../Animation/JointSocket.h"
#include "BindableManager.h"
#include "../Bindable/ComputeShader.h"
#include "../Bindable/Drawable.h"
#include "../Bindable/TransformBuffer.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Resource/ResourceManager.h"

namespace Engine
{
	Animation::Animation() :
		Bindable()
		, m_pCurrentSequence(nullptr)
		, m_pSkeleton(nullptr)
		, m_pComputeShader(StaticFindBindable<ComputeShader>("Sequence"))
		, m_pPostProcessShader(StaticFindBindable<ComputeShader>("PostProcess"))
		, m_pMidBuffer()
		, m_fTime(0.f)
		, m_pIKCBuffer(StaticFindBindable<ConstantBuffer<IKCBUFFER>>("IK"))
		, m_pOwner(nullptr)
		, m_fRate(1.f)
	{
		SetBindableType(Engine::BINDABLE_TYPE::ANIMATION);
	}

	Animation::Animation(const Animation& animation) :
		Bindable(animation)
		, m_mapSequence(animation.m_mapSequence)
		, m_pCurrentSequence(animation.m_pCurrentSequence)
		, m_pSkeleton(animation.m_pSkeleton)
		, m_pComputeShader(animation.m_pComputeShader)
		, m_pPostProcessShader(animation.m_pPostProcessShader)
		, m_pMidBuffer(m_pSkeleton ? std::make_shared<StructuredBuffer>(m_pSkeleton->GetBoneCount(), static_cast<int>(sizeof(Matrix))) : nullptr)
		, m_pFinalBuffer(m_pSkeleton ? std::make_shared<StructuredBuffer>(m_pSkeleton->GetBoneCount(), static_cast<int>(sizeof(Matrix))) : nullptr)
		, m_pPoseBuffer(m_pSkeleton ? std::make_shared<StructuredBuffer>(m_pSkeleton->GetBoneCount(), static_cast<int>(sizeof(Matrix))) : nullptr)
		, m_fTime(0.f)
		, m_pIKCBuffer(animation.m_pIKCBuffer)
		, m_pOwner(nullptr)
		, m_fRate(animation.m_fRate)
	{
	}

	void Animation::AddSequance(const std::string& strTag, const std::shared_ptr<Sequence>& pSequence)
	{
		if (!pSequence)
		{
			return;
		}

		std::shared_ptr<Sequence> pSequance = FindSequence(strTag);

		if (pSequance)
		{
#ifdef _DEBUG
			Sequence::PSEQUENCEINFO pPrevInfo = pSequance->GetSequenceInfo();

			if (!pPrevInfo)
			{
				assert(false);
				return;
			}

			Sequence::PSEQUENCEINFO pNewInfo = pSequence->GetSequenceInfo();

			if (!pNewInfo)
			{
				assert(false);
				return;
			}

			for (int i = 0; i < pPrevInfo->vecPose.size(); ++i)
			{
				for (int j = 0; j < pPrevInfo->vecPose[i].vecJoint.size(); ++j)
				{
					if (pPrevInfo->vecPose[i].vecJoint[j].dTime == pNewInfo->vecPose[i].vecJoint[j].dTime)
					{
						assert(pPrevInfo->vecPose[i].vecJoint[j].vPos == pNewInfo->vecPose[i].vecJoint[j].vPos);
						assert(pPrevInfo->vecPose[i].vecJoint[j].vScale == pNewInfo->vecPose[i].vecJoint[j].vScale);
						assert(pPrevInfo->vecPose[i].vecJoint[j].vQueternion == pNewInfo->vecPose[i].vecJoint[j].vQueternion);
					}
				}
			}
#endif
			return;
		}

		m_mapSequence.insert(std::make_pair(strTag, pSequence));

		if (!m_pCurrentSequence)
		{
			m_pCurrentSequence = pSequence;
		}
	}

	void Animation::AddSequance(const std::string& strTag, const std::vector<FbxLoader::FBXBONEKEYFRAME>& vecPose)
	{
		std::shared_ptr<Sequence> pSequence = std::make_shared<Sequence>();

		pSequence->SetTag(strTag);

		pSequence->SetSequance(vecPose);

		AddSequance(strTag, pSequence);
	}

	void Animation::ChangeSequence(const std::string& strTag)
	{
		std::shared_ptr<Sequence> pSequence = FindSequence(strTag);

		if (m_pCurrentSequence == pSequence)
		{
			return;
		}

		m_pCurrentSequence = pSequence;

		m_fTime = 0.f;
	}

	std::shared_ptr<JointSocket> Animation::AddSocket(int iJointIndex, const std::shared_ptr<Drawable>& pDrawable)
	{
		std::shared_ptr<JointSocket> pJointSocket = std::make_shared<JointSocket>();

		pJointSocket->SetDrawable(pDrawable);

		AddSocket(iJointIndex, pJointSocket);

		return pJointSocket;
	}

	std::shared_ptr<class Sequence> Animation::GetCurrentSequence() const
	{
		return m_pCurrentSequence;
	}

	const std::list<std::shared_ptr<class JointSocket>>& Animation::GetSocketList() const
	{
		return m_SocketList;
	}

	void Animation::AddSocket(const std::string& strJoint, const std::shared_ptr<JointSocket>& pSocket)
	{
		AddSocket(m_pSkeleton->FindJointIndex(strJoint), pSocket);
	}

	void Animation::AddSocket(int iJoint, const std::shared_ptr<JointSocket>& pSocket)
	{
		if (iJoint == -1)
		{
			return;
		}

		pSocket->SetParentIndex(iJoint);

		m_SocketList.push_back(pSocket);
	}

	std::shared_ptr<Sequence> Animation::FindSequence(const std::string& strTag) const
	{
		std::unordered_map<std::string, std::shared_ptr<Sequence>>::const_iterator iter = m_mapSequence.find(strTag);

		if (iter == m_mapSequence.end())
		{
			return nullptr;
		}

		return iter->second;
	}

	PIKINFO Animation::FindIkInfo(int iIndex)
	{
		for (int i = 0; i < static_cast<int>(m_vecIKInfo.size()); ++i)
		{
			if (m_vecIKInfo[i].iJointIndex == iIndex)
			{
				return &m_vecIKInfo[i];
			}
		}

		return nullptr;
	}

	void Animation::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (!m_pCurrentSequence)
		{
			return;
		}

		m_fTime += fDeltaTime * m_fRate;

		if (m_fTime >= m_pCurrentSequence->GetMaxTime())
		{
			m_fTime -= static_cast<int>((m_fTime / m_pCurrentSequence->GetMaxTime())) * m_pCurrentSequence->GetMaxTime();
		}

		std::list<std::shared_ptr<JointSocket>>::iterator iter = m_SocketList.begin();
		std::list<std::shared_ptr<JointSocket>>::iterator iterEnd = m_SocketList.end();

		for (; iter != iterEnd; ++iter)
		{
			std::shared_ptr<Transform> pTransform = m_pOwner->GetTransform();

			(*iter)->Update(m_pPoseBuffer, pTransform->GetTransformMatrix());
		}
	}

	void Animation::Bind()
	{
		__super::Bind();

		if (!m_pCurrentSequence)
		{
			return;
		}

		m_pMidBuffer->SetUAV(0);
		m_pPoseBuffer->SetUAV(1);

		UpdateMatrix();

		m_pMidBuffer->ResetUAV(0);
		m_pPoseBuffer->ResetUAV(1);

		MatrixPostProcess();

		m_pFinalBuffer->SetSRV(30);
	}

	void Animation::PostBind()
	{
		__super::PostBind();

		m_pMidBuffer->ResetSRV(30);
	}

	std::shared_ptr<Bindable> Animation::Clone()
	{
		return std::make_shared<Animation>(*this);
	}

	void Animation::Save(FILE* pFile)
	{
		__super::Save(pFile);

		int iSequenceCount = static_cast<int>(m_mapSequence.size());

		fwrite(&iSequenceCount, 4, 1, pFile);

		std::unordered_map<std::string, std::shared_ptr<Sequence>>::iterator iter = m_mapSequence.begin();
		std::unordered_map<std::string, std::shared_ptr<Sequence>>::iterator iterEnd = m_mapSequence.end();

		for (; iter != iterEnd; ++iter)
		{
			int iLength = static_cast<int>(iter->second->GetTag().length());

			fwrite(&iLength, 4, 1, pFile);

			assert(iLength);

			fwrite(iter->second->GetTag().c_str(), 1, iLength, pFile);
		}

		bool bCurrentSequence = static_cast<bool>(m_pCurrentSequence);

		fwrite(&bCurrentSequence, 1, 1, pFile);

		if (bCurrentSequence)
		{
			int iLength = static_cast<int>(m_pCurrentSequence->GetTag().length());

			fwrite(&iLength, 4, 1, pFile);

			fwrite(m_pCurrentSequence->GetTag().c_str(), 1, iLength, pFile);
		}

		int iJointCount = static_cast<int>(m_SocketList.size());

		fwrite(&iJointCount, 4, 1, pFile);

		std::list<std::shared_ptr<JointSocket>>::iterator iterJ = m_SocketList.begin();
		std::list<std::shared_ptr<JointSocket>>::iterator iterJEnd = m_SocketList.end();

		for(;iterJ!=iterJEnd;++iterJ)
		{
			(*iterJ)->Save(pFile);
		}

		bool bSkeleton = static_cast<bool>(m_pSkeleton);

		fwrite(&bSkeleton, 1, 1, pFile);

		if (bSkeleton)
		{
			size_t iLength = m_pSkeleton->GetTag().length();

			fwrite(&iLength, 4, 1, pFile);

			assert(iLength);

			fwrite(m_pSkeleton->GetTag().c_str(), 1, iLength, pFile);
		}

		fwrite(&m_fTime, 4, 1, pFile);

		int iIkCount = static_cast<int>(m_vecIKInfo.size());

		fwrite(&iIkCount, 4, 1, pFile);

		if (iIkCount)
		{
			fwrite(&m_vecIKInfo[0], sizeof(IKINFO), iIkCount, pFile);
		}

		fwrite(&m_fRate, 4, 1, pFile);
	}

	void Animation::Load(FILE* pFile)
	{
		__super::Load(pFile);

		int iSequenceCount = 0;

		fread(&iSequenceCount, 4, 1, pFile);

		for (int i = 0; i < iSequenceCount; ++i)
		{
			int iLength = 0;

			fread(&iLength, 4, 1, pFile);

			std::unique_ptr<char[]> strSequence = std::make_unique<char[]>(iLength + 1);

			strSequence[iLength] = 0;

			fread(strSequence.get(), 1, iLength, pFile);

			AddSequance(strSequence.get(), ResourceManager::GetInst()->FindSequence(strSequence.get()));
		}

		bool bSequence = false;

		fread(&bSequence, 1, 1, pFile);

		if (bSequence)
		{
			int iLength = 0;

			fread(&iLength, 4, 1, pFile);

			std::unique_ptr<char[]> strSeq = std::make_unique<char[]>(iLength + 1);

			strSeq[iLength] = 0;

			fread(strSeq.get(), 1, iLength, pFile);

			m_pCurrentSequence = FindSequence(strSeq.get());
		}

		int iJointCount = 0;

		fread(&iJointCount, 4, 1, pFile);

		for (int i = 0; i < iJointCount; ++i)
		{
			std::shared_ptr<JointSocket> pSocket = std::make_shared<JointSocket>();

			pSocket->Load(pFile);

			AddSocket(pSocket->GetParentIndex(), pSocket);
		}

		bool bSkeleton = false;

		fread(&bSkeleton, 1, 1, pFile);

		if (bSkeleton)
		{
			int iLength = 0;

			fread(&iLength, 4, 1, pFile);

			if (iLength)
			{
				std::unique_ptr<char[]> strSkeleton = std::make_unique<char[]>(iLength + 1);

				strSkeleton[iLength] = 0;

				fread(strSkeleton.get(), 1, iLength, pFile);

				SetSkeleton(ResourceManager::GetInst()->FindSkeleton(strSkeleton.get()));
			}
		}

		fread(&m_fTime, 4, 1, pFile);

		int iIkCount = 0;

		fread(&iIkCount, 4, 1, pFile);

		if (iIkCount)
		{
			m_vecIKInfo.resize(iIkCount);

			fread(&m_vecIKInfo[0], sizeof(IKINFO), iIkCount, pFile);
		}

		fread(&m_fRate, 4, 1, pFile);
	}

	void Animation::SetSkeleton(std::shared_ptr<Skeleton> pSkeleton)
	{
		m_pSkeleton = pSkeleton;

		if (m_pSkeleton)
		{
			m_pMidBuffer = std::make_shared<StructuredBuffer>(m_pSkeleton->GetBoneCount(), static_cast<int>(sizeof(Matrix)));
			m_pPoseBuffer = std::make_shared<StructuredBuffer>(m_pSkeleton->GetBoneCount(), static_cast<int>(sizeof(Matrix)));
			m_pFinalBuffer = std::make_shared<StructuredBuffer>(m_pSkeleton->GetBoneCount(), static_cast<int>(sizeof(Matrix)));
		}
	}

	std::shared_ptr<class Skeleton> Animation::GetSkeleton() const
	{
		return m_pSkeleton;
	}
	void Animation::UpdateMatrix()
	{
		m_pCurrentSequence->Update(m_fTime);

		m_pSkeleton->SetSRV();

		m_pComputeShader->Bind();

		//m_pIKCBuffer->UpdateBuffer(m_tIKInfo);

		//m_pIKCBuffer->Bind();

		int iJointCount = m_pSkeleton->GetBoneCount();

		m_pComputeShader->Dispatch(iJointCount / 32 + static_cast<bool>((iJointCount) % 32));

#ifdef _DEBUG
		//std::vector<Matrix> matFinal(iJointCount);

		//m_pFinalBuffer->DebugBuffer(&matFinal.front(), 0, 64 * iJointCount);
#endif

		m_pCurrentSequence->ResetResource();

		m_pComputeShader->PostBind();

		m_pSkeleton->ResetSRV();
	}
	void Animation::MatrixPostProcess()
	{
		std::vector<Matrix> vecKeyFrame(m_pSkeleton->GetBoneCount());

		m_pPoseBuffer->ReadBuffer(&vecKeyFrame.front(), 0, sizeof(Matrix) * m_pSkeleton->GetBoneCount());

		Vector3 pPos[256];

		for (int i = 0; i < m_pSkeleton->GetBoneCount(); ++i)
		{
			pPos[i].x = vecKeyFrame[i][0].w;
			pPos[i].y = vecKeyFrame[i][1].w;
			pPos[i].z = vecKeyFrame[i][2].w;
		}

		for (int j = 0; j < static_cast<int>(m_vecIKInfo.size()); ++j)
		{
			Vector3 vRootPos = { vecKeyFrame[m_vecIKInfo[j].iRootIndex][0].w ,vecKeyFrame[m_vecIKInfo[j].iRootIndex][1].w ,vecKeyFrame[m_vecIKInfo[j].iRootIndex][2].w};

			int iCurrentIndex = m_vecIKInfo[j].iJointIndex;

			const std::vector<PBONE>& vecBones = m_pSkeleton->GetBones();

			int iParentIndex = vecBones[m_vecIKInfo[j].iJointIndex]->iParent;

			std::vector<int> vecIndex;
			std::vector<float> vecDistance;

			Vector3 vPrevPos = m_vecIKInfo[j].vPosition;

			while (iParentIndex != vecBones[m_vecIKInfo[j].iRootIndex]->iParent)
			{
				vecIndex.push_back(iCurrentIndex);

				Vector3 vPos = pPos[iParentIndex];

				float fDistance = (vPos - pPos[iCurrentIndex]).Length();

				vecDistance.push_back(fDistance);

				iCurrentIndex = iParentIndex;

				iParentIndex = vecBones[iParentIndex]->iParent;
			}

			vecIndex.push_back(iCurrentIndex);

			for (int i = 0; i < 5; ++i)
			{
				pPos[m_vecIKInfo[j].iJointIndex] = m_vecIKInfo[j].vPosition;

				for (int i = 0; i < static_cast<int>(vecIndex.size()) - 1; ++i)
				{
					int iParentIndex = vecIndex[i + 1];

					int iCurrentIndex = vecIndex[i];

					if (vecDistance[i])
					{
						pPos[iParentIndex] = (pPos[iParentIndex] - pPos[iCurrentIndex]).Normalize() * vecDistance[i] + pPos[iCurrentIndex];
					}
					else
					{
						pPos[iParentIndex] = pPos[iCurrentIndex];
					}
				}

				pPos[vecIndex.back()] = vRootPos;

				for (int i = static_cast<int>(vecIndex.size()) - 1; i > 0; --i)
				{
					int iChildIndex = vecIndex[i - 1];

					int iCurrentIndex = vecIndex[i];

					if (vecDistance[i - 1])
					{
						pPos[iChildIndex] = (pPos[iChildIndex] - pPos[iCurrentIndex]).Normalize() * vecDistance[i - 1] + pPos[iCurrentIndex];
					}
					else
					{
						pPos[iChildIndex] = pPos[iCurrentIndex];
					}
				}
			}
		}

		for (int i = 0; i < m_pSkeleton->GetBoneCount(); ++i)
		{
			vecKeyFrame[i][0].w = pPos[i].x;
			vecKeyFrame[i][1].w = pPos[i].y;
			vecKeyFrame[i][2].w = pPos[i].z;
		}

		m_pPoseBuffer->WriteData(&vecKeyFrame.front(), 0, sizeof(Matrix)* m_pSkeleton->GetBoneCount());

		for (int i = 0; i < m_pSkeleton->GetBoneCount(); ++i)
		{
			vecKeyFrame[i].Transpose();

			vecKeyFrame[i] = m_pSkeleton->GetBone(i).matInvBindPose * vecKeyFrame[i];

			vecKeyFrame[i].Transpose();
		}

		m_pMidBuffer->WriteData(&vecKeyFrame.front(), 0, sizeof(Matrix) * m_pSkeleton->GetBoneCount());

		m_pFinalBuffer->SetUAV(0);

		m_pMidBuffer->SetSRV(30);

		m_pSkeleton->SetHierarchySRV();

		m_pPostProcessShader->Bind();

		int iJointCount = m_pSkeleton->GetBoneCount();

		m_pPostProcessShader->Dispatch(iJointCount / 32 + static_cast<bool>(iJointCount % 32));

		m_pSkeleton->ResetHierarchySRV();

		m_pMidBuffer->ResetSRV(30);

		m_pFinalBuffer->ResetUAV(0);
	}
	const std::unordered_map<std::string, std::shared_ptr<class Sequence>>& Animation::GetSequences() const
	{
		return m_mapSequence;
	}
	float Animation::GetTime() const
	{
		return m_fTime;
	}
	int Animation::GetCurrentAnimID() const
	{
		std::unordered_map<std::string, std::shared_ptr<Sequence>>::const_iterator iter = m_mapSequence.begin();
		std::unordered_map<std::string, std::shared_ptr<Sequence>>::const_iterator iterEnd = m_mapSequence.end();

		for (int i = 0; iter != iterEnd; ++iter, ++i)
		{
			if (m_pCurrentSequence == iter->second)
			{
				return i;
			}
		}

		return 0;
	}
	std::shared_ptr<StructuredBuffer> Animation::GetFinalBuffer() const
	{
		return m_pMidBuffer;
	}
	void Animation::AddIkInfo(int iJointIndex, int iRootIndex)
	{
		PIKINFO pInfo = FindIkInfo(iJointIndex);

		if (pInfo)
		{
			return;
		}

		IKINFO tInfo = {};

		tInfo.iJointIndex = iJointIndex;
		tInfo.iRootIndex = iRootIndex;

		m_vecIKInfo.push_back(tInfo);
	}
	void Animation::SetIkPosition(int iIndex, const Vector3& vPos)
	{
		IKINFO* pInfo = FindIkInfo(iIndex);

		if (!pInfo)
		{
			return;
		}

		pInfo->vPosition = vPos;
	}
	void Animation::SetOwner(Drawable* pOwner)
	{
		m_pOwner = pOwner;
	}
	void Animation::SetTime(float fTime)
	{
		m_fTime = fTime;
	}
	float Animation::GetRate() const
	{
		return m_fRate;
	}
	void Animation::SetRate(float fRate)
	{
		m_fRate = fRate;
	}
};;
