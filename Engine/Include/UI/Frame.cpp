#include "Frame.h"
#include "../Bindable/Transform.h"
#include "../Bindable/Texture.h"

Engine::Frame::Frame(const std::string& strTexture) :
	UIControl(strTexture)
	, m_iXStart()
	, m_iXEnd()
	, m_iYStart()
	, m_iYEnd()
{
}

void Engine::Frame::SetXStart(int fX)
{
	m_iXStart = fX;
}

void Engine::Frame::SetXEnd(int fX)
{
	m_iXEnd = fX;
}

void Engine::Frame::SetYStart(int fY)
{
	m_iYStart = fY;
}

void Engine::Frame::SetYEnd(int fY)
{
	m_iYEnd = fY;
}

void Engine::Frame::Bind()
{
	BindChild();

	const Vector3& vScale = GetTransform()->GetScale();
	const Vector3& vPos = GetTransform()->GetPosition();

	SetStartPos({ vPos.x, vPos.y });
	SetSize({ static_cast<float>(m_iXStart), static_cast<float>(m_iYStart) });

	SetStartUV({ 0.f, 1.f - m_iYStart / vScale.y });
	SetEndUV({ m_iXStart / vScale.x, 1.f });

	DrawQuad();

	std::shared_ptr<Texture> pTexture = GetTextures().front();

	int iImageWidth = pTexture->GetImageWidth();

	int iImageHeight = pTexture->GetImageHeight();

	int iTopRowCount = static_cast<int>(ceil((vScale.x - (iImageWidth - m_iXEnd - m_iXStart)) / static_cast<float>(m_iXEnd - m_iXStart)));

	int iLeftColCount = static_cast<int>(ceil((vScale.y - (iImageHeight - m_iYEnd - m_iYStart)) / static_cast<float>(m_iYEnd - m_iYStart)));

	SetStartPos({ vPos.x + (m_iXEnd - m_iXStart) * iTopRowCount, vPos.y });
	SetSize({ static_cast<float>(iImageWidth - m_iXEnd), static_cast<float>(m_iYStart) });

	SetStartUV({ m_iXEnd / vScale.x, 1.f - m_iYStart / vScale.y });
	SetEndUV({ 1.f, 1.f });

	DrawQuad();

	SetStartPos({ vPos.x, vPos.y + (m_iYEnd - m_iYStart) * iLeftColCount });
	SetSize({ static_cast<float>(m_iXStart), static_cast<float>(iImageHeight - m_iYEnd) });

	SetStartUV({ 0.f,  0.f });
	SetEndUV({ m_iXStart / vScale.x, m_iYEnd / vScale.y });

	DrawQuad();

	SetStartPos({ vPos.x + (m_iXEnd - m_iXStart) * iTopRowCount, vPos.y + (m_iYEnd - m_iYStart) * iLeftColCount });
	SetSize({ static_cast<float>(iImageWidth - m_iXEnd), static_cast<float>(iImageHeight - m_iYEnd) });

	SetStartUV({ 0.f,  0.f });
	SetEndUV({ m_iXEnd / vScale.x, m_iYEnd / vScale.y });

	DrawQuad();

	SetStartUV({ m_iXStart / vScale.x, 1.f - m_iYStart / vScale.y });
	SetEndUV({ m_iXEnd / vScale.x,  1.f });

	for (int i = 0; i < iTopRowCount; ++i)
	{
		SetStartPos({ vPos.x + (m_iXEnd - m_iXStart) * (i + 1), vPos.y });
		SetSize({ static_cast<float>(m_iXEnd - m_iXStart), static_cast<float>(m_iYStart) });

		DrawQuad();
	}

	SetStartUV({ 0.f , 1.f - m_iYEnd / vScale.y });
	SetEndUV({ m_iXStart / vScale.x,  1.f - m_iYStart / vScale.y });

	for (int i = 0; i < iLeftColCount; ++i)
	{
		SetStartPos({ vPos.x , vPos.y + (m_iYEnd - m_iYStart) * (i + 1) });
		SetSize({ static_cast<float>(m_iXStart), static_cast<float>(m_iYEnd - m_iYStart) });

		DrawQuad();
	}

	SetStartUV({ m_iXStart / vScale.x, 0.f  });
	SetEndUV({ m_iXEnd / vScale.x,  1.f - m_iYEnd / vScale.y });

	for (int i = 0; i < iTopRowCount; ++i)
	{
		SetStartPos({ vPos.x + (m_iXEnd - m_iXStart) * (i + 1), vPos.y + (m_iYEnd - m_iYStart) * iLeftColCount});
		SetSize({ static_cast<float>(m_iXEnd - m_iXStart), static_cast<float>(iImageHeight - m_iYEnd) });

		DrawQuad();
	}

	SetStartUV({ m_iXEnd / vScale.x, 1.f - m_iYEnd / vScale.y });
	SetEndUV({ 1.f,  1.f - m_iYStart / vScale.y });

	for (int i = 0; i < iLeftColCount; ++i)
	{
		SetStartPos({ vPos.x + (m_iXEnd - m_iXStart) * iTopRowCount , vPos.y + (m_iYEnd - m_iYStart) * (i + 1) });
		SetSize({ static_cast<float>(iImageWidth - m_iXEnd), static_cast<float>(m_iYEnd - m_iYStart) });

		DrawQuad();
	}

	SetStartUV({ m_iXStart / vScale.x, 1.f - m_iYEnd / vScale.y });
	SetEndUV({ m_iXEnd / vScale.x,  1.f - m_iYStart / vScale.y });

	for (int i = 0; i < iTopRowCount; ++i)
	{
		for (int j = 0; j < iLeftColCount; ++j)
		{
			SetStartPos({ vPos.x + (m_iXEnd - m_iXStart) * (i + 1) , vPos.y + (m_iYEnd - m_iYStart) * (j + 1) });
			SetSize({ static_cast<float>(m_iXEnd - m_iXStart), static_cast<float>(m_iYEnd - m_iYStart) });

			DrawQuad();
		}
	}
}
