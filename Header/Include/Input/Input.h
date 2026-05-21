#pragma once
#include <dinput.h>
#include "../Core/Ptr.h"

namespace Engine
{

	class ENGINE_DLL CInput
	{
	public:
		enum class KEY_STATE
		{
			DOWN,
			PRESS,
			UP,
			END
		};
		enum class MOUSE_TYPE
		{
			LEFT,
			RIGHT,
			WHEEL,
			END
		};
	private:
		typedef struct _tagKeyInfo
		{
			unsigned char iKey;
			bool bDown;
			bool bPressed;
			bool bUp;

			_tagKeyInfo(unsigned char key) :
				iKey(key)
				, bDown(false)
				, bPressed(false)
				, bUp(false)
			{
			}

			_tagKeyInfo() :
				iKey(0)
				, bDown(false)
				, bPressed(false)
				, bUp(false)
			{
			}
		}KEYINFO, * PKEYINFO;

		typedef struct _tagActionInfo
		{
			PKEYINFO pKeyInfo;
			std::function<void(float)> pCallBack[static_cast<int>(KEY_STATE::END)];
		}ACTIONINFO, * PACTIONINFO;

	private:
		CInput();
		~CInput();

	private:
		static CInput* m_pInst;

	public:
		static CInput* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new CInput;
			}

			return m_pInst;
		}

		static void DestroyInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		CPtr<IDirectInput8> pInput;
		CPtr<IDirectInputDevice8> pMouse;
		DIMOUSESTATE state;
		HWND m_hWnd;
		POINT m_tMousePos;
		CPtr<IDirectInputDevice8> pKeyboardDevice;
		char tKeyData[256];
		std::list<PKEYINFO> m_KeyList;
		std::unordered_map<std::string, PACTIONINFO>	m_mapAction;
		bool	m_bEnable;
		std::shared_ptr<class Mouse>	m_pMouse;
		KEYINFO m_pMouseButton[static_cast<int>(MOUSE_TYPE::END)];

	public:
		int GetMouseX()	const;
		int GetMouseY()	const;
		bool IsKey(KEY_STATE state, unsigned char iDikKey);
		void AddKey(unsigned char iKey);
		const PKEYINFO FindKey(unsigned char iKey)	const;
		int GetMouseDeltaX()	const;
		int GetMouseDeltaY()	const;
		int GetMouseDeltaZ()	const;
		bool IsMouseButtonDown(MOUSE_TYPE eType)	const;
		bool IsMouseButtonPress(MOUSE_TYPE eType)	const;
		bool IsMouseButtonUp(MOUSE_TYPE eType)	const;
		void Disable();
		void Enable();
		bool CreateAction(const std::string& strAction, unsigned char iKey);
		void AddAction(const std::string& strAction, KEY_STATE eState, void(*pFunc)(float));

		// Drops every registered action and its bound callbacks. Used by
		// ProjectModule on game-load so the previous world's input bindings
		// don't fire against the new scene's objects.
		void ClearActions();

		template <typename T>
		void AddAction(const std::string& strAction, KEY_STATE eState, T* pObj, void(T::* pFunc)(float))
		{
			PACTIONINFO pActionInfo = FindAction(strAction);

			if (!pActionInfo)
			{
				assert(false);
				return;
			}

			pActionInfo->pCallBack[static_cast<int>(eState)] = std::bind(pFunc, pObj, std::placeholders::_1);
		}

	public:
		bool Init(HINSTANCE hInst, HWND hWnd);
		bool InitKeyboard(HINSTANCE hInst);
		bool InitMouse(HINSTANCE hInst);
		void Update(float fDeltaTime);
		void UpdateKey();
		void UpdateMouse();
		void UpdateActions(float fDeltaTime);
		void SceneChanged();

	private:
		PACTIONINFO FindAction(const std::string& strAction)	const;
	};

}