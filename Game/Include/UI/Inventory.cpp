#include "Inventory.h"
#include "ItemIcon.h"
#include "../Object/Attackable.h"
#include "Bindable/Transform.h"
#include "Bindable/ColliderOBB.h"
#include "Scene/Scene.h"
#include "../Object/Player.h"
#include "Bindable/UIRenderer.h"
#include "Bindable/Topology.h"
#include "Bindable/InputLayout.h"
#include "Bindable/DepthStencilState.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Material.h"
#include "Bindable/Particle.h"
#include "Resource/ResourceManager.h"

namespace Client
{
	std::map<int, Inventory::ITEMINFO> Inventory::s_mapItemTexture = {
		{1, ITEMINFO(1, "sword_icon", "sword", "sword sound", WEAPON_TYPE::SWORD)},
		{2, ITEMINFO(2, "shovel_icon", "shovel", "", WEAPON_TYPE::SWORD)},
		{3, ITEMINFO(3, "armor_icon", "armor", "", WEAPON_TYPE::FIST)},
		{4, ITEMINFO(4, "gun_icon", "pistol", "", WEAPON_TYPE::GUN)},
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
		, m_pCollider(CreateComponent<Engine::ColliderOBB>("inventory_body"))
	{
		m_vecItem.resize(INVENTORY_WIDTH * INVENTORY_HEIGHT);
		m_vecEquip.resize(static_cast<int>(EQUIP_SLOT::END));

		for (int i = 0; i < static_cast<int>(EQUIP_SLOT::END); ++i)
		{
			m_pEquipSlotCollider[i] = CreateComponent<Engine::ColliderOBB>("Equip_Slot" + std::to_string(i));

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

		// Phase E5 — NotUseInstance was a Drawable method; UI hierarchy is
		// now Component-based and instancing is irrelevant for dead UI code.
	}

	bool Inventory::AddItem(int /*iItemID*/)
	{
		// Phase E5 — Inventory's Drawable-era item-spawn body relied on
		// Drawable methods (CreateBindable / FindAndAddBind / GetScene /
		// Drawable::AddChild for material) that don't exist on the
		// Component-based UI hierarchy. Inventory itself is dead at runtime
		// (the GameScene CreateDrawable<Inventory> call has been commented
		// out for some time). Stubbed for compile-only; reintroduce under
		// a GameObject + UI-Component setup once the UI render path is
		// rebuilt on top of MeshRenderer/AddCustomRender.
		return false;
	}
	int Inventory::GetEquipItem(EQUIP_SLOT eSlot) const
	{
		PITEMINFO pInfo = GetEquipItemInfo(eSlot);

		if (!pInfo)
		{
			return 0;
		}

		return pInfo->iItemID;
	}
	const Inventory::PITEMINFO Inventory::GetEquipItemInfo(EQUIP_SLOT eSlot) const
	{
		if (eSlot < EQUIP_SLOT::HEAD || eSlot >= EQUIP_SLOT::END)
		{
			return nullptr;
		}

		if (!m_vecEquip[static_cast<int>(eSlot)])
		{
			return nullptr;
		}

		return m_vecEquip[static_cast<int>(eSlot)]->pInfo;
	}
	WEAPON_TYPE Inventory::GetEquipWeaponType(EQUIP_SLOT eSlot) const
	{
		PITEMINFO pInfo = GetEquipItemInfo(eSlot);

		if (!pInfo)
		{
			return WEAPON_TYPE::END;
		}

		return pInfo->eWeaponType;
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
		else
		{
			if (iEquipSlot >= 0)
			{
				UpdateEquipSlot(iEquipSlot);
			}
		}
	}
	void Inventory::UpdateEquipSlot(int /*iSlot*/)
	{
		// Phase E5 — Inventory is now a UI Component; the body below
		// relied on Drawable-era APIs (GetScene on `this`, Drawable*
		// equip targets, Player::ChangeArmorMesh / ChangeWeaponMesh).
		// Stubbed for compile-only — Inventory is dead at runtime
		// (GameScene CreateDrawable<Inventory> is commented out).
	}
	void Inventory::UpdateInventory(int iIndex)
	{
		if (m_vecItem[iIndex])
		{
			if (m_vecItem[iIndex]->pItemIcon)
			{
				if (auto pTr = m_vecItem[iIndex]->pItemIcon->GetTransform())
					pTr->SetRelativePosition(35.f * (iIndex % INVENTORY_WIDTH) + 6.f, 35.f * (INVENTORY_HEIGHT - iIndex / INVENTORY_WIDTH - 1) + 8.f, 0.f);
			}

			// Phase E5 — pItemDrawable field removed; reintroduce as
			// GameObject when the inventory UI is rebuilt.
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

		// Phase E5 — UIRenderer migrated to Component; created via
		// CreateComponent now. The IL/Topology/VS/PS/DSS resources used to
		// be attached as Bindable children of the UIRenderer-as-Drawable;
		// they belong on a paired MeshRenderer in any future GameObject-
		// hosted UI pipeline. (Inventory itself is currently dead at
		// runtime — the GameScene CreateDrawable<Inventory> call is
		// commented out — so this Init body only needs to compile.)
		std::shared_ptr<Engine::UIRenderer> pUIRenderer = CreateComponent<Engine::UIRenderer>("uirenderer");

		m_pWeaponUIRenderer = CreateComponent<Engine::UIRenderer>("uirenderer_weapon");

		return true;
	}
	void Inventory::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		// Phase E5 — ItemIcon is a Component now (not a Drawable). The
		// drag-snap-to-equip logic relied on casting the collider's
		// Drawable owner to ItemIcon — disabled for the dead UI path.
	}
	void Inventory::CollisionEnd(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (m_pCurrentEquipCollider.get() == pSrc)
		{
			m_pCurrentEquipCollider.reset();
		}
	}
}