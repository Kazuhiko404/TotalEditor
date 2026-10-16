#include "common.h"
#include "uniform_store.h"
#include "log.h"

#include <kenshi/SaveManager.h>
#include <fstream>
#include <sstream>
#include <map>
#include <cctype>

namespace totaleditor
{
namespace UniformStore
{
namespace
{
struct UniformEntry
{
    std::string primary;
    std::string secondary;
    UniformEntry() {}
    UniformEntry(const std::string& p, const std::string& s) : primary(p), secondary(s) {}
};

typedef std::map<std::string, UniformEntry> StoreMap;
StoreMap g_store;
std::string g_currentFolder;

struct StoreMutex
{
    CRITICAL_SECTION cs;
    StoreMutex() { InitializeCriticalSection(&cs); }
    ~StoreMutex() { DeleteCriticalSection(&cs); }
} g_mutex;

struct Guard
{
    Guard() { EnterCriticalSection(&g_mutex.cs); }
    ~Guard() { LeaveCriticalSection(&g_mutex.cs); }
private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);
};

const char* const kFileName = "uniform_colours.json";

void SkipWhitespace(const std::string& str, size_t& pos)
{
    while (pos < str.size() && std::isspace(static_cast<unsigned char>(str[pos])))
        ++pos;
}

bool MatchChar(const std::string& str, size_t& pos, char expected)
{
    SkipWhitespace(str, pos);
    if (pos < str.size() && str[pos] == expected)
    {
        ++pos;
        return true;
    }
    return false;
}

bool ParseString(const std::string& str, size_t& pos, std::string& out)
{
    SkipWhitespace(str, pos);
    if (pos >= str.size() || str[pos] != '"')
        return false;
    ++pos;
    out.clear();
    while (pos < str.size())
    {
        char c = str[pos++];
        if (c == '"')
            return true;
        if (c == '\\' && pos < str.size())
        {
            char escaped = str[pos++];
            if (escaped == '"' || escaped == '\\' || escaped == '/')
                out += escaped;
            else if (escaped == 'b') out += '\b';
            else if (escaped == 'f') out += '\f';
            else if (escaped == 'n') out += '\n';
            else if (escaped == 'r') out += '\r';
            else if (escaped == 't') out += '\t';
            else out += escaped;
        }
        else
        {
            out += c;
        }
    }
    return false;
}

std::string SerializeJson(const StoreMap& entries)
{
    if (entries.empty())
        return "{}";

    std::ostringstream oss;
    oss << "{\n";
    for (StoreMap::const_iterator it = entries.begin(); it != entries.end();)
    {
        oss << "  \"" << it->first << "\": {\n";
        oss << "    \"primary\": \"" << it->second.primary << "\",\n";
        oss << "    \"secondary\": \"" << it->second.secondary << "\"\n";
        oss << "  }";
        ++it;
        if (it != entries.end())
            oss << ",";
        oss << "\n";
    }
    oss << "}\n";
    return oss.str();
}

bool ParseJson(const std::string& json, StoreMap* out)
{
    if (out == NULL) return false;
    out->clear();

    size_t pos = 0;
    if (!MatchChar(json, pos, '{')) return false;

    SkipWhitespace(json, pos);
    if (MatchChar(json, pos, '}')) return true;

    while (pos < json.size())
    {
        std::string charId;
        if (!ParseString(json, pos, charId)) return false;
        if (!MatchChar(json, pos, ':')) return false;
        if (!MatchChar(json, pos, '{')) return false;

        UniformEntry entry;
        bool hasFields = false;

        SkipWhitespace(json, pos);
        if (pos < json.size() && json[pos] != '}')
        {
            while (pos < json.size())
            {
                std::string fieldName;
                if (!ParseString(json, pos, fieldName)) return false;
                if (!MatchChar(json, pos, ':')) return false;
                std::string fieldValue;
                if (!ParseString(json, pos, fieldValue)) return false;

                if (fieldName == "primary") entry.primary = fieldValue;
                else if (fieldName == "secondary") entry.secondary = fieldValue;

                hasFields = true;

                SkipWhitespace(json, pos);
                if (pos < json.size() && json[pos] == ',')
                {
                    ++pos;
                    continue;
                }
                break;
            }
        }

        if (!MatchChar(json, pos, '}')) return false;
        if (hasFields) (*out)[charId] = entry;

        SkipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ',')
        {
            ++pos;
            continue;
        }
        break;
    }

