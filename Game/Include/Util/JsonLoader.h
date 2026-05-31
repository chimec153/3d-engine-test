#pragma once

#include "Core/PathManager.h"
#include "Core/Macro.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Client
{
    // Minimal JSON tree for static game data (enemies.json, rounds.json).
    // Values are tagged-union style: a single class can be any of the JSON
    // kinds. Recursion is allowed because std::vector<T> / std::map<K,T>
    // accept incomplete T in C++17, so a JsonValue can hold arrays/objects
    // of itself without pImpl.
    //
    // No floats below — JSON numbers are stored as double and cast at the
    // call site (AsInt, AsFloat). Bools and null are explicit kinds.
    //
    // Lookup helpers (Find, operator[]) return references / pointers into
    // the tree so callers walk the structure cheaply; never mutate. A
    // missing key returns a static "null" sentinel so chained Find/AsX
    // doesn't dereference junk.
    class JsonValue
    {
    public:
        enum Type { Null, Bool, Number, String, Array, Object };

        JsonValue() = default;

        Type GetType()  const { return m_eType; }
        bool IsNull()   const { return m_eType == Null; }
        bool IsBool()   const { return m_eType == Bool; }
        bool IsNumber() const { return m_eType == Number; }
        bool IsString() const { return m_eType == String; }
        bool IsArray()  const { return m_eType == Array; }
        bool IsObject() const { return m_eType == Object; }

        bool   AsBool  (bool   d = false) const { return m_eType == Bool   ? m_bBool                                   : d; }
        double AsNumber(double d = 0.0  ) const { return m_eType == Number ? m_dNumber                                 : d; }
        int    AsInt   (int    d = 0    ) const { return m_eType == Number ? static_cast<int>(m_dNumber)               : d; }
        float  AsFloat (float  d = 0.f  ) const { return m_eType == Number ? static_cast<float>(m_dNumber)             : d; }
        // Two AsString overloads — the no-arg form returns the held string
        // (or a function-local empty), the const char* form lets callers
        // provide an inline fallback like AsString("chase"). They're
        // separate functions instead of one with a default so we don't
        // have to deal with JsonValue-incomplete-at-class-scope issues
        // for any std::string sentinel.
        const std::string& AsString() const
        {
            static const std::string s_empty;
            return m_eType == String ? m_strString : s_empty;
        }
        std::string AsString(const char* pDefault) const
        {
            return m_eType == String ? m_strString : std::string(pDefault ? pDefault : "");
        }
        const std::vector<JsonValue>& AsArray() const
        {
            static const std::vector<JsonValue> s_empty;
            return m_eType == Array ? m_arr : s_empty;
        }
        const std::map<std::string, JsonValue>& AsObject() const
        {
            static const std::map<std::string, JsonValue> s_empty;
            return m_eType == Object ? m_obj : s_empty;
        }

        // Object key lookup. Returns null sentinel when not present, so
        // chained Find(...).AsInt(...) is safe.
        const JsonValue& Find(const std::string& strKey) const
        {
            static const JsonValue s_null;
            if (m_eType != Object) return s_null;
            auto it = m_obj.find(strKey);
            return it == m_obj.end() ? s_null : it->second;
        }

        size_t Size() const
        {
            if (m_eType == Array)  return m_arr.size();
            if (m_eType == Object) return m_obj.size();
            return 0;
        }

        // Tree builders — used by JsonLoader::Parse. Public so the parser
        // can populate without a friend declaration.
        void SetNull  ()                          { m_eType = Null; }
        void SetBool  (bool b)                    { m_eType = Bool;   m_bBool   = b; }
        void SetNumber(double d)                  { m_eType = Number; m_dNumber = d; }
        void SetString(std::string s)             { m_eType = String; m_strString = std::move(s); }
        std::vector<JsonValue>&            AsMutableArray()  { m_eType = Array;  return m_arr; }
        std::map<std::string, JsonValue>&  AsMutableObject() { m_eType = Object; return m_obj; }

    private:
        Type        m_eType   = Null;
        bool        m_bBool   = false;
        double      m_dNumber = 0.0;
        std::string m_strString;
        std::vector<JsonValue>            m_arr;
        std::map<std::string, JsonValue>  m_obj;
    };

    // Loader / parser. Same path convention as CSVLoader: paths with a
    // leading '/Mount/...' route through PathManager::ResolveMB, anything
    // else falls back to ROOT_PATH + filename.
    //
    // The parser is intentionally tiny (no comments, no trailing commas) —
    // enemies.json and rounds.json are hand-written but stay strict JSON.
    // On malformed input the loader returns a Null JsonValue and the
    // caller falls back to defaults (the same behaviour as CSVLoader).
    class JsonLoader
    {
    public:
        static JsonValue Load(const std::string& strPath)
        {
            char szResolved[MAX_PATH] = {};
            Engine::CPathManager::GetInst()->ResolveMB(
                strPath.c_str(), ROOT_PATH, szResolved);

            std::ifstream file(szResolved, std::ios::binary);
            if (!file.is_open()) return JsonValue{};

            std::stringstream buf;
            buf << file.rdbuf();
            const std::string contents = buf.str();
            return Parse(contents.c_str(), contents.size());
        }

        static JsonValue Parse(const char* pText, size_t iLen)
        {
            Parser p(pText, iLen);
            p.SkipWS();
            JsonValue root;
            if (!p.ParseValue(root)) return JsonValue{};
            return root;
        }

    private:
        // Hand-rolled recursive-descent parser. Position-tracking by raw
        // pointer; on any unexpected token the caller chain returns false
        // and the loader yields Null. No exceptions; no allocations beyond
        // the JsonValue tree.
        struct Parser
        {
            const char* pCur;
            const char* pEnd;

            Parser(const char* p, size_t n) : pCur(p), pEnd(p + n) {}

            void SkipWS()
            {
                while (pCur < pEnd)
                {
                    const char c = *pCur;
                    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') ++pCur;
                    else break;
                }
            }

            bool ParseValue(JsonValue& out)
            {
                SkipWS();
                if (pCur >= pEnd) return false;
                const char c = *pCur;
                if (c == '{') return ParseObject(out);
                if (c == '[') return ParseArray(out);
                if (c == '"') return ParseString(out);
                if (c == 't' || c == 'f') return ParseBool(out);
                if (c == 'n') return ParseNull(out);
                if (c == '-' || (c >= '0' && c <= '9')) return ParseNumber(out);
                return false;
            }

            bool ParseObject(JsonValue& out)
            {
                ++pCur;   // '{'
                auto& obj = out.AsMutableObject();
                SkipWS();
                if (pCur < pEnd && *pCur == '}') { ++pCur; return true; }

                while (pCur < pEnd)
                {
                    SkipWS();
                    if (pCur >= pEnd || *pCur != '"') return false;
                    JsonValue keyVal;
                    if (!ParseString(keyVal)) return false;
                    SkipWS();
                    if (pCur >= pEnd || *pCur != ':') return false;
                    ++pCur;
                    SkipWS();
                    JsonValue v;
                    if (!ParseValue(v)) return false;
                    obj.emplace(keyVal.AsString(), std::move(v));
                    SkipWS();
                    if (pCur < pEnd && *pCur == ',') { ++pCur; continue; }
                    if (pCur < pEnd && *pCur == '}') { ++pCur; return true; }
                    return false;
                }
                return false;
            }

            bool ParseArray(JsonValue& out)
            {
                ++pCur;   // '['
                auto& arr = out.AsMutableArray();
                SkipWS();
                if (pCur < pEnd && *pCur == ']') { ++pCur; return true; }

                while (pCur < pEnd)
                {
                    SkipWS();
                    JsonValue v;
                    if (!ParseValue(v)) return false;
                    arr.push_back(std::move(v));
                    SkipWS();
                    if (pCur < pEnd && *pCur == ',') { ++pCur; continue; }
                    if (pCur < pEnd && *pCur == ']') { ++pCur; return true; }
                    return false;
                }
                return false;
            }

            bool ParseString(JsonValue& out)
            {
                if (pCur >= pEnd || *pCur != '"') return false;
                ++pCur;
                std::string s;
                while (pCur < pEnd)
                {
                    const char c = *pCur++;
                    if (c == '"') { out.SetString(std::move(s)); return true; }
                    if (c == '\\')
                    {
                        if (pCur >= pEnd) return false;
                        const char esc = *pCur++;
                        switch (esc)
                        {
                            case '"':  s.push_back('"');  break;
                            case '\\': s.push_back('\\'); break;
                            case '/':  s.push_back('/');  break;
                            case 'b':  s.push_back('\b'); break;
                            case 'f':  s.push_back('\f'); break;
                            case 'n':  s.push_back('\n'); break;
                            case 'r':  s.push_back('\r'); break;
                            case 't':  s.push_back('\t'); break;
                            // \uXXXX is rare in our data files; emit the four
                            // hex chars literally rather than fail outright so
                            // a stray escape doesn't tank the whole load.
                            case 'u':
                                if (pCur + 4 <= pEnd) { s.append("\\u", 2); s.append(pCur, 4); pCur += 4; }
                                else return false;
                                break;
                            default: return false;
                        }
                    }
                    else s.push_back(c);
                }
                return false;
            }

            bool ParseBool(JsonValue& out)
            {
                if (pCur + 4 <= pEnd && std::strncmp(pCur, "true", 4) == 0)
                { out.SetBool(true); pCur += 4; return true; }
                if (pCur + 5 <= pEnd && std::strncmp(pCur, "false", 5) == 0)
                { out.SetBool(false); pCur += 5; return true; }
                return false;
            }

            bool ParseNull(JsonValue& out)
            {
                if (pCur + 4 <= pEnd && std::strncmp(pCur, "null", 4) == 0)
                { out.SetNull(); pCur += 4; return true; }
                return false;
            }

            bool ParseNumber(JsonValue& out)
            {
                const char* pStart = pCur;
                if (*pCur == '-') ++pCur;
                while (pCur < pEnd && ((*pCur >= '0' && *pCur <= '9') ||
                       *pCur == '.' || *pCur == 'e' || *pCur == 'E' ||
                       *pCur == '+' || *pCur == '-')) ++pCur;
                if (pCur == pStart) return false;
                // strtod stops at the first non-numeric; bounded by our
                // scan above, so it never overruns the buffer.
                std::string num(pStart, pCur);
                out.SetNumber(std::atof(num.c_str()));
                return true;
            }
        };
    };
}
