#include "common.h"
#include "catalog.h"
#include "anatomy.h"
#include "log.h"

#include <algorithm>

namespace totaleditor
{
namespace
{
std::vector<RaceEntry> g_races;
std::vector<StatEntry> g_stats;
bool g_racesLoaded = false;
bool g_statsLoaded = false;

std::string ToLower(const std::string& text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

bool ByName(const RaceEntry& a, const RaceEntry& b)
{
    return ToLower(a.name) < ToLower(b.name);
}

bool StatByName(const StatEntry& a, const StatEntry& b)
{
    return ToLower(a.name) < ToLower(b.name);
}

// Each guarded helper touches only PODs, because MSVC rejects __try in any
// function whose frame also holds a C++ object needing unwinding -- and it
// counts the whole frame, not just what sits inside the guard.

bool SafeGetDataOfType(itemType type, lektor<GameData*>* list)
{
    TE_TRY
    {
        ou->gamedata.getDataOfType(*list, type);
        return true;
    }
    TE_CATCH { return false; }
}

// name and stringID are std::string members, so only their c_str pointers
// cross the guard; no temporary is constructed inside it.
bool SafeReadNames(GameData* data, const char** name, const char** stringID)
{
    TE_TRY
    {
        *name = data->name.c_str();
        *stringID = data->stringID.c_str();
        return true;
    }
    TE_CATCH { return false; }
}

RaceData* SafeGetRaceData(GameData* data)
{
    TE_TRY { return RaceData::getRaceData(data); }
    TE_CATCH { return NULL; }
}

// getDataOfType fills a lektor the caller owns; the buffer is released here.
void SafeFreeLektor(lektor<GameData*>* list)
{
    TE_TRY
    {
        if (list->stuff != NULL)
        {
            free(list->stuff);
            list->stuff = NULL;
        }
    }
    TE_CATCH { }
}

bool SafeReadBloodRange(RaceData* race, float* minBlood, float* maxBlood)
{
    TE_TRY
    {
        *minBlood = race->originalBloodMin;
        *maxBlood = race->originalBloodMax;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeHasBonuses(RaceData* race, bool* has)
{
    TE_TRY
    {
        *has = !race->statMods.empty();
        return true;
    }
    TE_CATCH { return false; }
}

// Bonuses that actually change something.
//
// "Has a bonus table" is not the same as "has bonuses": a race cloned from
// another carries all thirty-eight multipliers set to exactly 1.0, which is a
// full table that changes nothing. Counting only the entries away from 1.0 is
// what tells a configured race from an inherited blank.
bool SafeCountRealBonuses(RaceData* race, int* count)
{
    TE_TRY
    {
        int found = 0;
        ogre_unordered_map<StatsEnumerated, float>::type::const_iterator it =
            race->statMods.begin();
        for (; it != race->statMods.end(); ++it)
        {
            if (it->second != 1.0f)
            {
                ++found;
            }
        }
        *count = found;
        return true;
    }
    TE_CATCH { return false; }
}

// Whether a race entry is one the player can actually meet.
//
// Kenshi loads 1952 race definitions and only a few dozen are races anyone
// sees. The engine gives no flag for this, so the test has to be built from
// what a real race has and a stub does not -- and two obvious candidates turn
// out to be useless:
//
//   * Blood. Rejected nine of 1952: nearly every stub carries a blood range
//     copied from whatever it was cloned from.
//   * "Has a bonus table". A cloned race carries all thirty-eight multipliers
//     set to exactly 1.0 -- a full table that changes nothing.
//
// So the two conditions that do carry information are a body the engine can
// build a character from, and at least one multiplier away from 1.0. Either
// alone admits stubs; requiring both is what narrows it.
bool LooksReal(RaceData* race, const std::string& name)
{
    if (race == NULL)
    {
        return false;
    }

    // A name that announces itself, checked first because it is the cheapest.
    const std::string lower = ToLower(name);
    if (lower.find("test") != std::string::npos ||
        lower.find("placeholder") != std::string::npos ||
        lower.find("dummy") != std::string::npos ||
        lower.find("unused") != std::string::npos ||
        lower.find("obsolete") != std::string::npos ||
        lower.find("deprecated") != std::string::npos ||
        lower.find("template") != std::string::npos ||
        lower.find("do not use") != std::string::npos)
    {
        return false;
    }

    int realBonuses = 0;
    SafeCountRealBonuses(race, &realBonuses);

    return HasAnatomy(race) && realBonuses > 0;
}

void LoadRaces()
{
    g_races.clear();

    lektor<GameData*> list;
    if (!SafeGetDataOfType(RACE, &list))
    {
        LogError("getDataOfType(RACE) failed; the race list will be empty.");
        return;
    }

    for (unsigned int i = 0; i < list.count; ++i)
    {
        GameData* data = list.stuff[i];
        if (data == NULL)
        {
            continue;
        }

        const char* name = NULL;
        const char* stringID = NULL;
        if (!SafeReadNames(data, &name, &stringID) || name == NULL)
        {
            continue;
        }

        // A race definition the engine has no RaceData for is one it never
        // built -- unreachable for editing, so it does not belong in a dropdown
        // that promises the edit will land somewhere.
        RaceData* race = SafeGetRaceData(data);
        if (race == NULL)
        {
            continue;
        }

        RaceEntry entry;
        entry.name = name;
        entry.stringID = (stringID != NULL) ? stringID : "";
        entry.race = race;
        entry.playable = LooksReal(race, entry.name);
        g_races.push_back(entry);
    }

    SafeFreeLektor(&list);
    std::sort(g_races.begin(), g_races.end(), ByName);

    // Each criterion counted separately: guessing which one narrows the list
    // is what produced a filter that rejected nine races out of 1952.
    int playable = 0;
    int withAnatomy = 0;
    int withBonuses = 0;
    for (size_t i = 0; i < g_races.size(); ++i)
    {
        if (g_races[i].playable)
        {
            ++playable;
        }
        if (HasAnatomy(g_races[i].race))
        {
            ++withAnatomy;
        }
        int bonuses = 0;
        SafeCountRealBonuses(g_races[i].race, &bonuses);
        if (bonuses > 0)
        {
            ++withBonuses;
        }
    }

    char total[16] = { 0 };
    char kept[16] = { 0 };
    char anatomy[16] = { 0 };
    char bonuses[16] = { 0 };
    _snprintf_s(total, sizeof(total), _TRUNCATE, "%d", (int)g_races.size());
    _snprintf_s(kept, sizeof(kept), _TRUNCATE, "%d", playable);
    _snprintf_s(anatomy, sizeof(anatomy), _TRUNCATE, "%d", withAnatomy);
    _snprintf_s(bonuses, sizeof(bonuses), _TRUNCATE, "%d", withBonuses);
    LogInfo("Races loaded: " + std::string(total) + ", kept " + std::string(kept) +
            " (with a body: " + std::string(anatomy) +
            ", with a real bonus: " + std::string(bonuses) + ").");
}

// getStatName returns std::string by value, so the call and the copy out of it
// both live in a helper the guard only jumps through: a guarded frame may hold
// nothing but PODs.
void CopyStatName(int stat, char* buffer, size_t size)
{
    strncpy_s(buffer, size, CharStats::getStatName((StatsEnumerated)stat).c_str(),
              _TRUNCATE);
}

bool SafeCopyStatName(int stat, char* buffer, size_t size)
{
    TE_TRY
    {
        CopyStatName(stat, buffer, size);
        return true;
    }
    TE_CATCH { return false; }
}

// Same reason as the stat names: getName returns std::string by value.
void CopyCharacterName(Character* character, char* buffer, size_t size)
{
    strncpy_s(buffer, size, character->getName().c_str(), _TRUNCATE);
}

bool SafeCopyCharacterName(Character* character, char* buffer, size_t size)
{
    TE_TRY
    {
        CopyCharacterName(character, buffer, size);
        return true;
    }
    TE_CATCH { return false; }
}

void LoadStats()
{
    g_stats.clear();

    // Strictly between the two markers: STAT_NONE is the absent value and
    // everything from STAT_END onward is derived rather than a skill.
    for (int stat = STAT_NONE + 1; stat < STAT_END; ++stat)
    {
        char name[128] = { 0 };
        if (!SafeCopyStatName(stat, name, sizeof(name)) || name[0] == '\0')
        {
            continue;
        }

        StatEntry entry;
        entry.name = name;
        entry.stat = stat;
        g_stats.push_back(entry);
    }

    std::sort(g_stats.begin(), g_stats.end(), StatByName);

    char count[16] = { 0 };
    _snprintf_s(count, sizeof(count), _TRUNCATE, "%d", (int)g_stats.size());
    LogInfo("Editable skills: " + std::string(count) + ".");
}
} // namespace

const std::vector<RaceEntry>& Races(bool reload)
{
    if (reload || !g_racesLoaded)
    {
        LoadRaces();
        g_racesLoaded = true;
    }
    return g_races;
}

const std::vector<StatEntry>& Stats()
{
    if (!g_statsLoaded)
    {
        LoadStats();
        g_statsLoaded = true;
    }
    return g_stats;
}

std::string StatDisplayName(int stat)
{
    // The cached list first: it is the same answer, already resolved.
    const std::vector<StatEntry>& stats = Stats();
    for (size_t i = 0; i < stats.size(); ++i)
    {
        if (stats[i].stat == stat)
        {
            return stats[i].name;
        }
    }

    // Not in the list means the engine gave no name when the list was built.
    // A race can still carry a bonus on it, so ask once more directly.
    char name[128] = { 0 };
    if (SafeCopyStatName(stat, name, sizeof(name)) && name[0] != '\0')
    {
        return std::string(name);
    }

    // Still nothing. The number is not friendly, but it identifies the row --
    // and the player can see that this one bonus is unnamed rather than
    // wondering why several rows look identical.
    char fallback[32] = { 0 };
    _snprintf_s(fallback, sizeof(fallback), _TRUNCATE, "skill #%d", stat);
    return std::string(fallback);
}

RaceData* RaceOf(Character* character)
{
    if (character == NULL)
    {
        return NULL;
    }

    // _NV_ because getRace is virtual: declaring a virtual in plugin code
    // builds a vtable of the plugin's own and calls whatever the engine happens
    // to keep at that slot.
    TE_TRY { return character->_NV_getRace(); }
    TE_CATCH { return NULL; }
}

Character* SelectedCharacter()
{
    TE_TRY
    {
        if (gui == NULL)
        {
            return NULL;
        }
        return gui->selectedObject.getCharacter();
    }
    TE_CATCH { return NULL; }
}

std::string RaceNameOf(RaceData* race)
{
    if (race == NULL)
    {
        return std::string();
    }

    // By pointer, not by name: within one run there is exactly one RaceData per
    // race, which is the same fact that makes these bonuses global.
    const std::vector<RaceEntry>& races = Races(false);
    for (size_t i = 0; i < races.size(); ++i)
    {
        if (races[i].race == race)
        {
            return races[i].name;
        }
    }
    return std::string();
}

std::string CharacterName(Character* character)
{
    if (character == NULL)
    {
        return std::string();
    }

    char buffer[128] = { 0 };
    if (!SafeCopyCharacterName(character, buffer, sizeof(buffer)))
    {
        return std::string();
    }
    return std::string(buffer);
}

void ClearCatalogCache()
{
    g_races.clear();
    g_racesLoaded = false;
}
} // namespace totaleditor
