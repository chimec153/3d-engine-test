#include "RoundDatabase.h"
#include "../Util/JsonLoader.h"

namespace Client
{
    namespace RoundDatabase_detail
    {
        SpawnPattern ParsePattern(const std::string& s)
        {
            if (s == "edge_all")    return SpawnPattern::EdgeAll;
            if (s == "point_burst") return SpawnPattern::PointBurst;
            if (s == "ring")        return SpawnPattern::Ring;
            return SpawnPattern::EdgeRandom;   // default + "edge_random"
        }
    }

    size_t RoundDatabase::LoadFromJSON(const std::string& strPath)
    {
        m_vecRounds.clear();
        m_iTotalRounds = 0;

        JsonValue root = JsonLoader::Load(strPath);
        if (!root.IsObject()) return 0;

        const JsonValue& config = root.Find("config");
        if (config.IsObject())
            m_iTotalRounds = config.Find("totalRounds").AsInt(0);

        const JsonValue& rounds = root.Find("rounds");
        if (!rounds.IsArray()) return 0;

        using namespace RoundDatabase_detail;

        m_vecRounds.reserve(rounds.Size());
        for (const JsonValue& r : rounds.AsArray())
        {
            if (!r.IsObject()) continue;

            RoundDef def;
            def.iRound            = r.Find("round").AsInt(0);
            def.fDuration         = r.Find("duration").AsFloat(30.f);
            def.bIsBoss           = r.Find("isBoss").AsBool(false);
            def.fHpMultiplier     = r.Find("hpMultiplier").AsFloat(1.f);
            def.fDamageMultiplier = r.Find("damageMultiplier").AsFloat(1.f);

            const JsonValue& spawns = r.Find("spawns");
            if (spawns.IsArray())
            {
                def.vecSpawns.reserve(spawns.Size());
                for (const JsonValue& s : spawns.AsArray())
                {
                    if (!s.IsObject()) continue;
                    RoundSpawn rs;
                    rs.strEnemyId = s.Find("enemyId").AsString();
                    if (rs.strEnemyId.empty()) continue;
                    rs.fStartTime = s.Find("startTime").AsFloat(0.f);
                    rs.fEndTime   = s.Find("endTime").AsFloat(def.fDuration);
                    rs.fInterval  = s.Find("interval").AsFloat(2.f);
                    rs.iBurst     = s.Find("burst").AsInt(1);
                    rs.ePattern   = ParsePattern(s.Find("pattern").AsString("edge_random"));
                    def.vecSpawns.push_back(std::move(rs));
                }
            }

            const JsonValue& boss = r.Find("boss");
            if (def.bIsBoss && boss.IsObject())
            {
                def.tBoss.strBossId  = boss.Find("id").AsString();
                def.tBoss.fSpawnTime = boss.Find("spawnTime").AsFloat(0.f);
                def.tBoss.ePattern   = ParsePattern(
                    boss.Find("spawnPattern").AsString("point_burst"));
            }

            m_vecRounds.push_back(std::move(def));
        }

        // If config didn't set totalRounds, fall back to whatever we parsed
        // — UI doesn't have to special-case the absent-config case.
        if (m_iTotalRounds <= 0)
            m_iTotalRounds = static_cast<int>(m_vecRounds.size());

        return m_vecRounds.size();
    }

    const RoundDef* RoundDatabase::Get(int iRound) const
    {
        for (const RoundDef& r : m_vecRounds)
            if (r.iRound == iRound) return &r;
        return nullptr;
    }
}
