#pragma once

#include "../Core/Macro.h"
namespace Engine
{

	template <typename T>
	class std::shared_ptr;

	class ENGINE_DLL ShaderManager
	{
	private:
		ShaderManager();
		~ShaderManager();

	private:
		static ShaderManager* m_pInst;

	public:
		static ShaderManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new ShaderManager;
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
		std::unordered_map<std::string, std::vector<class std::shared_ptr<class Bindable>>>	m_mapShader;
		GLOBALCBUFFER m_tCBuffer;
		std::shared_ptr<ConstantBuffer<GLOBALCBUFFER>>	m_pCBuffer;

	public:
		bool Init();
		void Update(float fDeltaTime, float fAccTime);
		const std::vector<std::shared_ptr<Bindable>>* FindShader(const std::string& strShader)	const;
	};

}