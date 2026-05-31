#pragma once

#include "../Util/CSVLoader.h"
#include <string>
#include <vector>
#include <cstdlib>

namespace Client
{
    // One level-up card, loaded from levelups.csv. The effect itself lives in
    // Player::ApplyStatUpgrade, keyed by `key`; this struct only carries the
    // data a designer tunes: which effect, how it reads on the card, how often
    // it rolls, and the magnitude passed to the effect.
    struct LevelUpDef
    {
        std::string  key;              // effect selector (Player::ApplyStatUpgrade)
        std::string  name;             // card title (ASCII)
        std::string  effect;           // card effect line (ASCII)
        unsigned int colorRGB = 0x606060;
        int          weight   = 1;     // relative roll odds (>=1)
        float        amount   = 0.f;   // effect magnitude (meaning depends on key)
    };

    // Data-driven catalogue of level-up cards. Header-only singleton in the
    // TowerManager / GameStateManager style (no .cpp, so no project edits).
    // Loaded once by GameScene::Init from levelups.csv; LevelUpChoices rolls a
    // weighted subset and shows each card's name/effect/colour.
    class LevelUpDatabase
    {
    public:
        static LevelUpDatabase& GetInst()
        {
            static LevelUpDatabase inst;
            return inst;
        }

        // Columns: key, name, effect, color (0xRRGGBB or decimal), weight, amount.
        // CSVLoader skips '#' comments and the first non-comment (header) line.
        size_t LoadFromCSV(const std::string& strPath)
        {
            m_vec.clear();
            for (const auto& row : CSVLoader::Load(strPath))
            {
                if (row.size() < 6) continue;          // malformed — skip
                if (row[0].empty()) continue;          // need a dispatch key
                LevelUpDef d;
                d.key      = row[0];
                d.name     = row[1];
                d.effect   = row[2];
                d.colorRGB = static_cast<unsigned int>(std::strtoul(row[3].c_str(), nullptr, 0));
                d.weight   = std::atoi(row[4].c_str());
                if (d.weight < 1) d.weight = 1;
                d.amount   = static_cast<float>(std::atof(row[5].c_str()));
                m_vec.push_back(std::move(d));
            }
            return m_vec.size();
        }

        const std::vector<LevelUpDef>& All() const { return m_vec; }
        size_t Count() const { return m_vec.size(); }

    private:
        LevelUpDatabase() = default;
        ~LevelUpDatabase() = default;
        LevelUpDatabase(const LevelUpDatabase&)            = delete;
        LevelUpDatabase& operator=(const LevelUpDatabase&) = delete;

        std::vector<LevelUpDef> m_vec;
    };
}
