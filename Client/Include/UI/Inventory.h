#pragma once

#include "UI/Image.h"

namespace Engine
{
    class ColliderOBB;
}

namespace Client
{
    class ItemIcon;
    class Inventory :
        public Engine::Image
    {
    public:
        enum class EQUIP_SLOT
        {
            HEAD,
            BODY,
            LEG,
            FOOT,
            HAND_RIGHT,
            HAND_LEFT,
            END
        };
    private:
        typedef struct _tagItemInfo
        {
            int iItemID;
            std::string strIcon;
            std::string strMesh;

            _tagItemInfo() :
                iItemID()
            {
            }

            _tagItemInfo(int iItemID, const std::string& strIcon, const std::string& strMesh) :
                iItemID(iItemID)
                , strIcon(strIcon)
                , strMesh(strMesh)
            {
            }

        }ITEMINFO, * PITEMINFO;
    private:
        static std::map<int, ITEMINFO>   s_mapItemTexture;
        static std::vector<Engine::Vector2>   s_mapEquipPosition;
    private:
        typedef struct _tagItemIconInfo
        {
            std::shared_ptr<ItemIcon> pItemIcon;
            PITEMINFO pInfo;

            _tagItemIconInfo() :
                pItemIcon()
                , pInfo()
            {
            }
        }ITEMICONINFO, *PITEMICONINFO;
    public:
        Inventory(const std::string& strTexture);
        virtual ~Inventory() override = default;

    private:
        std::vector<std::unique_ptr<ITEMICONINFO>> m_vecItem;
        std::vector<std::unique_ptr<ITEMICONINFO>> m_vecEquip;
        std::shared_ptr<Engine::ColliderOBB> m_pCollider;
        std::shared_ptr<Engine::ColliderOBB> m_pEquipSlotCollider[static_cast<int>(EQUIP_SLOT::END)];
        std::shared_ptr<Engine::ColliderOBB> m_pCurrentEquipCollider;
        std::shared_ptr<Engine::UIRenderer> m_pWeaponUIRenderer;

    public:
        bool AddItem(int iItemID);
        void DropItem(int x, int y, ItemIcon* pItem);
        void UpdateEquipSlot(int iSlot);
        void UpdateInventory(int iIndex);
        void ToggleInventory(float);

    public:
        virtual bool Init() override;

    public:
        void CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void CollisionEnd(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };

}