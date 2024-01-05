#include "Inventory.h"
#include "../Client.h"
#include "ItemIcon.h"
#include "Bindable/Transform.h"
#include "Bindable/ColliderOBB.h"
#include "Scene/Scene.h"
#include "../Object/Player.h"
#include "Bindable/UIRenderer.h"
#include "Bindable/Topology.h"
#include "Bindable/InputLayout.h"
#include "Bindable/DepthStencilState.h"

namespace Client
{
	std::map<int, Inventory::ITEMINFO> Inventory::s_mapItemTexture = {
		{1, ITEMINFO(1, "sword_icon", "sword")},
		{2, ITEMINFO(2, "shovel_icon", "shovel")},
	};
	std::vector<Engine::Vector2> Inventory::s_mapEquipPosition = {
		{209.f, 183.f + 35.f * 3.f},
		{209.f, 183.f + 35.f * 2.f},
		{209.f, 183.f + 35.f},
		{209.f, 183.f},
		{209.f + 35.f, 183.f+ 35.f * 2.f},
		{209.f - 35.f, 183.f+ 35.f * 2.f},
	};

	Inventory::Inventory(const std::string& strTexture) :
		Engine::Image(strTexture)
		, m_pCollider(CreateBindable<Engine::ColliderOBB>("inventory_body"))
	{
		m_vecItem.resize(INVENTORY_WIDTH * INVENTORY_HEIGHT);
		m_vecEquip.resize(static_cast<int>(EQUIP_SLOT::END));

		for (int i = 0; i < static_cast<int>(EQUIP_SLOT::END); ++i)
		{
			m_pEquipSlotCollider[i] = CreateBindable<Engine::ColliderOBB>("Equip_Slot" + std::to_string(i));

			if (!m_pEquipSlotCollider[i])
			{
				return;
			}

			m_pEquipSlotCollider[i]->SetScaleOffset({ 1.f, 1.f, 0.f });

			m_pEquipSlotCollider[i]->SetOffset({ s_mapEquipPosition[i].x, s_mapEquipPosition[i].y, 0.f});

			m_pEquipSlotCollider[i]->SetChannel(Engine::COLLISION_CHANNEL::UI);

			m_pEquipSlotCollider[i]->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &Inventory::CollisionStay);
			m_pEquipSlotCollider[i]->SetCallBack(Engine::COLLISION_TYPE::LAST, this, &Inventory::CollisionEnd);
		}

