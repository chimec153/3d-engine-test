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
		, m_bLoop(false)
	{
		m_tCBuffer.fBlendMaxTime = 0.5f;
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

	bool Sequence::SetSequance(const std::vector<FbxLoader::FBXBONEKEYFRAME>& vecJoint)
	{
		m_vecBlendPalette.clear();

		m_iMaxFrame = INT_MIN;

		for (int i = 0; i < static_cast<int>(vecJoint.size()); ++i)
		{
			if (m_iMaxFrame < static_cast<int>(vecJoint[i].vecKeyFrame.size()))
			{
				m_iMaxFrame = static_cast<int>(vecJoint[i].vecKeyFrame.size());
			}
		}

		if (!m_iMaxFrame)
		{
			//assert(false);
			return false;
		}

		m_tCBuffer.iMaxJoint = static_cast<int>(vecJoint.size());

		m_vecInfo.emplace_back(dbg_new SEQUENCEINFO());

		PSEQUENCEINFO pInfo = m_vecInfo.back();

		for (size_t i = 0; i < vecJoint.size(); ++i)
		{
			pInfo->vecJoint.emplace_back();

			JOINT& tJoint = pInfo->vecJoint.back();

			float fTotal = 0.f;

			for (size_t j = 0; j < vecJoint[i].vecKeyFrame.size(); ++j)
			{
				if (fTotal < static_cast<float>(vecJoint[i].vecKeyFrame[j].dTime))
				{
					fTotal = static_cast<float>(vecJoint[i].vecKeyFrame[j].dTime);
				}

				if (fTotal > m_tCBuffer.fMaxTime)
				{
					m_tCBuffer.fMaxTime = fTotal;
				}

				if (m_iMaxFrame > static_cast<int>(vecJoint[i].vecKeyFrame.size()) && !vecJoint[i].vecKeyFrame.empty())
				{
					m_iMaxFrame = static_cast<int>(vecJoint[i].vecKeyFrame.size());
				}

				JOINTFRAME tFrame;

				fbxsdk::FbxVector4 vTranslate = vecJoint[i].vecKeyFrame[j].matTransform.GetT();

				tFrame.vPos.x = static_cast<float>(vTranslate[0]);
				tFrame.vPos.y = static_cast<float>(vTranslate[1]);
				tFrame.vPos.z = static_cast<float>(vTranslate[2]);

				fbxsdk::FbxVector4 vScale = vecJoint[i].vecKeyFrame[j].matTransform.GetS();

				tFrame.vScale.x = static_cast<float>(vScale[0]);
				tFrame.vScale.y = static_cast<float>(vScale[1]);
				tFrame.vScale.z = static_cast<float>(vScale[2]);

				fbxsdk::FbxQuaternion vRotate = vecJoint[i].vecKeyFrame[j].matTransform.GetQ();

				tFrame.vQueternion.x = static_cast<float>(vRotate[0]);
				tFrame.vQueternion.y = static_cast<float>(vRotate[1]);
				tFrame.vQueternion.z = static_cast<float>(vRotate[2]);
				tFrame.vQueternion.w = static_cast<float>(vRotate[3]);

				tJoint.vecFrame.push_back(tFrame);
			}

			m_vecBlendPalette.push_back(0.f);
		}

		CreateSequenceBuffer();

		m_tCBuffer.iMaxFrame = m_iMaxFrame;
		fMaxTime = m_tCBuffer.fMaxTime;
		fTime = 0.f;

		return true;
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

	void Sequence::SetFramePosition(int iBone, int iFrame, const Vector3& vPos)
	{
		if (!m_pBuffer)
		{
			return;
		}

		m_vecInfo[0]->vecJoint[iBone].vecFrame[iFrame].vPos = vPos;

		//m_pBuffer->WriteData(&vPos.x, 40 * (iBone * m_iMaxFrame + iFrame), 12);

		CreateSequenceBuffer();
	}

	void Sequence::SetFrameRotation(int iBone, int iFrame, const Vector4& vQuternion)
	{
		if (!m_pBuffer)
		{
			return;
		}

		m_vecInfo[0]->vecJoint[iBone].vecFrame[iFrame].vQueternion = vQuternion;

		//m_pBuffer->WriteData(&vQuternion.x, 40 * (iBone * m_iMaxFrame + iFrame) + 12, 16);

		CreateSequenceBuffer();
	}

	void Sequence::SetFrameScale(int iBone, int iFrame, const Vector3& vScale)
	{
		if (!m_pBuffer)
		{
			return;
		}

		m_vecInfo[0]->vecJoint[iBone].vecFrame[iFrame].vScale = vScale;

		//m_pBuffer->WriteData(&vScale.x, 40 * (iBone * m_iMaxFrame + iFrame) + 28, 12);

		CreateSequenceBuffer();
	}

	void Sequence::CreateSequenceBuffer()
	{
		std::vector<TRANSFORM> vecTransform(m_tCBuffer.iMaxJoint * m_iMaxFrame);

		for (int i = 0; i < m_vecInfo.size(); ++i)
		{
			for (int j = 0; j < m_vecInfo[i]->vecJoint.size(); ++j)
			{
				for (int k = 0; k < m_vecInfo[i]->vecJoint[j].vecFrame.size(); ++k)
				{
					vecTransform[j * m_iMaxFrame + k].vPos = m_vecInfo[i]->vecJoint[j].vecFrame[k].vPos;
					vecTransform[j * m_iMaxFrame + k].vScale = m_vecInfo[i]->vecJoint[j].vecFrame[k].vScale;
					vecTransform[j * m_iMaxFrame + k].vQueternion = m_vecInfo[i]->vecJoint[j].vecFrame[k].vQueternion;
				}
			}
		}

		if (vecTransform.size())
		{
			m_pBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(vecTransform.size()), static_cast<int>(sizeof(TRANSFORM)), &vecTransform.front());
		}
	}

	void Sequence::Loop()
	{
		m_bLoop = true;
	}

	bool Sequence::IsLoop() const
	{
		return m_bLoop;
	}

	void Sequence::SetNextSequence(const std::string& strSeq)
	{
		m_strNextSequence = strSeq;
	}

	const std::string& Sequence::GetNextSequence() const
	{
		return m_strNextSequence;
	}

	const std::vector<float>& Sequence::GetBlendPalette() const
	{
		return m_vecBlendPalette;
	}

	void Sequence::SetBlendFactor(int iJoint, float fBlendFactor)
	{
		if (iJoint < 0 || iJoint >= static_cast<int>(m_vecBlendPalette.size()))
		{
			assert(false);
			return;
		}

		m_vecBlendPalette[iJoint] = fBlendFactor;
	}

	const BONEINFO& Sequence::GetBoneInfo() const
	{
		return m_tCBuffer;
	}

	void Sequence::Update(float fDeltaTime, int iSlot, int iIndex)
	{
		Vector3 vRootPos = {};

		m_tCBuffer.fTime = fDeltaTime / m_tCBuffer.fMaxTime * m_tCBuffer.iMaxFrame;

		m_tCBuffer.iFrame = static_cast<int>(m_tCBuffer.fTime);

		m_tCBuffer.fTime -= static_cast<float>(m_tCBuffer.iFrame);

		m_tCBuffer.iFrame %= m_tCBuffer.iMaxFrame;

		m_tCBuffer.iNextFrame = (m_tCBuffer.iFrame + 1) % m_tCBuffer.iMaxFrame;

		m_tCBuffer.fSequenceTime = fDeltaTime;

		for (size_t i = 0; i < m_vecInfo.size(); ++i)
		{
			if (m_bRootMotion)
			{
				if (m_vecInfo[i]->vecJoint.size())
				{
					if (m_vecInfo[i]->vecJoint[0].vecFrame.size() > m_tCBuffer.iFrame)
					{
						Vector3 vInterpolatedPos = m_vecInfo[i]->vecJoint[0].vecFrame[m_tCBuffer.iFrame].vPos * (1.f - m_tCBuffer.fTime) + m_vecInfo[i]->vecJoint[0].vecFrame[m_tCBuffer.iNextFrame].vPos * m_tCBuffer.fTime;

						vRootPos = { vInterpolatedPos.x, vInterpolatedPos.y, vInterpolatedPos.z };

						break;
					}
				}
			}
		}

		m_tCBuffer.vRootPos = vRootPos;

#ifdef _DEBUG
		/*std::vector<TRANSFORM> vecSrc(m_pBuffer->GetCount());
		m_pBuffer->DebugBuffer(&vecSrc.front(), 0, sizeof(TRANSFORM));*/
#endif
		if (m_pBuffer)
		{
			m_pBuffer->SetSRV(iSlot);
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

			unsigned char iSize = static_cast<unsigned char>(m_vecInfo[i]->vecJoint.size());

			fwrite(&iSize, 1, 1, pFile);

			for (unsigned char j = 0; j < iSize; ++j)
			{
				unsigned int iJointCount = static_cast<unsigned int>(m_vecInfo[i]->vecJoint[j].vecFrame.size());

				fwrite(&iJointCount, 4, 1, pFile);

				for (unsigned int k = 0; k < iJointCount; ++k)
				{
					fwrite(&m_vecInfo[i]->vecJoint[j].vecFrame[k].vPos, sizeof(Vector3), 1, pFile);
					fwrite(&m_vecInfo[i]->vecJoint[j].vecFrame[k].vQueternion, sizeof(Vector4), 1, pFile);
					fwrite(&m_vecInfo[i]->vecJoint[j].vecFrame[k].vScale, sizeof(Vector3), 1, pFile);
				}
			}
		}

		int iBlendCount = static_cast<int>(m_vecBlendPalette.size());

		fwrite(&iBlendCount, 4, 1, pFile);

		if (iBlendCount > 0)
		{
			fwrite(&m_vecBlendPalette[0], 4, iBlendCount, pFile);
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

			pInfo->vecJoint.resize(iJointCount);

			for (unsigned char j = 0; j < iJointCount; ++j)
			{
				unsigned int iPoseCount;

				fread(&iPoseCount, 4, 1, pFile);

				pInfo->vecJoint[j].vecFrame.resize(iPoseCount);

				for (unsigned int k = 0; k < iPoseCount; ++k)
				{
					fread(&pInfo->vecJoint[j].vecFrame[k].vPos, sizeof(Vector3), 1, pFile);
					fread(&pInfo->vecJoint[j].vecFrame[k].vQueternion, sizeof(Vector4), 1, pFile);
					fread(&pInfo->vecJoint[j].vecFrame[k].vScale, sizeof(Vector3), 1, pFile);

					if (static_cast<int>(k) < m_iMaxFrame)
					{
						vecTransform[k + j * m_iMaxFrame].vPos = pInfo->vecJoint[j].vecFrame[k].vPos;
						vecTransform[k + j * m_iMaxFrame].vQueternion = pInfo->vecJoint[j].vecFrame[k].vQueternion;
						vecTransform[k + j * m_iMaxFrame].vScale = pInfo->vecJoint[j].vecFrame[k].vScale;
					}
				}
			}

			if (vecTransform.size())
			{
				m_pBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(vecTransform.size()), static_cast<int>(sizeof(TRANSFORM)), &vecTransform.front());
			}
		}

		int iBlendCount = 0;

		fread(&iBlendCount, 4, 1, pFile);

		if (iBlendCount)
		{
			m_vecBlendPalette.resize(iBlendCount);

			fread(&m_vecBlendPalette[0], 4, iBlendCount, pFile);
		}
	}
	std::shared_ptr<Sequence> Sequence::Clone()
	{
		return std::make_shared<Sequence>(*this);
	}
}