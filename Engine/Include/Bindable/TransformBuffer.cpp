#include "TransformBuffer.h"
#include "Drawable.h"
#include "../Core/Graphics.h"
#include "BindableManager.h"
#include "DomainCBuffer.h"
#include "PointLight.h"
#include "../Shader/StructuredBuffer.h"

namespace Engine
{
	TransformBuffer::TransformBuffer() :
		Bindable()
		, m_pVertexCBuffer(StaticFindBindable<VertexCBuffer<_tagTransformBuffer>>("Transform"))
		, m_pDomainCBuffer(StaticFindBindable<DomainCBuffer<_tagTransformBuffer>>("Transform"))
		, m_pParentTrasnform(nullptr)
		, m_vPosition(0.f, 0.f, 0.f)
		, m_vVelocity(0.f, 0.f, 0.f)
		, m_vRotation(0.f, 0.f, 0.f)
		, m_vRotationVelocity(0.f, 0.f, 0.f)
		, m_vScale(1.f, 1.f, 1.f)
		, m_vScaleVelocity()
		, m_vRelativePosition()
		, m_vRelativeRotation()
		, m_vRelativeScale(1.f, 1.f, 1.f)
		, m_matTransform()
		, m_matParent(Matrix::matIdentity)
		, m_vAxis()
		, m_bUpdateRotation(true)
		, m_bUpdatePosition(true)
		, m_bUpdateScale(true)
	{
		m_tBuffer.matJoint = Matrix::matIdentity;
		SetBindableType(BINDABLE_TYPE::TRANSFORM);
	}

	TransformBuffer::TransformBuffer(const TransformBuffer& buffer) :
		Bindable(buffer)
		, m_pVertexCBuffer(buffer.m_pVertexCBuffer)
		, m_pDomainCBuffer(buffer.m_pDomainCBuffer)
		, m_pParentTrasnform(nullptr)
		, m_vPosition(buffer.m_vPosition)
		, m_vVelocity(buffer.m_vVelocity)
		, m_vRotation(buffer.m_vRotation)
		, m_vRotationVelocity(buffer.m_vRotationVelocity)
		, m_vScale(buffer.m_vScale)
		, m_vScaleVelocity(buffer.m_vScaleVelocity)
		, m_vRelativePosition(buffer.m_vRelativePosition)
		, m_vRelativeRotation(buffer.m_vRelativeRotation)
		, m_vRelativeScale(buffer.m_vRelativeScale)
		, m_matTransform(buffer.m_matTransform)
		, m_matParent(buffer.m_matParent)
		, m_vAxis()
		, m_bUpdateRotation(true)
		, m_bUpdatePosition(true)
		, m_bUpdateScale(true)
	{
		m_tBuffer.matJoint = Matrix::matIdentity;
	}

	void TransformBuffer::SetParentTransform(TransformBuffer* pParent)
	{
		m_pParentTrasnform = pParent;
	}

	void TransformBuffer::AddChildTransform(TransformBuffer* pChild)
	{
		m_ChildTransformList.push_back(pChild);
	}

	const std::shared_ptr<class DomainCBuffer<_tagTransformBuffer>>& TransformBuffer::GetDomainCBuffer() const
	{
		return m_pDomainCBuffer;
	}

	const _tagTransformBuffer& TransformBuffer::GetBuffer() const
	{
		return m_tBuffer;
	}

