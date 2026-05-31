#include "WeaponCombiner.h"
#include "UI/Button.h"
#include "UI/ScrollView.h"
#include "UI/NumberField.h"
#include "UI/EditBox.h"
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
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <vector>

namespace Client
{
    namespace WeaponCombiner_detail
    {
        // Slot / category indices — also the row order in the inventory.
        enum Category { CAT_ORIGIN = 0, CAT_MOVE, CAT_LIFETIME, CAT_FIRERATE, CAT_ONHIT, CAT_SIZE, CAT_LEVELUP, CAT_ACCEL, CAT_IMPACT };

        // The four categories driven by typed/slider NumberFields instead of
        // preset cards: they have no inventory card row and no top slot, and
        // AssembleWeaponDef reads their values directly (not via m_iSlot).
        inline bool IsNumericCat(int c)
        {
            return c == CAT_LIFETIME || c == CAT_FIRERATE || c == CAT_SIZE || c == CAT_ACCEL;
        }

        // The five categories that still use selectable cards + a top slot.
        const int kCardCats[] = { CAT_ORIGIN, CAT_MOVE, CAT_ONHIT, CAT_LEVELUP, CAT_IMPACT };
        constexpr int kCardCatCount = 5;

        // Korean category captions for the slots and inventory rows.
        const wchar_t* kCatNames[9] = {
            L"발사 원점", L"이동 방식", L"지속 시간", L"연사 속도", L"피격 효과", L"크기", L"레벨업", L"가속도", L"충격 효과"
        };

