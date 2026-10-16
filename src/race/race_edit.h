#pragma once

#include <string>
#include <vector>

#include "store.h"

class Character;
class RaceData;

namespace totaleditor
{
// Every read and write against engine-owned memory the editor performs.
//
// Two scales live here and they are not interchangeable. A race bonus is a
// multiplier the engine applies to a skill (1.15 = fifteen percent more), and a
// character stat is the skill value itself (0..200). The window draws them with
// different controls for that reason.

// The bonuses a race currently carries, sorted by skill name.
std::vector<BonusEntry> ReadBonuses(RaceData* race);

// Adds or replaces one bonus. A multiplier of exactly 1.0 is still stored,
// because "explicitly unchanged" and "absent" read the same in the game and
// differently to someone editing the list.
bool WriteBonus(RaceData* race, int stat, float value);

// Removes one bonus, which is what the X beside each row does.
bool RemoveBonus(RaceData* race, int stat);

// Replaces a race's whole bonus set. Used to revert to the stored baseline and
// to reapply a saved override, both of which are "make it exactly this".
bool ReplaceBonuses(RaceData* race, const std::vector<BonusEntry>& bonuses);

// The physical traits a race carries: blood, healing, speed, hunger, and the
// flags the engine branches on.
RaceTraits ReadTraits(RaceData* race);

// Writes a trait set onto a race. An invalid set is ignored rather than writing
// zeroes over a working race.
bool WriteTraits(RaceData* race, const RaceTraits& traits);

// The boolean traits of a race, as the editor lists them.
//
// Several are written as prohibitions -- noHats is true when the race CANNOT
// wear hats -- so each entry carries whether its meaning is inverted. The
// window shows them all as permissions ("can wear hats"), because a tick box
// that has to be read backwards is a tick box people get wrong.
enum RaceFlag
{
    FLAG_NO_HATS,
    FLAG_NO_SHIRTS,
    FLAG_NO_SHOES,
    FLAG_CAN_GO_INDOORS,
    FLAG_SWIMS,
    FLAG_CARRIABLE,
    FLAG_ROBOT,
    FLAG_VAMPIRIC,
    FLAG_GIGANTIC,
    FLAG_SINGLE_GENDER,
    FLAG_COUNT
};

// Reads one flag as the engine stores it, without inverting anything.
bool ReadFlag(RaceData* race, int flag, bool* value);

// Writes one flag as the engine stores it.
bool WriteFlag(RaceData* race, int flag, bool value);

// Enables or disables the equipment slot a clothing flag governs, on one
// character, immediately.
//
// The flag alone is not enough: the engine reads noHats and friends when it
// BUILDS a character, in setupInventorySections, and passes them straight
// through as the section's `enabled` argument -- KEP's RaceLimiterExtension
// shows the call verbatim, `!race->noHats`. A character already in the world
// has its sections built, so changing the race changes nothing for them until
// the next one is created.
//
// The sections always exist; only their enabled flag differs. So this finds the
// section by its attach slot and flips that flag, which is the same thing the
// constructor does and needs no section created or destroyed.
bool ApplySlotToCharacter(Character* character, int flag, bool raceForbids);

// Whether a flag governs an equipment slot at all. Only the three clothing
// prohibitions do; the rest change behaviour rather than inventory.
bool FlagHasSlot(int flag);

// Sets the race's blood range. The engine interpolates between the two across
// a character's age, so min is a newborn's and max a full adult's.
bool WriteBloodRange(RaceData* race, float minBlood, float maxBlood);

// Recomputes one character's blood from their race's current range.
//
// Blood is set when a character is built, by interpolating the race's min and
// max across their age -- KEP's MedicalFix shows the line verbatim:
//   blood = lerp(getAge(), race->originalBloodMin, race->originalBloodMax)
// so editing the range leaves everyone already alive on the old value. This
// runs the same interpolation again on demand.
bool ReapplyBlood(Character* character);

// Copies one race's bonuses onto another, replacing whatever the target had.
//
// Replacing rather than merging: after a copy the two races carry the same
// bonuses, which is what "copy" reads as. Merging would leave the target with a
// mixture neither race ever had, and undoing it one bonus at a time is far
// harder than pressing revert.
// `withTraits` also carries the physical traits across. Copying only the
// multipliers leaves a Skeleton that still does not bleed and a Hive member
// that still eats like one, which is not what "copy this race" means.
bool CopyBonuses(RaceData* from, RaceData* to, bool withTraits);

// One character's current value for a skill.
bool ReadStat(Character* character, int stat, float* out);

// Sets one skill on one character, clamped to the configured range, then asks
// the engine to recompute everything derived from it.
//
// Writing the field alone leaves the character walking around with stale
// carry weight and attack speed until something else happens to trigger a
// recalculation, so the write and the recompute are one operation here.
bool WriteStat(Character* character, int stat, float value);
} // namespace totaleditor
