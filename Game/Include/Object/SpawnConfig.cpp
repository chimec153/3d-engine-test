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
    }

    bool SpawnConfig::LoadFromCSV(const std::string& strPath)
    {
        auto rows = CSVLoader::Load(strPath);
        if (rows.empty()) return false;

        // Expected column order (2 fields), single data row:
        //   0  enemy_interval (seconds)
        //   1  enemy_radius   (cells)
        const auto& row = rows[0];
        if (row.size() < 2) return false;

        using namespace SpawnConfig_detail;
        m_fEnemyInterval = ToFloat(row[0]);
        m_fEnemyRadius   = ToFloat(row[1]);
        return true;
    }
}