        // Per-category panel hues (behavioural categories share a hue so a
        // filled slot visibly belongs to its row). Level-up-field cards
        // carry their own per-variant colour instead.
        constexpr unsigned int kHueOrigin = 0x4FC3F7; // light blue
        constexpr unsigned int kHueMove   = 0x81C784; // green
        constexpr unsigned int kHueFire   = 0xFFB74D; // orange
        constexpr unsigned int kHueOnHit  = 0xE57373; // red
        constexpr unsigned int kHueSize   = 0xBA68C8; // purple
        constexpr unsigned int kHueFireRate = 0xFFCA28; // amber (fire rate)
        constexpr unsigned int kHueAccel  = 0x4DB6AC; // teal (acceleration)
        constexpr unsigned int kHueImpact = 0xFF8A65; // deep orange (impact)

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
                { CAT_MOVE,   0, L"Straight",  kHueMove,   0.f,  0.f },
                { CAT_MOVE,   1, L"Spiral",    kHueMove,   0.f,  0.f },
                { CAT_MOVE,   2, L"Fixed",     kHueMove,   0.f,  0.f },
                { CAT_MOVE,   3, L"Orbital",   kHueMove,   0.9f, 0.f },       // circles at r=0.9
                { CAT_MOVE,   3, L"Follow",    kHueMove,   0.f,  0.f },       // r=0: centered on player
                { CAT_MOVE,   3, L"SpiralOut", kHueMove,   0.9f, 0.f, 3.0f }, // r=0.9 growing 3/s outward
                { CAT_MOVE,   4, L"Homing",    kHueMove,   0.f,  0.f },                   // steers toward nearest enemy
                { CAT_MOVE,   4, L"HomingWeak", kHueMove,  0.f,  0.f, 0.f, 0.f, 1.f },    // aim_mode 1 = LowestHP
                { CAT_MOVE,   4, L"HomingRand", kHueMove,  0.f,  0.f, 0.f, 0.f, 2.f },    // aim_mode 2 = Random
                { CAT_LIFETIME, 0, L"Short",   kHueFire,   0.f, 0.5f },   // duration = projectile lifetime (s)
                { CAT_LIFETIME, 1, L"Medium",  kHueFire,   0.f, 2.0f },
                { CAT_LIFETIME, 2, L"Long",    kHueFire,   0.f, 5.0f },
                { CAT_FIRERATE, 0, L"Rapid",   kHueFireRate, 0.2f, 0.f },   // amount = cooldown (s)
                { CAT_FIRERATE, 1, L"Normal",  kHueFireRate, 0.5f, 0.f },
                { CAT_FIRERATE, 2, L"Slow",    kHueFireRate, 1.0f, 0.f },
                { CAT_FIRERATE, 3, L"Continuous", kHueFireRate, 0.f, 0.f }, // 0 = Sustained (persistent)
                { CAT_ONHIT,  0, L"Vanish",    kHueOnHit,  0.f, 0.f },
                { CAT_ONHIT,  1, L"NoChange",  kHueOnHit,  0.f, 0.f },
                { CAT_ONHIT,  2, L"Reflect1",  kHueOnHit,  1.f, 0.f },  // amount = reflect/pierce count
                { CAT_ONHIT,  2, L"Reflect3",  kHueOnHit,  3.f, 0.f },
                { CAT_ONHIT,  2, L"Reflect5",  kHueOnHit,  5.f, 0.f },
                { CAT_ONHIT,  2, L"Reflect7",  kHueOnHit,  7.f, 0.f },
                { CAT_ONHIT,  3, L"Multiply1", kHueOnHit,  1.f, 0.f },  // amount = split generations
                { CAT_ONHIT,  3, L"Multiply3", kHueOnHit,  3.f, 0.f },
                { CAT_ONHIT,  3, L"Multiply5", kHueOnHit,  5.f, 0.f },
                { CAT_ONHIT,  3, L"Multiply7", kHueOnHit,  7.f, 0.f },
                { CAT_ONHIT,  4, L"Field",     kHueOnHit,  0.f, 0.f },
                { CAT_SIZE,   0, L"10",        kHueSize,  10.f, 0.f },
                { CAT_SIZE,   1, L"20",        kHueSize,  20.f, 0.f },
                { CAT_SIZE,   2, L"30",        kHueSize,  30.f, 0.f },
                { CAT_LEVELUP, 0, L"Damage",   0xE53935, 2.0f, 0.f },
                { CAT_LEVELUP, 1, L"Cooldown", 0x26C6DA, 0.9f, 0.f },
                { CAT_LEVELUP, 2, L"Count",    0x9CCC65, 1.0f, 0.f },
                { CAT_LEVELUP, 3, L"Speed",    0xFFEE58, 3.0f, 0.f },
                { CAT_LEVELUP, 4, L"Size",     0xAB47BC, 0.1f, 0.f },
                { CAT_ACCEL,  0, L"Decel",     kHueAccel, -3.f, 0.f }, // amount = speed delta/sec
                { CAT_ACCEL,  1, L"Constant",  kHueAccel,  0.f, 0.f },
                { CAT_ACCEL,  2, L"Accel",     kHueAccel,  6.f, 0.f },
                // Impact modules (multi-select). amount = effect strength;
                // growth = Gather AoE radius; duration = Burn seconds. Damage
                // is the baseline.
                { CAT_IMPACT, 0, L"Damage",    kHueImpact, 0.f, 0.f },
                { CAT_IMPACT, 1, L"Knockback", kHueImpact, 6.f, 0.f },
                { CAT_IMPACT, 2, L"Gather",    kHueImpact, 6.f, 0.f, 4.f },
                { CAT_IMPACT, 3, L"Burn",      kHueImpact, 3.f, 3.f },        // amount=tick dmg, duration=seconds
                { CAT_IMPACT, 4, L"Slow",      kHueImpact, 0.5f, 2.5f },      // amount=speed mult, duration=seconds
            };
        }

        // parts.csv "category" column -> Category index (-1 if unknown).
        int ParsePartCategory(std::string s)
        {
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s == "origin")   return CAT_ORIGIN;
            if (s == "movement") return CAT_MOVE;
            if (s == "firemode" || s == "lifetime") return CAT_LIFETIME;
            if (s == "firerate") return CAT_FIRERATE;
            if (s == "onhit")    return CAT_ONHIT;
            if (s == "size")     return CAT_SIZE;
            if (s == "levelup")  return CAT_LEVELUP;
            if (s == "acceleration" || s == "accel") return CAT_ACCEL;
            if (s == "impact")   return CAT_IMPACT;
            return -1;
        }

        // Accepts "0xRRGGBB" or decimal — strtoul(base 0) handles both.
        unsigned int ParseColorHex(const std::string& s)
        {
            if (s.empty()) return 0xFFFFFF;
            return static_cast<unsigned int>(std::strtoul(s.c_str(), nullptr, 0));
        }

        // Valid variant count for a category — the variant column must be the
        // category's enum value (not a row index). Size has no enum (it's an
        // index), so it accepts any value.
        int VariantCount(int cat)
        {
            switch (cat)
            {
            case CAT_ORIGIN:  return static_cast<int>(SpawnOrigin::COUNT);
            case CAT_MOVE:    return static_cast<int>(MovementType::COUNT);
            case CAT_ONHIT:   return static_cast<int>(OnHitEvent::COUNT);
            case CAT_LEVELUP: return static_cast<int>(LevelUpField::COUNT_);
            case CAT_IMPACT:  return kImpactModuleCount;
            default:          return 0x7fffffff;   // CAT_SIZE / CAT_FIRERATE / CAT_LIFETIME: free index
            }
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
                const int variant = std::atoi(row[1].c_str());
                // Skip out-of-range variants: a bad number (e.g. Multiply
                // written as 5 instead of 3) would cast to an invalid enum
                // and silently misbehave (despawn on first hit). Dropping the
                // row makes the mistake visible — the card just won't appear.
                if (variant < 0 || variant >= VariantCount(cat)) continue;
                PartCard p;
                p.iCategory = cat;
                p.iVariant  = variant;
                p.wLabel.assign(row[2].begin(), row[2].end());   // ASCII label
                p.uColor    = ParseColorHex(row[3]);
                p.fAmount   = (row.size() > 4) ? static_cast<float>(std::atof(row[4].c_str())) : 0.f;
                p.fDuration = (row.size() > 5) ? static_cast<float>(std::atof(row[5].c_str())) : 0.f;
                p.fGrowth   = (row.size() > 6) ? static_cast<float>(std::atof(row[6].c_str())) : 0.f;
                p.fDamageInterval = (row.size() > 7) ? static_cast<float>(std::atof(row[7].c_str())) : 0.f;
                p.fAimMode  = (row.size() > 8) ? static_cast<float>(std::atof(row[8].c_str())) : 0.f;
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

        // HSV (h,s,v in 0..1) -> 0xRRGGBB. Used to spread crafted-weapon
        // colours across the hue wheel so different builds look different.
        unsigned int HsvToRgb(float h, float s, float v)
        {
            const int   i = static_cast<int>(h * 6.0f);
            const float f = h * 6.0f - i;
            const float p = v * (1.0f - s);
            const float q = v * (1.0f - f * s);
            const float t = v * (1.0f - (1.0f - f) * s);
            float r = 0.f, g = 0.f, b = 0.f;
            switch (((i % 6) + 6) % 6)
            {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default:r = v; g = p; b = q; break;
            }
            return (static_cast<unsigned int>(r * 255.f) << 16)
                 | (static_cast<unsigned int>(g * 255.f) <<  8)
                 |  static_cast<unsigned int>(b * 255.f);
        }

        // Deterministic colour for a build: FNV-1a hash of the part labels
        // -> hue, fixed vivid S/V. Same combo → same colour, different combos
        // → (mostly) different hues, so weapons are visually distinguishable.
        unsigned int ComboColor(const std::string& strKey)
        {
            unsigned int h = 2166136261u;
            for (unsigned char c : strKey) { h ^= c; h *= 16777619u; }
            return HsvToRgb((h % 360u) / 360.f, 0.72f, 0.95f);
        }

        // Darken an 0xRRGGBB colour (for the un-equipped registry cells).
        unsigned int Dim(unsigned int uRGB, float fScale)
        {
            const unsigned int r = static_cast<unsigned int>(((uRGB >> 16) & 0xFF) * fScale);
            const unsigned int g = static_cast<unsigned int>(((uRGB >>  8) & 0xFF) * fScale);
            const unsigned int b = static_cast<unsigned int>(((uRGB      ) & 0xFF) * fScale);
            return (r << 16) | (g << 8) | b;
        }

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

        // Display texture for a part card. Movement parts ship an icon under
        // Resource/Texture/Icon whose file name matches the label (Straight,
        // Orbital, SpiralOut, …) — load and cache it. Movement variants
        // without an icon (Homing, Aimed, SpiralOutNear/Wide) and every
        // non-movement category fall back to the flat category-colour swatch.
        std::shared_ptr<Engine::Texture> PartTexture(const PartCard& a)
        {
            if (a.iCategory == CAT_MOVE)
            {
                std::string strTag = "wc_movicon_";
                for (wchar_t wc : a.wLabel) strTag += static_cast<char>(wc);
                auto pTex = Engine::StaticFindBindable<Engine::Texture>(strTag.c_str());
                if (!pTex)
                {
                    const std::wstring wPath = L"/Game/Texture/Icon/" + a.wLabel + L".png";
                    pTex = Engine::StaticCreateBindable<Engine::Texture>(
                        strTag.c_str(), wPath.c_str(), TEXTURE_PATH, 0);
                }
                // GetSRV() is null when the file was missing — fall through.
                if (pTex && pTex->GetSRV()) return pTex;
            }
            return EnsureSolidTexture(a.uColor);
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
        m_pTitleText = makeText("wc_title_txt", m_pTitleFont, 0.f, H * 0.02f, W, H * 0.06f, L"무기 편집");

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
        // (Core stat fields — damage / count / speed — now live in the left
        // attribute scroll, built below alongside the other number fields.)

        // Editable weapon name — a text-mode EditBox in the top bar (right of the
        // centred title), with a caption. Type A-Z / digits / space; the typed
        // string is written back as the weapon's name on save.
        {
            const float fNameW = (std::max)(160.f, W * 0.18f);
            const float fNameH = (std::max)(28.f,  H * 0.04f);
            const float fNameX = W * 0.66f;
            const float fNameY = H * 0.035f;
            makeText("wc_name_cap", m_pSmallFont, fNameX - W * 0.06f, fNameY, W * 0.055f, fNameH, L"이름");
            m_pNameBox = CreateComponent<Engine::EditBox>("wc_name_box");
            if (m_pNameBox)
            {
                m_pNameBox->SetTextMode(true);
                m_pNameBox->SetFont(m_pSmallFont);
                m_pNameBox->SetBoxRect(fNameX, fNameY, fNameW, fNameH);
                m_pNameBox->SetText(L"");
            }
        }

        // Six type-fixed slots in a centred row, each with the category
        // caption above and the equipped attribute name inside.
        const float fSlot   = (std::min)((std::max)(56.f, H * 0.09f), 120.f);
        const float fSlotGap = W * 0.012f;
        const float fSlotsW  = kCardCatCount * fSlot + (kCardCatCount - 1) * fSlotGap;
        const float fSlotsX  = (W - fSlotsW) * 0.5f;
        const float fCatY    = H * 0.105f;
        const float fSlotsY  = H * 0.135f;

        // Only the five card categories get a top slot; the numeric categories
        // (lifetime / fire rate / size / accel) live as NumberFields below.
        for (int i = 0; i < kCardCatCount; ++i)
        {
            const int   c = kCardCats[i];
            const float x = fSlotsX + i * (fSlot + fSlotGap);
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
            m_pCraftButton->SetOnClick([this] { OnSave(); });
        }
        m_pCraftText = makeText("wc_craft_txt", m_pMidFont, fCraftX, fCraftY, fCraftW, fCraftH, L"저장");

        // Live power-score preview, sitting in the gap above the craft
        // button. RefreshCraft fills it once all six slots are equipped.
        m_pScoreText = makeText("wc_score_txt", m_pSmallFont,
            0.f, fSlotsY + fSlot + H * 0.008f, W, H * 0.04f, L"");

        const float fResultY = fCraftY + fCraftH + H * 0.015f;
        m_pResultText = makeText("wc_result_txt", m_pMidFont, 0.f, fResultY, W, H * 0.04f, L"");

        // ---- Left attribute panel — one vertical-scrolling list ----
        // Card categories show their cards as a wrapped grid; numeric
        // categories and the three core-stat rows (damage / count / speed) show
        // a labelled NumberField. Everything registers with one vertical
        // ScrollView so the whole column scrolls together (top slots stay put).
        const float fInvHeaderY = fResultY + H * 0.05f;
        const float fInvLeftX   = W * 0.06f;
        const float fBandRight  = W * 0.50f;
        m_pInvHeader = CreateComponent<Engine::Text>("wc_inv_header");
        if (m_pInvHeader)
        {
            m_pInvHeader->SetFont(m_pMidFont);
            m_pInvHeader->SetColor(0xFFFFFFFFu);
            m_pInvHeader->SetHAlign(Engine::Text::HAlign::Left);
            m_pInvHeader->SetVAlign(Engine::Text::VAlign::Center);
            m_pInvHeader->SetRect(fInvLeftX, fInvHeaderY, fBandRight - fInvLeftX, H * 0.035f);
            m_pInvHeader->SetString(L"무기 속성 — 더블클릭 장착 / 값 입력 (휠 스크롤)");
        }

        const float fInvTop  = fInvHeaderY + H * 0.04f;
        const float fInvBot  = H * 0.96f;
        const float fInvW    = fBandRight - fInvLeftX;
        const float fCatColW = W * 0.085f;
        const float fIconsX  = fInvLeftX + fCatColW;
        const float fIconGap = W * 0.008f;
        const float fIconSz  = (std::max)(38.f, H * 0.05f);
        const float fLineH   = (std::max)(24.f, H * 0.034f);
        const float fRowGap  = H * 0.010f;
        const float fFieldW  = fBandRight - fIconsX;
        const int   perLine  = (std::max)(1, static_cast<int>((fBandRight - fIconsX) / (fIconSz + fIconGap)));

        m_pInvScroll = CreateComponent<Engine::ScrollView>("wc_inv_scroll");
        if (m_pInvScroll)
        {
            m_pInvScroll->SetAxis(Engine::ScrollView::Axis::Vertical);
            m_pInvScroll->SetViewport(fInvLeftX, fInvTop, fInvW, fInvBot - fInvTop);
        }

        const int iPartCount = static_cast<int>(m_parts.size());
        m_iconButtons.assign(iPartCount, nullptr);
        m_iconTexts.assign(iPartCount, nullptr);

        auto reg = [&](const std::shared_ptr<Engine::UIControl>& p, float x, float y, float w, float h)
        {
            if (p && m_pInvScroll) m_pInvScroll->AddItem(p, x, y, w, h);
        };
        // Caption (left) + NumberField (right) on one content line.
        auto numRow = [&](std::shared_ptr<Engine::NumberField>& out, const std::string& tag,
                          const std::wstring& cap, float fMin, float fMax, int dec, float fInit,
                          float fW, float& cyRef)
        {
            auto pcap = makeText(tag + "_cap", m_pSmallFont, fInvLeftX, cyRef, fCatColW, fLineH, cap);
            reg(pcap, fInvLeftX, cyRef, fCatColW, fLineH);
            out = CreateComponent<Engine::NumberField>(tag);
            if (out)
            {
                out->SetFont(m_pSmallFont);
                out->SetRange(fMin, fMax);
                out->SetDecimals(dec);
                out->SetFieldRect(fIconsX, cyRef, fW, fLineH);
                out->SetValue(fInit);
                out->SetOnChange([this](float) { RefreshCraft(); });
            }
            reg(out, fIconsX, cyRef, fW, fLineH);
        };

        const float fSustainW = fFieldW * 0.26f;
        float cy = fInvTop;
        for (int c = 0; c < kCatCount; ++c)
        {
            if (c == CAT_LIFETIME)
            {
                numRow(m_pNumLifetime, "wc_num_life", kCatNames[c], 0.1f, 10.f, 1, 2.0f, fFieldW, cy);
                cy += fLineH + fRowGap;
            }
            else if (c == CAT_FIRERATE)
            {
                const float fCdW = fFieldW - fSustainW - fIconGap;
                numRow(m_pNumCooldown, "wc_num_cd", kCatNames[c], 0.05f, 3.f, 2, 0.5f, fCdW, cy);
                const float sx = fIconsX + fCdW + fIconGap;
                m_pSustainBtn = CreateComponent<Engine::Button>("wc_sustain_btn");
                if (m_pSustainBtn)
                {
                    m_pSustainBtn->SetRect(sx, cy, fSustainW, fLineH);
                    m_pSustainBtn->SetTexture(EnsureSolidTexture(kEmptySlotColor));
                    m_pSustainBtn->SetOnClick([this] { OnSustainToggle(); });
                }
                m_pSustainText = makeText("wc_sustain_txt", m_pSmallFont, sx, cy, fSustainW, fLineH, L"지속형");
                reg(m_pSustainBtn,  sx, cy, fSustainW, fLineH);
                reg(m_pSustainText, sx, cy, fSustainW, fLineH);
                cy += fLineH + fRowGap;
            }
            else if (c == CAT_SIZE)
            {
                numRow(m_pNumSize, "wc_num_size", kCatNames[c], 0.05f, 30.f, 2, 1.0f, fFieldW, cy);
                cy += fLineH + fRowGap;
            }
            else if (c == CAT_ACCEL)
            {
                numRow(m_pNumAccel, "wc_num_accel", kCatNames[c], -10.f, 10.f, 1, 0.0f, fFieldW, cy);
                cy += fLineH + fRowGap;
            }
            else
            {
                // Card category — full-width caption line, then wrapped icons.
                auto pcap = makeText("wc_invcat_" + std::to_string(c), m_pSmallFont,
                    fInvLeftX, cy, fInvW, fLineH, kCatNames[c]);
                reg(pcap, fInvLeftX, cy, fInvW, fLineH);
                cy += fLineH;

                float ix = fIconsX, iy = cy;
                int   col = 0;
                for (int i = 0; i < iPartCount; ++i)
                {
                    if (m_parts[i].iCategory != c) continue;
                    if (col > 0 && (col % perLine) == 0) { ix = fIconsX; iy += fIconSz + fIconGap; }
                    const PartCard& a = m_parts[i];
                    m_iconButtons[i] = CreateComponent<Engine::Button>("wc_icon_btn_" + std::to_string(i));
                    if (m_iconButtons[i])
                    {
                        m_iconButtons[i]->SetRect(ix, iy, fIconSz, fIconSz);
                        m_iconButtons[i]->SetTexture(PartTexture(a));
                        const int idx = i;
                        m_iconButtons[i]->SetOnClick([this, idx] { OnIconClick(idx); });
                    }
                    m_iconTexts[i] = makeText("wc_icon_txt_" + std::to_string(i), m_pSmallFont,
                        ix, iy, fIconSz, fIconSz, a.wLabel);
                    reg(m_iconButtons[i], ix, iy, fIconSz, fIconSz);
                    reg(m_iconTexts[i],   ix, iy, fIconSz, fIconSz);
                    ix += fIconSz + fIconGap;
                    ++col;
                }
                cy = (col > 0 ? iy + fIconSz : cy) + fRowGap;
            }
        }

        // Core stat rows — appended at the end of the same scroll list.
        numRow(m_pNumDamage, "wc_num_dmg", L"데미지", 1.f, 999.f, 0, 5.f, fFieldW, cy); cy += fLineH + fRowGap;
        numRow(m_pNumCount,  "wc_num_cnt", L"발사수", 1.f,  50.f, 0, 1.f, fFieldW, cy); cy += fLineH + fRowGap;
        numRow(m_pNumSpeed,  "wc_num_spd", L"속도",   0.f,  60.f, 1, 8.f, fFieldW, cy); cy += fLineH + fRowGap;

        if (m_pInvScroll) m_pInvScroll->RebuildContent();

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
            m_pRegHeader->SetString(L"무기 목록 — 클릭하여 편집");
        }

        const int   kCols     = 2;
        const float fCellGapX = W * 0.010f;
        const float fCellGapY = H * 0.012f;
        // Fixed (readable) cell height — the list overflows the panel and the
        // scroll view pages through it, so we no longer shrink cells to fit.
        const float fRegRowH  = (std::max)(44.f, H * 0.058f);
        m_fRegX0    = fPanelX0;
        m_fRegTopY  = fInvTop;
        m_fRegCellW = (fPanelW - (kCols - 1) * fCellGapX) / kCols;
        m_fRegRowH  = fRegRowH;
        m_fRegCellH = fRegRowH - fCellGapY;
        m_fRegGapX  = fCellGapX;

        // Vertical scroll for the weapon list. Cells are placed in content
        // space + registered by RefreshRegistry; the view clips/scrolls them.
        m_pRegScroll = CreateComponent<Engine::ScrollView>("wc_reg_scroll");
        if (m_pRegScroll)
        {
            m_pRegScroll->SetAxis(Engine::ScrollView::Axis::Vertical);
            m_pRegScroll->SetViewport(fPanelX0, fInvTop, fPanelW, H * 0.95f - fInvTop);
        }

        for (int i = 0; i < kRegCells; ++i)
        {
            // Positions are assigned per-refresh (content space); created here.
            m_pRegButton[i] = CreateComponent<Engine::Button>("wc_reg_btn_" + std::to_string(i));
            if (m_pRegButton[i])
            {
                m_pRegButton[i]->SetTexture(EnsureSolidTexture(kRegUnequipColor));
                const int idx = i;
                m_pRegButton[i]->SetOnClick([this, idx] { OnRegistryClick(idx); });
            }
            m_pRegText[i] = makeText("wc_reg_txt_" + std::to_string(i), m_pSmallFont,
                0.f, 0.f, m_fRegCellW, m_fRegCellH, L"");
        }

        RefreshCraft();
        RefreshRegistry();   // repopulate from any weapons crafted earlier this session
        RefreshImpactSlot(); // set the impact row's initial (all-dimmed) state
        RefreshLevelUpSlot();// same for the multi-select level-up row
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

        // Impact is multi-select: a weapon can stack several modules, so toggle
        // this card's membership instead of replacing a single slot.
        if (c == CAT_IMPACT)
        {
            auto it = std::find(m_impactSel.begin(), m_impactSel.end(), iPaletteIndex);
            if (it != m_impactSel.end()) m_impactSel.erase(it);
            else                         m_impactSel.push_back(iPaletteIndex);
            RefreshImpactSlot();
            RefreshCraft();
            return;
        }

        // Level-up is multi-select too — a weapon can grow several stats/level.
        if (c == CAT_LEVELUP)
        {
            auto it = std::find(m_levelUpSel.begin(), m_levelUpSel.end(), iPaletteIndex);
            if (it != m_levelUpSel.end()) m_levelUpSel.erase(it);
            else                          m_levelUpSel.push_back(iPaletteIndex);
            RefreshLevelUpSlot();
            RefreshCraft();
            return;
        }

        m_iSlot[c] = iPaletteIndex;

        if (m_pSlotButton[c])   m_pSlotButton[c]->SetTexture(PartTexture(a));
        if (m_pSlotNameText[c]) m_pSlotNameText[c]->SetString(a.wLabel);

        RefreshCraft();
    }

    void WeaponCombiner::RefreshCraft()
    {
        using namespace WeaponCombiner_detail;
        bool bReady = true;
        for (int c = 0; c < kCatCount; ++c)
        {
            if (c == CAT_IMPACT)  continue;   // optional — Damage baseline always applies
            if (c == CAT_LEVELUP) continue;   // optional + multi-select (m_levelUpSel)
            if (IsNumericCat(c)) continue;   // number fields always have a value
            if (m_iSlot[c] < 0) { bReady = false; break; }
        }

        m_bCraftReady = bReady;
        if (m_pCraftButton)
            m_pCraftButton->SetTexture(EnsureSolidTexture(bReady ? kCraftOnColor : kCraftOffColor));

        // Live power-score preview: current (level 1) plus the gain per
        // level-up, so the chosen LevelUp part visibly contributes.
        if (m_pScoreText)
        {
            if (bReady)
            {
                const WeaponDef def = AssembleWeaponDef();
                const int iCur   = static_cast<int>(CalcPowerScore(def, 1));
                const int iPerLv = static_cast<int>(CalcPowerScore(def, 2)) - iCur;
                m_pScoreText->SetString(L"Power Score: " + std::to_wstring(iCur)
                    + L" (+" + std::to_wstring(iPerLv) + L"/Lv)");
            }
            else
            {
                m_pScoreText->SetString(L"Power Score: -");
            }
        }
    }

    void WeaponCombiner::RefreshImpactSlot()
    {
        using namespace WeaponCombiner_detail;

        // Per-icon feedback: equipped impact cards show the full hue, the rest
        // are dimmed — so the multi-select state is visible in the row.
        for (int i = 0; i < static_cast<int>(m_parts.size()); ++i)
        {
            if (m_parts[i].iCategory != CAT_IMPACT || !m_iconButtons[i]) continue;
            const bool bOn =
                std::find(m_impactSel.begin(), m_impactSel.end(), i) != m_impactSel.end();
            m_iconButtons[i]->SetTexture(
                EnsureSolidTexture(bOn ? kHueImpact : Dim(kHueImpact, 0.4f)));
        }

        // The impact slot summarises the equipped set: hue when any equipped,
        // empty-slot colour otherwise; the name shows the module count.
        if (m_pSlotButton[CAT_IMPACT])
            m_pSlotButton[CAT_IMPACT]->SetTexture(EnsureSolidTexture(
                m_impactSel.empty() ? kEmptySlotColor : kHueImpact));
        if (m_pSlotNameText[CAT_IMPACT])
            m_pSlotNameText[CAT_IMPACT]->SetString(
                m_impactSel.empty() ? L"-"
                                    : (std::to_wstring(static_cast<int>(m_impactSel.size())) + L" 모듈"));
    }

    void WeaponCombiner::RefreshLevelUpSlot()
    {
        using namespace WeaponCombiner_detail;

        // Each level-up card keeps its own hue; selected = full colour, the rest
        // dimmed, so the multi-select set reads at a glance.
        for (int i = 0; i < static_cast<int>(m_parts.size()); ++i)
        {
            if (m_parts[i].iCategory != CAT_LEVELUP || !m_iconButtons[i]) continue;
            const bool bOn =
                std::find(m_levelUpSel.begin(), m_levelUpSel.end(), i) != m_levelUpSel.end();
            m_iconButtons[i]->SetTexture(
                bOn ? PartTexture(m_parts[i]) : EnsureSolidTexture(Dim(m_parts[i].uColor, 0.4f)));
        }

        // Slot summarises the set: first chosen card's colour + the stat count.
        if (m_pSlotButton[CAT_LEVELUP])
            m_pSlotButton[CAT_LEVELUP]->SetTexture(
                m_levelUpSel.empty() ? EnsureSolidTexture(kEmptySlotColor)
                                     : PartTexture(m_parts[m_levelUpSel.front()]));
        if (m_pSlotNameText[CAT_LEVELUP])
            m_pSlotNameText[CAT_LEVELUP]->SetString(
                m_levelUpSel.empty() ? L"-"
                                     : (std::to_wstring(static_cast<int>(m_levelUpSel.size())) + L" 스탯"));
    }

    WeaponDef WeaponCombiner::AssembleWeaponDef() const
    {
        using namespace WeaponCombiner_detail;
        WeaponDef def;
        const PartCard& move = m_parts[m_iSlot[CAT_MOVE]];
        def.eOrigin   = static_cast<SpawnOrigin>    (m_parts[m_iSlot[CAT_ORIGIN]].iVariant);
        def.eMovement = static_cast<MovementType>   (move.iVariant);
        def.fOrbitRadius = move.fAmount;   // Orbital radius; 0 = centered follow
        def.fRadialSpeed = move.fGrowth;   // >0 = orbit spirals outward (SpiralOut)
        // AimMode rides the movement card, but only the auto-targeting movers
        // (Homing / Aimed) take its enemy-seeking value; every other movement
        // fires Forward (player facing), preserving the legacy combiner aim.
        if (move.iVariant == static_cast<int>(MovementType::Homing) ||
            move.iVariant == static_cast<int>(MovementType::Aimed))
        {
            int iAim = static_cast<int>(move.fAimMode);
            if (iAim < 0 || iAim >= static_cast<int>(AimMode::COUNT)) iAim = 0;
            def.eAimMode = static_cast<AimMode>(iAim);
        }
        else
        {
            def.eAimMode = AimMode::Forward;
        }
        // OnHit part: variant = behaviour, amount = max hits before despawn
        // (0 = unlimited; caps NoChange/Reflect piercing).
        const PartCard& onhit = m_parts[m_iSlot[CAT_ONHIT]];
        def.eOnHit    = static_cast<OnHitEvent>(onhit.iVariant);
        def.iMaxHits  = static_cast<int>(onhit.fAmount);
        // Field zones tick on this cadence; 0 (any non-Field card, or an old
        // parts.csv without the column) leaves the WeaponDef default (0.5s),
        // which Bullet::Configure floors to anyway.
        if (onhit.fDamageInterval > 0.f) def.fDamageInterval = onhit.fDamageInterval;
        def.eShape    = ProjectileShape::Sphere;   // shape is no longer a part — fixed

        // Numeric inputs (typed / slider) instead of preset cards. fSize is the
        // uniform mesh scale; fCd the cooldown seconds; fLife the projectile
        // lifetime; fAccel the speed delta/sec.
        float fSz = m_pNumSize ? m_pNumSize->GetValue() : 0.25f;
        if (fSz <= 0.f) fSz = 0.25f;

        // Fire rate: the Sustained toggle (not a 0-value) picks the mode now.
        //   Sustained -> one persistent instance (Orbital + Sustained = orb),
        //                lifetime is effectively permanent.
        //   Cooldown  -> fires every fCd seconds (floored against per-frame fire).
        const bool bSustained = m_bSustained;
        float fCd = m_pNumCooldown ? m_pNumCooldown->GetValue() : 0.5f;
        if (bSustained)       fCd = 0.5f;    // unused by Sustained, kept safe
        else if (fCd < 0.05f) fCd = 0.05f;   // floor against every-frame fire

        float fLife = bSustained ? 9999.f
                                 : (m_pNumLifetime ? m_pNumLifetime->GetValue() : 2.f);
        if (fLife <= 0.f) fLife = 2.f;

        const float fAccel = m_pNumAccel ? m_pNumAccel->GetValue() : 0.f;

        // Level-up is multi-select: each chosen card bumps one stat per level.
        for (int idx : m_levelUpSel)
        {
            if (idx < 0 || idx >= static_cast<int>(m_parts.size())) continue;
            const PartCard& lvl = m_parts[idx];
            if (lvl.iVariant >= 0 && lvl.iVariant < static_cast<int>(LevelUpField::COUNT_))
                def.fLevelUpAmt[lvl.iVariant] = lvl.fAmount;
        }

        // Name = first 2 chars of each part's label, in slot order, so the
        // name encodes the build (e.g. "FrStCoVa10Da"). Colour = a hash of
        // the full part labels, so different combos get different hues and
        // the weapon is easy to tell apart by icon. (Labels are ASCII.)
        std::string strName, strKey;
        auto foldLabel = [&](const std::wstring& wLbl)
        {
            for (size_t k = 0; k < wLbl.size() && k < 2; ++k)
                strName += static_cast<char>(wLbl[k]);
            for (wchar_t wc : wLbl) strKey += static_cast<char>(wc);
        };
        for (int c = 0; c < kCatCount; ++c)
        {
            // CAT_IMPACT is multi-select (its m_iSlot entry stays -1 — folded in
            // from m_impactSel below); guard any unfilled slot so we never index
            // m_parts[-1] (this crashed the combiner).
            if (c == CAT_IMPACT || m_iSlot[c] < 0) continue;
            foldLabel(m_parts[m_iSlot[c]].wLabel);
        }
        // Fold equipped impact modules so different impact loadouts read as
        // different weapons (name + colour).
        for (int idx : m_impactSel)
            if (idx >= 0 && idx < static_cast<int>(m_parts.size()))
                foldLabel(m_parts[idx].wLabel);
        // Fold the numeric inputs into the colour key so two builds differing
        // only in a typed number still read as distinct weapons by colour.
        {
            char num[64];
            std::snprintf(num, sizeof(num), "|%.1f|%.2f|%.2f|%.1f|%d",
                          fLife, fCd, fSz, fAccel, bSustained ? 1 : 0);
            strKey += num;
        }
        def.strName   = strName;
        def.uColorRGB = ComboColor(strKey);

        // Fire mode comes from the Sustained toggle (fCd / fLife were computed
        // from the number fields above).
        def.eFireMode = bSustained ? FireMode::Sustained : FireMode::Cooldown;

        // Core stats now come from the editor's number fields (defaults match
        // the old hard-coded values when a field is somehow absent).
        def.iDamage          = m_pNumDamage ? (std::max)(1, static_cast<int>(m_pNumDamage->GetValue() + 0.5f)) : 5;
        def.fCooldown        = fCd;
        def.fProjectileSpeed = m_pNumSpeed ? m_pNumSpeed->GetValue() : 8.f;
        def.fLifetime        = fLife;
        def.iCount           = m_pNumCount ? (std::max)(1, static_cast<int>(m_pNumCount->GetValue() + 0.5f)) : 1;
        def.fSize            = fSz;
        // Acceleration: speed delta per second (Bullet::Update applies
        // speed += fAcceleration * dt). 0 = constant speed.
        def.fAcceleration    = fAccel;

        // Impact modules (multi-select). Damage is the always-on baseline; OR
        // in each equipped module's bit (variant = bit index) and copy its
        // tuning from the card (amount = strength, or Gather pull fraction 0..1;
        // growth = Gather radius; duration = Burn/Slow seconds).
        unsigned int uMask = Impact_Damage;
        float fKnock    = def.fKnockback;     // struct defaults if no card sets them
        float fPull     = def.fGatherPull;
        float fRadius   = def.fGatherRadius;
        int   iBurnDmg  = def.iBurnDamage;
        float fBurnDur  = def.fBurnDuration;
        float fSlowFac  = def.fSlowFactor;
        float fSlowDur  = def.fSlowDuration;
        for (int idx : m_impactSel)
        {
            if (idx < 0 || idx >= static_cast<int>(m_parts.size())) continue;
            const PartCard& imp = m_parts[idx];
            uMask |= (1u << imp.iVariant);
            if      (imp.iVariant == 1) fKnock = imp.fAmount;                          // Knockback
            else if (imp.iVariant == 2) { fPull = imp.fAmount; fRadius = imp.fGrowth; } // Gather
            else if (imp.iVariant == 3) { iBurnDmg = static_cast<int>(imp.fAmount); fBurnDur = imp.fDuration; } // Burn
            else if (imp.iVariant == 4) { fSlowFac = imp.fAmount; fSlowDur = imp.fDuration; } // Slow
        }
        if (fRadius  <= 0.f) fRadius  = 4.f;   // guard a 0-radius Gather no-op
        if (fBurnDur <= 0.f) fBurnDur = 3.f;   // guard a 0-duration Burn no-op
        if (fSlowFac <= 0.f || fSlowFac > 1.f) fSlowFac = 0.5f;   // valid speed multiplier
        if (fSlowDur <= 0.f) fSlowDur = 2.5f;  // guard a 0-duration Slow no-op
        def.uImpactMask   = uMask;
        def.fKnockback    = fKnock;
        def.fGatherPull   = fPull;
        def.fGatherRadius = fRadius;
        def.iBurnDamage   = iBurnDmg;
        def.fBurnDuration = fBurnDur;
        def.fSlowFactor   = fSlowFac;
        def.fSlowDuration = fSlowDur;
        return def;
    }

    void WeaponCombiner::OnSave()
    {
        using namespace WeaponCombiner_detail;
        if (!m_bCraftReady) return;   // every card slot must be filled

        auto& db = WeaponDatabase::GetInst();
        const WeaponDef* pBase = db.Get(m_iEditId);
        if (m_iEditId < 0 || !pBase)
        {
            if (m_pResultText)
                m_pResultText->SetString(L"먼저 오른쪽 목록에서 편집할 무기를 선택하세요");
            return;
        }

        // Graft the editor's fields onto the original so we preserve what the
        // editor doesn't control (id / name / colour / price / evolution /
        // shop_available / trail / shape).
        const WeaponDef edited = AssembleWeaponDef();
        WeaponDef out = *pBase;
        out.eOrigin          = edited.eOrigin;
        out.eMovement        = edited.eMovement;
        out.fOrbitRadius     = edited.fOrbitRadius;
        out.fRadialSpeed     = edited.fRadialSpeed;
        out.eAimMode         = edited.eAimMode;
        out.eOnHit           = edited.eOnHit;
        out.iMaxHits         = edited.iMaxHits;
        out.fDamageInterval  = edited.fDamageInterval;
        out.eFireMode        = edited.eFireMode;
        out.fCooldown        = edited.fCooldown;
        out.fLifetime        = edited.fLifetime;
        out.fSize            = edited.fSize;
        out.fAcceleration    = edited.fAcceleration;
        for (int k = 0; k < static_cast<int>(LevelUpField::COUNT_); ++k)
            out.fLevelUpAmt[k] = edited.fLevelUpAmt[k];
        out.uImpactMask      = edited.uImpactMask;
        out.fKnockback       = edited.fKnockback;
        out.fGatherPull      = edited.fGatherPull;
        out.fGatherRadius    = edited.fGatherRadius;
        out.iBurnDamage      = edited.iBurnDamage;
        out.fBurnDuration    = edited.fBurnDuration;
        out.fSlowFactor      = edited.fSlowFactor;
        out.fSlowDuration    = edited.fSlowDuration;
        out.iDamage          = edited.iDamage;
        out.iCount           = edited.iCount;
        out.fProjectileSpeed = edited.fProjectileSpeed;
        // Editable name (ASCII) — keep the original if the field was cleared.
        if (m_pNameBox)
        {
            const std::wstring w = m_pNameBox->GetText();
            if (!w.empty()) out.strName = std::string(w.begin(), w.end());
        }
        // Mirror the loader's Sustained-lifetime convention so the value
        // round-trips (a Sustained non-Straight weapon stores a huge lifetime).
        if (out.eFireMode == FireMode::Sustained && out.eMovement != MovementType::Straight)
            out.fLifetime = 9999.f;

        db.UpdateWeapon(m_iEditId, out);
        db.SaveToCSV("/Game/Data/Weapons/weapons_v2.csv");
        RefreshRegistry();

        if (m_pResultText)
        {
            std::wstring w(out.strName.begin(), out.strName.end());
            m_pResultText->SetString(L"저장됨: " + w);
        }
    }

    void WeaponCombiner::OnSustainToggle()
    {
        using namespace WeaponCombiner_detail;
        m_bSustained = !m_bSustained;

        // Green when Sustained, empty-slot grey when off.
        if (m_pSustainBtn)
            m_pSustainBtn->SetTexture(EnsureSolidTexture(m_bSustained ? kCraftOnColor : kEmptySlotColor));

        // Sustained ignores the cooldown and runs forever, so grey both fields.
        if (m_pNumCooldown) m_pNumCooldown->SetEnabled(!m_bSustained);
        if (m_pNumLifetime) m_pNumLifetime->SetEnabled(!m_bSustained);

        RefreshCraft();
    }

    void WeaponCombiner::OnRegistryClick(int iCell)
    {
        // The list shows the weapons_v2 catalogue in order; a cell maps 1:1 to
        // db.All()[iCell]. Clicking loads that weapon into the editor.
        auto& db = WeaponDatabase::GetInst();
        const auto& all = db.All();
        if (iCell < 0 || iCell >= static_cast<int>(all.size())) return;   // empty cell
        LoadWeaponIntoEditor(all[iCell].iId);
    }

    void WeaponCombiner::LoadWeaponIntoEditor(int iWeaponId)
    {
        using namespace WeaponCombiner_detail;
        const WeaponDef* p = WeaponDatabase::GetInst().Get(iWeaponId);
        if (!p) return;
        const WeaponDef& d = *p;
        m_iEditId = iWeaponId;
        if (m_pNameBox) m_pNameBox->SetText(std::wstring(d.strName.begin(), d.strName.end()));

        auto approx = [](float a, float b) { float e = a - b; return (e < 0 ? -e : e) < 0.05f; };

        // Best palette index for a single-card category: must match the
        // category enum (iVariant); a secondary score breaks ties between cards
        // that share a variant (e.g. Orbital vs Follow vs SpiralOut).
        auto pickCard = [&](int cat, int variant, auto score) -> int
        {
            int best = -1, bestScore = -1;
            for (int k = 0; k < static_cast<int>(m_parts.size()); ++k)
            {
                if (m_parts[k].iCategory != cat || m_parts[k].iVariant != variant) continue;
                const int s = score(m_parts[k]);
                if (s > bestScore) { bestScore = s; best = k; }
            }
            return best;
        };
        auto setSlot = [&](int cat, int palette)
        {
            m_iSlot[cat] = palette;
            if (palette >= 0)
            {
                if (m_pSlotButton[cat])   m_pSlotButton[cat]->SetTexture(PartTexture(m_parts[palette]));
                if (m_pSlotNameText[cat]) m_pSlotNameText[cat]->SetString(m_parts[palette].wLabel);
            }
            else
            {
                if (m_pSlotButton[cat])   m_pSlotButton[cat]->SetTexture(EnsureSolidTexture(kEmptySlotColor));
                if (m_pSlotNameText[cat]) m_pSlotNameText[cat]->SetString(L"-");
            }
        };

        setSlot(CAT_ORIGIN, pickCard(CAT_ORIGIN, static_cast<int>(d.eOrigin),
            [](const PartCard&) { return 0; }));

        const int mv = static_cast<int>(d.eMovement);
        setSlot(CAT_MOVE, pickCard(CAT_MOVE, mv, [&](const PartCard& c)
        {
            int s = 0;
            if (approx(c.fAmount, d.fOrbitRadius)) ++s;
            if (approx(c.fGrowth, d.fRadialSpeed)) ++s;
            if ((mv == static_cast<int>(MovementType::Homing) ||
                 mv == static_cast<int>(MovementType::Aimed)) &&
                approx(c.fAimMode, static_cast<float>(static_cast<int>(d.eAimMode)))) ++s;
            return s;
        }));

        setSlot(CAT_ONHIT, pickCard(CAT_ONHIT, static_cast<int>(d.eOnHit),
            [&](const PartCard& c) { return approx(c.fAmount, static_cast<float>(d.iMaxHits)) ? 1 : 0; }));

        // Level-up: multi-select — pick a card for each stat with a non-zero
        // per-level amount, preferring the card whose amount matches.
        m_levelUpSel.clear();
        for (int f = 0; f < static_cast<int>(LevelUpField::COUNT_); ++f)
        {
            const float amt = d.fLevelUpAmt[f];
            if (amt == 0.f) continue;
            const int pick = pickCard(CAT_LEVELUP, f,
                [&](const PartCard& c) { return approx(c.fAmount, amt) ? 1 : 0; });
            if (pick >= 0) m_levelUpSel.push_back(pick);
        }

        // Impact modules: select the first card for each bit set in the mask.
        m_impactSel.clear();
        for (int k = 0; k < static_cast<int>(m_parts.size()); ++k)
        {
            if (m_parts[k].iCategory != CAT_IMPACT) continue;
            const unsigned int bit = 1u << m_parts[k].iVariant;
            if (!(d.uImpactMask & bit)) continue;
            bool dup = false;
            for (int sel : m_impactSel)
                if ((1u << m_parts[sel].iVariant) == bit) { dup = true; break; }
            if (!dup) m_impactSel.push_back(k);
        }

        // Number fields. Sustained non-Straight stores 9999 lifetime — show a
        // sane editable value instead (save re-applies the convention).
        if (m_pNumLifetime) m_pNumLifetime->SetValue(d.fLifetime >= 9999.f ? 2.f : d.fLifetime);
        if (m_pNumCooldown) m_pNumCooldown->SetValue(d.fCooldown);
        if (m_pNumSize)     m_pNumSize->SetValue(d.fSize);
        if (m_pNumAccel)    m_pNumAccel->SetValue(d.fAcceleration);
        if (m_pNumDamage)   m_pNumDamage->SetValue(static_cast<float>(d.iDamage));
        if (m_pNumCount)    m_pNumCount->SetValue(static_cast<float>(d.iCount));
        if (m_pNumSpeed)    m_pNumSpeed->SetValue(d.fProjectileSpeed);

        // Sustained toggle + cooldown/lifetime greying (mirror OnSustainToggle).
        m_bSustained = (d.eFireMode == FireMode::Sustained);
        if (m_pSustainBtn)  m_pSustainBtn->SetTexture(EnsureSolidTexture(m_bSustained ? kCraftOnColor : kEmptySlotColor));
        if (m_pNumCooldown) m_pNumCooldown->SetEnabled(!m_bSustained);
        if (m_pNumLifetime) m_pNumLifetime->SetEnabled(!m_bSustained);

        RefreshImpactSlot();
        RefreshLevelUpSlot();
        RefreshCraft();
        RefreshRegistry();
        if (m_pResultText)
        {
            std::wstring w(d.strName.begin(), d.strName.end());
            m_pResultText->SetString(L"편집 중: " + w);
        }
    }

    void WeaponCombiner::RefreshRegistry()
    {
        using namespace WeaponCombiner_detail;
        auto& db = WeaponDatabase::GetInst();
        const auto& all = db.All();
        const int n = static_cast<int>(all.size());
        const int kCols = 2;

        if (m_pRegScroll) m_pRegScroll->ClearItems();

        for (int i = 0; i < kRegCells; ++i)
        {
            if (i >= n)
            {
                if (m_pRegButton[i]) m_pRegButton[i]->Disable();
                if (m_pRegText[i])   m_pRegText[i]->Disable();
                continue;
            }

            // Content-space cell rect (the scroll view re-places + clips it).
            const int   col = i % kCols;
            const int   row = i / kCols;
            const float x = m_fRegX0 + col * (m_fRegCellW + m_fRegGapX);
            const float y = m_fRegTopY + row * m_fRegRowH;

            const WeaponDef&   cd      = all[i];
            const unsigned int uCol    = cd.uColorRGB;
            const bool         bEdit   = (cd.iId == m_iEditId);

            if (m_pRegButton[i])
            {
                m_pRegButton[i]->SetRect(x, y, m_fRegCellW, m_fRegCellH);
                m_pRegButton[i]->SetTexture(EnsureSolidTexture(bEdit ? uCol : Dim(uCol, 0.45f)));
                m_pRegButton[i]->Enable();
            }
            if (m_pRegText[i])
            {
                m_pRegText[i]->SetRect(x, y, m_fRegCellW, m_fRegCellH);
                std::wstring w(cd.strName.begin(), cd.strName.end());
                w += L" (" + std::to_wstring(static_cast<int>(CalcPowerScore(cd))) + L")";
                if (bEdit) w += L" ✎";
                m_pRegText[i]->SetString(w);
                m_pRegText[i]->Enable();
            }
            if (m_pRegScroll)
            {
                if (m_pRegButton[i]) m_pRegScroll->AddItem(m_pRegButton[i], x, y, m_fRegCellW, m_fRegCellH);
                if (m_pRegText[i])   m_pRegScroll->AddItem(m_pRegText[i],   x, y, m_fRegCellW, m_fRegCellH);
            }
        }

        if (m_pRegScroll) m_pRegScroll->RebuildContent();

        if (m_pRegHeader)
            m_pRegHeader->SetString(L"무기 목록 ("
                + std::to_wstring(n) + L") — 클릭하여 편집");
    }

    void WeaponCombiner::Update(float fDeltaTime)
    {
        // Advance our clock first so the double-click test inside the
        // child buttons' OnClick (fired during the base Update's child
        // recursion) reads the current frame time.
        m_fTime += fDeltaTime;
        Engine::UIControl::Update(fDeltaTime);
        // Inventory rows scroll themselves — each row's Engine::ScrollView
        // (a child of this control) handles wheel input and re-places its
        // icons in its own Update.
    }

    std::shared_ptr<Engine::Component> WeaponCombiner::Clone()
    {
        return std::make_shared<WeaponCombiner>(*this);
    }
}