	void TransformBuffer::Update(float fDeltaTime)
	{
		if (m_bUpdateRotation)
		{
			m_bUpdateRotation = false;
			m_bUpdatePosition = false;

			m_matRotation = Matrix::RotationXYZ(m_vRotation);

			m_vAxis[static_cast<int>(AXIS_TYPE::X)] = Vector3::Axis[static_cast<int>(AXIS_TYPE::X)] * m_matRotation;

			m_vAxis[static_cast<int>(AXIS_TYPE::Y)] = Vector3::Axis[static_cast<int>(AXIS_TYPE::Y)] * m_matRotation;

			m_vAxis[static_cast<int>(AXIS_TYPE::Z)] = Vector3::Axis[static_cast<int>(AXIS_TYPE::Z)] * m_matRotation;

			m_matRotationTranslation = m_matRotation * Matrix::TranslateFromVector(m_vPosition);
		}
		else if (m_bUpdatePosition)
		{
			m_bUpdatePosition = false;

			m_matRotationTranslation = m_matRotation * Matrix::TranslateFromVector(m_vPosition);
		}

		m_matTransform = Matrix::Scaling(m_vScale) * m_matRotationTranslation * m_tBuffer.matJoint;

		m_matWV = m_matParent * m_matTransform * Graphics::GetInst()->GetView();

		m_tBuffer.matWorldViewProject = m_matWV * Graphics::GetInst()->GetProjectMatrix();

		m_tBuffer.matWorldView = m_matWV;

		const std::shared_ptr<PointLight>& pLight = Graphics::GetInst()->GetLight();

		if (pLight)
		{
			m_tBuffer.matLightWVP = m_matTransform * pLight->GetViewProject();

			m_tBuffer.matLightWVP.Transpose();
		}

		m_tBuffer.matWorldViewProject.Transpose();

		m_tBuffer.matWorldView.Transpose();
	}

	void TransformBuffer::Bind()
	{
		if (m_pJointSequenceBuffer)
		{
			m_pJointSequenceBuffer->SetSRV(32);
		}

		m_pVertexCBuffer->UpdateBuffer(m_tBuffer);

		m_pVertexCBuffer->Bind();
		m_pDomainCBuffer->Bind();
	}

	std::shared_ptr<Bindable> TransformBuffer::Clone()
	{
		return std::make_shared<TransformBuffer>(*this);
	}

	void TransformBuffer::PostBind()
	{
		__super::PostBind();

		if (m_pJointSequenceBuffer)
		{
			m_pJointSequenceBuffer->ResetSRV(32);
		}
	}

