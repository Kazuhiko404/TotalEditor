#include "common.h"
#include "anatomy.h"
#include "config.h"
#include "log.h"

#include <kenshi/util/TripleInt.h>
#include <kenshi/Character.h>
#include <kenshi/MedicalSystem.h>

namespace totaleditor
{
namespace
{
// The FCS calls this list "combat anatomy", but the label it shows and the key
// the data uses are not always the same string, and GameData offers no way to
// enumerate the lists an object carries. So the name is discovered once, by
// asking listExists for each candidate, and remembered.
//
// The spellings follow the convention every other list in the engine uses --
// lower case, words separated by spaces -- which is what KEP's calls to
// getAllFromListAsDatas show ("animation files", "weapon crafts", "leader").
const char* const kCandidateNames[] =
{
    "combat anatomy",
    "anatomy",
    "body parts",
    "bodyparts",
    "combat_anatomy",
    "body part",
    "limbs"
};
const int kCandidateCount = 7;

std::string g_listName;
bool g_listSearched = false;

// Every list call takes a const std::string&, so passing a char* builds a
// temporary -- and C2712 counts any object in the frame, not just the ones
// inside the guard. Each of these is therefore a pair: a plain function free to
// build the string, and a guarded one holding nothing but pointers.

bool ListExists(GameData* data, const char* name)
{
    return data->listExistsAndNotEmpty(name);
}

bool SafeListExists(GameData* data, const char* name, bool* exists)
{
    TE_TRY
    {
        *exists = ListExists(data, name);
        return true;
    }
    TE_CATCH { return false; }
}

int ListSize(GameData* data, const char* name)
{
    return data->getListSize(name);
}

bool SafeListSize(GameData* data, const char* name, int* size)
{
    TE_TRY
    {
        *size = ListSize(data, name);
        return true;
    }
    TE_CATCH { return false; }
}

// getFromList hands back the entry's string ID -- "31-gamedata.quack" -- which
// is what the list stores, not something to show a player. The ID is resolved
// through the data container to reach the body part's own GameData, whose name
// is the readable one ("Right arm").
void CopyEntryName(GameData* data, const char* list, int index,
                   char* buffer, size_t size)
{
    const std::string& stringID = data->getFromList(list, index);

    GameData* part = ou->gamedata.getData(stringID);
    if (part != NULL && !part->name.empty())
    {
        strncpy_s(buffer, size, part->name.c_str(), _TRUNCATE);
        return;
    }

    // No entry for that ID. The ID itself is still better than nothing: it
    // identifies the row even if it does not read well.
    strncpy_s(buffer, size, stringID.c_str(), _TRUNCATE);
}

bool SafeCopyEntryName(GameData* data, const char* list, int index,
                       char* buffer, size_t size)
{
    TE_TRY
    {
        CopyEntryName(data, list, index, buffer, size);
        return true;
    }
    TE_CATCH { return false; }
}

// The two numbers of one entry. TripleInt is three public ints, so reading them
// needs no exported symbol -- which matters, because its operator[] does not
// link even though the header declares it.
void ReadValues(GameData* data, const char* list, int index,
                int* first, int* second, int* third)
{
    const TripleInt& value = data->getValueFromList(list, index);
    *first = value.value[0];
    *second = value.value[1];
    *third = value.value[2];
}

bool SafeReadValues(GameData* data, const char* list, int index,
                    int* first, int* second, int* third)
{
    TE_TRY
    {
        ReadValues(data, list, index, first, second, third);
        return true;
    }
    TE_CATCH { return false; }
}

// Writing goes through the same reference. getValueFromList is const, so the
// const is cast away -- the object it returns is the engine's own storage, and
// TripleInt::value is a public array.
void WriteValue(GameData* data, const char* list, int index, int slot, int value)
{
    const TripleInt& readOnly = data->getValueFromList(list, index);
    TripleInt* writable = const_cast<TripleInt*>(&readOnly);
    writable->value[slot] = value;
}

bool SafeWriteValue(GameData* data, const char* list, int index, int slot, int value)
{
    TE_TRY
    {
        WriteValue(data, list, index, slot, value);
        return true;
    }
    TE_CATCH { return false; }
}

GameData* SafeRaceData(RaceData* race)
{
    TE_TRY { return race->data; }
    TE_CATCH { return NULL; }
}

// Finds which of the candidate names this build actually uses, once.
//
// A race with no anatomy list is not a failure: plenty of race definitions
// never receive a body. The search only concludes when some race answers yes.
void FindListName(GameData* data)
{
    for (int i = 0; i < kCandidateCount; ++i)
    {
        bool exists = false;
        if (SafeListExists(data, kCandidateNames[i], &exists) && exists)
        {
            g_listName = kCandidateNames[i];
            g_listSearched = true;
            LogInfo("Anatomy list found under the name: '" + g_listName + "'.");
            return;
        }
    }
}
} // namespace

std::vector<BodyPart> ReadAnatomy(RaceData* race)
{
    std::vector<BodyPart> parts;
    if (race == NULL)
    {
        return parts;
    }

    GameData* data = SafeRaceData(race);
    if (data == NULL)
    {
        return parts;
    }

    if (!g_listSearched)
    {
        FindListName(data);
    }
    if (g_listName.empty())
    {
        // Nothing found yet. Try this race too: the first race looked at may
        // simply have had no body.
        FindListName(data);
        if (g_listName.empty())
        {
            return parts;
        }
    }

    int count = 0;
    if (!SafeListSize(data, g_listName.c_str(), &count) || count <= 0)
    {
        return parts;
    }

    for (int i = 0; i < count; ++i)
    {
        char name[128] = { 0 };
        SafeCopyEntryName(data, g_listName.c_str(), i, name, sizeof(name));

        int first = 0;
        int second = 0;
        int third = 0;
        if (!SafeReadValues(data, g_listName.c_str(), i, &first, &second, &third))
        {
            continue;
        }

        BodyPart part;
        part.name = (name[0] != '\0') ? name : "part";
        // The FCS shows hit chance first and health second; the third slot is
        // unused by this list.
        part.hitChance = first;
        part.health = second;
        part.index = i;
        parts.push_back(part);
    }

    return parts;
}

bool WritePartHealth(RaceData* race, int index, int health)
{
    if (race == NULL || index < 0 || g_listName.empty())
    {
        return false;
    }

    GameData* data = SafeRaceData(race);
    if (data == NULL)
    {
        return false;
    }

    const Config& config = GetConfig();
    if (health < config.partHealthMin) { health = config.partHealthMin; }
    if (health > config.partHealthMax) { health = config.partHealthMax; }

    return SafeWriteValue(data, g_listName.c_str(), index, 1, health);
}

namespace
{
// One part of a living character brought in line with its entry in the race's
// list. The match is by GameData pointer: the character's part remembers which
// body-part definition it was built from, and so does the list entry.
// Resolving a list entry builds a std::string for the list name, so the call
// lives in a plain function and only the guard crosses into it.
GameData* EntryAt(GameData* raceData, const char* list, int index)
{
    return raceData->getFromListAsData(list, index, &ou->gamedata, raceData->type);
}

void ApplyPart(GameData* raceData, const char* list, int index,
               MedicalSystem::HealthPartStatus* part)
{
    const TripleInt& values = raceData->getValueFromList(list, index);
    part->_maxHealth = (float)values.value[1];

    // Raising the maximum alone leaves a wounded part healing to the old
    // ceiling -- what the modding forums warn about -- so the damage goes too.
    // These are the fields KEP clears when it restores a limb.
    part->hitChanceMult = 1.0f;
    part->wearDamage = 0.0f;
    part->flesh = part->maxHealth();
    part->fleshStun = 0.0f;
    part->bandaging = 0.0f;
    part->juryRigging = 0.0f;

    part->updateDerivedHealths();
}

bool SafeApplyPart(GameData* raceData, const char* list, int count,
                   MedicalSystem::HealthPartStatus* part)
{
    if (part == NULL)
    {
        return false;
    }

    TE_TRY
    {
        if (part->data == NULL)
        {
            return false;
        }

        for (int i = 0; i < count; ++i)
        {
            if (EntryAt(raceData, list, i) != part->data)
            {
                continue;
            }
            ApplyPart(raceData, list, i, part);
            return true;
        }
        return false;
    }
    TE_CATCH { return false; }
}

bool SafePartCount(Character* character, int* count)
{
    TE_TRY
    {
        *count = (int)character->medical.anatomy.count;
        return true;
    }
    TE_CATCH { return false; }
}

MedicalSystem::HealthPartStatus* SafePartAt(Character* character, int index)
{
    TE_TRY { return character->medical.anatomy.stuff[index]; }
    TE_CATCH { return NULL; }
}
} // namespace

int ApplyAnatomyToCharacter(Character* character, RaceData* race)
{
    if (character == NULL || race == NULL || g_listName.empty())
    {
        return 0;
    }

    GameData* data = SafeRaceData(race);
    if (data == NULL)
    {
        return 0;
    }

    int listCount = 0;
    if (!SafeListSize(data, g_listName.c_str(), &listCount) || listCount <= 0)
    {
        return 0;
    }

    int partCount = 0;
    if (!SafePartCount(character, &partCount) || partCount <= 0)
    {
        return 0;
    }

    int applied = 0;
    for (int i = 0; i < partCount; ++i)
    {
        MedicalSystem::HealthPartStatus* part = SafePartAt(character, i);
        if (SafeApplyPart(data, g_listName.c_str(), listCount, part))
        {
            ++applied;
        }
    }
    return applied;
}

bool HasAnatomy(RaceData* race)
{
    if (race == NULL)
    {
        return false;
    }

    GameData* data = SafeRaceData(race);
    if (data == NULL)
    {
        return false;
    }

    if (g_listName.empty())
    {
        FindListName(data);
        if (g_listName.empty())
        {
            return false;
        }
    }

    int count = 0;
    return SafeListSize(data, g_listName.c_str(), &count) && count > 0;
}

int TotalHitWeight(const std::vector<BodyPart>& parts)
{
    int total = 0;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        total += parts[i].hitChance;
    }
    return total;
}

bool WritePartHitChance(RaceData* race, int index, int hitChance)
{
    if (race == NULL || index < 0 || g_listName.empty())
    {
        return false;
    }

    GameData* data = SafeRaceData(race);
    if (data == NULL)
    {
        return false;
    }

    // Zero is allowed -- it means a part the engine never targets -- but a
    // negative weight would corrupt the total every other part is measured
    // against.
    if (hitChance < 0)
    {
        hitChance = 0;
    }

    return SafeWriteValue(data, g_listName.c_str(), index, 0, hitChance);
}

const std::string& AnatomyListName()
{
    return g_listName;
}

} // namespace totaleditor
