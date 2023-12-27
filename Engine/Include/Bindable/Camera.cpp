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
		SetBindableType(BINDABLE_TYPE::CAMERA);
		Reset();
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
		if (!__super::Init())
		{
			return false;
		}

		if (!CInput::GetInst()->CreateAction(GetTag() + "_W", DIK_W))
		{
			return false;
		}

		return true;
	}

	void Camera::Input(float fDeltaTime)
	{
		if (m_bControl)
		{
			m_fSpeed += CInput::GetInst()->GetMouseDeltaZ() * !Window::GetInst()->IsCursorEnabled();

			m_fSpeed = m_fSpeed < 0.f ? 0.f : m_fSpeed;
		}
	}

	void Camera::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		UpdateView();

		if (Graphics::GetInst()->GetCamera().get() == this)
		{
			Graphics::GetInst()->SetVeiw(matView);
		}
	}

	void Camera::Collision(float fDeltaTime)
	{
		__super::Collision(fDeltaTime);
	}

	void Camera::PostUpdate(float fDeltaTime)
	{
		__super::PostUpdate(fDeltaTime);
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

	void Camera::Save(FILE* pFile)
	{
		__super::Save(pFile);

		bool bMainCamera = Graphics::GetInst()->GetCamera().get() == this;

		fwrite(&bMainCamera, 1, 1, pFile);
	}

	void Camera::Load(FILE* pFile)
	{
		__super::Load(pFile);

		bool bMainCamera = false;

		fread(&bMainCamera, 1, 1, pFile);

		if (bMainCamera)
		{
			Graphics::GetInst()->SetCamera(std::static_pointer_cast<Camera>(shared_from_this()));
		}
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