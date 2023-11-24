#include "Input.h"
#include "../Bindable/Mouse.h"
#include "../Core/Window.h"
#include "../Scene/SceneManager.h"
#include "../Scene/Scene.h"

namespace Engine
{
	CInput* CInput::m_pInst = nullptr;

	CInput::CInput() :
		pInput(nullptr)
		, pMouse(nullptr)
		, state()
		, m_hWnd()
		, m_tMousePos()
		, pKeyboardDevice(nullptr)
		, tKeyData()
		, m_bEnable(true)
		, m_pMouse(nullptr)
		, m_pMouseButton()
	{
		for (int i = 0; i < static_cast<int>(MOUSE_TYPE::END); ++i)
		{
			m_pMouseButton[i].iKey = i;
		}
		ShowCursor(FALSE);
	}

	CInput::~CInput()
	{
		if (pMouse != nullptr)
		{
			pMouse->Unacquire();
		}

		if (pKeyboardDevice != nullptr)
		{
			pKeyboardDevice->Unacquire();
		}

		Safe_Delete_VecList(m_KeyList);
		Safe_Delete_Map(m_mapAction);
	}

	void CInput::Update(float fDeltaTime)
	{
		if (FAILED(pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &state)))
		{
			pMouse->Acquire();
		}

		if (FAILED(pKeyboardDevice->GetDeviceState(sizeof(tKeyData), tKeyData)))
		{
			pKeyboardDevice->Acquire();
		}

		UpdateKey();

		UpdateMouse();

		UpdateActions(fDeltaTime);

		GetCursorPos(&m_tMousePos);

