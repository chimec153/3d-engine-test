#include "Frame.h"

Engine::Frame::Frame() :
	m_iXStart()
	, m_iXEnd()
	, m_iYStart()
	, m_iYEnd()
{
}

Engine::Frame::Frame(const std::string& strTexture) :
	UIControl(strTexture)
	, m_iXStart()
	, m_iXEnd()
	, m_iYStart()
	, m_iYEnd()
{
}

void Engine::Frame::SetXStart(int fX) { m_iXStart = fX; }
void Engine::Frame::SetXEnd(int fX)   { m_iXEnd = fX; }
void Engine::Frame::SetYStart(int fY) { m_iYStart = fY; }
void Engine::Frame::SetYEnd(int fY)   { m_iYEnd = fY; }

std::shared_ptr<Engine::Component> Engine::Frame::Clone()
{
	return std::make_shared<Frame>(*this);
}
