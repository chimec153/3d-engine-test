#include "Skeleton.h"
#include "JointSocket.h"
#include "../Core/Graphics.h"
#include "../Core/Window.h"
#include "../Shader/StructuredBuffer.h"

namespace Engine
{
	Skeleton::Skeleton()	:
		m_pBuffer()
	{
	}

	Skeleton::~Skeleton()
	{
		Safe_Delete_VecList(m_vecJoint);
	}

	void Skeleton::SetBone(const std::vector<BONE>& vecBone)
	{
		std::vector<int> vecParent;

		for (int i = 0; i < static_cast<int>(vecBone.size()); ++i)
		{
			m_vecJoint.push_back(dbg_new BONE);

			*m_vecJoint.back() = vecBone[i];

			vecParent.push_back(vecBone[i].iParent);
		}

		m_pBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(m_vecJoint.size()), static_cast<int>(sizeof(Matrix)));

		if (!m_pBuffer)
		{
			assert(false);
			return;
		}

		m_pJointHierarchy = std::make_shared<StructuredBuffer>(static_cast<int>(m_vecJoint.size()), 4, &vecParent.front());

		if (!m_pJointHierarchy)
		{
			assert(false);
			return;
		}
	}

	const BONE& Skeleton::GetBone(int iIndex) const
	{
		return *m_vecJoint[iIndex];
	}

	const std::vector<PBONE>& Skeleton::GetBones() const
	{
		return m_vecJoint;
	}

	int Skeleton::GetBoneCount() const
	{
		return static_cast<int>(m_vecJoint.size());
	}

	int Skeleton::FindJointIndex(const std::string& strJoint) const
	{
		for (int i = 0; i < static_cast<int>(m_vecJoint.size()); ++i)
		{
			if (m_vecJoint[i]->strName == strJoint)
			{
				return i;
			}
		}

		return -1;
	}

	const std::vector<PBONE> Skeleton::GetJoints() const
	{
		return m_vecJoint;
	}

	void Skeleton::SetSRV()
	{
		m_pBuffer->SetSRV(30);
	}

	void Skeleton::ResetSRV()
	{
		m_pBuffer->ResetSRV(30);
	}

	std::shared_ptr<class StructuredBuffer> Skeleton::GetBuffer() const
	{
		return m_pBuffer;
	}

	void Skeleton::SetHierarchySRV()
	{
		m_pJointHierarchy->SetSRV(35);
	}

	void Skeleton::ResetHierarchySRV()
	{
		m_pJointHierarchy->ResetSRV(35);
	}

	void Skeleton::Save(FILE* pFile)
	{
		__super::Save(pFile);

		unsigned char iSize = static_cast<unsigned char>(m_vecJoint.size());

		fwrite(&iSize, 1, 1, pFile);

		for (unsigned char i = 0; i < iSize; ++i)
		{
			int iLength = static_cast<int>(m_vecJoint[i]->strName.length());

			fwrite(&iLength, 4, 1, pFile);

			if (iLength)
			{
				fwrite(m_vecJoint[i]->strName.c_str(), 1, iLength, pFile);
			}

			fwrite(&m_vecJoint[i]->iParent, 4, 1, pFile);
			fwrite(&m_vecJoint[i]->matInvBindPose, sizeof(Matrix), 1, pFile);
		}
	}

	void Skeleton::Load(FILE* pFile)
	{
		__super::Load(pFile);

		unsigned char iSize;

		fread(&iSize, 1, 1, pFile);

		std::vector<int> vecParent(iSize);

		std::vector<Matrix> vecMatrix;

		for (unsigned char i = 0; i < iSize; ++i)
		{
			m_vecJoint.emplace_back(dbg_new BONE());

			int iLength;

			fread(&iLength, 4, 1, pFile);

			if (iLength)
			{
				std::unique_ptr<char[]> pName = std::make_unique<char[]>(iLength + 1);

				pName[iLength] = 0;

				fread(pName.get(), 1, iLength, pFile);

				m_vecJoint[i]->strName = pName.get();
			}

			fread(&m_vecJoint[i]->iParent, 4, 1, pFile);
			fread(&m_vecJoint[i]->matInvBindPose, sizeof(Matrix), 1, pFile);

			vecParent[i] = m_vecJoint[i]->iParent;

			Matrix mat = m_vecJoint[i]->matInvBindPose;

			vecMatrix.push_back(mat.Transpose());
		}

		m_pBuffer = std::make_shared<StructuredBuffer>(static_cast<int>(vecMatrix.size()), static_cast<int>(sizeof(Matrix)), &vecMatrix.front());

		m_pBuffer->ReadBuffer(&vecMatrix[0], 0, static_cast<int>(sizeof(Matrix)) * vecMatrix.size());

		m_pJointHierarchy = std::make_shared<StructuredBuffer>(static_cast<int>(vecParent.size()), 4, &vecParent.front());
	}

}