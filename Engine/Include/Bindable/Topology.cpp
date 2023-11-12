#include "Topology.h"


namespace Engine
{
	Topology::Topology(D3D_PRIMITIVE_TOPOLOGY topology) :
		Bindable()
		, m_eTopology(topology)
		, m_ePrevTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
	{
		SetBindableType(BINDABLE_TYPE::TOPOLOGY);
	}

	Topology::~Topology() noexcept
	{
	}

	void Topology::Update(float fDeltaTime)
	{
	}

	void Topology::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(m_eTopology);
	}

	std::shared_ptr<Bindable> Topology::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}

	void Topology::GetAndBind()
	{
		Graphics::GetInst()->GetDeviceContext()->IAGetPrimitiveTopology(&m_ePrevTopology);

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(m_eTopology);
	}

	void Topology::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(m_ePrevTopology);
	}
}