		ScreenToClient(m_hWnd, &m_tMousePos);
	}

	bool CInput::InitMouse(HINSTANCE hInst)
	{
		if (FAILED(pInput->CreateDevice(GUID_SysMouse, &pMouse, nullptr)))
		{
			return false;
		}

		if (FAILED(pMouse->Initialize(hInst, DIRECTINPUT_VERSION, GUID_SysMouse)))
		{
			return false;
		}

		if (FAILED(pMouse->SetDataFormat(&c_dfDIMouse)))
		{
			return false;
		}

		HRESULT hr = pMouse->Acquire();

		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}

	int CInput::GetMouseX() const
	{
		return m_tMousePos.x;
	}

	bool CInput::IsKey(KEY_STATE state, unsigned char iDikKey)
	{
		PKEYINFO pInfo = FindKey(iDikKey);

		if (!pInfo)
		{
			return false;
		}

		switch (state)
		{
		case CInput::KEY_STATE::DOWN:
			return pInfo->bDown;
		case CInput::KEY_STATE::PRESS:
			return pInfo->bPressed;
		case CInput::KEY_STATE::UP:
			return pInfo->bUp;
		}

		return false;
	}

	bool CInput::InitKeyboard(HINSTANCE hInst)
	{
		if (FAILED(pInput->CreateDevice(GUID_SysKeyboard, &pKeyboardDevice, nullptr)))
		{
			return false;
		}

		if (FAILED(pKeyboardDevice->Initialize(hInst, DIRECTINPUT_VERSION, GUID_SysKeyboard)))
		{
			return false;
		}

		if (FAILED(pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard)))
		{
			return false;
		}

		if (FAILED(pKeyboardDevice->Acquire()))
		{
			return false;
		}

		return true;
	}

	int CInput::GetMouseY() const
	{
		return m_tMousePos.y;
	}

	int CInput::GetMouseDeltaX() const
	{
		return state.lX;
	}

	void CInput::Disable()
	{
		m_bEnable = false;
	}

	int CInput::GetMouseDeltaZ() const
	{
		return state.lZ * m_bEnable;
	}

	bool CInput::IsMouseButtonDown(MOUSE_TYPE eType) const
	{
		if (eType < MOUSE_TYPE::LEFT || static_cast<int>(eType) >= sizeof(state.rgbButtons) / sizeof(state.rgbButtons[0]))
		{
			return false;
		}

		return m_pMouseButton[static_cast<int>(eType)].bDown;
	}

	bool CInput::IsMouseButtonPress(MOUSE_TYPE eType) const
	{
		if (eType < MOUSE_TYPE::LEFT || static_cast<int>(eType) >= sizeof(state.rgbButtons) / sizeof(state.rgbButtons[0]))
		{
			return false;
		}

		return m_pMouseButton[static_cast<int>(eType)].bPressed;
	}

	bool CInput::IsMouseButtonUp(MOUSE_TYPE eType) const
	{
		if (eType < MOUSE_TYPE::LEFT || static_cast<int>(eType) >= sizeof(state.rgbButtons) / sizeof(state.rgbButtons[0]))
		{
			return false;
		}

		return m_pMouseButton[static_cast<int>(eType)].bUp;
	}

	void CInput::Enable()
	{
		m_bEnable = true;
	}

	int CInput::GetMouseDeltaY() const
	{
		return state.lY;
	}

	void CInput::UpdateKey()
	{
		std::list<PKEYINFO>::iterator iter = m_KeyList.begin();
		std::list<PKEYINFO>::iterator iterEnd = m_KeyList.end();

		for (; iter != iterEnd; ++iter)
		{
			if (tKeyData[(*iter)->iKey] && m_bEnable)
			{
				if (!(*iter)->bDown)
				{
					(*iter)->bDown = true;
					(*iter)->bPressed = false;
					(*iter)->bUp = false;
				}

				else
				{
					(*iter)->bDown = false;
					(*iter)->bPressed = true;
					(*iter)->bUp = false;
				}
			}

			else
			{
				if ((*iter)->bPressed || (*iter)->bDown)
				{
					(*iter)->bDown = false;
					(*iter)->bPressed = false;
					(*iter)->bUp = true;
				}
				else
				{
					(*iter)->bUp = false;
				}
			}
		}
	}

	void CInput::UpdateMouse()
	{
		for (int i = 0; i < static_cast<int>(MOUSE_TYPE::END); ++i)
		{
			if (state.rgbButtons[m_pMouseButton[i].iKey] && m_bEnable)
			{
				if (!m_pMouseButton[i].bDown)
				{
					m_pMouseButton[i].bDown = true;
					m_pMouseButton[i].bUp = false;
					m_pMouseButton[i].bPressed = false;
				}
				else if(!m_pMouseButton[i].bPressed)
				{
					m_pMouseButton[i].bDown = false;
					m_pMouseButton[i].bUp = false;
					m_pMouseButton[i].bPressed = true;
				}
			}
			else
			{
				if (m_pMouseButton[i].bDown || m_pMouseButton[i].bPressed)
				{
					m_pMouseButton[i].bUp = true;
					m_pMouseButton[i].bDown = false;
					m_pMouseButton[i].bPressed = false;
				}
				else
				{
					m_pMouseButton[i].bUp = false;
					m_pMouseButton[i].bDown = false;
					m_pMouseButton[i].bPressed = false;
				}
			}
		}
	}

	void CInput::AddAction(const std::string& strAction, KEY_STATE eState, void(*pFunc)(float))
	{
		PACTIONINFO pActionInfo = FindAction(strAction);

		if (!pActionInfo)
		{
			return;
		}

		pActionInfo->pCallBack[static_cast<int>(eState)] = std::bind(pFunc, std::placeholders::_1);
	}

	bool CInput::CreateAction(const std::string& strAction, unsigned char iKey)
	{
		PACTIONINFO pActionInfo = FindAction(strAction);

		if (pActionInfo)
		{
			return false;
		}

		PKEYINFO pKeyInfo = FindKey(iKey);

		if (!pKeyInfo)
		{
			AddKey(iKey);

			pKeyInfo = FindKey(iKey);
		}

		pActionInfo = dbg_new ACTIONINFO;

		pActionInfo->pKeyInfo = pKeyInfo;

		m_mapAction.insert(std::make_pair(strAction, pActionInfo));

		return true;
	}

	void CInput::UpdateActions(float fDeltaTime)
	{
		std::unordered_map<std::string, PACTIONINFO>::iterator iter = m_mapAction.begin();
		std::unordered_map<std::string, PACTIONINFO>::iterator iterEnd = m_mapAction.end();

		for (; iter != iterEnd; ++iter)
		{
			if (iter->second->pKeyInfo->bDown &&
				iter->second->pCallBack[static_cast<int>(KEY_STATE::DOWN)])
			{
				iter->second->pCallBack[static_cast<int>(KEY_STATE::DOWN)](fDeltaTime);
			}
			else if (iter->second->pKeyInfo->bPressed &&
				iter->second->pCallBack[static_cast<int>(KEY_STATE::PRESS)])
			{
				iter->second->pCallBack[static_cast<int>(KEY_STATE::PRESS)](fDeltaTime);
			}
			else if (iter->second->pKeyInfo->bUp &&
				iter->second->pCallBack[static_cast<int>(KEY_STATE::UP)])
			{
				iter->second->pCallBack[static_cast<int>(KEY_STATE::UP)](fDeltaTime);
			}
		}
	}

	void CInput::SceneChanged()
	{
		Scene* pScene = SceneManager::GetInst()->GetScene();

		m_pMouse = pScene->CreateDrawable<Mouse>("Mouse", SceneManager::GetInst()->GetScene()->FindLayer(DEFAULT_LAYER));
	}

	CInput::PACTIONINFO CInput::FindAction(const std::string& strAction) const
	{
		std::unordered_map<std::string, PACTIONINFO>::const_iterator iter = m_mapAction.find(strAction);

		if (iter == m_mapAction.end())
		{
			return nullptr;
		}

		return iter->second;
	}

	void CInput::AddKey(unsigned char iKey)
	{
		m_KeyList.push_back(dbg_new KEYINFO(iKey));
	}

	const CInput::PKEYINFO CInput::FindKey(unsigned char iKey) const
	{
		std::list<PKEYINFO>::const_iterator iter = m_KeyList.begin();
		std::list<PKEYINFO>::const_iterator iterEnd = m_KeyList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->iKey == iKey)
			{
				return (*iter);
			}
		}

		return nullptr;
	}

	bool CInput::Init(HINSTANCE hInst, HWND hWnd)
	{
		m_hWnd = hWnd;

		if (FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&pInput), nullptr)))
		{
			return false;
		}

		if (!InitMouse(hInst))
		{
			return false;
		}

		if (!InitKeyboard(hInst))
		{
			return false;
		}

		SceneChanged();

		AddKey(DIK_SPACE);
		AddKey(DIK_W);
		AddKey(DIK_A);
		AddKey(DIK_S);
		AddKey(DIK_D);
		AddKey(DIK_Q);
		AddKey(DIK_E);

		return true;
	}
}