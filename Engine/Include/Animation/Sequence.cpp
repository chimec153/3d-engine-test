#include "Sequence.h"
#include "../Bindable/BindableManager.h"
#include "Skeleton.h"
#include "../Resource/ResourceManager.h"
#include "../Shader/StructuredBuffer.h"
#include "../Bindable/ConstantBuffer.h"

namespace Engine
{
	Sequence::Sequence() :
		m_pBoneConstantBuffer(StaticFindBindable<ConstantBuffer<BONECBUFFER>>("Bone"))
		, m_vecInfo()
		, m_bRootMotion(false)
		, m_pBuffer()
		, m_iMaxFrame(0)
		, fTime(0.f)
		, fMaxTime(0.f)
		, m_tCBuffer()
	{
	}

	Sequence::Sequence(const Sequence& seq)	:
		m_pBoneConstantBuffer(seq.m_pBoneConstantBuffer)
		, m_vecInfo(seq.m_vecInfo)
		, m_bRootMotion(seq.m_bRootMotion)
		, m_pBuffer(seq.m_pBuffer)
		, m_iMaxFrame(seq.m_iMaxFrame)
		, fTime(seq.fTime)
		, fMaxTime(seq.fMaxTime)
		, m_tCBuffer(seq.m_tCBuffer)
	{
		for (int i = 0; i < static_cast<int>(m_vecInfo.size()); ++i)
		{
			m_vecInfo[i] = dbg_new SEQUENCEINFO(*m_vecInfo[i]);
		}
	}

	Sequence::~Sequence()
	{
		Safe_Delete_VecList(m_vecInfo);
	}