	void TransformBuffer::UpdatePosition()
	{
		m_bUpdatePosition = true;

		Vector3 vPrevPos = m_vPosition;

		if (m_pParentTrasnform)
		{
			m_vPosition = (Matrix::Scaling(m_pParentTrasnform->GetScale()) * Matrix::RotationXYZ(m_pParentTrasnform->GetRotation())).TransformCoord(m_vRelativePosition) + m_pParentTrasnform->GetPosition();
		}
		else
		{
			m_vPosition = m_vRelativePosition;
		}

		m_vVelocity += m_vPosition - vPrevPos;

		std::list<TransformBuffer*>::iterator iter = m_ChildTransformList.begin();
		std::list<TransformBuffer*>::iterator iterEnd = m_ChildTransformList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->UpdatePosition();
		}
	}

	void TransformBuffer::UpdateRelativeRotation()
	{
		m_vRelativeRotation = m_vRotation;

		if (m_pParentTrasnform)
		{
			m_vRelativeRotation -= m_pParentTrasnform->GetRotation();
		}

		std::list<TransformBuffer*>::iterator iter = m_ChildTransformList.begin();
		std::list<TransformBuffer*>::iterator iterEnd = m_ChildTransformList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->UpdateRotation();
		}
	}

	void TransformBuffer::UpdateRotation()
	{
		m_bUpdateRotation = true;

		m_vRotation = m_vRelativeRotation;

		if (m_pParentTrasnform)
		{
			m_vRotation += m_pParentTrasnform->GetRotation();

			m_vPosition = (Matrix::Scaling(m_pParentTrasnform->GetScale()) * Matrix::RotationXYZ(m_pParentTrasnform->GetRotation())).TransformCoord(m_vRelativePosition) + m_pParentTrasnform->GetPosition();

			m_bUpdatePosition = true;
		}

		std::list<TransformBuffer*>::iterator iter = m_ChildTransformList.begin();
		std::list<TransformBuffer*>::iterator iterEnd = m_ChildTransformList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->UpdateRotation();
		}
	}

	void TransformBuffer::UpdateRelativeScale()
	{
		m_vRelativeScale = m_vScale;

		if (m_pParentTrasnform)
		{
			m_vRelativeScale /= m_pParentTrasnform->GetScale();
		}

		std::list<TransformBuffer*>::iterator iter = m_ChildTransformList.begin();
		std::list<TransformBuffer*>::iterator iterEnd = m_ChildTransformList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->UpdateScale();
		}
	}

	void TransformBuffer::UpdateScale()
	{
		m_bUpdateScale = true;

		m_vScale = m_vRelativeScale;

		if (m_pParentTrasnform)
		{
			m_vScale *= m_pParentTrasnform->GetScale();
		}

		std::list<TransformBuffer*>::iterator iter = m_ChildTransformList.begin();
		std::list<TransformBuffer*>::iterator iterEnd = m_ChildTransformList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->UpdateScale();
		}
	}

	void TransformBuffer::Reset()
	{
		SetPosition({ 0.f, 0.f, 0.f });
		SetRotation({ 0.f, 0.f, 0.f });
		SetScale({ 1.f, 1.f, 1.f });
	}

	void TransformBuffer::UpdateRelativePosition()
	{
		m_vRelativePosition = m_vPosition;

		if (m_pParentTrasnform)
		{
			m_vRelativePosition = Matrix::RotationXYZ(m_pParentTrasnform->GetRotation()).Transpose().TransformNormal(m_vPosition - m_pParentTrasnform->GetPosition());
		}

		std::list<TransformBuffer*>::iterator iter = m_ChildTransformList.begin();
		std::list<TransformBuffer*>::iterator iterEnd = m_ChildTransformList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->UpdatePosition();
		}
	}

	void TransformBuffer::SetX(float _x)
	{
		m_vVelocity.x += _x - m_vPosition.x;

		m_vPosition.x = _x;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::SetY(float _y)
	{
		m_vVelocity.y += _y - m_vPosition.y;

		m_vPosition.y = _y;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::SetZ(float _z)
	{
		m_vVelocity.z += _z - m_vPosition.z;

		m_vPosition.z = _z;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::AddX(float _x)
	{
		m_vVelocity.x += _x;

		m_vPosition.x += _x;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::AddY(float _y)
	{
		m_vVelocity.y += _y;

		m_vPosition.y += _y;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::AddZ(float _z)
	{
		m_vVelocity.z += _z;

		m_vPosition.z += _z;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	float TransformBuffer::GetX() const
	{
		return m_vPosition.x;
	}

	float TransformBuffer::GetY() const
	{
		return m_vPosition.y;
	}

	float TransformBuffer::GetZ() const
	{
		return m_vPosition.z;
	}

	void TransformBuffer::SetDX(float x)
	{
		m_vVelocity.x = x;
	}

	void TransformBuffer::SetDY(float y)
	{
		m_vVelocity.y = y;
	}

	void TransformBuffer::SetDZ(float z)
	{
		m_vVelocity.z = z;
	}

	void TransformBuffer::AddDX(float x)
	{
		m_vVelocity.x += x;
	}

	void TransformBuffer::AddDY(float y)
	{
		m_vVelocity.y += y;
	}

	void TransformBuffer::AddDZ(float z)
	{
		m_vVelocity.z += z;
	}

	float TransformBuffer::GetDX() const
	{
		return m_vVelocity.x;
	}

	float TransformBuffer::GetDY() const
	{
		return m_vVelocity.y;
	}

	float TransformBuffer::GetDZ() const
	{
		return m_vVelocity.z;
	}

	void TransformBuffer::SetRX(float x)
	{
		m_vRotation.x = x;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	void TransformBuffer::SetRY(float y)
	{
		m_vRotation.y = y;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	void TransformBuffer::SetRZ(float z)
	{
		m_vRotation.z = z;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	void TransformBuffer::AddRX(float x)
	{
		m_vRotation.x += x;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	void TransformBuffer::AddRY(float y)
	{
		m_vRotation.y += y;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	void TransformBuffer::AddRZ(float z)
	{
		m_vRotation.z += z;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	float TransformBuffer::GetRX() const
	{
		return m_vRotation.x;
	}

	float TransformBuffer::GetRY() const
	{
		return m_vRotation.y;
	}

	float TransformBuffer::GetRZ() const
	{
		return m_vRotation.z;
	}

	void TransformBuffer::SetDRX(float x)
	{
		m_vRotationVelocity.x = x;
	}

	void TransformBuffer::SetDRY(float y)
	{
		m_vRotationVelocity.y = y;
	}

	void TransformBuffer::SetDRZ(float z)
	{
		m_vRotationVelocity.z = z;
	}

	void TransformBuffer::AddDRX(float x)
	{
		m_vRotationVelocity.x += x;
	}

	void TransformBuffer::AddDRY(float y)
	{
		m_vRotationVelocity.y += y;
	}

	void TransformBuffer::AddDRZ(float z)
	{
		m_vRotationVelocity.z += z;
	}

	float TransformBuffer::GetDRX() const
	{
		return m_vRotationVelocity.x;
	}

	float TransformBuffer::GetDRY() const
	{
		return m_vRotationVelocity.y;
	}

	float TransformBuffer::GetDRZ() const
	{
		return m_vRotationVelocity.z;
	}

	void TransformBuffer::SetRandomPosAndRotation()
	{
		SetX((float)(rand() % 5 - 2));
		SetY((float)(rand() % 5 - 2));
		SetZ((float)(rand() % 5 - 2));
		SetDX((float)(rand() % 50) / 10.f - 2.5f);
		SetDY((float)(rand() % 50) / 10.f - 2.5f);
		SetDZ((float)(rand() % 50) / 10.f - 2.5f);
		SetRX((float)(rand() % 50));
		SetRY((float)(rand() % 50));
		SetRZ((float)(rand() % 50));
		SetDRX((float)(rand() % 50 - 25) / 20.f);
		SetDRY((float)(rand() % 50 - 25) / 20.f);
		SetDRZ((float)(rand() % 50 - 25) / 20.f);
	}

	const Vector3& TransformBuffer::GetAxis(AXIS_TYPE type) const
	{
		return m_vAxis[static_cast<int>(type)];
	}

	const Vector3& TransformBuffer::GetPosition() const
	{
		return m_vPosition;
	}

	void TransformBuffer::SetPosition(const Vector3& pos)
	{
		m_vVelocity += pos - m_vPosition;

		m_vPosition = pos;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::SetPosition(float x, float y, float z)
	{
		SetPosition({ x,y,z });
	}

	void TransformBuffer::AddPosition(const Vector3& pos)
	{
		m_vVelocity += pos;

		m_vPosition += pos;

		m_bUpdatePosition = true;

		UpdateRelativePosition();
	}

	void TransformBuffer::SetRelativePosition(const Vector3& pos)
	{
		m_vRelativePosition = pos;

		UpdatePosition();
	}

	void TransformBuffer::SetRelativePosition(float x, float y, float z)
	{
		SetRelativePosition({ x,y,z });
	}

	void TransformBuffer::SetRelativeScale(float x, float y, float z)
	{
		m_vRelativeScale.x = x;
		m_vRelativeScale.y = y;
		m_vRelativeScale.z = z;

		UpdateScale();
	}

	void TransformBuffer::SetRelativeRotation(float x, float y, float z)
	{
		SetRelativeRotation({ x,y,z });
	}

	void TransformBuffer::SetRelativeRotation(const Vector3& vRotation)
	{
		m_vRelativeRotation = vRotation;

		UpdateRotation();
	}

	const Matrix& TransformBuffer::GetTransformMatrix() const
	{
		return m_matTransform;
	}

	void TransformBuffer::SetScale(const Vector3& scale)
	{
		m_vScaleVelocity += scale - m_vScale;

		m_vScale = scale;

		UpdateRelativeScale();
	}

	void TransformBuffer::SetScale(float x, float y, float z)
	{
		m_vScaleVelocity.x += x - m_vScale.x;
		m_vScaleVelocity.y += y - m_vScale.y;
		m_vScaleVelocity.z += z - m_vScale.z;

		m_vScale.x = x;
		m_vScale.y = y;
		m_vScale.z = z;

		UpdateRelativeScale();
	}

	void TransformBuffer::SetRotation(const Vector3& rot)
	{
		m_vRotation = rot;

		m_bUpdateRotation = true;

		UpdateRelativeRotation();
	}

	void TransformBuffer::SetRotation(float x, float y, float z)
	{
		SetRotation({ x,y,z });
	}

	const Vector3& TransformBuffer::GetRotation() const
	{
		return m_vRotation;
	}

	const Vector3& TransformBuffer::GetScale() const
	{
		return m_vScale;
	}

	const Vector3& TransformBuffer::GetVelocity() const
	{
		return m_vVelocity;
	}

	const Matrix& TransformBuffer::GetRotationMatrix() const
	{
		return m_matRotation;
	}

	const Matrix& TransformBuffer::GetRotationTranslationMatrix() const
	{
		return m_matRotationTranslation;
	}

	const Matrix& TransformBuffer::GetWV() const
	{
		return m_matWV;
	}

	void TransformBuffer::SetVelocity(const Vector3& vVelocity)
	{
		m_vVelocity = vVelocity;
	}

	void TransformBuffer::SetAxis(AXIS_TYPE eType, const Vector3& vAxisZ, const Vector3& vUp)
	{
		m_vAxis[static_cast<int>(eType)] = vAxisZ;

		m_vAxis[static_cast<int>(eType)].Normalize();

		m_vAxis[(static_cast<int>(eType) + 1) % static_cast<int>(AXIS_TYPE::END)] = vUp.Cross(m_vAxis[static_cast<int>(eType)]).Normalize();

		m_vAxis[(static_cast<int>(eType) + 2) % static_cast<int>(AXIS_TYPE::END)] = m_vAxis[static_cast<int>(eType)].Cross(m_vAxis[(static_cast<int>(eType) + 1) % static_cast<int>(AXIS_TYPE::END)]);

		SetRotation({ atan2(m_vAxis[static_cast<int>(AXIS_TYPE::Y)].z, m_vAxis[static_cast<int>(AXIS_TYPE::Z)].z),
			atan2(-m_vAxis[static_cast<int>(AXIS_TYPE::X)].z, sqrtf(m_vAxis[static_cast<int>(AXIS_TYPE::Y)].z * m_vAxis[static_cast<int>(AXIS_TYPE::Y)].z + m_vAxis[static_cast<int>(AXIS_TYPE::Z)].z * m_vAxis[static_cast<int>(AXIS_TYPE::Z)].z)),
			atan2(m_vAxis[static_cast<int>(AXIS_TYPE::X)].y, m_vAxis[static_cast<int>(AXIS_TYPE::X)].x) });

		/*m_matRotation.v[0] = m_vAxis[static_cast<int>(AXIS_TYPE::X)];
		m_matRotation.v[1] = m_vAxis[static_cast<int>(AXIS_TYPE::Y)];
		m_matRotation.v[2] = m_vAxis[static_cast<int>(AXIS_TYPE::Z)];

		m_matRotationTranslation = m_matRotation * Matrix::TranslateFromVector(m_vPosition);

		m_matTransform = Matrix::Scaling(m_vScale) * m_matRotationTranslation * m_matParent;*/
	}

	void TransformBuffer::SetParentMatrix(const Matrix& matParent, int iJointIndex, std::shared_ptr<class StructuredBuffer> pBuffer)
	{
		m_tBuffer.matJoint = matParent;

		//m_tBuffer.iJointSocket = iJointIndex;
		//m_pJointSequenceBuffer = pBuffer;
	}
	void TransformBuffer::SetRotationTranslationMatrix(const Matrix& mat)
	{
		m_matRotationTranslation = mat;
	}
}