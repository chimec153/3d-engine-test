#pragma once

#include "Core/PathManager.h"
#include "Core/Macro.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Client
{
    // Minimal CSV reader for static game data (weapons.csv etc).
    //
    // - Comma-separated, no quoting (numeric/enum/path values only — no
    //   embedded commas needed for the game data files we ship).
    // - Lines starting with '#' are comments and skipped.
    // - Blank lines skipped.
    // - First non-comment line is assumed to be the header and discarded.
    //
    // Paths are resolved through CPathManager::ResolveMB so callers can
    // pass virtual paths like "/Game/Data/Weapons/weapons.csv" — the
    // mount registry maps that to <exe-dir>\Resource\Data\Weapons\... at
    // runtime. A non-virtual (no leading '/') path falls back to
    // ROOT_PATH + filename, same convention as the rest of the engine.
    //
    // Returns rows of trimmed string fields. Parsing into typed structs
    // is the caller's job.
    class CSVLoader
    {
    public:
        // Each row is a vector<string>; the whole file is a vector of rows.
        // Returns empty on file-not-found; callers can detect that via empty().
        static std::vector<std::vector<std::string>> Load(const std::string& strPath)
        {
            std::vector<std::vector<std::string>> rows;

            char szResolved[MAX_PATH] = {};
            Engine::CPathManager::GetInst()->ResolveMB(
                strPath.c_str(), ROOT_PATH, szResolved);

            std::ifstream file(szResolved);
            if (!file.is_open()) return rows;

            std::string line;
            bool bHeaderSkipped = false;
            while (std::getline(file, line))
            {
                // Strip trailing CR (Windows line endings on Unix-built files).
                if (!line.empty() && line.back() == '\r') line.pop_back();

                // Skip blank / comment lines before the header check so the
                // file can carry a top-of-file comment block.
                std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed[0] == '#') continue;

                if (!bHeaderSkipped)
                {
                    bHeaderSkipped = true;
                    continue;
                }

                rows.push_back(Split(line));
            }
            return rows;
        }

    private:
        static std::string Trim(const std::string& s)
        {
            size_t start = s.find_first_not_of(" \t");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \t");
            return s.substr(start, end - start + 1);
        }

        static std::vector<std::string> Split(const std::string& line)
        {
            std::vector<std::string> out;
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ','))
                out.push_back(Trim(cell));
            return out;
        }
    };
}
