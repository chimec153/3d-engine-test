#include "SpawnConfig.h"
#include "../Util/CSVLoader.h"
#include <cstdlib>

namespace Client
{
    namespace SpawnConfig_detail
    {
        float ToFloat(const std::string& s)
        {
            return s.empty() ? 0.f : static_cast<float>(std::atof(s.c_str()));
        }
        int ToInt(const std::string& s)
        {
            return s.empty() ? 0 : std::atoi(s.c_str());
        }
    }

    size_t SpawnConfig::LoadFromCSV(const std::string& strPath)
    {
        auto rows = CSVLoader::Load(strPath);
        if (rows.empty()) return 0;

        // Expected column order (5 fields), one or more data rows:
        //   0  enemy_interval    (seconds between spawns)
        //   1  enemy_radius      (cells)
        //   2  first_spawn_time  (scene-elapsed seconds before first spawn)
        //   3  last_spawn_time   (scene-elapsed seconds when spawns stop;
        //                        0 or negative = no end)
        //   4  enemy_id          (EnemyDatabase id; ≤ 0 = round-robin)
        //
        // Multiple rows describe parallel waves — EnemySpawner runs
        // each rule independently with its own accumulator.
        std::vector<SpawnRule> vecRules;
        vecRules.reserve(rows.size());
        using namespace SpawnConfig_detail;
        for (const auto& row : rows)
        {
            if (row.size() < 2) continue;
            SpawnRule rule;
            rule.fInterval  = ToFloat(row[0]);
            rule.fRadius    = ToFloat(row[1]);
            // Back-compat — shorter rows fall back to defaults.
            rule.fFirstTime = row.size() > 2 ? ToFloat(row[2]) : 0.0f;
            rule.fLastTime  = row.size() > 3 ? ToFloat(row[3]) : 0.0f;
            rule.iEnemyId   = row.size() > 4 ? ToInt  (row[4]) : 0;
            vecRules.push_back(rule);
        }
        if (vecRules.empty()) return 0;
        m_vecRules = std::move(vecRules);
        return m_vecRules.size();
    }
}
