#include "EnemyDatabase.h"
#include "../Util/JsonLoader.h"
#include <algorithm>
#include <cctype>

namespace Client
{
    namespace EnemyDatabase_detail
    {
        EnemyTier ParseTier(const std::string& s)
        {
            if (s == "elite") return EnemyTier::Elite;
            if (s == "boss")  return EnemyTier::Boss;
            return EnemyTier::Basic;
        }

        EnemyKind KindFromTier(EnemyTier e)
        {
            // basic → Box (current red enemy mesh), elite/boss → Capsule (green
            // mesh). Phase 1 only has two meshes so this is the only
            // differentiation we can render; Phase 2 will add per-id meshes.
            return (e == EnemyTier::Basic) ? EnemyKind::Box : EnemyKind::Capsule;
        }

        // Default colour per id-string. JSON doesn't carry colours (the spec
        // is purely numeric); pick distinct hues so each archetype reads as
        // a different threat at a glance. Falls back to a neutral grey if a
        // future id-string lands here without a mapping.
        unsigned int ColorFor(const std::string& strIdKey)
        {
            if (strIdKey == "crawler")      return 0xFF1A1A;   // red — the baseline mob
            if (strIdKey == "swarmling")    return 0xFFAA33;   // orange — fast & weak
            if (strIdKey == "brute")        return 0x880000;   // dark red — tanky
            if (strIdKey == "dasher")       return 0xFF55FF;   // magenta — danger-on-the-move
            if (strIdKey == "spitter")      return 0x33FFAA;   // teal — ranged threat
            if (strIdKey == "bomber")       return 0xFF6633;   // orange-red — fuse-lit
            if (strIdKey == "splitter")     return 0xAAFF33;   // yellow-green — divides on death
            if (strIdKey == "shieldbearer") return 0x6688FF;   // pale blue — armoured front
            if (strIdKey == "summoner")     return 0xAA33FF;   // purple — high priority
            if (strIdKey == "phantom")      return 0xCCCCFF;   // ghostly pale blue
            // Boss colours (Phase 3).
            if (strIdKey == "devourer")     return 0x550000;   // very dark red
            if (strIdKey == "hive_queen")   return 0x880088;   // dark magenta
            return 0xCCCCCC;
        }

        AbilityType ParseAbility(const std::string& s)
        {
            if (s == "charge")  return AbilityType::Charge;
            if (s == "slam")    return AbilityType::Slam;
            if (s == "barrage") return AbilityType::Barrage;
            if (s == "summon")  return AbilityType::Summon;
            return AbilityType::None;
        }

        // Reads one phase row out of bosses[].phases[]. Populates the
        // ability block matching `ability.type` and the optional
        // alsoSummon (hive_queen phase 3).
        BossPhase ParsePhase(const JsonValue& p)
        {
            BossPhase ph;
            ph.fHpThreshold   = p.Find("hpThreshold").AsFloat(1.f);
            ph.fMoveSpeedMult = p.Find("moveSpeedMult").AsFloat(1.f);
            ph.strName        = p.Find("name").AsString();

            const JsonValue& ability = p.Find("ability");
            if (ability.IsObject())
            {
                const std::string strType = ability.Find("type").AsString();
                ph.eAbility         = ParseAbility(strType);
                ph.fAbilityCooldown = ability.Find("cooldown").AsFloat(0.f);
                ph.fTelegraphTime   = ability.Find("telegraphTime").AsFloat(0.f);
                if (strType == "charge")
                {
                    ph.fChargeSpeedPx = ability.Find("chargeSpeed").AsFloat(0.f);
                }
                else if (strType == "slam")
                {
                    ph.fSlamRadiusPx = ability.Find("slamRadius").AsFloat(0.f);
                    ph.iSlamDamage   = ability.Find("slamDamage").AsInt(0);
                }
                else if (strType == "barrage")
                {
                    ph.fProjSpeedPx     = ability.Find("projectileSpeed").AsFloat(0.f);
                    ph.iProjDamage      = ability.Find("projectileDamage").AsInt(0);
                    ph.iShotsPerVolley  = ability.Find("shotsPerVolley").AsInt(0);
                    ph.fSpreadDegrees   = ability.Find("spreadDegrees").AsFloat(360.f);
                }
                else if (strType == "summon")
                {
                    ph.strSummonId  = ability.Find("spawnId").AsString();
                    ph.iSummonCount = ability.Find("summonCount").AsInt(0);
                }
            }

            const JsonValue& alsoSummon = p.Find("alsoSummon");
            if (alsoSummon.IsObject())
            {
                ph.strAltSummonId      = alsoSummon.Find("spawnId").AsString();
                ph.iAltSummonCount     = alsoSummon.Find("summonCount").AsInt(0);
                ph.fAltSummonCooldown  = alsoSummon.Find("cooldown").AsFloat(0.f);
            }
            return ph;
        }
    }

