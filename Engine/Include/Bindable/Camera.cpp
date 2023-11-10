#include "Camera.h"
#include "TransformBuffer.h"
#include "../Input/Input.h"
#include "../Core/Window.h"
#ifdef _DEBUG
#include "../Scene/SceneManager.h"
#include "../Scene/Scene.h"
#endif

namespace Engine
{
	Camera::Camera() :
		Drawable()
		, matView(Matrix::matIdentity)
		, m_fSpeed(100.f)
#ifdef _DEBUG
		//, m_pDebugDrawable(CreateDrawable<Drawable>("ViewFrustom"))
#endif
		, m_bControl(true)
	{
		Reset();

		StartImGui();
	}

	const Matrix& Camera::GetView() const
	{
		return matView;
	}

	void Camera::Reset()
	{
#ifdef _DEBUG
		/*m_pDebugDrawable->FindAndAddBind<class InputLayout>("TPNT");
		m_pDebugDrawable->FindAndAddBind<VertexBuffer>("ViewFrustom");
		m_pDebugDrawable->FindAndAddBind<IndexBuffer>("ViewFrustomIndex");
		m_pDebugDrawable->FindAndAddBind<VertexShader>("anisotropic_microfacet VS");
		m_pDebugDrawable->FindAndAddBind<PixelShader>("DebugPS");
		m_pDebugDrawable->FindAndAddBind<class Topology>("LineList");
		m_pDebugDrawable->FindAndAddBind<class Material>("Brick");
		m_pDebugDrawable->FindAndAddBind<class DepthStencilState>("DepthAlways");
		m_pDebugDrawable->FindAndAddBind<class RasterizerState>("WireFrame");*/
#endif

		const std::shared_ptr<Transform>& pTransform = GetTransform();

		if (pTransform == nullptr)
		{
			return;
		}

		pTransform->SetPosition({ 0.f, 0.f, -25.f });
		pTransform->SetRX(0.f);
		pTransform->SetRY(0.f);
		pTransform->SetRZ(0.f);
	}

	void Camera::UpdateView()
	{
		const std::shared_ptr<Transform>& pTransform = GetTransform();

		Matrix mat = Matrix::matIdentity;

		mat.v[0] = pTransform->GetAxis(AXIS_TYPE::X);
		mat.v[1] = pTransform->GetAxis(AXIS_TYPE::Y);
		mat.v[2] = pTransform->GetAxis(AXIS_TYPE::Z);

		const Vector3& vPosition = pTransform->GetPosition();

		mat.v[0].w = -static_cast<Vector3>(mat.v[0]).Dot(vPosition);
		mat.v[1].w = -static_cast<Vector3>(mat.v[1]).Dot(vPosition);
		mat.v[2].w = -static_cast<Vector3>(mat.v[2]).Dot(vPosition);

		matView = mat.Transpose();

		//T^-1 * R'
		//	1	0	0	0		Xx	Yx	Zx	0		Xx	Yx	Zx	0
		//	0	1	0	0	*	Xy	Yy	Zy	0	=	Xy	Yy	Zy	0	
		//	0	0	1	0		Xz	Yz	Zz	0		Xz	Yz	Zz	0
		//	-x	-y	-z	1		0	0	0	1		-X.P -Y.P -Z.P	1
	}

	const Matrix& Camera::GetInvView() const
	{
		return GetTransform()->GetTransformMatrix();
	}