    if (!MatchChar(json, pos, '}')) return false;
    SkipWhitespace(json, pos);
    return pos == json.size();
}

std::string SaveFolderOf(const std::string& location, const std::string& name)
{
    if (location.empty() || name.empty())
        return location;
    std::string::size_type end = location.size();
    while (end > 0 && (location[end - 1] == '/' || location[end - 1] == '\\'))
        --end;
    const std::string::size_type slash = location.find_last_of("/\\", end > 0 ? end - 1 : 0);
    const std::string::size_type start = slash == std::string::npos ? 0 : slash + 1;
    if (end - start == name.size() && location.compare(start, name.size(), name) == 0)
        return location.substr(0, end);
    return location.substr(0, end) + "/" + name;
}

int (*g_loadGameOriginal)(SaveManager*, const std::string&, const std::string&) = NULL;
int (*g_saveGameOriginal)(SaveManager*, const std::string&, const std::string&) = NULL;

int LoadGameHook(SaveManager* self, const std::string& location, const std::string& name)
{
    const std::string folder = SaveFolderOf(location, name);
    LoadFromFolder(folder);
    return g_loadGameOriginal(self, location, name);
}

int SaveGameHook(SaveManager* self, const std::string& location, const std::string& name)
{
    const std::string folder = SaveFolderOf(location, name);
    SaveToFolder(folder);
    return g_saveGameOriginal(self, location, name);
}

} // namespace

void Set(const std::string& characterId, const std::string& primary, const std::string& secondary)
{
    Guard lock;
    g_store[characterId] = UniformEntry(primary, secondary);
}

bool Get(const std::string& characterId, std::string* primary, std::string* secondary)
{
    Guard lock;
    StoreMap::const_iterator it = g_store.find(characterId);
    if (it == g_store.end())
        return false;
    if (primary != NULL)
        *primary = it->second.primary;
    if (secondary != NULL)
        *secondary = it->second.secondary;
    return true;
}

void Remove(const std::string& characterId)
{
    Guard lock;
    g_store.erase(characterId);
}

void Clear()
{
    Guard lock;
    g_store.clear();
    g_currentFolder.clear();
}

void SetCurrentSaveFolder(const std::string& folder)
{
    Guard lock;
    g_currentFolder = folder;
}

const std::string& CurrentSaveFolder()
{
    Guard lock;
    return g_currentFolder;
}

bool LoadFromFolder(const std::string& saveFolder)
{
    if (saveFolder.empty())
        return false;

    SetCurrentSaveFolder(saveFolder);

    const std::string filePath = saveFolder + "/" + kFileName;
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        LogDebug("[uniform] " + filePath + " not found; starting with empty uniforms.");
        Guard lock;
        g_store.clear();
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    file.close();

    StoreMap loaded;
    if (!ParseJson(ss.str(), &loaded))
    {
        LogError("[uniform] Failed to parse " + filePath + ".");
        return false;
    }

    {
        Guard lock;
        g_store = loaded;
    }

    LogInfo("[uniform] Loaded uniforms from " + filePath + ".");
    return true;
}

bool SaveToFolder(const std::string& saveFolder)
{
    if (saveFolder.empty())
        return false;

    SetCurrentSaveFolder(saveFolder);

    std::string jsonStr;
    {
        Guard lock;
        jsonStr = SerializeJson(g_store);
    }

    const std::string filePath = saveFolder + "/" + kFileName;
    std::ofstream file(filePath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        LogError("[uniform] Failed to open " + filePath + " for writing.");
        return false;
    }

    file << jsonStr;
    file.close();

    LogInfo("[uniform] Saved uniforms to " + filePath + ".");
    return true;
}

bool InstallHooks()
{
    SaveManager* sm = SaveManager::getSingleton();
    if (sm == NULL)
    {
        LogError("[uniform] SaveManager::getSingleton returned NULL.");
        return false;
    }

    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::loadGame),
                           &LoadGameHook, &g_loadGameOriginal) != KenshiLib::SUCCESS)
    {
        LogError("[uniform] Failed to hook SaveManager::loadGame.");
        return false;
    }

    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::saveGame),
                           &SaveGameHook, &g_saveGameOriginal) != KenshiLib::SUCCESS)
    {
        LogError("[uniform] Failed to hook SaveManager::saveGame.");
        return false;
    }

    LogInfo("[uniform] SaveManager hooks installed successfully.");
    return true;
}

} // namespace UniformStore
} // namespace totaleditor
