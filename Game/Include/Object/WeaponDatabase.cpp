#include "WeaponDatabase.h"
#include "../Util/CSVLoader.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

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

        // Expected column order (17 fields):
        //   0  id
        //   1  name
        //   2  spawn_origin
        //   3  movement
        //   4  fire_mode
        //   5  on_hit
        //   6  shape
        //   7  damage
        //   8  cooldown
        //   9  projectile_speed
        //  10  lifetime
        //  11  count
        //  12  color_rgb
        //  13  level_up_field
        //  14  level_up_amount
        //  15  size            (visual scale; collider radius derives)
        //  16  acceleration    (speed delta / sec; 0 = constant speed)
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
            def.fProjectileSpeed= ToFloat (row[9]);
            def.fLifetime       = ToFloat (row[10]);
            def.iCount          = ToInt   (row[11]);
            def.uColorRGB       = ToColor (row[12]);
            def.eLevelUpField   = ParseLevelUpField(row[13]);
            def.fLevelUpAmount  = ToFloat (row[14]);
            // Back-compat — shorter old rows keep WeaponDef defaults
            // (size 0.25 = legacy hard-coded scale, acceleration 0 =
            // constant speed).
            if (row.size() > 15) def.fSize         = ToFloat(row[15]);
            if (row.size() > 16) def.fAcceleration = ToFloat(row[16]);

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

    std::vector<int> WeaponDatabase::EquippedLiveIds() const
    {
        std::vector<int> out;
        for (const auto& e : m_vecCrafted)
            if (e.bEquipped && e.iLiveId >= 0)
                out.push_back(e.iLiveId);
        return out;
    }
}
