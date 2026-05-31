#include "WeaponDatabase.h"
#include "../Util/CSVLoader.h"
#include "EnemyData.h"   // kPxPerCell (px -> world-unit scale)
#include "Core/PathManager.h"
#include "Core/Macro.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>

namespace Client
{
    namespace WeaponDatabase_detail
    {
        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        SpawnOrigin ParseOrigin(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "around") return SpawnOrigin::Around;
            if (v == "mouse")  return SpawnOrigin::Mouse;
            if (v == "random") return SpawnOrigin::Random;
            return SpawnOrigin::Front;
        }
        MovementType ParseMovement(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "spiral")  return MovementType::Spiral;
            if (v == "fixed")   return MovementType::Fixed;
            if (v == "orbital") return MovementType::Orbital;
            if (v == "homing")  return MovementType::Homing;
            if (v == "aimed")   return MovementType::Aimed;
            if (v == "follow")  return MovementType::Follow;
            return MovementType::Straight;
        }
        FireMode ParseFireMode(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "sustained") return FireMode::Sustained;
            return FireMode::Cooldown;
        }
        OnHitEvent ParseOnHit(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "nochange") return OnHitEvent::NoChange;
            if (v == "reflect")  return OnHitEvent::Reflect;
            if (v == "multiply") return OnHitEvent::Multiply;
            if (v == "field")    return OnHitEvent::Field;
            if (v == "chain")    return OnHitEvent::Chain;
            return OnHitEvent::Vanish;
        }
        ProjectileShape ParseShape(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "box")      return ProjectileShape::Box;
            if (v == "triangle") return ProjectileShape::Triangle;
            return ProjectileShape::Sphere;
        }
        LevelUpField ParseLevelUpField(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "cooldown") return LevelUpField::Cooldown;
            if (v == "count")    return LevelUpField::Count;
            if (v == "speed")    return LevelUpField::Speed;
            if (v == "size")     return LevelUpField::Size;
            return LevelUpField::Damage;
        }
        AimMode ParseAimMode(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "lowesthp") return AimMode::LowestHP;
            if (v == "random")   return AimMode::Random;
            if (v == "forward")  return AimMode::Forward;
            if (v == "cursor")   return AimMode::Cursor;
            if (v == "radial")   return AimMode::Radial;
            return AimMode::Nearest;
        }
        TrailStyle ParseTrailStyle(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "none")   return TrailStyle::None;
            if (v == "plasma") return TrailStyle::Plasma;
            if (v == "spark")  return TrailStyle::Spark;
            if (v == "comet")  return TrailStyle::Comet;
            return TrailStyle::Tracer;   // default / "tracer" / unknown
        }

        // strtol/strtof tolerate trailing garbage (commas already split
        // away). Empty cells default-to-zero which is fine for optional
        // numeric columns.
        int ToInt(const std::string& s) { return s.empty() ? 0 : std::atoi(s.c_str()); }
        float ToFloat(const std::string& s) { return s.empty() ? 0.f : static_cast<float>(std::atof(s.c_str())); }

        // Accepts both "0xRRGGBB" and decimal "16777215". Hex is the common
        // case so it's worth handling natively (atoi would only see 0).
        unsigned int ToColor(const std::string& s)
        {
            if (s.empty()) return 0xFFFFFF;
            return static_cast<unsigned int>(std::strtoul(s.c_str(), nullptr, 0));
        }
    }

    size_t WeaponDatabase::LoadFromCSV(const std::string& strPath)
    {
        m_vecWeapons.clear();
        m_mapIdToIndex.clear();

        auto rows = CSVLoader::Load(strPath);
        m_vecWeapons.reserve(rows.size());

        // Column order. Cols 0-16 match the legacy weapons.csv; the v2 schema
        // (weapons_v2.csv) appends 17-25. Optional columns are size-guarded so
        // a legacy 17-col file still loads.
        //   0  id                9  projectile_speed   18  spread_deg
        //   1  name             10  lifetime           19  max_hits
        //   2  spawn_origin     11  count              20  damage_interval
        //   3  movement         12  color_rgb          21  knockback
        //   4  fire_mode        13  level_up_field     22  evolves_into
        //   5  on_hit           14  level_up_amount    23  evolve_min_level
        //   6  shape            15  size               24  shop_available (0/1)
        //   7  damage           16  acceleration       25  trail_style
        //   8  cooldown         17  aim_mode
        // Unknown enum values fall back via the Parse* defaults: movement
        // Follow -> Straight, on_hit Chain -> Vanish (until Phases 4 / 3 land).
        for (const auto& row : rows)
        {
            if (row.size() < 15) continue;   // malformed row — skip silently

            using namespace WeaponDatabase_detail;
            WeaponDef def;
            def.iId             = ToInt   (row[0]);
            def.strName         = row[1];
            def.eOrigin         = ParseOrigin  (row[2]);
            def.eMovement       = ParseMovement(row[3]);
            def.eFireMode       = ParseFireMode(row[4]);
            def.eOnHit          = ParseOnHit   (row[5]);
            def.eShape          = ParseShape   (row[6]);
            def.iDamage         = ToInt   (row[7]);
            def.fCooldown       = ToFloat (row[8]);
            // Speed (and acceleration below) are authored in px/sec like
            // enemies.json; convert to world units/sec via kPxPerCell.
            def.fProjectileSpeed= ToFloat (row[9]) / kPxPerCell;
            def.fLifetime       = ToFloat (row[10]);
            def.iCount          = ToInt   (row[11]);
            def.uColorRGB       = ToColor (row[12]);
            def.eLevelUpField   = ParseLevelUpField(row[13]);
            def.fLevelUpAmount  = ToFloat (row[14]);
            // Back-compat — shorter old rows keep WeaponDef defaults
            // (size 0.25 = legacy hard-coded scale, acceleration 0 =
            // constant speed).
            if (row.size() > 15) def.fSize          = ToFloat(row[15]);
            if (row.size() > 16) def.fAcceleration  = ToFloat(row[16]) / kPxPerCell;
            // v2 appended columns (17+), all size-guarded so a legacy 17-col
            // weapons.csv still loads (those keep the WeaponDef defaults).
            if (row.size() > 17) def.eAimMode        = ParseAimMode(row[17]);
            if (row.size() > 18) def.fSpreadDeg      = ToFloat(row[18]);
            if (row.size() > 19) def.iMaxHits        = ToInt   (row[19]);
            if (row.size() > 20) def.fDamageInterval = ToFloat(row[20]);
            // knockback: + shoves the struck enemy away, - pulls it toward the
            // impact point (gravity). Drives the Knockback impact effect, which
            // already uses the sign for direction; only enabled when non-zero.
            if (row.size() > 21)
            {
                def.fKnockback = ToFloat(row[21]);
                if (def.fKnockback != 0.f) def.uImpactMask |= Impact_Knockback;
            }
            // Evolution + shop pool (Phase 6). shop_available defaults true so a
            // legacy row without the column still appears in the shop.
            if (row.size() > 22) def.iEvolvesInto    = ToInt(row[22]);
            if (row.size() > 23) def.iEvolveMinLevel = ToInt(row[23]);
            if (row.size() > 24) def.bShopAvailable  = ToInt(row[24]) != 0;
            // Tracer-trail preset (col 25). Size-guarded: a row without it
            // keeps WeaponDef's Tracer default (the standard streak).
            if (row.size() > 25) def.eTrailStyle     = ParseTrailStyle(row[25]);

            // Sustained weapons persist (their lifetime column is 0 by
            // convention) -- mirror the combiner and hand them a huge lifetime
            // so Bullet's despawn check (m_fLifeAcc >= m_fLifetime) never trips
            // and they don't vanish on frame 1. A 0/blank lifetime on a
            // Cooldown weapon falls back to the 2s default.
            // Exception: a Sustained+Straight weapon is a laser Beam (driven
            // by the Player, never a despawning Bullet), so it keeps its
            // authored lifetime -- Beam reads it as the duty-cycle "on" time.
            if (def.eFireMode == FireMode::Sustained)
            {
                if (def.eMovement != MovementType::Straight) def.fLifetime = 9999.f;
            }
            else if (def.fLifetime <= 0.f)                   def.fLifetime = 2.f;

            m_mapIdToIndex[def.iId] = m_vecWeapons.size();
            m_vecWeapons.push_back(def);
        }

        // Re-apply session-crafted weapons so they survive this reload
        // (GameScene calls LoadFromCSV on entry, which clears the vectors
        // above). Fresh ids avoid colliding with the CSV rows just loaded;
        // the equip flags are preserved and each entry's live id is
        // refreshed so EquippedLiveIds keeps pointing at the right rows.
        for (auto& crafted : m_vecCrafted)
        {
            WeaponDef def = crafted.def;
            def.iId = NextId();
            m_mapIdToIndex[def.iId] = m_vecWeapons.size();
            m_vecWeapons.push_back(def);
            crafted.iLiveId = def.iId;
        }

        return m_vecWeapons.size();
    }

    const WeaponDef* WeaponDatabase::Get(int iId) const
    {
        auto it = m_mapIdToIndex.find(iId);
        if (it == m_mapIdToIndex.end()) return nullptr;
        return &m_vecWeapons[it->second];
    }

    int WeaponDatabase::NextId() const
    {
        int iMax = 0;
        for (const auto& def : m_vecWeapons)
            iMax = (std::max)(iMax, def.iId);
        return iMax + 1;
    }

    int WeaponDatabase::Add(const WeaponDef& defIn)
    {
        WeaponDef def = defIn;
        def.iId = NextId();
        m_mapIdToIndex[def.iId] = m_vecWeapons.size();
        m_vecWeapons.push_back(def);

        CraftedEntry entry;
        entry.def       = def;
        entry.bEquipped = false;   // equipped manually in the combo scene
        entry.iLiveId   = def.iId;
        m_vecCrafted.push_back(entry);
        return def.iId;
    }

    const WeaponDef& WeaponDatabase::CraftedDef(int iIndex) const
    {
        return m_vecCrafted[iIndex].def;
    }

    bool WeaponDatabase::IsEquipped(int iIndex) const
    {
        if (iIndex < 0 || iIndex >= static_cast<int>(m_vecCrafted.size())) return false;
        return m_vecCrafted[iIndex].bEquipped;
    }

    int WeaponDatabase::EquippedCount() const
    {
        int n = 0;
        for (const auto& e : m_vecCrafted)
            if (e.bEquipped) ++n;
        return n;
    }

    bool WeaponDatabase::ToggleEquip(int iIndex)
    {
        if (iIndex < 0 || iIndex >= static_cast<int>(m_vecCrafted.size())) return false;
        CraftedEntry& e = m_vecCrafted[iIndex];
        if (!e.bEquipped)
        {
            if (EquippedCount() >= kMaxEquipped) return false;   // cap — stays off
            e.bEquipped = true;
        }
        else
        {
            e.bEquipped = false;
        }
        return e.bEquipped;
    }

    void WeaponDatabase::RemoveCrafted(int iIndex)
    {
        if (iIndex < 0 || iIndex >= static_cast<int>(m_vecCrafted.size())) return;
        m_vecCrafted.erase(m_vecCrafted.begin() + iIndex);
        // The matching m_vecWeapons / m_mapIdToIndex entry stays until the
        // next LoadFromCSV rebuild, but nothing references it once it's gone
        // from m_vecCrafted (EquippedLiveIds and the combo scene both read
        // the registry), so it's harmless to leave.
    }

    std::vector<int> WeaponDatabase::EquippedLiveIds() const
    {
        std::vector<int> out;
        for (const auto& e : m_vecCrafted)
            if (e.bEquipped && e.iLiveId >= 0)
                out.push_back(e.iLiveId);
        return out;
    }

    std::vector<int> WeaponDatabase::AllCraftedLiveIds() const
    {
        std::vector<int> out;
        for (const auto& e : m_vecCrafted)
            if (e.iLiveId >= 0)
                out.push_back(e.iLiveId);
        return out;
    }

    std::vector<int> WeaponDatabase::ShopWeaponIds() const
    {
        // The shop buy pool: every loaded weapon flagged shop_available.
        // Evolution-only forms (shop_available=0) are excluded so they only
        // arrive by evolving. Re-applied crafted weapons default to
        // shop-available, so they appear here alongside the v2 catalogue.
        std::vector<int> out;
        for (const auto& def : m_vecWeapons)
            if (def.iId >= 0 && def.bShopAvailable)
                out.push_back(def.iId);
        return out;
    }

    size_t WeaponDatabase::LoadCrafted(const std::string& strPath)
    {
        // Once per process — repeated scene-init calls must not wipe
        // weapons crafted earlier this session.
        if (m_bCraftedLoaded) return m_vecCrafted.size();
        m_bCraftedLoaded = true;

        using namespace WeaponDatabase_detail;
        // Columns written by SaveCrafted (enums stored as their int value):
        //   0 name              9  lifetime
        //   1 origin            10 count
        //   2 movement          11 color
        //   3 fire_mode         12 level_up_field
        //   4 on_hit            13 level_up_amount
        //   5 shape             14 size
        //   6 damage            15 acceleration
        //   7 cooldown          16 equipped (0/1)
        //   8 projectile_speed
        for (const auto& row : CSVLoader::Load(strPath))
        {
            if (row.size() < 17) continue;

            CraftedEntry entry;
            WeaponDef& def = entry.def;
            def.strName          = row[0];
            def.eOrigin          = static_cast<SpawnOrigin>    (ToInt(row[1]));
            def.eMovement        = static_cast<MovementType>   (ToInt(row[2]));
            def.eFireMode        = static_cast<FireMode>       (ToInt(row[3]));
            def.eOnHit           = static_cast<OnHitEvent>     (ToInt(row[4]));
            def.eShape           = static_cast<ProjectileShape>(ToInt(row[5]));
            def.iDamage          = ToInt   (row[6]);
            def.fCooldown        = ToFloat (row[7]);
            def.fProjectileSpeed = ToFloat (row[8]);
            def.fLifetime        = ToFloat (row[9]);
            def.iCount           = ToInt   (row[10]);
            def.uColorRGB        = ToColor (row[11]);
            def.eLevelUpField    = static_cast<LevelUpField>   (ToInt(row[12]));
            def.fLevelUpAmount   = ToFloat (row[13]);
            def.fSize            = ToFloat (row[14]);
            def.fAcceleration    = ToFloat (row[15]);
            entry.bEquipped      = ToInt(row[16]) != 0;
            // Appended columns are optional so older saves still load:
            //   17 orbit_radius (default 0.9),  18 max_hits (default 0),
            //   19 radial_speed (default 0 = fixed-radius orbit).
            def.fOrbitRadius     = (row.size() > 17) ? ToFloat(row[17]) : 0.9f;
            def.iMaxHits         = (row.size() > 18) ? ToInt  (row[18]) : 0;
            def.fRadialSpeed     = (row.size() > 19) ? ToFloat(row[19]) : 0.f;
            // Impact modules — appended later, so older saves default to a
            // Damage-only weapon:
            //   20 impact_mask (default Impact_Damage), 21 knockback,
            //   22 gather_pull, 23 gather_radius, 24 burn_damage, 25 burn_duration,
            //   26 slow_factor, 27 slow_duration.
            def.uImpactMask      = (row.size() > 20) ? static_cast<unsigned int>(ToInt(row[20])) : Impact_Damage;
            def.fKnockback       = (row.size() > 21) ? ToFloat(row[21]) : 6.f;
            def.fGatherPull      = (row.size() > 22) ? ToFloat(row[22]) : 1.f;
            def.fGatherRadius    = (row.size() > 23) ? ToFloat(row[23]) : 4.f;
            def.iBurnDamage      = (row.size() > 24) ? ToInt  (row[24]) : 3;
            def.fBurnDuration    = (row.size() > 25) ? ToFloat(row[25]) : 3.f;
            def.fSlowFactor      = (row.size() > 26) ? ToFloat(row[26]) : 0.5f;
            def.fSlowDuration    = (row.size() > 27) ? ToFloat(row[27]) : 2.5f;
            //   28 damage_interval (DoT tick seconds; default 0 = single hit.
            //      Field weapons fall back to 0.5 at runtime via TicksDamage).
            def.fDamageInterval  = (row.size() > 28) ? ToFloat(row[28]) : 0.f;
            //   29 aim_mode (heading / target mode; default Forward = legacy facing).
            def.eAimMode         = (row.size() > 29) ? static_cast<AimMode>(ToInt(row[29])) : AimMode::Forward;
            entry.iLiveId        = -1;   // assigned when LoadFromCSV re-applies
            m_vecCrafted.push_back(std::move(entry));
        }
        return m_vecCrafted.size();
    }

    void WeaponDatabase::SaveCrafted(const std::string& strPath) const
    {
        char szResolved[MAX_PATH] = {};
        Engine::CPathManager::GetInst()->ResolveMB(strPath.c_str(), ROOT_PATH, szResolved);

        std::ofstream file(szResolved, std::ios::trunc);
        if (!file.is_open()) return;

        file << "# crafted weapon cards — saved automatically on each craft / equip change.\n";
        file << "name,origin,movement,fire_mode,on_hit,shape,damage,cooldown,"
                "projectile_speed,lifetime,count,color,level_up_field,level_up_amount,"
                "size,acceleration,equipped,orbit_radius,max_hits,radial_speed,"
                "impact_mask,knockback,gather_pull,gather_radius,"
                "burn_damage,burn_duration,slow_factor,slow_duration,"
                "damage_interval,aim_mode\n";
        for (const auto& entry : m_vecCrafted)
        {
            const WeaponDef& d = entry.def;
            file << d.strName << ','
                 << static_cast<int>(d.eOrigin)    << ','
                 << static_cast<int>(d.eMovement)  << ','
                 << static_cast<int>(d.eFireMode)  << ','
                 << static_cast<int>(d.eOnHit)     << ','
                 << static_cast<int>(d.eShape)     << ','
                 << d.iDamage          << ','
                 << d.fCooldown        << ','
                 << d.fProjectileSpeed << ','
                 << d.fLifetime        << ','
                 << d.iCount           << ','
                 << d.uColorRGB        << ','
                 << static_cast<int>(d.eLevelUpField) << ','
                 << d.fLevelUpAmount   << ','
                 << d.fSize            << ','
                 << d.fAcceleration    << ','
                 << (entry.bEquipped ? 1 : 0) << ','
                 << d.fOrbitRadius     << ','
                 << d.iMaxHits         << ','
                 << d.fRadialSpeed     << ','
                 << d.uImpactMask      << ','
                 << d.fKnockback       << ','
                 << d.fGatherPull      << ','
                 << d.fGatherRadius    << ','
                 << d.iBurnDamage      << ','
                 << d.fBurnDuration    << ','
                 << d.fSlowFactor      << ','
                 << d.fSlowDuration    << ','
                 << d.fDamageInterval  << ','
                 << static_cast<int>(d.eAimMode) << '\n';
        }
    }
}
