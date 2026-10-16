#pragma once

#include <map>
#include <string>
#include <vector>

class RaceData;

namespace totaleditor
{
// The physical traits of a race, beside its skill bonuses.
//
// These are what make a Skeleton not bleed and a Hive member eat differently:
// blood, healing, speed, hunger and the flags the engine branches on. Copying
// a race without them copies only half of it, which is why they travel with the
// bonuses and are stored alongside them.
//
// Two fields of RaceData are deliberately absent. `data` is the race's own
// GameData -- copying it would tell a race it is a different race. `raceGroup`
// points at RaceGroupData, which no header in the SDK defines, so it cannot be
// handled safely.
struct RaceTraits
{
    float runSpeedMaxSkill;
    float runSpeedMinSkill;
    float originalBloodMin;
    float originalBloodMax;
    float healRate;
    float bleedRate;
    float swimSpeed;
    float walkSpeed;
    float swimOffset;
    float waterAvoidance;
    float hungerRate;
    float visionMultiplier;

    bool gigantic;
    bool swims;
    bool carriable;
    bool singleGender;
    bool robot;
    bool vampiric;
    bool noHats;
    bool noShirts;
    bool noShoes;
    bool canGoIndoors;

    int extraAttackSlots;
    int firstAidSkill;   // StatsEnumerated, kept as int so this header stays engine-free.

    float bloodColourR;
    float bloodColourG;
    float bloodColourB;
    float bloodColourA;

    // False until something has actually been read into it, so a stored race
    // with no traits recorded is told apart from one whose traits are all zero.
    bool valid;

    RaceTraits()
        : runSpeedMaxSkill(0.0f), runSpeedMinSkill(0.0f)
        , originalBloodMin(0.0f), originalBloodMax(0.0f)
        , healRate(0.0f), bleedRate(0.0f)
        , swimSpeed(0.0f), walkSpeed(0.0f), swimOffset(0.0f), waterAvoidance(0.0f)
        , hungerRate(0.0f), visionMultiplier(0.0f)
        , gigantic(false), swims(false), carriable(false), singleGender(false)
        , robot(false), vampiric(false), noHats(false), noShirts(false)
        , noShoes(false), canGoIndoors(false)
        , extraAttackSlots(0), firstAidSkill(0)
        , bloodColourR(0.0f), bloodColourG(0.0f), bloodColourB(0.0f), bloodColourA(0.0f)
        , valid(false)
    {
    }
};

// One race bonus: the skill it multiplies and by how much.
struct BonusEntry
{
    int stat;      // StatsEnumerated
    float value;   // A multiplier: 1.15 means "fifteen percent more".

    BonusEntry() : stat(0), value(1.0f) {}
    BonusEntry(int s, float v) : stat(s), value(v) {}
};

// Race edits do not survive a reload.
//
// RaceData is rebuilt from the FCS data every time the game starts, and
// AllRaces is only a runtime cache of that. So the mod keeps its own file: the
// edits to reapply, and -- written once, the first time a race is touched --
// the values that race shipped with, which is the only way back to vanilla
// after the engine has forgotten them.
//
// Every function here works on names rather than RaceData pointers, because the
// pointers are different objects on the next run and the names are not.

// Loads the store from disk. A missing file is the normal first run.
void LoadStore(const std::string& modDirectory);

// Writes the store. Returns false when the file could not be written, which the
// caller reports rather than swallowing: an edit the player believes is saved
// and is not would come back as a bug report about the mod forgetting things.
bool SaveStore();

// Records a race's shipped bonuses, once. Later calls for the same race do
// nothing, so the baseline is always what the game started with rather than
// whatever the last edit left behind.
void RememberOriginal(const std::string& raceName, RaceData* race);

// Whether the mod has a stored baseline for this race.
bool HasOriginal(const std::string& raceName);

// The bonuses this race shipped with. Empty when nothing was recorded.
const std::vector<BonusEntry>& OriginalBonuses(const std::string& raceName);

// The traits this race shipped with. Invalid when nothing was recorded, which
// is what stops a revert writing zeroes over a working race.
const RaceTraits& OriginalTraits(const std::string& raceName);

// The traits stored as an override, if the race has one.
const RaceTraits& OverrideTraits(const std::string& raceName);
bool HasOverrideTraits(const std::string& raceName);

// Records the current bonuses of a race as the set to reapply on the next run.
void RecordOverride(const std::string& raceName, RaceData* race);

// Drops a race's override, so it stops being reapplied. The baseline is kept:
// reverting is not the same as forgetting what vanilla looked like.
void ForgetOverride(const std::string& raceName);

// Writes the race's recorded body parts back onto it, and reports how many
// changed. Zero when no baseline was captured -- a store written before parts
// were saved has none, and neither does a race this session never touched.
//
// Parts are matched by name: the index one sat at belongs to the run that read
// it, and the live list supplies the current one.
int RestoreOriginalParts(const std::string& raceName, RaceData* race);

// Names of every race carrying an override.
std::vector<std::string> OverriddenRaces();

// Applies every stored override to the races currently loaded. Called at
// startup, which is what makes the edits appear to persist across sessions.
// Returns how many races were restored.
int ApplyStoredOverrides();
} // namespace totaleditor