	bool Sequence::SetSequance(const std::vector<FbxLoader::FBXBONEKEYFRAME>& vecPose)
	{
		m_iMaxFrame = INT_MIN;

		for (int i = 0; i < static_cast<int>(vecPose.size()); ++i)
		{
			if (m_iMaxFrame < static_cast<int>(vecPose[i].vecKeyFrame.size()))
			{
				m_iMaxFrame = vecPose[i].vecKeyFrame.size();
			}
		}

		if (!m_iMaxFrame)
		{
			assert(false);
			return false;
		}

		m_tCBuffer.iMaxJoint = static_cast<int>(vecPose.size());

		m_vecInfo.emplace_back(dbg_new SEQUENCEINFO());

		PSEQUENCEINFO pInfo = m_vecInfo.back();

		std::vector<TRANSFORM> vecTransform(m_tCBuffer.iMaxJoint * m_iMaxFrame);

		for (size_t i = 0; i < vecPose.size(); ++i)
		{
			pInfo->vecPose.emplace_back();

			POSE& tPose = pInfo->vecPose.back();

			float fTotal = 0.f;

			for (size_t j = 0; j < vecPose[i].vecKeyFrame.size(); ++j)
			{
				if (fTotal < static_cast<float>(vecPose[i].vecKeyFrame[j].dTime))
				{
					fTotal = static_cast<float>(vecPose[i].vecKeyFrame[j].dTime);
				}

				JOINT tJoint;

				fbxsdk::FbxVector4 vTranslate = vecPose[i].vecKeyFrame[j].matTransform.GetT();

				tJoint.vPos.x = static_cast<float>(vTranslate[0]);
				tJoint.vPos.y = static_cast<float>(vTranslate[1]);
				tJoint.vPos.z = static_cast<float>(vTranslate[2]);

				fbxsdk::FbxVector4 vScale = vecPose[i].vecKeyFrame[j].matTransform.GetS();

				tJoint.vScale.x = static_cast<float>(vScale[0]);
				tJoint.vScale.y = static_cast<float>(vScale[1]);
				tJoint.vScale.z = static_cast<float>(vScale[2]);

				fbxsdk::FbxQuaternion vRotate = vecPose[i].vecKeyFrame[j].matTransform.GetQ();

				tJoint.vQueternion.x = static_cast<float>(vRotate[0]);
				tJoint.vQueternion.y = static_cast<float>(vRotate[1]);
				tJoint.vQueternion.z = static_cast<float>(vRotate[2]);
				tJoint.vQueternion.w = static_cast<float>(vRotate[3]);

				tPose.vecJoint.push_back(tJoint);

				vecTransform[i * m_iMaxFrame + j].vPos = tJoint.vPos;
				vecTransform[i * m_iMaxFrame + j].vScale = tJoint.vScale;
				vecTransform[i * m_iMaxFrame + j].vQueternion = tJoint.vQueternion;
			}

			if (fTotal > m_tCBuffer.fMaxTime)
			{
				m_tCBuffer.fMaxTime = fTotal;
			}

			if (m_iMaxFrame > static_cast<int>(vecPose[i].vecKeyFrame.size()) && !vecPose[i].vecKeyFrame.empty())
			{
				m_iMaxFrame = static_cast<int>(vecPose[i].vecKeyFrame.size());
			}
		}

		m_tCBuffer.iMaxFrame = m_iMaxFrame;
		fMaxTime = m_tCBuffer.fMaxTime;
		fTime = 0.f;

		if (vecTransform.size())
		{
			m_pBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(vecTransform.size()), static_cast<int>(sizeof(TRANSFORM)), &vecTransform.front());
		}
	}

	void Sequence::UseRootMotion()
	{
		m_bRootMotion = true;
	}

	float Sequence::GetMaxTime() const
	{
		return fMaxTime;
	}

	int Sequence::GetMaxFrame() const
	{
		return m_iMaxFrame;
	}

	Sequence::PSEQUENCEINFO Sequence::GetSequenceInfo(int iIndex) const
	{
		if (m_vecInfo.size() <= iIndex || 
			iIndex<0)
		{
			return nullptr;
		}

		return m_vecInfo[iIndex];
	}

	bool Sequence::IsRootMotion() const
	{
		return m_bRootMotion;
	}

	int Sequence::GetFrame() const
	{
		return m_tCBuffer.iFrame;
	}

	void Sequence::Update(float fDeltaTime)
	{
		Vector3 vRootPos = {};

		m_tCBuffer.fTime = fDeltaTime / m_tCBuffer.fMaxTime * m_tCBuffer.iMaxFrame;

		m_tCBuffer.iFrame = static_cast<int>(m_tCBuffer.fTime);

		m_tCBuffer.fTime -= static_cast<float>(m_tCBuffer.iFrame);

		m_tCBuffer.iFrame %= m_tCBuffer.iMaxFrame;

		m_tCBuffer.iNextFrame = (m_tCBuffer.iFrame + 1) % m_tCBuffer.iMaxFrame;

		m_tCBuffer.iInfoCount = static_cast<int>(m_vecInfo.size());

		for (size_t i = 0; i < m_vecInfo.size(); ++i)
		{
			if (m_bRootMotion)
			{
				if (m_vecInfo[i]->vecPose.size())
				{
					if (m_vecInfo[i]->vecPose[0].vecJoint.size() > m_tCBuffer.iFrame)
					{
						Vector3 vInterpolatedPos = m_vecInfo[i]->vecPose[0].vecJoint[m_tCBuffer.iFrame].vPos * (1.f - m_tCBuffer.fTime) + m_vecInfo[i]->vecPose[0].vecJoint[m_tCBuffer.iNextFrame].vPos * m_tCBuffer.fTime;

						vRootPos = { vInterpolatedPos.x, vInterpolatedPos.y, vInterpolatedPos.z };

						break;
					}
				}
			}
		}

		m_tCBuffer.vRootPos = vRootPos;

		m_pBoneConstantBuffer->UpdateBuffer(m_tCBuffer);

		m_pBoneConstantBuffer->Bind();

#ifdef _DEBUG
		/*std::vector<TRANSFORM> vecSrc(m_pBuffer->GetCount());
		m_pBuffer->DebugBuffer(&vecSrc.front(), 0, sizeof(TRANSFORM));*/
#endif
		if (m_pBuffer)
		{
			m_pBuffer->SetSRV(31);
		}
	}

	void Sequence::ResetResource()
	{
		if (m_pBuffer)
		{
			m_pBuffer->ResetSRV(31);
		}
	}

	void Sequence::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_iMaxFrame, 4, 1, pFile);

		unsigned char iContainerSize = static_cast<unsigned char>(m_vecInfo.size());

		fwrite(&iContainerSize, 1, 1, pFile);

		for (unsigned char i = 0; i < iContainerSize; ++i)
		{
			fwrite(&fMaxTime, 4, 1, pFile);
			fwrite(&m_iMaxFrame, 4, 1, pFile);

			unsigned char iSize = static_cast<unsigned char>(m_vecInfo[i]->vecPose.size());

			fwrite(&iSize, 1, 1, pFile);

			for (unsigned char j = 0; j < iSize; ++j)
			{
				unsigned int iJointCount = static_cast<unsigned int>(m_vecInfo[i]->vecPose[j].vecJoint.size());

				fwrite(&iJointCount, 4, 1, pFile);

				for (unsigned int k = 0; k < iJointCount; ++k)
				{
					fwrite(&m_vecInfo[i]->vecPose[j].vecJoint[k].vPos, sizeof(Vector3), 1, pFile);
					fwrite(&m_vecInfo[i]->vecPose[j].vecJoint[k].vQueternion, sizeof(Vector4), 1, pFile);
					fwrite(&m_vecInfo[i]->vecPose[j].vecJoint[k].vScale, sizeof(Vector3), 1, pFile);
				}
			}
		}
	}

	void Sequence::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_iMaxFrame, 4, 1, pFile);

		unsigned char iContainerCount;

		fread(&iContainerCount, 1, 1, pFile);

		for (int i = 0; i < iContainerCount; ++i)
		{
			m_vecInfo.emplace_back(dbg_new SEQUENCEINFO);

			PSEQUENCEINFO pInfo = m_vecInfo.back();

			fread(&fMaxTime, 4, 1, pFile);
			fread(&m_iMaxFrame, 4, 1, pFile);

			if (m_iMaxFrame > m_tCBuffer.iMaxFrame)
			{
				m_tCBuffer.iMaxFrame = m_iMaxFrame;
			}

			if (fMaxTime > m_tCBuffer.fMaxTime)
			{
				m_tCBuffer.fMaxTime = fMaxTime;
			}

			unsigned char iJointCount;

			fread(&iJointCount, 1, 1, pFile);

			std::vector<TRANSFORM> vecTransform(m_iMaxFrame * iJointCount);

			if (iJointCount > m_tCBuffer.iMaxJoint)
			{
				m_tCBuffer.iMaxJoint = iJointCount;
			}

			pInfo->vecPose.resize(iJointCount);

			for (unsigned char j = 0; j < iJointCount; ++j)
			{
				unsigned int iPoseCount;

				fread(&iPoseCount, 4, 1, pFile);

				pInfo->vecPose[j].vecJoint.resize(iPoseCount);

				for (unsigned int k = 0; k < iPoseCount; ++k)
				{
					fread(&pInfo->vecPose[j].vecJoint[k].vPos, sizeof(Vector3), 1, pFile);
					fread(&pInfo->vecPose[j].vecJoint[k].vQueternion, sizeof(Vector4), 1, pFile);
					fread(&pInfo->vecPose[j].vecJoint[k].vScale, sizeof(Vector3), 1, pFile);

					if (static_cast<int>(k) < m_iMaxFrame)
					{
						vecTransform[k + j * m_iMaxFrame].vPos = pInfo->vecPose[j].vecJoint[k].vPos;
						vecTransform[k + j * m_iMaxFrame].vQueternion = pInfo->vecPose[j].vecJoint[k].vQueternion;
						vecTransform[k + j * m_iMaxFrame].vScale = pInfo->vecPose[j].vecJoint[k].vScale;
					}
				}
			}

			if (vecTransform.size())
			{
				m_pBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(vecTransform.size()), static_cast<int>(sizeof(TRANSFORM)), &vecTransform.front());
			}
		}
	}
	std::shared_ptr<Sequence> Sequence::Clone()
	{
		return std::make_shared<Sequence>(*this);
	}
}