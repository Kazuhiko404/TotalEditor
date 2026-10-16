#include "common.h"
#include "store.h"
#include "anatomy.h"
#include "catalog.h"
#include "json.h"
#include "log.h"
#include "race_edit.h"

namespace totaleditor
{
namespace
{
// Keyed by race name rather than by RaceData*, because the pointers are
// different objects on the next run and the names are not.
std::map<std::string, std::vector<BonusEntry> > g_originals;
std::map<std::string, std::vector<BonusEntry> > g_overrides;

// Traits travel with the bonuses, keyed the same way. Separate maps rather than
// one struct so a store written before traits existed still loads: its bonuses
// come back and its traits simply read as invalid.
std::map<std::string, RaceTraits> g_originalTraits;
std::map<std::string, RaceTraits> g_overrideTraits;

// Body parts, keyed by race and then by part NAME rather than by position. The
// index a part sits at is an implementation detail of one run; the name is what
// still identifies it after a reload, and what lets a store written for one
// race be read back against a list the engine rebuilt.
//
// Anatomy lives in the race's GameData, not in RaceData, so it is not covered
// by the traits above and needs its own pair of maps.
typedef std::map<std::string, BodyPart> PartsByName;
std::map<std::string, PartsByName> g_originalParts;
std::map<std::string, PartsByName> g_overrideParts;

std::string g_path;
bool g_loaded = false;

const std::vector<BonusEntry>& EmptyBonuses()
{
    static std::vector<BonusEntry> empty;
    return empty;
}

const RaceTraits& EmptyTraits()
{
    static RaceTraits empty;
    return empty;
}

// Traits are stored by name, unlike the number-keyed bonuses: these are fixed
// fields of the engine rather than an open-ended set, and a named field keeps
// the file readable for anyone editing it by hand.
JsonValue TraitsToJson(const RaceTraits& t)
{
    JsonValue o = JsonValue::MakeObject();
    o.Set("runSpeedMaxSkill", JsonValue::MakeNumber(t.runSpeedMaxSkill));
    o.Set("runSpeedMinSkill", JsonValue::MakeNumber(t.runSpeedMinSkill));
    o.Set("bloodMin",         JsonValue::MakeNumber(t.originalBloodMin));
    o.Set("bloodMax",         JsonValue::MakeNumber(t.originalBloodMax));
    o.Set("healRate",         JsonValue::MakeNumber(t.healRate));
    o.Set("bleedRate",        JsonValue::MakeNumber(t.bleedRate));
    o.Set("swimSpeed",        JsonValue::MakeNumber(t.swimSpeed));
    o.Set("walkSpeed",        JsonValue::MakeNumber(t.walkSpeed));
    o.Set("swimOffset",       JsonValue::MakeNumber(t.swimOffset));
    o.Set("waterAvoidance",   JsonValue::MakeNumber(t.waterAvoidance));
    o.Set("hungerRate",       JsonValue::MakeNumber(t.hungerRate));
    o.Set("visionMultiplier", JsonValue::MakeNumber(t.visionMultiplier));
    o.Set("gigantic",     JsonValue::MakeBool(t.gigantic));
    o.Set("swims",        JsonValue::MakeBool(t.swims));
    o.Set("carriable",    JsonValue::MakeBool(t.carriable));
    o.Set("singleGender", JsonValue::MakeBool(t.singleGender));
    o.Set("robot",        JsonValue::MakeBool(t.robot));
    o.Set("vampiric",     JsonValue::MakeBool(t.vampiric));
    o.Set("noHats",       JsonValue::MakeBool(t.noHats));
    o.Set("noShirts",     JsonValue::MakeBool(t.noShirts));
    o.Set("noShoes",      JsonValue::MakeBool(t.noShoes));
    o.Set("canGoIndoors", JsonValue::MakeBool(t.canGoIndoors));
    o.Set("extraAttackSlots", JsonValue::MakeNumber(t.extraAttackSlots));
    o.Set("firstAidSkill",    JsonValue::MakeNumber(t.firstAidSkill));
    o.Set("bloodColourR", JsonValue::MakeNumber(t.bloodColourR));
    o.Set("bloodColourG", JsonValue::MakeNumber(t.bloodColourG));
    o.Set("bloodColourB", JsonValue::MakeNumber(t.bloodColourB));
    o.Set("bloodColourA", JsonValue::MakeNumber(t.bloodColourA));
    return o;
}

RaceTraits TraitsFromJson(const JsonValue& o)
{
    RaceTraits t;
    if (!o.isObject())
    {
        return t;
    }

    t.runSpeedMaxSkill = (float)o.Child("runSpeedMaxSkill").AsNumber(0.0);
    t.runSpeedMinSkill = (float)o.Child("runSpeedMinSkill").AsNumber(0.0);
    t.originalBloodMin = (float)o.Child("bloodMin").AsNumber(0.0);
    t.originalBloodMax = (float)o.Child("bloodMax").AsNumber(0.0);
    t.healRate         = (float)o.Child("healRate").AsNumber(0.0);
    t.bleedRate        = (float)o.Child("bleedRate").AsNumber(0.0);
    t.swimSpeed        = (float)o.Child("swimSpeed").AsNumber(0.0);
    t.walkSpeed        = (float)o.Child("walkSpeed").AsNumber(0.0);
    t.swimOffset       = (float)o.Child("swimOffset").AsNumber(0.0);
    t.waterAvoidance   = (float)o.Child("waterAvoidance").AsNumber(0.0);
    t.hungerRate       = (float)o.Child("hungerRate").AsNumber(0.0);
    t.visionMultiplier = (float)o.Child("visionMultiplier").AsNumber(0.0);
    t.gigantic     = o.Child("gigantic").AsBool(false);
    t.swims        = o.Child("swims").AsBool(false);
    t.carriable    = o.Child("carriable").AsBool(false);
    t.singleGender = o.Child("singleGender").AsBool(false);
    t.robot        = o.Child("robot").AsBool(false);
    t.vampiric     = o.Child("vampiric").AsBool(false);
    t.noHats       = o.Child("noHats").AsBool(false);
    t.noShirts     = o.Child("noShirts").AsBool(false);
    t.noShoes      = o.Child("noShoes").AsBool(false);
    t.canGoIndoors = o.Child("canGoIndoors").AsBool(false);
    t.extraAttackSlots = (int)o.Child("extraAttackSlots").AsNumber(0.0);
    t.firstAidSkill    = (int)o.Child("firstAidSkill").AsNumber(0.0);
    t.bloodColourR = (float)o.Child("bloodColourR").AsNumber(0.0);
    t.bloodColourG = (float)o.Child("bloodColourG").AsNumber(0.0);
    t.bloodColourB = (float)o.Child("bloodColourB").AsNumber(0.0);
    t.bloodColourA = (float)o.Child("bloodColourA").AsNumber(0.0);
    t.valid = true;
    return t;
}

void ReadTraitSection(const JsonValue& root, const char* name,
                      std::map<std::string, RaceTraits>* target)
{
    target->clear();

    const JsonValue& section = root.Child(name);
    if (!section.isObject())
    {
        return;
    }

    const std::vector<std::string>& races = section.Keys();
    for (size_t i = 0; i < races.size(); ++i)
    {
        (*target)[races[i]] = TraitsFromJson(section.Child(races[i]));
    }
}

// The race's anatomy as a name-keyed map, which is how it is stored.
PartsByName PartsOf(RaceData* race)
{
    PartsByName parts;
    const std::vector<BodyPart> live = ReadAnatomy(race);
    for (size_t i = 0; i < live.size(); ++i)
    {
        parts[live[i].name] = live[i];
    }
    return parts;
}

// Writes a stored set of parts back onto a race, matching by name and ignoring
// any the race does not have. The stored index is meaningless across runs, so
// the live list supplies it.
int ApplyPartsTo(RaceData* race, const PartsByName& parts)
{
    if (race == NULL || parts.empty())
    {
        return 0;
    }

    int written = 0;
    const std::vector<BodyPart> live = ReadAnatomy(race);
    for (size_t i = 0; i < live.size(); ++i)
    {
        PartsByName::const_iterator found = parts.find(live[i].name);
        if (found == parts.end())
        {
            continue;
        }

        if (WritePartHealth(race, live[i].index, found->second.health) &&
            WritePartHitChance(race, live[i].index, found->second.hitChance))
        {
            ++written;
        }
    }
    return written;
}

// One race's parts: a name to its two numbers. The index is deliberately not
// written -- it is where the part sat in this run's list, and the part is found
// by name when the values are applied.
JsonValue PartsToJson(const PartsByName& parts)
{
    JsonValue o = JsonValue::MakeObject();
    PartsByName::const_iterator it;
    for (it = parts.begin(); it != parts.end(); ++it)
    {
        JsonValue part = JsonValue::MakeObject();
        part.Set("health",    JsonValue::MakeNumber((double)it->second.health));
        part.Set("hitChance", JsonValue::MakeNumber((double)it->second.hitChance));
        o.Set(it->first, part);
    }
    return o;
}

PartsByName PartsFromJson(const JsonValue& value)
{
    PartsByName parts;
    if (!value.isObject())
    {
        return parts;
    }

    const std::vector<std::string>& names = value.Keys();
    for (size_t i = 0; i < names.size(); ++i)
    {
        const JsonValue& part = value.Child(names[i]);

        BodyPart entry;
        entry.name = names[i];
        entry.health = (int)part.Child("health").AsNumber(0.0);
        entry.hitChance = (int)part.Child("hitChance").AsNumber(0.0);
        // Resolved against the live race when the values are written back.
        entry.index = -1;
        parts[names[i]] = entry;
    }
    return parts;
}

void ReadPartSection(const JsonValue& root, const char* name,
                     std::map<std::string, PartsByName>* target)
{
    target->clear();

    const JsonValue& section = root.Child(name);
    if (!section.isObject())
    {
        return;
    }

    const std::vector<std::string>& races = section.Keys();
    for (size_t i = 0; i < races.size(); ++i)
    {
        (*target)[races[i]] = PartsFromJson(section.Child(races[i]));
    }
}

JsonValue WritePartSection(const std::map<std::string, PartsByName>& source)
{
    JsonValue section = JsonValue::MakeObject();
    std::map<std::string, PartsByName>::const_iterator it;
    for (it = source.begin(); it != source.end(); ++it)
    {
        if (!it->second.empty())
        {
            section.Set(it->first, PartsToJson(it->second));
        }
    }
    return section;
}

JsonValue WriteTraitSection(const std::map<std::string, RaceTraits>& source)
{
    JsonValue section = JsonValue::MakeObject();
    std::map<std::string, RaceTraits>::const_iterator it;
    for (it = source.begin(); it != source.end(); ++it)
    {
        if (it->second.valid)
        {
            section.Set(it->first, TraitsToJson(it->second));
        }
    }
    return section;
}

// Bonuses are stored as an object keyed by the skill's own number rather than
// by its name: the engine owns the numbering, and a renamed skill should not
// silently detach a saved bonus from the skill it belongs to.
JsonValue BonusesToJson(const std::vector<BonusEntry>& bonuses)
{
    JsonValue object = JsonValue::MakeObject();
    for (size_t i = 0; i < bonuses.size(); ++i)
    {
        char key[16] = { 0 };
        _snprintf_s(key, sizeof(key), _TRUNCATE, "%d", bonuses[i].stat);
        object.Set(key, JsonValue::MakeNumber((double)bonuses[i].value));
    }
    return object;
}

std::vector<BonusEntry> BonusesFromJson(const JsonValue& object)
{
    std::vector<BonusEntry> out;
    if (!object.isObject())
    {
        return out;
    }

    const std::vector<std::string>& keys = object.Keys();
    for (size_t i = 0; i < keys.size(); ++i)
    {
        const int stat = atoi(keys[i].c_str());
        if (stat <= STAT_NONE || stat >= STAT_END)
        {
            continue;
        }
        out.push_back(BonusEntry(
            stat, (float)object.Child(keys[i]).AsNumber(1.0)));
    }
    return out;
}

void ReadSection(const JsonValue& root, const char* name,
                 std::map<std::string, std::vector<BonusEntry> >* target)
{
    target->clear();

    const JsonValue& section = root.Child(name);
    if (!section.isObject())
    {
        return;
    }

    const std::vector<std::string>& races = section.Keys();
    for (size_t i = 0; i < races.size(); ++i)
    {
        (*target)[races[i]] = BonusesFromJson(section.Child(races[i]));
    }
}

JsonValue WriteSection(const std::map<std::string, std::vector<BonusEntry> >& source)
{
    JsonValue section = JsonValue::MakeObject();
    std::map<std::string, std::vector<BonusEntry> >::const_iterator it;
    for (it = source.begin(); it != source.end(); ++it)
    {
        section.Set(it->first, BonusesToJson(it->second));
    }
    return section;
}

RaceData* FindRace(const std::string& name)
{
    const std::vector<RaceEntry>& races = Races(false);
    for (size_t i = 0; i < races.size(); ++i)
    {
        if (races[i].name == name)
        {
            return races[i].race;
        }
    }
    return NULL;
}
} // namespace

void LoadStore(const std::string& modDirectory)
{
    g_originals.clear();
    g_overrides.clear();
    g_loaded = false;

    if (modDirectory.empty())
    {
        LogError("No mod directory; edits cannot be saved this session.");
        return;
    }

    g_path = modDirectory + "\\races.json";

    std::string text;
    if (!ReadFile(g_path, &text))
    {
        // The normal first run: nothing has been edited yet.
        g_loaded = true;
        LogInfo("No races.json yet; starting from the game's own values.");
        return;
    }

    JsonValue root;
    if (!ParseJson(text, &root) || !root.isObject())
    {
        // Refusing to load is the safe answer: overwriting a broken file would
        // destroy the only record of what vanilla looked like.
        LogError("races.json is malformed; it will not be loaded or overwritten. "
                 "Fix or delete it to resume saving.");
        return;
    }

    ReadSection(root, "originals", &g_originals);
    ReadSection(root, "overrides", &g_overrides);
    ReadTraitSection(root, "originalTraits", &g_originalTraits);
    ReadTraitSection(root, "overrideTraits", &g_overrideTraits);
    ReadPartSection(root, "originalParts", &g_originalParts);
    ReadPartSection(root, "overrideParts", &g_overrideParts);
    g_loaded = true;

    char count[16] = { 0 };
    _snprintf_s(count, sizeof(count), _TRUNCATE, "%d", (int)g_overrides.size());
    LogInfo("races.json loaded (" + std::string(count) + " races overridden).");
}

bool SaveStore()
{
    if (!g_loaded)
    {
        LogError("Not saving: races.json failed to load, and writing now would "
                 "destroy the stored original values.");
        return false;
    }

    if (g_path.empty())
    {
        LogError("Not saving: no mod directory was resolved at startup.");
        return false;
    }

    JsonValue root = JsonValue::MakeObject();
    root.Set("originals", WriteSection(g_originals));
    root.Set("overrides", WriteSection(g_overrides));
    root.Set("originalTraits", WriteTraitSection(g_originalTraits));
    root.Set("overrideTraits", WriteTraitSection(g_overrideTraits));
    root.Set("originalParts", WritePartSection(g_originalParts));
    root.Set("overrideParts", WritePartSection(g_overrideParts));

    if (!WriteFile(g_path, root.Serialise(0) + "\n"))
    {
        LogError("Could not write races.json; this session's edits are not saved.");
        return false;
    }

    LogInfo("races.json saved.");
    return true;
}

void RememberOriginal(const std::string& raceName, RaceData* race)
{
    if (raceName.empty() || race == NULL)
    {
        return;
    }

    // Once only. A second call would record whatever the last edit left behind
    // as though it were the shipped value, and revert would stop meaning
    // anything.
    if (g_originals.find(raceName) != g_originals.end())
    {
        return;
    }

    g_originals[raceName] = ReadBonuses(race);
    g_originalTraits[raceName] = ReadTraits(race);
    g_originalParts[raceName] = PartsOf(race);
}

bool HasOriginal(const std::string& raceName)
{
    return g_originals.find(raceName) != g_originals.end();
}

const std::vector<BonusEntry>& OriginalBonuses(const std::string& raceName)
{
    std::map<std::string, std::vector<BonusEntry> >::const_iterator found =
        g_originals.find(raceName);
    return (found != g_originals.end()) ? found->second : EmptyBonuses();
}

void RecordOverride(const std::string& raceName, RaceData* race)
{
    if (raceName.empty() || race == NULL)
    {
        return;
    }
    g_overrides[raceName] = ReadBonuses(race);
    g_overrideTraits[raceName] = ReadTraits(race);
    g_overrideParts[raceName] = PartsOf(race);
}

void ForgetOverride(const std::string& raceName)
{
    g_overrides.erase(raceName);
    g_overrideTraits.erase(raceName);
    g_overrideParts.erase(raceName);
}

int RestoreOriginalParts(const std::string& raceName, RaceData* race)
{
    std::map<std::string, PartsByName>::const_iterator found =
        g_originalParts.find(raceName);
    if (found == g_originalParts.end())
    {
        return 0;
    }
    return ApplyPartsTo(race, found->second);
}

const RaceTraits& OriginalTraits(const std::string& raceName)
{
    std::map<std::string, RaceTraits>::const_iterator found =
        g_originalTraits.find(raceName);
    return (found != g_originalTraits.end()) ? found->second : EmptyTraits();
}

const RaceTraits& OverrideTraits(const std::string& raceName)
{
    std::map<std::string, RaceTraits>::const_iterator found =
        g_overrideTraits.find(raceName);
    return (found != g_overrideTraits.end()) ? found->second : EmptyTraits();
}

bool HasOverrideTraits(const std::string& raceName)
{
    std::map<std::string, RaceTraits>::const_iterator found =
        g_overrideTraits.find(raceName);
    return (found != g_overrideTraits.end()) && found->second.valid;
}

std::vector<std::string> OverriddenRaces()
{
    std::vector<std::string> names;
    std::map<std::string, std::vector<BonusEntry> >::const_iterator it;
    for (it = g_overrides.begin(); it != g_overrides.end(); ++it)
    {
        names.push_back(it->first);
    }
    return names;
}

int ApplyStoredOverrides()
{
    int applied = 0;
    std::map<std::string, std::vector<BonusEntry> >::const_iterator it;
    for (it = g_overrides.begin(); it != g_overrides.end(); ++it)
    {
        RaceData* race = FindRace(it->first);
        if (race == NULL)
        {
            // A race from a mod that is no longer loaded. Its override stays in
            // the file: removing it would lose the player's edit the moment
            // they played one session without that mod.
            LogInfo("Override kept but not applied, race not loaded: " + it->first);
            continue;
        }

        // The baseline has to be captured before the override lands on top of
        // it, or the engine's own values are gone for good.
        RememberOriginal(it->first, race);

        if (ReplaceBonuses(race, it->second))
        {
            // Traits only when the store actually has them: a file written
            // before traits existed carries none, and writing an empty set
            // would zero the race's blood and speed.
            if (HasOverrideTraits(it->first))
            {
                WriteTraits(race, OverrideTraits(it->first));
            }

            // Body parts likewise, and likewise only when stored: anatomy
            // lives in GameData rather than RaceData, so a file written before
            // it was saved carries none.
            std::map<std::string, PartsByName>::const_iterator parts =
                g_overrideParts.find(it->first);
            if (parts != g_overrideParts.end())
            {
                ApplyPartsTo(race, parts->second);
            }
            ++applied;
        }
        else
        {
            LogError("Could not apply the stored override for: " + it->first);
        }
    }
    return applied;
}
} // namespace totaleditor
