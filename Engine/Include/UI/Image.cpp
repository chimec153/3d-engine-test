#include "Image.h"

namespace Engine
{
	Image::Image(const std::string& strTexture, const Vector2& vUVStart, const Vector2& vUVEnd) :
		UIControl(strTexture)
		, m_vStart(vUVStart)
		, m_vEnd(vUVEnd)
	{
		SetStartUV(vUVStart);
		SetEndUV(vUVEnd);

		SetBindableType(BINDABLE_TYPE::UI_IMAGE);
	}

}