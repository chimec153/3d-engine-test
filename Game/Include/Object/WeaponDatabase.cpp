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

        // Expected column order (15 fields):
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

            m_mapIdToIndex[def.iId] = m_vecWeapons.size();
            m_vecWeapons.push_back(def);
        }

        return m_vecWeapons.size();
    }

    const WeaponDef* WeaponDatabase::Get(int iId) const
    {
        auto it = m_mapIdToIndex.find(iId);
        if (it == m_mapIdToIndex.end()) return nullptr;
        return &m_vecWeapons[it->second];
    }
}
