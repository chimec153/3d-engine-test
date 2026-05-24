#include "EnemyDatabase.h"
#include "../Util/CSVLoader.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace Client
{
    namespace EnemyDatabase_detail
    {
        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        EnemyKind ParseKind(const std::string& s)
        {
            std::string v = ToLower(s);
            if (v == "capsule") return EnemyKind::Capsule;
            return EnemyKind::Box;
        }

        int   ToInt  (const std::string& s) { return s.empty() ? 0   : std::atoi(s.c_str()); }
        float ToFloat(const std::string& s) { return s.empty() ? 0.f : static_cast<float>(std::atof(s.c_str())); }

        // Accepts "0xRRGGBB" (native) or plain decimal. strtoul reads
        // the 0x prefix when base = 0. Empty → default white-ish so a
        // half-filled CSV row doesn't render pitch-black enemies.
        unsigned int ToColor(const std::string& s)
        {
            if (s.empty()) return 0xFFFFFF;
            return static_cast<unsigned int>(std::strtoul(s.c_str(), nullptr, 0));
        }
    }

    size_t EnemyDatabase::LoadFromCSV(const std::string& strPath)
    {
        m_vecEnemies.clear();
        m_mapIdToIndex.clear();

        auto rows = CSVLoader::Load(strPath);
        m_vecEnemies.reserve(rows.size());

        // Expected column order (10 fields):
        //   0  id
        //   1  name
        //   2  kind            (Box | Capsule)
        //   3  max_hp
        //   4  speed           (cells / sec)
        //   5  attack_range    (world units)
        //   6  attack_cooldown (seconds)
        //   7  color_rgb       (0xRRGGBB material diffuse)
        //   8  attack_min      (int, melee damage low end)
        //   9  attack_max      (int, melee damage high end)
        for (const auto& row : rows)
        {
            if (row.size() < 7) continue;   // malformed row — skip silently

            using namespace EnemyDatabase_detail;
            EnemyDef def;
            def.iId             = ToInt  (row[0]);
            def.strName         = row[1];
            def.eKind           = ParseKind(row[2]);
            def.iMaxHP          = ToInt  (row[3]);
            def.fSpeed          = ToFloat(row[4]);
            def.fAttackRange    = ToFloat(row[5]);
            def.fAttackCooldown = ToFloat(row[6]);
            // Back-compat — shorter old rows keep the defaults from
            // EnemyDef (red diffuse, 1-2 damage).
            if (row.size() > 7) def.uColorRGB  = ToColor(row[7]);
            if (row.size() > 8) def.iAttackMin = ToInt  (row[8]);
            if (row.size() > 9) def.iAttackMax = ToInt  (row[9]);

            m_mapIdToIndex[def.iId] = m_vecEnemies.size();
            m_vecEnemies.push_back(def);
        }

        return m_vecEnemies.size();
    }

    const EnemyDef* EnemyDatabase::Get(int iId) const
    {
        auto it = m_mapIdToIndex.find(iId);
        if (it == m_mapIdToIndex.end()) return nullptr;
        return &m_vecEnemies[it->second];
    }
}
