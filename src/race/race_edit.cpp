#include "common.h"
#include "race_edit.h"
#include "anatomy.h"
#include "catalog.h"
#include "config.h"
#include "log.h"

#include <algorithm>

#include <kenshi/MedicalSystem.h>
#include <kenshi/Item.h>
#include <kenshi/Inventory.h>

namespace totaleditor
{
namespace
{
// statMods is a public member at a known offset, so it needs no exported
// symbol: with a RaceData* in hand the map is read and written directly. That
// matters, because RaceData::AllRaces -- the obvious way in -- is declared in
// the header but exports nothing to link against.

// Counting first lets the caller size a POD buffer, so the copy out of an
// engine-owned map never happens inside a guarded frame.
bool SafeCountBonuses(RaceData* race, int* out)
{
    TE_TRY
    {
        *out = (int)race->statMods.size();
        return true;
    }
    TE_CATCH { return false; }
}

// Copies into plain arrays rather than a vector, because a guarded frame may
// hold nothing that needs unwinding.
bool SafeCopyBonuses(RaceData* race, int* stats, float* values, int capacity,
                     int* written)
{
    TE_TRY
    {
        int count = 0;
        ogre_unordered_map<StatsEnumerated, float>::type::const_iterator it =
            race->statMods.begin();
        for (; it != race->statMods.end() && count < capacity; ++it)
        {
            stats[count] = (int)it->first;
            values[count] = it->second;
            ++count;
        }
        *written = count;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeWriteBonus(RaceData* race, int stat, float value)
{
    TE_TRY
    {
        race->statMods[(StatsEnumerated)stat] = value;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeRemoveBonus(RaceData* race, int stat)
{
    TE_TRY
    {
        race->statMods.erase((StatsEnumerated)stat);
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeClearBonuses(RaceData* race)
{
    TE_TRY
    {
        race->statMods.clear();
        return true;
    }
    TE_CATCH { return false; }
}

// RaceData's traits are public members at known offsets, so they are read and
// written directly. Every field here is a POD, which is what lets the whole
// copy sit inside one guard.
bool SafeReadTraits(RaceData* race, RaceTraits* out)
{
    TE_TRY
    {
        out->runSpeedMaxSkill = race->runSpeedMaxSkill;
        out->runSpeedMinSkill = race->runSpeedMinSkill;
        out->originalBloodMin = race->originalBloodMin;
        out->originalBloodMax = race->originalBloodMax;
        out->healRate         = race->healRate;
        out->bleedRate        = race->bleedRate;
        out->swimSpeed        = race->swimSpeed;
        out->walkSpeed        = race->walkSpeed;
        out->swimOffset       = race->swimOffset;
        out->waterAvoidance   = race->waterAvoidance;
        out->hungerRate       = race->hungerRate;
        out->visionMultiplier = race->visionMultiplier;

        out->gigantic     = race->gigantic;
        out->swims        = race->swims;
        out->carriable    = race->carriable;
        out->singleGender = race->singleGender;
        out->robot        = race->robot;
        out->vampiric     = race->vampiric;
        out->noHats       = race->noHats;
        out->noShirts     = race->noShirts;
        out->noShoes      = race->noShoes;
        out->canGoIndoors = race->canGoIndoors;

        out->extraAttackSlots = race->extraAttackSlots;
        out->firstAidSkill    = (int)race->firstAidSkill;

        out->bloodColourR = race->bloodColour.r;
        out->bloodColourG = race->bloodColour.g;
        out->bloodColourB = race->bloodColour.b;
        out->bloodColourA = race->bloodColour.a;

        out->valid = true;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeWriteTraits(RaceData* race, const RaceTraits& traits)
{
    TE_TRY
    {
        race->runSpeedMaxSkill = traits.runSpeedMaxSkill;
        race->runSpeedMinSkill = traits.runSpeedMinSkill;
        race->originalBloodMin = traits.originalBloodMin;
        race->originalBloodMax = traits.originalBloodMax;
        race->healRate         = traits.healRate;
        race->bleedRate        = traits.bleedRate;
        race->swimSpeed        = traits.swimSpeed;
        race->walkSpeed        = traits.walkSpeed;
        race->swimOffset       = traits.swimOffset;
        race->waterAvoidance   = traits.waterAvoidance;
        race->hungerRate       = traits.hungerRate;
        race->visionMultiplier = traits.visionMultiplier;

        race->gigantic     = traits.gigantic;
        race->swims        = traits.swims;
        race->carriable    = traits.carriable;
        race->singleGender = traits.singleGender;
        race->robot        = traits.robot;
        race->vampiric     = traits.vampiric;
        race->noHats       = traits.noHats;
        race->noShirts     = traits.noShirts;
        race->noShoes      = traits.noShoes;
        race->canGoIndoors = traits.canGoIndoors;

        race->extraAttackSlots = traits.extraAttackSlots;
        race->firstAidSkill    = (StatsEnumerated)traits.firstAidSkill;

        race->bloodColour.r = traits.bloodColourR;
        race->bloodColour.g = traits.bloodColourG;
        race->bloodColour.b = traits.bloodColourB;
        race->bloodColour.a = traits.bloodColourA;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeReadStatValue(Character* character, int stat, float* out)
{
    TE_TRY
    {
        *out = character->stats->getStatRef((StatsEnumerated)stat);
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeWriteStatValue(Character* character, int stat, float value)
{
    TE_TRY
    {
        character->stats->getStatRef((StatsEnumerated)stat) = value;
        return true;
    }
    TE_CATCH { return false; }
}

// The sequence KEP runs after touching a character's stats, in the same order.
//
// _NV__recalculateStats is the non-virtual thunk: _recalculateStats itself is
// virtual, and a plugin that declares a virtual builds a vtable of its own and
// calls whatever the engine keeps at that slot -- which crashes the game.
//
// The blood clamp is not decoration. Max blood is derived from the stats that
// were just written, so lowering toughness can leave a character carrying more
// blood than their body now holds.
void Recalculate(Character* character)
{
    const float maxBlood = character->medical.getMaxBlood();
    if (maxBlood < character->medical.blood)
    {
        character->medical.blood = maxBlood;
    }

    Item* item = character->stats->weapon.getItem();
    if (item != NULL)
    {
        character->stats->setWeapon(item->isWeapon());
    }

    character->stats->_NV__recalculateStats();
}

bool SafeRecalculate(Character* character)
{
    TE_TRY
    {
        Recalculate(character);
        return true;
    }
    TE_CATCH { return false; }
}

bool ByStatName(const BonusEntry& a, const BonusEntry& b)
{
    const std::vector<StatEntry>& stats = Stats();
    std::string nameA;
    std::string nameB;
    for (size_t i = 0; i < stats.size(); ++i)
    {
        if (stats[i].stat == a.stat) { nameA = stats[i].name; }
        if (stats[i].stat == b.stat) { nameB = stats[i].name; }
    }
    return nameA < nameB;
}

float Clamp(float value, float low, float high)
{
    if (value < low)  { return low; }
    if (value > high) { return high; }
    return value;
}
} // namespace

std::vector<BonusEntry> ReadBonuses(RaceData* race)
{
    std::vector<BonusEntry> out;
    if (race == NULL)
    {
        return out;
    }

    int count = 0;
    if (!SafeCountBonuses(race, &count) || count <= 0)
    {
        return out;
    }

    // One slot per enumerator is the most a stat-keyed map can hold, so this
    // bound cannot truncate a legitimate set.
    const int kCapacity = STAT_END + 1;
    if (count > kCapacity)
    {
        count = kCapacity;
    }

    std::vector<int> stats((size_t)kCapacity, 0);
    std::vector<float> values((size_t)kCapacity, 0.0f);
    int written = 0;
    if (!SafeCopyBonuses(race, &stats[0], &values[0], kCapacity, &written))
    {
        LogError("Could not read the bonuses of a race.");
        return out;
    }

    for (int i = 0; i < written; ++i)
    {
        // A bonus on a derived value would be edited here and overwritten by
        // the engine on the next frame, so it is not offered for editing.
        if (stats[i] <= STAT_NONE || stats[i] >= STAT_END)
        {
            continue;
        }
        out.push_back(BonusEntry(stats[i], values[i]));
    }

    std::sort(out.begin(), out.end(), ByStatName);
    return out;
}

bool WriteBonus(RaceData* race, int stat, float value)
{
    if (race == NULL || stat <= STAT_NONE || stat >= STAT_END)
    {
        return false;
    }

    const Config& config = GetConfig();
    return SafeWriteBonus(race, stat, Clamp(value, config.bonusMin, config.bonusMax));
}

bool RemoveBonus(RaceData* race, int stat)
{
    if (race == NULL)
    {
        return false;
    }
    return SafeRemoveBonus(race, stat);
}

bool ReplaceBonuses(RaceData* race, const std::vector<BonusEntry>& bonuses)
{
    if (race == NULL)
    {
        return false;
    }

    if (!SafeClearBonuses(race))
    {
        LogError("Could not clear the bonuses of a race.");
        return false;
    }

    for (size_t i = 0; i < bonuses.size(); ++i)
    {
        if (!SafeWriteBonus(race, bonuses[i].stat, bonuses[i].value))
        {
            return false;
        }
    }
    return true;
}

RaceTraits ReadTraits(RaceData* race)
{
    RaceTraits traits;
    if (race == NULL)
    {
        return traits;
    }
    SafeReadTraits(race, &traits);
    return traits;
}

bool WriteTraits(RaceData* race, const RaceTraits& traits)
{
    // An unread trait set is all zeroes, and writing those would leave a race
    // with no blood and no walk speed. Refusing is the safe answer.
    if (race == NULL || !traits.valid)
    {
        return false;
    }
    return SafeWriteTraits(race, traits);
}

namespace
{
// One switch rather than an array of member pointers: the fields are plain
// bools at fixed offsets, and a pointer-to-member table would not survive the
// guarded-frame rule any more cleanly than this does.
bool* FlagAddress(RaceData* race, int flag)
{
    switch (flag)
    {
    case FLAG_NO_HATS:        return &race->noHats;
    case FLAG_NO_SHIRTS:      return &race->noShirts;
    case FLAG_NO_SHOES:       return &race->noShoes;
    case FLAG_CAN_GO_INDOORS: return &race->canGoIndoors;
    case FLAG_SWIMS:          return &race->swims;
    case FLAG_CARRIABLE:      return &race->carriable;
    case FLAG_ROBOT:          return &race->robot;
    case FLAG_VAMPIRIC:       return &race->vampiric;
    case FLAG_GIGANTIC:       return &race->gigantic;
    case FLAG_SINGLE_GENDER:  return &race->singleGender;
    default:                  return NULL;
    }
}

bool SafeReadFlag(RaceData* race, int flag, bool* value)
{
    TE_TRY
    {
        bool* address = FlagAddress(race, flag);
        if (address == NULL)
        {
            return false;
        }
        *value = *address;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeWriteFlag(RaceData* race, int flag, bool value)
{
    TE_TRY
    {
        bool* address = FlagAddress(race, flag);
        if (address == NULL)
        {
            return false;
        }
        *address = value;
        return true;
    }
    TE_CATCH { return false; }
}
} // namespace

namespace
{
// The attach slot each clothing flag governs, matching the sections
// CharacterHuman builds: "head" on ATTACH_HAT, "shirt" on ATTACH_SHIRT and
// "boots" on ATTACH_BOOTS.
AttachSlot SlotForFlag(int flag)
{
    switch (flag)
    {
    case FLAG_NO_HATS:   return ATTACH_HAT;
    case FLAG_NO_SHIRTS: return ATTACH_SHIRT;
    case FLAG_NO_SHOES:  return ATTACH_BOOTS;
    default:             return ATTACH_NONE;
    }
}

bool SafeSetSlotEnabled(Character* character, AttachSlot slot, bool enabled)
{
    TE_TRY
    {
        if (character->inventory == NULL)
        {
            return false;
        }

        InventorySection* section = character->inventory->getSectionOfType(slot);
        if (section == NULL)
        {
            return false;
        }

        section->setEnabled(enabled);
        return true;
    }
    TE_CATCH { return false; }
}
} // namespace

namespace
{
// The engine's own interpolation, copied from the line that sets blood when a
// character is created. age is 0..1 across a lifetime, so this is a plain
// linear blend between the race's two limits.
float BlendBlood(float age, float low, float high)
{
    if (age < 0.0f) { age = 0.0f; }
    if (age > 1.0f) { age = 1.0f; }
    return low + (high - low) * age;
}

bool SafeReapplyBlood(Character* character)
{
    TE_TRY
    {
        RaceData* race = character->_NV_getRace();
        if (race == NULL)
        {
            return false;
        }

        const float age = character->_NV_getAge();
        const float blood = BlendBlood(age, race->originalBloodMin,
                                       race->originalBloodMax);

        character->medical.blood = blood;
        return true;
    }
    TE_CATCH { return false; }
}
} // namespace

namespace
{
bool SafeWriteBloodRange(RaceData* race, float minBlood, float maxBlood)
{
    TE_TRY
    {
        race->originalBloodMin = minBlood;
        race->originalBloodMax = maxBlood;
        return true;
    }
    TE_CATCH { return false; }
}
} // namespace

bool WriteBloodRange(RaceData* race, float minBlood, float maxBlood)
{
    if (race == NULL || minBlood < 0.0f || maxBlood < minBlood)
    {
        return false;
    }
    return SafeWriteBloodRange(race, minBlood, maxBlood);
}

bool ReapplyBlood(Character* character)
{
    if (character == NULL)
    {
        return false;
    }
    return SafeReapplyBlood(character);
}

bool FlagHasSlot(int flag)
{
    return SlotForFlag(flag) != ATTACH_NONE;
}

bool ApplySlotToCharacter(Character* character, int flag, bool raceForbids)
{
    const AttachSlot slot = SlotForFlag(flag);
    if (character == NULL || slot == ATTACH_NONE)
    {
        return false;
    }

    // The race field is a prohibition and the section's flag is a permission,
    // so the value is inverted here exactly as the engine inverts it.
    return SafeSetSlotEnabled(character, slot, !raceForbids);
}

bool ReadFlag(RaceData* race, int flag, bool* value)
{
    if (race == NULL || flag < 0 || flag >= FLAG_COUNT)
    {
        return false;
    }
    return SafeReadFlag(race, flag, value);
}

bool WriteFlag(RaceData* race, int flag, bool value)
{
    if (race == NULL || flag < 0 || flag >= FLAG_COUNT)
    {
        return false;
    }
    return SafeWriteFlag(race, flag, value);
}

void LogAnatomyCopied(int parts)
{
    char detail[48] = { 0 };
    _snprintf_s(detail, sizeof(detail), _TRUNCATE, "%d", parts);
    LogDebug("Body parts copied: " + std::string(detail) + ".");
}

// Body part health and hit weight, from one race onto another.
//
// Parts are matched by name rather than by position: two races do not carry the
// same anatomy list, and copying a Skeleton's third entry onto a Greenlander's
// third entry would put an arm's health on a chest. A part the target does not
// have is skipped -- there is nothing to write it to.
//
// Anatomy lives in the race's GameData rather than in RaceData, which is why it
// rides along here instead of inside WriteTraits: it is the same "physical"
// half of a race the traits are, and the tick box that asks for one means both.
int CopyAnatomy(RaceData* from, RaceData* to)
{
    const std::vector<BodyPart> source = ReadAnatomy(from);
    const std::vector<BodyPart> target = ReadAnatomy(to);
    if (source.empty() || target.empty())
    {
        return 0;
    }

    int copied = 0;
    for (size_t i = 0; i < target.size(); ++i)
    {
        for (size_t j = 0; j < source.size(); ++j)
        {
            if (target[i].name != source[j].name)
            {
                continue;
            }

            // Health and hit weight are written separately, and a failure on
            // one part is not a reason to stop: the rest of the body still
            // copies.
            if (WritePartHealth(to, target[i].index, source[j].health) &&
                WritePartHitChance(to, target[i].index, source[j].hitChance))
            {
                ++copied;
            }
            break;
        }
    }
    return copied;
}

bool CopyBonuses(RaceData* from, RaceData* to, bool withTraits)
{
    if (from == NULL || to == NULL)
    {
        return false;
    }

    // Copying a race onto itself would clear it and write it back identical --
    // harmless, but it would record a pointless override, so it is refused.
    if (from == to)
    {
        return false;
    }

    if (!ReplaceBonuses(to, ReadBonuses(from)))
    {
        return false;
    }

    if (withTraits)
    {
        // Traits failing is reported but does not undo the bonuses: a partial
        // copy the player can see and fix beats silently rolling back work that
        // did succeed.
        if (!WriteTraits(to, ReadTraits(from)))
        {
            LogError("Bonuses copied, but the physical traits could not be.");
            return false;
        }

        // Anatomy failing is not fatal either: a race with no body parts is
        // normal, and a partial copy stays visible in the panel.
        LogAnatomyCopied(CopyAnatomy(from, to));
    }
    return true;
}

bool ReadStat(Character* character, int stat, float* out)
{
    if (character == NULL || stat <= STAT_NONE || stat >= STAT_END)
    {
        return false;
    }
    return SafeReadStatValue(character, stat, out);
}

bool WriteStat(Character* character, int stat, float value)
{
    if (character == NULL || stat <= STAT_NONE || stat >= STAT_END)
    {
        return false;
    }

    const Config& config = GetConfig();
    if (!SafeWriteStatValue(character, stat, Clamp(value, config.statMin, config.statMax)))
    {
        return false;
    }

    // A written skill that nothing recomputes leaves the character with stale
    // carry weight and attack speed, so the recompute is part of the write.
    if (!SafeRecalculate(character))
    {
        LogError("Stat written but the engine recalculation failed.");
        return false;
    }
    return true;
}
} // namespace totaleditor
