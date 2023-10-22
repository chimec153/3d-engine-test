#include "Agent.h"
#include "NavMesh.h"
#include "TransformBuffer.h"

Engine::Agent::Agent(std::shared_ptr<TransformBuffer> pTransform, class NavMesh* pNavMesh, const Vector3& pos)	:
	Bindable()
	, m_pTransform(pTransform)
	, m_pNavMesh(pNavMesh)
	, m_iAgentIndex(CreateAgent(pos))
{
	SetBindableType(BINDABLE_TYPE::AGENT);
}

Engine::Agent::Agent(const Agent& agent)	:
	Bindable(agent)
	, m_pTransform(nullptr)
	, m_pNavMesh(agent.m_pNavMesh)
	, m_iAgentIndex(-1)
{
}

void Engine::Agent::SetTargetPos(const Vector3& pos)
{
	if (m_pNavMesh)
	{
		m_pNavMesh->SetTargetPos(m_iAgentIndex, pos);
	}
}

const Engine::Vector3 Engine::Agent::GetAgentVelocity() const
{
	if (!m_pNavMesh)
	{
		return Vector3();
	}

	return m_pNavMesh->GetAgentVelocity(m_iAgentIndex);
}

int Engine::Agent::CreateAgent(const Vector3& pos)
{
	return m_pNavMesh ? m_pNavMesh->CreateAgent(pos) : -1;
}

void Engine::Agent::SetTransform(std::shared_ptr<TransformBuffer> pTransform)
{
	if (m_iAgentIndex != -1)
	{
		if (m_pNavMesh)
		{
			m_pNavMesh->DeleteAgent(m_iAgentIndex);
		}
	}

	m_pTransform = pTransform;

	if (m_pTransform)
	{
		m_iAgentIndex = CreateAgent(m_pTransform->GetPosition());
	}
}

void Engine::Agent::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	if (m_pNavMesh && m_pTransform)
	{
		m_pTransform->SetPosition(m_pNavMesh->GetAgentPos(m_iAgentIndex));

		Vector3 vVelocity = m_pNavMesh->GetAgentVelocity(m_iAgentIndex);

		float fLength = vVelocity.Length();

		if (fLength)
		{
			vVelocity /= fLength;

			m_pTransform->SetAxis(AXIS_TYPE::Z, vVelocity);
		}
	}
}

void Engine::Agent::Bind()
{
}

std::shared_ptr<Engine::Bindable> Engine::Agent::Clone()
{
	return std::make_shared<Agent>(*this);
}
