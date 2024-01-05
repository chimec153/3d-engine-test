#include "Camera.h"
#include "Transform.h"
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
		, m_matProj(Matrix::matIdentity)
		, m_matVP(Matrix::matIdentity)
		, m_eProjType(PROJECT_TYPE::PERSPECTIVE)
		, m_fAngle(DegToRad(45.f))
		, m_fRatio(Window::GetInst()->GetWidth() / static_cast<float>(Window::GetInst()->GetHeight()))
		, m_fNear(0.5f)
		, m_eCameraType(CAMERA_TYPE::NORMAL)
#ifdef _DEBUG
		//, m_pDebugDrawable(CreateDrawable<Drawable>("ViewFrustom"))
#endif
		, m_bControl(true)
	{
		SetBindableType(BINDABLE_TYPE::CAMERA);
		Reset();
	}

	Camera::Camera(const Camera& cam)	:
		Drawable(cam)
		, matView(cam.matView)
		, m_fSpeed(cam.m_fSpeed)
		, m_bControl(cam.m_bControl)
		, m_matProj(cam.m_matProj)
		, m_matVP(cam.m_matVP)
		, m_eProjType(cam.m_eProjType)
		, m_fAngle(cam.m_fAngle)
		, m_fRatio(cam.m_fRatio)
		, m_fNear(cam.m_fNear)
		, m_eCameraType(cam.m_eCameraType)
	{
	}

	const Matrix& Camera::GetView() const noexcept
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

		m_matVP = matView * m_matProj;

		//T^-1 * R'
		//	1	0	0	0		Xx	Yx	Zx	0		Xx	Yx	Zx	0
		//	0	1	0	0	*	Xy	Yy	Zy	0	=	Xy	Yy	Zy	0	
		//	0	0	1	0		Xz	Yz	Zz	0		Xz	Yz	Zz	0
		//	-x	-y	-z	1		0	0	0	1		-X.P -Y.P -Z.P	1
	}

	const Matrix& Camera::GetInvView() const noexcept
	{
		return GetTransform()->GetTransformMatrix();
	}

	void Camera::SetProjectType(PROJECT_TYPE eType)
	{
		m_eProjType = eType;

		switch (m_eProjType)
		{
		case Engine::Camera::PROJECT_TYPE::ORTHOGONAL:
			m_matProj = Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 5000.f);
			break;
		case Engine::Camera::PROJECT_TYPE::PERSPECTIVE:
			m_matProj = Matrix::PerspectiveFovLHInfinity(atanf(tanf(m_fAngle) / m_fRatio), m_fRatio, m_fNear);
			break;
		default:
			assert(false);
			break;
		}
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

	const Vector3& Camera::CameraPosToWorldPos(const Vector2& vCameraPos) const
	{
		const std::shared_ptr<Transform>& pTransform = GetTransform();

		const Matrix& matProject = GetProjectMatrix();

		Vector3 vViewPos = {};

		vViewPos.z = matProject[3][2] / -matProject[2][2];

		vViewPos.x = vCameraPos.x / matProject[0][0] * vViewPos.z;
		vViewPos.y = vCameraPos.y / matProject[1][1] * vViewPos.z;

		return pTransform->GetRotationTranslationMatrix().TransformCoord(vViewPos);
	}

	void Camera::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		UpdateView();
	}

	void Camera::SetCameraType(CAMERA_TYPE eType)
	{
		m_eCameraType = eType;
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

		bool bMainCamera = Graphics::GetInst()->GetCamera(m_eCameraType).get() == this;

		fwrite(&bMainCamera, 1, 1, pFile);
		fwrite(&m_eProjType, 4, 1, pFile);
		fwrite(&m_fAngle, 4, 1, pFile);
		fwrite(&m_fNear, 4, 1, pFile);
		fwrite(&m_eCameraType, 4, 1, pFile);
	}

	void Camera::Load(FILE* pFile)
	{
		__super::Load(pFile);

		bool bMainCamera = false;

		fread(&bMainCamera, 1, 1, pFile);
		fread(&m_eProjType, 4, 1, pFile);
		fread(&m_fAngle, 4, 1, pFile);
		fread(&m_fNear, 4, 1, pFile);
		fread(&m_eCameraType, 4, 1, pFile);

		if (bMainCamera)
		{
			Graphics::GetInst()->SetCamera(std::static_pointer_cast<Camera>(shared_from_this()), m_eCameraType);
		}

		SetProjectType(m_eProjType);
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

	const Vector3& Camera::ScreenPosToClipPos(const Vector2& vScreenPos) const
	{
		return {
		vScreenPos.x / Window::GetInst()->GetWidth() * 2.f - 1.f,
		vScreenPos.y / Window::GetInst()->GetHeight() * 2.f - 1.f,
		m_fNear
		};
		// TODO: insert return statement here
	}

	float Camera::GetAngle() const noexcept
	{
		return m_fAngle;
	}

	float Camera::GetRatio() const noexcept
	{
		return m_fRatio;
	}

	float Camera::GetNear() const noexcept
	{
		return m_fNear;
	}
	const Matrix& Camera::GetProjectMatrix() const noexcept
	{
		return m_matProj;
	}
	const Matrix& Camera::GetViewProject()    const noexcept
	{
		return m_matVP;
	}
}