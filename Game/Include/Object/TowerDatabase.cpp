#include "TowerData.h"
#include "../Util/CSVLoader.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace Client
{
    namespace TowerDatabase_detail
    {
        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }
        TowerKind ParseKind(const std::string& s)
        {
            return ToLower(s) == "heal" ? TowerKind::Heal : TowerKind::Attack;
        }
        int   ToInt  (const std::string& s) { return s.empty() ? 0   : std::atoi(s.c_str()); }
        float ToFloat(const std::string& s) { return s.empty() ? 0.f : static_cast<float>(std::atof(s.c_str())); }
    }

    size_t TowerDatabase::LoadFromCSV(const std::string& strPath)
    {
        m_vecTowers.clear();
        m_mapIdToIndex.clear();

        using namespace TowerDatabase_detail;
        // Column order (towers.csv):
        //   0 id           5 defense        10 price         13 heal_interval
        //   1 name         6 attack_speed   11 aggro         14 heal_radius
        //   2 kind         7 crit_chance    12 heal_amount
        //   3 hp           8 crit_mult
        //   4 attack       9 range
        for (const auto& row : CSVLoader::Load(strPath))
        {
            if (row.size() < 9) continue;   // malformed row — skip silently

            TowerDef def;   // members already default to the GameDefs constants
            def.iId          = ToInt  (row[0]);
            def.strName      = row[1];
            def.eKind        = ParseKind(row[2]);
            def.iHP          = ToInt  (row[3]);
            def.fAttack      = ToFloat(row[4]);
            def.fDefense     = ToFloat(row[5]);
            def.fAttackSpeed = ToFloat(row[6]);
            def.fCritChance  = ToFloat(row[7]);
            def.fCritMult    = ToFloat(row[8]);
            // Optional trailing columns — size-guarded so a shorter row keeps
            // the TowerDef defaults.
            if (row.size() > 9)  def.fRange        = ToFloat(row[9]);
            if (row.size() > 10) def.iPrice        = ToInt  (row[10]);
            if (row.size() > 11) def.iAggro        = ToInt  (row[11]);
            if (row.size() > 12) def.iHealAmount   = ToInt  (row[12]);
            if (row.size() > 13) def.fHealInterval = ToFloat(row[13]);
            if (row.size() > 14) def.fHealRadius   = ToFloat(row[14]);

            m_mapIdToIndex[def.iId] = m_vecTowers.size();
            m_vecTowers.push_back(def);
        }
        return m_vecTowers.size();
    }

    const TowerDef* TowerDatabase::Get(int iId) const
    {
        auto it = m_mapIdToIndex.find(iId);
        if (it == m_mapIdToIndex.end()) return nullptr;
        return &m_vecTowers[it->second];
    }

    const TowerDef* TowerDatabase::FirstOfKind(TowerKind eKind) const
    {
        for (const auto& def : m_vecTowers)
            if (def.eKind == eKind) return &def;
        return nullptr;
    }
}