    size_t EnemyDatabase::LoadFromJSON(const std::string& strPath)
    {
        m_vecEnemies.clear();
        m_mapIdToIndex.clear();
        m_mapKeyToIndex.clear();

        JsonValue root = JsonLoader::Load(strPath);
        if (!root.IsObject()) return 0;

        const JsonValue& enemies = root.Find("enemies");
        if (!enemies.IsArray()) return 0;

        using namespace EnemyDatabase_detail;

        m_vecEnemies.reserve(enemies.Size());
        int iNextId = 1;
        for (const JsonValue& e : enemies.AsArray())
        {
            if (!e.IsObject()) continue;

            EnemyDef def;
            def.strIdKey        = e.Find("id").AsString();
            if (def.strIdKey.empty()) continue;

            def.iId             = iNextId++;
            def.strName         = e.Find("name").AsString();
            def.strBehavior     = e.Find("behavior").AsString("chase");
            def.eTier           = ParseTier(e.Find("tier").AsString("basic"));
            def.eKind           = KindFromTier(def.eTier);

            def.iBaseHp         = e.Find("baseHp").AsInt(10);
            def.iContactDamage  = e.Find("contactDamage").AsInt(0);
            def.fMoveSpeedPx    = e.Find("moveSpeed").AsFloat(100.f);
            def.fHitboxRadiusPx = e.Find("hitboxRadius").AsFloat(16.f);
            def.iGoldReward     = e.Find("goldReward").AsInt(1);
            def.iXpReward       = e.Find("xpReward").AsInt(1);

            def.uColorRGB       = ColorFor(def.strIdKey);

            // Seed runtime fields = base. The spawner makes a per-spawn
            // copy and rewrites these with round multipliers before
            // handing the def to Enemy::ApplyDef.
            def.iMaxHP          = def.iBaseHp;
            def.iAttackMin      = def.iContactDamage;
            def.iAttackMax      = def.iContactDamage;
            def.fSpeed          = def.fMoveSpeedPx / kPxPerCell;

            // Special params (Phase 2). EnemyDef stores all blocks flat;
            // a block stays zero-initialised when this enemy's special
            // doesn't match its type. behavior+special together tell the
            // Enemy tick which branch to drive at runtime.
            const JsonValue& special = e.Find("special");
            if (special.IsObject())
            {
                const std::string strType = special.Find("type").AsString();
                if (strType == "dash")
                {
                    def.fDashSpeedPx   = special.Find("dashSpeed").AsFloat(0.f);
                    def.fDashCooldown  = special.Find("dashCooldown").AsFloat(0.f);
                    def.fDashRangePx   = special.Find("dashRange").AsFloat(0.f);
                    def.fDashTelegraph = special.Find("telegraphTime").AsFloat(0.f);
                }
                else if (strType == "ranged")
                {
                    def.fProjSpeedPx      = special.Find("projectileSpeed").AsFloat(0.f);
                    def.iProjDamage       = special.Find("projectileDamage").AsInt(0);
                    def.fFireCooldown     = special.Find("fireCooldown").AsFloat(0.f);
                    def.fPreferredRangePx = special.Find("preferredRange").AsFloat(0.f);
                }
                else if (strType == "explode")
                {
                    def.fExplodeRadiusPx = special.Find("explodeRadius").AsFloat(0.f);
                    def.iExplodeDamage   = special.Find("explodeDamage").AsInt(0);
                    def.fFuseTime        = special.Find("fuseTime").AsFloat(0.f);
                    def.fTriggerRangePx  = special.Find("triggerRange").AsFloat(0.f);
                }
                else if (strType == "split")
                {
                    def.strSplitId   = special.Find("spawnId").AsString();
                    def.iSplitCount  = special.Find("spawnCount").AsInt(0);
                }
                else if (strType == "shield")
                {
                    def.fShieldArcDegrees = special.Find("shieldArcDegrees").AsFloat(0.f);
                    def.fShieldReduction  = special.Find("frontDamageReduction").AsFloat(0.f);
                }
                else if (strType == "summon")
                {
                    def.strSummonId       = special.Find("spawnId").AsString();
                    def.iSummonCount      = special.Find("summonCount").AsInt(0);
                    def.fSummonCooldown   = special.Find("summonCooldown").AsFloat(0.f);
                    // Summoner JSON also carries preferredRange (for the
                    // kite movement) — already read above on the ranged
                    // branch path, but the JSON shape has it under
                    // special.type=="summon" here.
                    def.fPreferredRangePx = special.Find("preferredRange").AsFloat(0.f);
                }
                else if (strType == "blink")
                {
                    def.fBlinkCooldown   = special.Find("blinkCooldown").AsFloat(0.f);
                    def.fBlinkDistancePx = special.Find("blinkDistance").AsFloat(0.f);
                }
            }

            m_mapIdToIndex [def.iId]      = m_vecEnemies.size();
            m_mapKeyToIndex[def.strIdKey] = m_vecEnemies.size();
            m_vecEnemies.push_back(std::move(def));
        }

        // Bosses share the EnemyDef table — same Get(string) lookup. They
        // carry bIsBoss + vecPhases so the spawner can branch on boss
        // sizing + the phase tick driver runs.
        const JsonValue& bosses = root.Find("bosses");
        if (bosses.IsArray())
        {
            for (const JsonValue& b : bosses.AsArray())
            {
                if (!b.IsObject()) continue;
                EnemyDef def;
                def.strIdKey        = b.Find("id").AsString();
                if (def.strIdKey.empty()) continue;

                def.iId             = iNextId++;
                def.strName         = b.Find("name").AsString();
                def.strBehavior     = "chase";   // bosses use chase as the baseline; phase abilities layer on top
                def.eTier           = EnemyTier::Boss;
                def.eKind           = EnemyKind::Capsule;  // visually distinct from mob bucket
                def.bIsBoss         = true;
                def.iAppearsRound   = b.Find("appearsRound").AsInt(0);

                def.iBaseHp         = b.Find("baseHp").AsInt(1000);
                def.iContactDamage  = b.Find("contactDamage").AsInt(0);
                def.fMoveSpeedPx    = b.Find("moveSpeed").AsFloat(70.f);
                def.fHitboxRadiusPx = b.Find("hitboxRadius").AsFloat(70.f);
                def.iGoldReward     = b.Find("goldReward").AsInt(0);
                def.iXpReward       = b.Find("xpReward").AsInt(0);

                def.uColorRGB       = ColorFor(def.strIdKey);

                def.iMaxHP          = def.iBaseHp;
                def.iAttackMin      = def.iContactDamage;
                def.iAttackMax      = def.iContactDamage;
                def.fSpeed          = def.fMoveSpeedPx / kPxPerCell;

                const JsonValue& phases = b.Find("phases");
                if (phases.IsArray())
                {
                    def.vecPhases.reserve(phases.Size());
                    for (const JsonValue& p : phases.AsArray())
                    {
                        if (!p.IsObject()) continue;
                        def.vecPhases.push_back(ParsePhase(p));
                    }
                }

                m_mapIdToIndex [def.iId]      = m_vecEnemies.size();
                m_mapKeyToIndex[def.strIdKey] = m_vecEnemies.size();
                m_vecEnemies.push_back(std::move(def));
            }
        }

        return m_vecEnemies.size();
    }

    const EnemyDef* EnemyDatabase::Get(int iId) const
    {
        auto it = m_mapIdToIndex.find(iId);
        if (it == m_mapIdToIndex.end()) return nullptr;
        return &m_vecEnemies[it->second];
    }

    const EnemyDef* EnemyDatabase::Get(const std::string& strIdKey) const
    {
        auto it = m_mapKeyToIndex.find(strIdKey);
        if (it == m_mapKeyToIndex.end()) return nullptr;
        return &m_vecEnemies[it->second];
    }
}
