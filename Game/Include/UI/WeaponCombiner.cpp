#include "WeaponCombiner.h"
#include "UI/Button.h"
#include "../Scene/StartScene.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Core/Window.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Scene/SceneManager.h"
#include "../Util/CSVLoader.h"
#include "Types.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace Client
{
    namespace WeaponCombiner_detail
    {
        // Slot / category indices — also the row order in the inventory.
        enum Category { CAT_ORIGIN = 0, CAT_MOVE, CAT_FIRE, CAT_ONHIT, CAT_SHAPE, CAT_LEVELUP };

        // Korean category captions for the slots and inventory rows.
        const wchar_t* kCatNames[6] = {
            L"발사 원점", L"이동 방식", L"발사 방식", L"피격 효과", L"투사체", L"레벨업"
        };

        // Per-category panel hues (behavioural categories share a hue so a
        // filled slot visibly belongs to its row). Level-up-field cards
        // carry their own per-variant colour instead.
        constexpr unsigned int kHueOrigin = 0x4FC3F7; // light blue
        constexpr unsigned int kHueMove   = 0x81C784; // green
        constexpr unsigned int kHueFire   = 0xFFB74D; // orange
        constexpr unsigned int kHueOnHit  = 0xE57373; // red
        constexpr unsigned int kHueShape  = 0xBA68C8; // purple

        // Built-in part catalogue — used when parts.csv is missing/empty so
        // the combiner always has cards to show. Mirrors the CSV columns:
        // { category, variant, label, colour, level-up amount, duration }.
        // (FireMode parts carry duration = weapon lifetime in seconds.)
        std::vector<PartCard> DefaultParts()
        {
            return {
                { CAT_ORIGIN, 0, L"Front",     kHueOrigin, 0.f, 0.f },
                { CAT_ORIGIN, 1, L"Around",    kHueOrigin, 0.f, 0.f },
                { CAT_ORIGIN, 2, L"Mouse",     kHueOrigin, 0.f, 0.f },
                { CAT_ORIGIN, 3, L"Random",    kHueOrigin, 0.f, 0.f },
                { CAT_MOVE,   0, L"Straight",  kHueMove,   0.f, 0.f },
                { CAT_MOVE,   1, L"Spiral",    kHueMove,   0.f, 0.f },
                { CAT_MOVE,   2, L"Fixed",     kHueMove,   0.f, 0.f },
                { CAT_MOVE,   3, L"Orbital",   kHueMove,   0.f, 0.f },
                { CAT_FIRE,   0, L"Cooldown",  kHueFire,   0.f, 2.0f },
                { CAT_FIRE,   1, L"Sustained", kHueFire,   0.f, 9999.0f },
                { CAT_ONHIT,  0, L"Vanish",    kHueOnHit,  0.f, 0.f },
                { CAT_ONHIT,  1, L"NoChange",  kHueOnHit,  0.f, 0.f },
                { CAT_ONHIT,  2, L"Reflect",   kHueOnHit,  0.f, 0.f },
                { CAT_ONHIT,  3, L"Multiply",  kHueOnHit,  0.f, 0.f },
                { CAT_SHAPE,  0, L"Sphere",    kHueShape,  0.f, 0.f },
                { CAT_SHAPE,  1, L"Box",       kHueShape,  0.f, 0.f },
                { CAT_SHAPE,  2, L"Triangle",  kHueShape,  0.f, 0.f },
                { CAT_LEVELUP, 0, L"Damage",   0xE53935, 2.0f, 0.f },
                { CAT_LEVELUP, 1, L"Cooldown", 0x26C6DA, 0.9f, 0.f },
                { CAT_LEVELUP, 2, L"Count",    0x9CCC65, 1.0f, 0.f },
                { CAT_LEVELUP, 3, L"Speed",    0xFFEE58, 3.0f, 0.f },
                { CAT_LEVELUP, 4, L"Size",     0xAB47BC, 0.1f, 0.f },
            };
        }

        // parts.csv "category" column -> Category index (-1 if unknown).
        int ParsePartCategory(std::string s)
        {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s == "origin")   return CAT_ORIGIN;
            if (s == "movement") return CAT_MOVE;
            if (s == "firemode") return CAT_FIRE;
            if (s == "onhit")    return CAT_ONHIT;
            if (s == "shape")    return CAT_SHAPE;
            if (s == "levelup")  return CAT_LEVELUP;
            return -1;
        }

        // Accepts "0xRRGGBB" or decimal — strtoul(base 0) handles both.
        unsigned int ParseColorHex(const std::string& s)
        {
            if (s.empty()) return 0xFFFFFF;
            return static_cast<unsigned int>(std::strtoul(s.c_str(), nullptr, 0));
        }

        // Load the part catalogue from parts.csv; fall back to DefaultParts
        // when the file is absent or has no usable rows.
        std::vector<PartCard> LoadParts()
        {
            std::vector<PartCard> out;
            for (const auto& row : CSVLoader::Load("/Game/Data/Weapons/parts.csv"))
            {
                if (row.size() < 4) continue;          // need cat, variant, label, colour
                const int cat = ParsePartCategory(row[0]);
                if (cat < 0) continue;
                PartCard p;
                p.iCategory = cat;
                p.iVariant  = std::atoi(row[1].c_str());
                p.wLabel.assign(row[2].begin(), row[2].end());   // ASCII label
                p.uColor    = ParseColorHex(row[3]);
                p.fAmount   = (row.size() > 4) ? static_cast<float>(std::atof(row[4].c_str())) : 0.f;
                p.fDuration = (row.size() > 5) ? static_cast<float>(std::atof(row[5].c_str())) : 0.f;
                out.push_back(std::move(p));
            }
            if (out.empty()) out = DefaultParts();
            return out;
        }

        constexpr unsigned int kEmptySlotColor = 0x37474F; // dark slate
        constexpr unsigned int kCraftOffColor  = 0x424242; // grey (locked)
        constexpr unsigned int kCraftOnColor   = 0x2E7D32; // green (ready)
        constexpr unsigned int kBackColor      = 0x455A64;
        constexpr unsigned int kRegUnequipColor = 0x546E7A; // blue-grey (registered, not equipped)

        constexpr float kDoubleClickSec = 0.35f;

        // ABGR memory layout (bytes R,G,B,A) — matches LevelUpChoices.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        // 1x1 solid-colour panel texture, cached by tag.
        std::shared_ptr<Engine::Texture> EnsureSolidTexture(unsigned int uRGB)
        {
            std::string strTag = "wc_solid_" + std::to_string(uRGB);
            if (auto p = Engine::StaticFindBindable<Engine::Texture>(strTag.c_str())) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>(strTag.c_str());
            if (!pNew) return nullptr;
            unsigned int uColor = PackABGR(uRGB);
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &uColor;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }
    }

    WeaponCombiner::WeaponCombiner()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool WeaponCombiner::Init()
    {
        using namespace WeaponCombiner_detail;
        if (!Engine::UIControl::Init()) return false;

        // Part catalogue from parts.csv (built-in fallback if absent).
        m_parts = LoadParts();

        const float W = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float H = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        // Malgun Gothic so the Korean captions render with real glyphs.
        m_pTitleFont = Engine::FontManager::GetInst()->CreateFont(
            "wc_title", L"Malgun Gothic", (std::max)(22.f, H * 0.045f), DWRITE_FONT_WEIGHT_BOLD);
        m_pMidFont = Engine::FontManager::GetInst()->CreateFont(
            "wc_mid", L"Malgun Gothic", (std::max)(16.f, H * 0.028f), DWRITE_FONT_WEIGHT_BOLD);
        m_pSmallFont = Engine::FontManager::GetInst()->CreateFont(
            "wc_small", L"Malgun Gothic", (std::max)(11.f, H * 0.017f), DWRITE_FONT_WEIGHT_BOLD);

        auto makeText = [&](const std::string& tag, const std::shared_ptr<Engine::Font>& pFont,
                            float x, float y, float w, float h, const std::wstring& s)
            -> std::shared_ptr<Engine::Text>
        {
            auto p = CreateComponent<Engine::Text>(tag);
            if (p)
            {
                p->SetFont(pFont);
                p->SetColor(0xFFFFFFFFu);
                p->SetHAlign(Engine::Text::HAlign::Center);
                p->SetVAlign(Engine::Text::VAlign::Center);
                p->SetRect(x, y, w, h);
                p->SetString(s);
            }
            return p;
        };

        // Title + back button.
        m_pTitleText = makeText("wc_title_txt", m_pTitleFont, 0.f, H * 0.02f, W, H * 0.06f, L"무기 조합");

        const float fBackW = (std::max)(110.f, W * 0.10f);
        const float fBackH = (std::max)(36.f,  H * 0.05f);
        m_pBackButton = CreateComponent<Engine::Button>("wc_back_btn");
        if (m_pBackButton)
        {
            m_pBackButton->SetRect(W * 0.02f, H * 0.02f, fBackW, fBackH);
            m_pBackButton->SetTexture(EnsureSolidTexture(kBackColor));
            m_pBackButton->SetOnClick([] {
                Engine::SceneManager::GetInst()->CreateScene<Client::StartScene>(); });
        }
        m_pBackText = makeText("wc_back_txt", m_pSmallFont, W * 0.02f, H * 0.02f, fBackW, fBackH, L"← 메뉴");

        // Six type-fixed slots in a centred row, each with the category
        // caption above and the equipped attribute name inside.
        const float fSlot   = (std::min)((std::max)(56.f, H * 0.09f), 120.f);
        const float fSlotGap = W * 0.012f;
        const float fSlotsW  = kCatCount * fSlot + (kCatCount - 1) * fSlotGap;
        const float fSlotsX  = (W - fSlotsW) * 0.5f;
        const float fCatY    = H * 0.105f;
        const float fSlotsY  = H * 0.135f;

        for (int c = 0; c < kCatCount; ++c)
        {
            const float x = fSlotsX + c * (fSlot + fSlotGap);
            m_pSlotCatText[c] = makeText("wc_slotcat_" + std::to_string(c), m_pSmallFont,
                x, fCatY, fSlot, H * 0.028f, kCatNames[c]);

            m_pSlotButton[c] = CreateComponent<Engine::Button>("wc_slot_" + std::to_string(c));
            if (m_pSlotButton[c])
            {
                m_pSlotButton[c]->SetRect(x, fSlotsY, fSlot, fSlot);
                m_pSlotButton[c]->SetTexture(EnsureSolidTexture(kEmptySlotColor));
            }
            m_pSlotNameText[c] = makeText("wc_slotname_" + std::to_string(c), m_pSmallFont,
                x, fSlotsY, fSlot, fSlot, L"-");
        }

        // Craft button (locked grey until all six slots are filled).
        const float fCraftW = (std::max)(200.f, W * 0.18f);
        const float fCraftH = (std::max)(44.f,  H * 0.06f);
        const float fCraftX = (W - fCraftW) * 0.5f;
        const float fCraftY = fSlotsY + fSlot + H * 0.06f;
        m_pCraftButton = CreateComponent<Engine::Button>("wc_craft_btn");
        if (m_pCraftButton)
        {
            m_pCraftButton->SetRect(fCraftX, fCraftY, fCraftW, fCraftH);
            m_pCraftButton->SetTexture(EnsureSolidTexture(kCraftOffColor));
            m_pCraftButton->SetOnClick([this] { OnCraft(); });
        }
        m_pCraftText = makeText("wc_craft_txt", m_pMidFont, fCraftX, fCraftY, fCraftW, fCraftH, L"무기 제작");

        const float fResultY = fCraftY + fCraftH + H * 0.015f;
        m_pResultText = makeText("wc_result_txt", m_pMidFont, 0.f, fResultY, W, H * 0.04f, L"");

        // Inventory: one row per category, caption on the left, attribute
        // icons to the right. Double-click an icon to equip it.
        const float fInvHeaderY = fResultY + H * 0.05f;
        const float fInvLeftX   = W * 0.08f;
        m_pInvHeader = CreateComponent<Engine::Text>("wc_inv_header");
        if (m_pInvHeader)
        {
            m_pInvHeader->SetFont(m_pMidFont);
            m_pInvHeader->SetColor(0xFFFFFFFFu);
            m_pInvHeader->SetHAlign(Engine::Text::HAlign::Left);
            m_pInvHeader->SetVAlign(Engine::Text::VAlign::Center);
            m_pInvHeader->SetRect(fInvLeftX, fInvHeaderY, W * 0.5f, H * 0.035f);
            m_pInvHeader->SetString(L"인벤토리 — 더블클릭하여 장착");
        }

        const float fInvTop  = fInvHeaderY + H * 0.045f;
        const float fRowH    = (H * 0.95f - fInvTop) / kCatCount;
        const float fIconSz  = (std::min)(fRowH * 0.78f, H * 0.07f);
        const float fCatColW = W * 0.12f;
        const float fIconsX  = fInvLeftX + fCatColW;
        const float fIconGap = W * 0.010f;

        // Row captions (one per category).
        for (int c = 0; c < kCatCount; ++c)
        {
            const float y = fInvTop + c * fRowH;
            auto p = CreateComponent<Engine::Text>("wc_invcat_" + std::to_string(c));
            if (p)
            {
                p->SetFont(m_pSmallFont);
                p->SetColor(0xFFFFFFFFu);
                p->SetHAlign(Engine::Text::HAlign::Left);
                p->SetVAlign(Engine::Text::VAlign::Center);
                p->SetRect(fInvLeftX, y, fCatColW, fRowH);
                p->SetString(kCatNames[c]);
            }
        }

        // Icons, flowed left-to-right within their category's row.
        const int iPartCount = static_cast<int>(m_parts.size());
        m_iconButtons.resize(iPartCount);
        m_iconTexts.resize(iPartCount);
        float fNextX[kCatCount];
        for (int c = 0; c < kCatCount; ++c) fNextX[c] = fIconsX;

        for (int i = 0; i < iPartCount; ++i)
        {
            const PartCard& a = m_parts[i];
            const float y = fInvTop + a.iCategory * fRowH + (fRowH - fIconSz) * 0.5f;
            const float x = fNextX[a.iCategory];
            fNextX[a.iCategory] += fIconSz + fIconGap;

            m_iconButtons[i] = CreateComponent<Engine::Button>("wc_icon_btn_" + std::to_string(i));
            if (m_iconButtons[i])
            {
                m_iconButtons[i]->SetRect(x, y, fIconSz, fIconSz);
                m_iconButtons[i]->SetTexture(EnsureSolidTexture(a.uColor));
                const int idx = i;
                m_iconButtons[i]->SetOnClick([this, idx] { OnIconClick(idx); });
            }
            m_iconTexts[i] = makeText("wc_icon_txt_" + std::to_string(i), m_pSmallFont,
                x, y, fIconSz, fIconSz, a.wLabel);
        }

        // Right-hand loadout panel: a grid of crafted-weapon cells. Click a
        // cell to equip/unequip it; only equipped weapons reach the stage.
        const float fPanelX0 = W * 0.52f;
        const float fPanelX1 = W * 0.96f;
        const float fPanelW  = fPanelX1 - fPanelX0;

        m_pRegHeader = CreateComponent<Engine::Text>("wc_reg_header");
        if (m_pRegHeader)
        {
            m_pRegHeader->SetFont(m_pMidFont);
            m_pRegHeader->SetColor(0xFFFFFFFFu);
            m_pRegHeader->SetHAlign(Engine::Text::HAlign::Left);
            m_pRegHeader->SetVAlign(Engine::Text::VAlign::Center);
            m_pRegHeader->SetRect(fPanelX0, fInvHeaderY, fPanelW, H * 0.035f);
            m_pRegHeader->SetString(L"제작 무기");
        }

        const int   kCols     = 2;
        const int   kRows     = kRegCells / kCols;   // 6
        const float fCellGapX = W * 0.010f;
        const float fCellGapY = H * 0.012f;
        const float fCellW    = (fPanelW - (kCols - 1) * fCellGapX) / kCols;
        const float fRegRowH  = (H * 0.95f - fInvTop) / kRows;
        const float fCellH    = fRegRowH - fCellGapY;

        for (int i = 0; i < kRegCells; ++i)
        {
            const int   col = i % kCols;
            const int   row = i / kCols;
            const float x = fPanelX0 + col * (fCellW + fCellGapX);
            const float y = fInvTop + row * fRegRowH;

            m_pRegButton[i] = CreateComponent<Engine::Button>("wc_reg_btn_" + std::to_string(i));
            if (m_pRegButton[i])
            {
                m_pRegButton[i]->SetRect(x, y, fCellW, fCellH);
                m_pRegButton[i]->SetTexture(EnsureSolidTexture(kRegUnequipColor));
                const int idx = i;
                m_pRegButton[i]->SetOnClick([this, idx] { OnRegistryClick(idx); });
            }
            m_pRegText[i] = makeText("wc_reg_txt_" + std::to_string(i), m_pSmallFont,
                x, y, fCellW, fCellH, L"");
        }

        RefreshCraft();
        RefreshRegistry();   // repopulate from any weapons crafted earlier this session
        return true;
    }

    void WeaponCombiner::OnIconClick(int iPaletteIndex)
    {
        // Second click on the same icon inside the window = double-click.
        const bool bDouble = (iPaletteIndex == m_iLastClickPalette)
            && (m_fTime - m_fLastClickTime) <= WeaponCombiner_detail::kDoubleClickSec;

        if (bDouble)
        {
            Equip(iPaletteIndex);
            m_iLastClickPalette = -1;     // require two fresh clicks next time
            m_fLastClickTime    = -10.f;
        }
        else
        {
            m_iLastClickPalette = iPaletteIndex;
            m_fLastClickTime    = m_fTime;
        }
    }

    void WeaponCombiner::Equip(int iPaletteIndex)
    {
        using namespace WeaponCombiner_detail;
        if (iPaletteIndex < 0 || iPaletteIndex >= static_cast<int>(m_parts.size())) return;

        const PartCard& a = m_parts[iPaletteIndex];
        const int c = a.iCategory;
        m_iSlot[c] = iPaletteIndex;

        if (m_pSlotButton[c])   m_pSlotButton[c]->SetTexture(EnsureSolidTexture(a.uColor));
        if (m_pSlotNameText[c]) m_pSlotNameText[c]->SetString(a.wLabel);

        RefreshCraft();
    }

    void WeaponCombiner::RefreshCraft()
    {
        using namespace WeaponCombiner_detail;
        bool bReady = true;
        for (int c = 0; c < kCatCount; ++c)
            if (m_iSlot[c] < 0) { bReady = false; break; }

        m_bCraftReady = bReady;
        if (m_pCraftButton)
            m_pCraftButton->SetTexture(EnsureSolidTexture(bReady ? kCraftOnColor : kCraftOffColor));
    }

    void WeaponCombiner::OnCraft()
    {
        using namespace WeaponCombiner_detail;
        if (!m_bCraftReady) return;

        // Registry display is bounded — refuse once the panel is full.
        if (WeaponDatabase::GetInst().CraftedCount() >= kRegCells)
        {
            if (m_pResultText)
                m_pResultText->SetString(L"제작 목록이 가득 찼습니다 (최대 "
                    + std::to_wstring(kRegCells) + L"개)");
            return;
        }

        WeaponDef def;
        const PartCard& fire = m_parts[m_iSlot[CAT_FIRE]];
        def.eOrigin   = static_cast<SpawnOrigin>    (m_parts[m_iSlot[CAT_ORIGIN]].iVariant);
        def.eMovement = static_cast<MovementType>   (m_parts[m_iSlot[CAT_MOVE]].iVariant);
        def.eFireMode = static_cast<FireMode>       (fire.iVariant);
        def.eOnHit    = static_cast<OnHitEvent>     (m_parts[m_iSlot[CAT_ONHIT]].iVariant);
        def.eShape    = static_cast<ProjectileShape>(m_parts[m_iSlot[CAT_SHAPE]].iVariant);

        // The LevelUp part carries both the bump amount and the weapon tint.
        const PartCard& lvl = m_parts[m_iSlot[CAT_LEVELUP]];
        def.eLevelUpField  = static_cast<LevelUpField>(lvl.iVariant);
        def.fLevelUpAmount = lvl.fAmount;
        def.uColorRGB      = lvl.uColor;

        // Lifetime comes from the chosen FireMode part's duration, so two
        // parts of the same fire mode can differ. Fall back to a fire-mode
        // default when the CSV leaves it unset (Sustained orbs effectively
        // permanent, Cooldown projectiles a 2s cap).
        float fLife = fire.fDuration;
        if (fLife <= 0.f) fLife = (def.eFireMode == FireMode::Sustained) ? 9999.f : 2.f;

        // Remaining stats use sensible defaults.
        def.iDamage          = 5;
        def.fCooldown        = 0.5f;
        def.fProjectileSpeed = 8.f;
        def.fLifetime        = fLife;
        def.iCount           = 1;
        def.fSize            = 0.25f;
        def.fAcceleration    = 0.f;

        const std::wstring wName = L"Custom " + std::to_wstring(++m_iCraftCounter);
        def.strName.assign(wName.begin(), wName.end());

        WeaponDatabase::GetInst().Add(def);

        // Auto-equip the new weapon when the loadout has room, so a
        // craft → play flow works without a separate equip step (the
        // stage pool is equipped-only). The just-added entry is last and
        // starts unequipped, so ToggleEquip here only ever equips — and
        // no-ops once the cap is reached. The list still lets you
        // unequip/swap when you've crafted more than the cap.
        auto& db = WeaponDatabase::GetInst();
        const bool bEquipped = db.ToggleEquip(db.CraftedCount() - 1);

        RefreshRegistry();   // show the new weapon in the loadout panel

        if (m_pResultText)
        {
            if (bEquipped)
                m_pResultText->SetString(L"제작 완료: " + wName + L" — 장착됨 ("
                    + std::to_wstring(db.EquippedCount()) + L"/"
                    + std::to_wstring(WeaponDatabase::kMaxEquipped) + L")");
            else
                m_pResultText->SetString(L"제작 완료: " + wName + L" — 장착 슬롯 가득("
                    + std::to_wstring(WeaponDatabase::kMaxEquipped)
                    + L"), 우측 목록에서 교체하세요");
        }

        // Consume the attribute slots — a successful craft empties all six
        // so the next weapon starts from an empty bench (craft re-locks).
        for (int c = 0; c < kCatCount; ++c)
        {
            m_iSlot[c] = -1;
            if (m_pSlotButton[c])   m_pSlotButton[c]->SetTexture(EnsureSolidTexture(kEmptySlotColor));
            if (m_pSlotNameText[c]) m_pSlotNameText[c]->SetString(L"-");
        }
        RefreshCraft();
    }

    void WeaponCombiner::OnRegistryClick(int iCell)
    {
        auto& db = WeaponDatabase::GetInst();
        if (iCell < 0 || iCell >= db.CraftedCount()) return;   // empty cell

        const bool bWasEquipped = db.IsEquipped(iCell);
        const bool bNowEquipped = db.ToggleEquip(iCell);
        // Tried to equip but the loadout is full (state unchanged).
        if (!bWasEquipped && !bNowEquipped && m_pResultText)
            m_pResultText->SetString(L"최대 "
                + std::to_wstring(WeaponDatabase::kMaxEquipped) + L"개까지 장착할 수 있습니다");

        RefreshRegistry();
    }

    void WeaponCombiner::RefreshRegistry()
    {
        using namespace WeaponCombiner_detail;
        auto& db = WeaponDatabase::GetInst();
        const int n = db.CraftedCount();

        for (int i = 0; i < kRegCells; ++i)
        {
            const bool bUsed = (i < n);

            if (m_pRegButton[i])
            {
                if (bUsed)
                {
                    m_pRegButton[i]->Enable();
                    const bool bEq = db.IsEquipped(i);
                    m_pRegButton[i]->SetTexture(EnsureSolidTexture(
                        bEq ? db.CraftedDef(i).uColorRGB : kRegUnequipColor));
                }
                else
                {
                    m_pRegButton[i]->Disable();
                }
            }

            if (m_pRegText[i])
            {
                if (bUsed)
                {
                    m_pRegText[i]->Enable();
                    const std::string& strName = db.CraftedDef(i).strName;
                    std::wstring w(strName.begin(), strName.end());
                    if (db.IsEquipped(i)) w += L"  ✓";
                    m_pRegText[i]->SetString(w);
                }
                else
                {
                    m_pRegText[i]->Disable();
                }
            }
        }

        if (m_pRegHeader)
            m_pRegHeader->SetString(L"제작 무기  장착 "
                + std::to_wstring(db.EquippedCount())
                + L"/" + std::to_wstring(WeaponDatabase::kMaxEquipped));
    }

    void WeaponCombiner::Update(float fDeltaTime)
    {
        // Advance our clock first so the double-click test inside the
        // child buttons' OnClick (fired during the base Update's child
        // recursion) reads the current frame time.
        m_fTime += fDeltaTime;
        Engine::UIControl::Update(fDeltaTime);
    }

    std::shared_ptr<Engine::Component> WeaponCombiner::Clone()
    {
        return std::make_shared<WeaponCombiner>(*this);
    }
}