		NotUseInstance();
	}

	bool Inventory::AddItem(int iItemID)
	{
		std::map<int, ITEMINFO>::iterator iter = s_mapItemTexture.find(iItemID);

		if (iter == s_mapItemTexture.end())
		{
			return false;
		}

		for (int i = 0; i < m_vecItem.size(); ++i)
		{
			if (!m_vecItem[i])
			{
				m_vecItem[i] = std::make_unique<ITEMICONINFO>();

				m_vecItem[i]->pItemIcon = CreateBindable<ItemIcon>("itemicon", iter->second.strIcon);

				if (!m_vecItem[i]->pItemIcon)
				{
					return false;
				}					

				m_vecItem[i]->pItemIcon->SetOwner(std::static_pointer_cast<Engine::UIControl>(std::shared_ptr(weak_from_this())));

				std::shared_ptr<Engine::Transform> pItemIconTransform = m_vecItem[i]->pItemIcon->GetTransform();

				if (!pItemIconTransform)
				{
					return false;
				}

				pItemIconTransform->SetRelativePosition(35.f * (i % INVENTORY_WIDTH) + 6.f, 35.f * (INVENTORY_HEIGHT - i / INVENTORY_WIDTH - 1) + 8.f, 0.f);

				m_vecItem[i]->pInfo = &iter->second;

				return true;
			}
		}

		return false;
	}
	void Inventory::DropItem(int x, int y, ItemIcon* pItem)
	{
		std::unique_ptr<ITEMICONINFO> pSrcItem = nullptr;
		int iStartIndex = -1;

		for (int i = 0; i < m_vecItem.size(); ++i)
		{
			if (m_vecItem[i] && m_vecItem[i]->pItemIcon.get() == pItem)
			{
				pSrcItem = std::move(m_vecItem[i]);
				iStartIndex = i;
				break;
			}
		}

		int iEquipSlot = -1;

		for (int i = 0; i < static_cast<int>(EQUIP_SLOT::END); ++i)
		{
			if (m_vecEquip[i] && m_vecEquip[i]->pItemIcon.get() == pItem)
			{
				m_vecEquip[i].swap(pSrcItem);
				iEquipSlot = i;
				break;
			}
		}

		const Engine::Vector3& vPos = GetTransform()->GetPosition();

		float fX = (x - vPos.x - 6.f) / 35.f;
		float fY = INVENTORY_HEIGHT - (y - vPos.y - 8.f) / 35.f;

		if (fX < 0.f || fY < 0.f || 
			fX >= INVENTORY_WIDTH || fY >= INVENTORY_HEIGHT)
		{
			if (pItem)
			{
				for (int i = 0; i < static_cast<int>(EQUIP_SLOT::END); ++i)
				{
					if ((m_pCurrentEquipCollider && m_pEquipSlotCollider[i] == m_pCurrentEquipCollider) || (!m_pCurrentEquipCollider && i == iEquipSlot))
					{
						m_vecEquip[i].swap(pSrcItem);

						UpdateEquipSlot(i);

						if (pSrcItem)
						{
							if (iStartIndex >= 0)
							{
								m_vecItem[iStartIndex].swap(pSrcItem);

								UpdateInventory(iStartIndex);
							}
							else 
							{
								m_vecEquip[iEquipSlot].swap(pSrcItem);

								UpdateEquipSlot(iEquipSlot);
							}
						}

						m_pCurrentEquipCollider.reset();
						return;
					}
				}

				m_vecItem[iStartIndex].swap(pSrcItem);

				UpdateInventory(iStartIndex);
			}
			return;
		}

		int iEndIndex = static_cast<int>(fX) + static_cast<int>(fY) * INVENTORY_WIDTH;

		pSrcItem.swap(m_vecItem[iEndIndex]);

		UpdateInventory(iEndIndex);

		if (pSrcItem)
		{
			if (iStartIndex >= 0)
			{
				m_vecItem[iStartIndex].swap(pSrcItem);

				UpdateInventory(iStartIndex);
			}
			else if (iEquipSlot >= 0)
			{
				m_vecEquip[iEquipSlot].swap(pSrcItem);

				UpdateEquipSlot(iEquipSlot);
			}
		}
	}
	void Inventory::UpdateEquipSlot(int iSlot)
	{
		if (m_vecEquip[iSlot])
		{
			m_vecEquip[iSlot]->pItemIcon->GetTransform()->SetRelativePosition(s_mapEquipPosition[iSlot].x - 16.f, s_mapEquipPosition[iSlot].y - 16.f, 0.f);

			switch (static_cast<EQUIP_SLOT>(iSlot))
			{
			case Inventory::EQUIP_SLOT::HAND_RIGHT:
			{
				std::shared_ptr<Player> pPlayer = std::static_pointer_cast<Player>(GetScene()->FindBindable("player"));

				pPlayer->ChangeWeaponMesh(m_vecEquip[iSlot]->pInfo->strMesh);

				m_pWeaponUIRenderer->SetTarget(pPlayer->GetWeapon());
			}
			break;
			default:
				break;
			}
		}
	}
	void Inventory::UpdateInventory(int iIndex)
	{
		if (m_vecItem[iIndex])
		{
			m_vecItem[iIndex]->pItemIcon->GetTransform()->SetRelativePosition(35.f * (iIndex % INVENTORY_WIDTH) + 6.f, 35.f * (INVENTORY_HEIGHT - iIndex / INVENTORY_WIDTH - 1) + 8.f, 0.f);
		}
	}
	void Inventory::ToggleInventory(float)
	{
		if (IsEnable())
		{
			Disable();
		}
		else
		{
			Enable();
		}
	}
	bool Inventory::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		std::shared_ptr<Engine::UIRenderer> pUIRenderer = CreateBindable<Engine::UIRenderer>("uirenderer");

		pUIRenderer->FindAndAddBind<Engine::InputLayout>("Standard");
		pUIRenderer->FindAndAddBind<Engine::Topology>("TriangleList");
		pUIRenderer->FindAndAddBind<Engine::VertexShader>(STANDARD_ANIM_VS);
		pUIRenderer->FindAndAddBind<Engine::PixelShader>("AlphaNoUVNoShadowPS");
		pUIRenderer->FindAndAddBind<Engine::DepthStencilState>("NoDepth");

		m_pWeaponUIRenderer = CreateBindable<Engine::UIRenderer>("uirenderer_weapon");

		m_pWeaponUIRenderer->FindAndAddBind<Engine::InputLayout>("Standard");
		m_pWeaponUIRenderer->FindAndAddBind<Engine::Topology>("TriangleList");
		m_pWeaponUIRenderer->FindAndAddBind<Engine::VertexShader>(STANDARD_ANIM_VS);
		m_pWeaponUIRenderer->FindAndAddBind<Engine::PixelShader>("AlphaNoUVNoShadowPS");
		m_pWeaponUIRenderer->FindAndAddBind<Engine::DepthStencilState>("NoDepth");

		return true;
	}
	void Inventory::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		Engine::Bindable* pParent = pDest->GetParent();

		if (typeid(*pParent) == typeid(ItemIcon))
		{
			if (static_cast<ItemIcon*>(pParent)->IsDrag())
			{
				m_pCurrentEquipCollider = std::static_pointer_cast<Engine::ColliderOBB>(pSrc->shared_from_this());
			}
		}
	}
	void Inventory::CollisionEnd(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (m_pCurrentEquipCollider.get() == pSrc)
		{
			m_pCurrentEquipCollider.reset();
		}
	}
}