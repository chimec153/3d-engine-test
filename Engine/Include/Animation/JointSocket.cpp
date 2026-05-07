#include "JointSocket.h"
#include "../Bindable/Drawable.h"
#include "../Bindable/Transform.h"
#include "../Shader/StructuredBuffer.h"

namespace Engine
{
	JointSocket::JointSocket() :
		m_iParentIndex(-1)
		, m_vScale(1.f, 1.f, 1.f)
		, m_vPosition()
		, m_vRotation(0.f, 0.f, 0.f)
	{
		UpdateJointMatrix();
	}

	void JointSocket::SetParentIndex(int iIndex)
	{
		m_iParentIndex = iIndex;
	}

	void JointSocket::UpdateJointMatrix()
	{
		m_matJoint = Matrix::Scaling(m_vScale) * Matrix::RotationXYZ(m_vRotation) * Matrix::TranslateFromVector(m_vPosition);
	}

	int JointSocket::GetParentIndex() const
	{
		return m_iParentIndex;
	}

	const Matrix& JointSocket::GetJoint() const
	{
		return m_matJoint;
	}

	void JointSocket::SetTransformTarget(std::shared_ptr<Transform> pTransform)
	{
		m_pTargetTransform = pTransform;
	}

	void JointSocket::SetScale(const Vector3& vScale)
	{
		m_vScale = vScale;

		UpdateJointMatrix();
	}

	void JointSocket::SetScale(float x, float y, float z)
	{
		SetScale({ x,y,z });
	}

	void JointSocket::SetPosition(const Vector3& vPos)
	{
		m_vPosition = vPos;

		UpdateJointMatrix();
	}

	void JointSocket::SetRotation(const Vector3& vQuter)
	{
		m_vRotation = vQuter;

		UpdateJointMatrix();
	}

	void JointSocket::AddRX(float x)
	{
		m_vRotation.x += x;

		UpdateJointMatrix();
	}

	void JointSocket::AddRY(float y)
	{
		m_vRotation.y += y;

		UpdateJointMatrix();
	}

	void JointSocket::AddRZ(float z)
	{
		m_vRotation.z += z;

		UpdateJointMatrix();
	}

	void JointSocket::Update(std::shared_ptr<class StructuredBuffer> pBuffer, const Matrix& matParent)
	{
		// Phase E5 — Transform target only. Drawable fallback removed
		// (no more Drawable instances exist live).
		std::shared_ptr<Transform> pTransform = m_pTargetTransform;

		if (pTransform)
		{
			{
				Matrix matSRT;

				pBuffer->ReadBuffer(&matSRT, static_cast<int>(m_iParentIndex * sizeof(Matrix)), static_cast<int>(sizeof(Matrix)));

				matSRT.Transpose();

				Matrix matJointFinal = Matrix::Scaling(m_vScale) * Matrix::RotationXYZ(m_vRotation) * Matrix::TranslateFromVector(m_vPosition) * matSRT * matParent ;

				Vector3 vPos = {};

				Vector3 vRotation = {};

				Vector3 vScale = {};

				matJointFinal.GetSRT(vScale, vRotation, vPos);

				pTransform->SetPosition(vPos);

				pTransform->SetScale(vScale);

				pTransform->SetRotation(vRotation);
			}
		}
	}
	const Vector3& JointSocket::GetScale() const
	{
		return m_vScale;
	}
	const Vector3& JointSocket::GetPosition() const
	{
		return m_vPosition;
	}
	const Vector3& JointSocket::GetRotation() const
	{
		return m_vRotation;
	}
	void JointSocket::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_iParentIndex, 4, 1, pFile);
		fwrite(&m_vScale, 12, 1, pFile);
		fwrite(&m_vPosition, 12, 1, pFile);
		fwrite(&m_vRotation, 12, 1, pFile);
	}
	void JointSocket::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_iParentIndex, 4, 1, pFile);
		fread(&m_vScale, 12, 1, pFile);
		fread(&m_vPosition, 12, 1, pFile);
		fread(&m_vRotation, 12, 1, pFile);

		UpdateJointMatrix();
	}
}