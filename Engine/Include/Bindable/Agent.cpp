#include "Agent.h"
#include "NavMesh.h"
#include "Transform.h"
#include "../Scene/Scene.h"
#include "Drawable.h"

Engine::Agent::Agent()	:
	m_pTransform()
	, m_pNavMesh()
	, m_iAgentIndex(-1)
	, m_pCrowdParams(std::make_unique<dtCrowdAgentParams>())
{
	SetBindableType(BINDABLE_TYPE::AGENT);
}

Engine::Agent::Agent(std::shared_ptr<Transform> pTransform, std::weak_ptr<NavMesh> pNavMesh, const Vector3& pos)	:
	Bindable()
	, m_pTransform(pTransform)
	, m_pNavMesh(pNavMesh)
	, m_iAgentIndex(-1)
	, m_pCrowdParams(std::make_unique<dtCrowdAgentParams>())
{
	m_pCrowdParams->maxAcceleration = 8.0f;
	m_pCrowdParams->maxSpeed = 2.0f;
	m_pCrowdParams->updateFlags = 0;
	m_pCrowdParams->obstacleAvoidanceType = 0;
	m_pCrowdParams->separationWeight = 0.f;

	m_iAgentIndex = CreateAgent(pos);

	SetBindableType(BINDABLE_TYPE::AGENT);
}

Engine::Agent::Agent(const Agent& agent)	:
	Bindable(agent)
	, m_pTransform(nullptr)
	, m_pNavMesh(agent.m_pNavMesh)
	, m_iAgentIndex(-1)
	, m_pCrowdParams(std::make_unique<dtCrowdAgentParams>())
{
	*m_pCrowdParams = *agent.m_pCrowdParams;
}

Engine::Agent::~Agent()
{
	if (!m_pNavMesh.expired())
	{
		m_pNavMesh.lock()->DeleteAgent(m_iAgentIndex);
		m_pNavMesh.reset();

		m_iAgentIndex = -1;
	}
}

void Engine::Agent::SetTargetPos(const Vector3& pos)
{
	if (!m_pNavMesh.expired())
	{
		m_pNavMesh.lock()->SetTargetPos(m_iAgentIndex, pos);
	}
}

const Engine::Vector3 Engine::Agent::GetAgentVelocity() const
{
	if (m_pNavMesh.expired())
	{
		return Vector3();
	}

	return m_pNavMesh.lock()->GetAgentVelocity(m_iAgentIndex);
}

int Engine::Agent::CreateAgent(const Vector3& pos)
{
	return !m_pNavMesh.expired() ? m_pNavMesh.lock()->CreateAgent(pos, *m_pCrowdParams) : -1;
}

void Engine::Agent::SetTransform(std::shared_ptr<Transform> pTransform)
{
	if (m_iAgentIndex != -1)
	{
		if (!m_pNavMesh.expired())
		{
			m_pNavMesh.lock()->DeleteAgent(m_iAgentIndex);
		}
	}

	m_pTransform = pTransform;

	if (m_pTransform)
	{
		m_iAgentIndex = CreateAgent(m_pTransform->GetPosition());
	}
}

void Engine::Agent::SetNavMesh(std::weak_ptr<NavMesh> pNavMesh)
{
	m_pNavMesh = pNavMesh;
}

void Engine::Agent::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	if (!m_pNavMesh.expired() && m_pTransform)
	{
		m_pTransform->SetPosition(m_pNavMesh.lock()->GetAgentPos(m_iAgentIndex));

		Vector3 vVelocity = m_pNavMesh.lock()->GetAgentVelocity(m_iAgentIndex);

		float fLength = vVelocity.Length();

		if (fLength)
		{
			vVelocity /= fLength;

			m_pTransform->SetAxis(AXIS_TYPE::Z, -vVelocity);
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

void Engine::Agent::Save(FILE* pFile)
{
	__super::Save(pFile);

	fwrite(&m_pCrowdParams->maxAcceleration, 4, 1, pFile);
	fwrite(&m_pCrowdParams->maxSpeed, 4, 1, pFile);
	fwrite(&m_pCrowdParams->separationWeight, 4, 1, pFile);
	fwrite(&m_pCrowdParams->updateFlags, 1, 1, pFile);
	fwrite(&m_pCrowdParams->obstacleAvoidanceType, 1, 1, pFile);
	fwrite(&m_pCrowdParams->queryFilterType, 1, 1, pFile);

	bool bOwner = static_cast<bool>(m_pTransform) && m_pTransform->GetParent();

	fwrite(&bOwner, 1, 1, pFile);

	if (bOwner)
	{
		const std::string& strTag = m_pTransform->GetParent()->GetTag();

		int iLength = static_cast<int>(strTag.length());

		fwrite(&iLength, 4, 1, pFile);

		if (iLength)
		{
			fwrite(strTag.c_str(), 1, iLength, pFile);
		}
	}
}

void Engine::Agent::Load(FILE* pFile)
{
	__super::Load(pFile);

	fread(&m_pCrowdParams->maxAcceleration, 4, 1, pFile);
	fread(&m_pCrowdParams->maxSpeed, 4, 1, pFile);
	fread(&m_pCrowdParams->separationWeight, 4, 1, pFile);
	fread(&m_pCrowdParams->updateFlags, 1, 1, pFile);
	fread(&m_pCrowdParams->obstacleAvoidanceType, 1, 1, pFile);
	fread(&m_pCrowdParams->queryFilterType, 1, 1, pFile);

	bool bOwner = false;

	fread(&bOwner, 1, 1, pFile);

	if (bOwner)
	{
		int iLength = 0;

		fread(&iLength, 4, 1, pFile);

		if (iLength)
		{
			std::unique_ptr<char[]> strTag = std::make_unique<char[]>(iLength + 1);

			strTag[iLength] = 0;

			fread(strTag.get(), 1, iLength, pFile);

			std::shared_ptr<Bindable> pBindable = GetScene()->FindBindable(strTag.get());

			if (pBindable)
			{
				SetTransform(std::static_pointer_cast<Drawable>(pBindable)->GetTransform());
			}
		}
	}
}