	bool Camera::Init()
	{
		if (!CInput::GetInst()->CreateAction(GetTag() + "_W", DIK_W))
		{
			return false;
		}

		CInput::GetInst()->AddAction(GetTag() + "_W", CInput::KEY_STATE::PRESS, this, &Camera::CameraMoveFront);
		if (!CInput::GetInst()->CreateAction(GetTag() + "_S", DIK_S))
		{
			return false;
		}
		CInput::GetInst()->AddAction(GetTag() + "_S", CInput::KEY_STATE::PRESS, this, &Camera::CameraMoveBack);
		if (!CInput::GetInst()->CreateAction(GetTag() + "_A", DIK_A))
		{
			return false;
		}
		CInput::GetInst()->AddAction(GetTag() + "_A", CInput::KEY_STATE::PRESS, this, &Camera::CameraMoveLeft);
		if (!CInput::GetInst()->CreateAction(GetTag() + "_D", DIK_D))
		{
			return false;
		}
		CInput::GetInst()->AddAction(GetTag() + "_D", CInput::KEY_STATE::PRESS, this, &Camera::CameraMoveRight);
		if (!CInput::GetInst()->CreateAction(GetTag() + "_Q", DIK_Q))
		{
			return false;
		}
		CInput::GetInst()->AddAction(GetTag() + "_Q", CInput::KEY_STATE::PRESS, this, &Camera::CameraMoveUp);
		if (!CInput::GetInst()->CreateAction(GetTag() + "_E", DIK_E))
		{
			return false;
		}
		CInput::GetInst()->AddAction(GetTag() + "_E", CInput::KEY_STATE::PRESS, this, &Camera::CameraMoveDown);

		return true;
	}

	void Camera::Input(float fDeltaTime)
	{
		if (m_bControl)
		{
			if (!Window::GetInst()->IsLockRotation())
			{
				const std::shared_ptr<Transform>& pTransform = GetTransform();

				int iDeltaX = CInput::GetInst()->GetMouseDeltaX();
				int iDeltaY = CInput::GetInst()->GetMouseDeltaY();

				pTransform->AddRY(iDeltaX * fDeltaTime);
				pTransform->AddRX(iDeltaY * fDeltaTime);

				if (pTransform->GetRX() >= PI)
				{
					pTransform->SetRX(PI);
				}

				else if (pTransform->GetRX() <= -PI)
				{
					pTransform->SetRX(-PI);
				}
			}

			m_fSpeed += CInput::GetInst()->GetMouseDeltaZ() * !Window::GetInst()->IsCursorEnabled();

			m_fSpeed = m_fSpeed < 0.f ? 0.f : m_fSpeed;
		}
	}

	void Camera::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		UpdateView();
	}

	void Camera::Collision(float fDeltaTime)
	{
	}

	void Camera::PreDraw(float fDeltaTime)
	{
	}

	void Camera::Bind()
	{
		__super::Bind();
	}

	std::shared_ptr<Bindable> Camera::Clone()
	{
		return std::make_shared<Camera>(*this);
	}

	void Camera::CameraMoveFront(float fDeltaTime)
	{
		if (!m_bControl)
		{
			return;
		}

		GetTransform()->AddPosition(GetTransform()->GetAxis(AXIS_TYPE::Z) * fDeltaTime * m_fSpeed);
	}

	void Camera::CameraMoveBack(float fDeltaTime)
	{
		if (!m_bControl)
		{
			return;
		}

		GetTransform()->AddPosition(-GetTransform()->GetAxis(AXIS_TYPE::Z) * fDeltaTime * m_fSpeed);
	}

	void Camera::CameraMoveLeft(float fDeltaTime)
	{
		if (!m_bControl)
		{
			return;
		}

		GetTransform()->AddPosition(-GetTransform()->GetAxis(AXIS_TYPE::X) * fDeltaTime * m_fSpeed);
	}

	void Camera::CameraMoveRight(float fDeltaTime)
	{
		if (!m_bControl)
		{
			return;
		}

		GetTransform()->AddPosition(GetTransform()->GetAxis(AXIS_TYPE::X) * fDeltaTime * m_fSpeed);
	}

	void Camera::CameraMoveUp(float fDeltaTime)
	{
		if (!m_bControl)
		{
			return;
		}

		GetTransform()->AddPosition(GetTransform()->GetAxis(AXIS_TYPE::Y) * fDeltaTime * m_fSpeed);
	}

	void Camera::CameraMoveDown(float fDeltaTime)
	{
		if (!m_bControl)
		{
			return;
		}

		GetTransform()->AddPosition(GetTransform()->GetAxis(AXIS_TYPE::Y) * fDeltaTime * -m_fSpeed);
	}
}