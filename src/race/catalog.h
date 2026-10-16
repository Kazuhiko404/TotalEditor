#pragma once

#include <string>
#include <vector>

class GameData;
class RaceData;
class Character;

namespace totaleditor
{
// One race, as the dropdown needs it: a name to show and the engine object the
// edits land on.
struct RaceEntry
{
    std::string name;
    std::string stringID;
    RaceData* race;

    // False for the placeholders and test entries Kenshi's data is full of:
    // definitions with no body and nothing to edit. They still load, so the
    // dropdown would list them beside the real races and bury them.
    bool playable;

    RaceEntry() : race(NULL), playable(false) {}
};

// One skill, as the dropdowns need it. The engine numbers these; the name comes
// from CharStats::getStatName, so the list never hardcodes the 38 strings and
// cannot drift from the engine's own naming.
struct StatEntry
{
    std::string name;
    int stat; // StatsEnumerated, kept as int so this header stays engine-free.

    StatEntry() : stat(0) {}
};

// Every race in the loaded game data, sorted by name.
//
// This walks GameData rather than RaceData::AllRaces: that static map is
// declared in the header but exports no symbol, so it cannot be linked against.
// getDataOfType(RACE) plus getRaceData(GameData*) reaches the same objects
// through the public API.
const std::vector<RaceEntry>& Races(bool reload);

// Every real skill: the enumerators strictly between STAT_NONE and STAT_END.
//
// The values past STAT_END are derived -- carry weight, weapon damage and the
// like -- which the engine recomputes every frame. Offering them would let the
// player edit numbers that are overwritten before they see the effect.
const std::vector<StatEntry>& Stats();

// The engine's name for one skill, asked directly rather than looked up in the
// list above.
//
// Stats() drops any skill the engine gives no name for, but a race can still
// carry a bonus on one -- and a row reading "?" tells the player nothing about
// what they are editing. This asks the engine again and, failing that, falls
// back to the enumerator's number, which at least identifies the row.
std::string StatDisplayName(int stat);

// The race a character belongs to, or NULL. Calls the _NV_ thunk, because
// getRace is virtual and a plugin declaring a virtual builds a vtable of its
// own and calls whatever lives at that slot.
RaceData* RaceOf(Character* character);

// The character the player currently has selected, or NULL when the selection
// is empty or is not a character.
Character* SelectedCharacter();

// The listed name of a race object, or an empty string when it is not one of
// the races the catalogue knows. Resolves through the same list the dropdowns
// use, so what the character panel names is exactly what the race panel edits.
std::string RaceNameOf(RaceData* race);

// A character's display name, or an empty string when it cannot be read.
std::string CharacterName(Character* character);

void ClearCatalogCache();
} // namespace totaleditor
