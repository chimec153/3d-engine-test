#pragma once

#include "../Types.h"

namespace Engine
{
	class ENGINE_DLL Notify
	{
	public:
		Notify();
		~Notify() = default;

	private:
		std::string m_strTag;
		int m_iFrame;
		float m_fTime;
		std::function<void(int, float, class Bindable*)> m_pFunc;
		Bindable* m_pOwner;
		bool m_bCalled;

	public:
		void SetFrame(int iFrame);
		void SetTime(float fTime);
		void SetTag(const std::string& strTag);
		template <typename T>
		void SetCallBack(T* pObj, void(T::* pFunc)(int, float, Bindable*))
		{
			m_pFunc = std::bind(pFunc, pObj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}
		void SetCallBack(std::function<void(int, float, Bindable*)>);
		void SetCallBack(void(*pFunc)(int, float, Bindable*));
		void SetOwner(Bindable* pOwner);
		const std::string& GetTag()	const;

	public:
		void Clear();
		void Update(float fTime, int iFrame);
	};

}