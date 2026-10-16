#pragma once

#include <string>
#include <vector>

class Character;
class GameData;
class RaceData;

namespace totaleditor
{
// One body part of a race: what it is called, how likely it is to be hit, and
// how much health it has.
//
// This is the "combat anatomy" list the FCS shows on a race, and it is where a
// Shek's 125 per limb and a Skeleton's 200 actually live -- not in RaceData,
// whose fields carry no per-part health at all. Each entry of the list is a
// body-part GameData plus a TripleInt holding the two numbers.
struct BodyPart
{
    std::string name;
    // A relative weight, not a percentage. The engine divides each part's
    // weight by the total across the race, so a Greenlander's chest at 140 out
    // of 640 is 21.9% -- the raw number means nothing without the total.
    int hitChance;
    int health;      // What the medical panel shows as the part's maximum.
    int index;       // Position in the race's list, needed to write back.

    BodyPart() : hitChance(0), health(0), index(-1) {}
};

// The body parts of a race, in list order. Empty when the race has no anatomy
// list -- which is normal for races the engine never gives a body.
std::vector<BodyPart> ReadAnatomy(RaceData* race);

// The sum of every part's hit weight, which is what turns one part's weight
// into the percentage the player actually cares about.
int TotalHitWeight(const std::vector<BodyPart>& parts);

// Sets the hit weight of one body part. Raising one part's weight lowers every
// other part's share, because the percentage is always against the total.
bool WritePartHitChance(RaceData* race, int index, int hitChance);

// Sets the health of one body part, clamped to the configured range.
//
// Writes into the race's own GameData, so it changes the template every
// character of that race is built from, exactly as the bonuses do.
bool WritePartHealth(RaceData* race, int index, int health);

// Pushes the race's current part values onto one living character.
//
// Part health is read when the medical system is built, so editing the race
// leaves everyone already alive on the old numbers. This walks the character's
// own anatomy, matches each part to its entry in the race's list, and copies
// the health across -- the same thing KEP's DevToolsPanel does when it restores
// a robotic limb.
//
// The damage is cleared as well. Raising a maximum without doing so is close to
// useless -- the engine still heals a wounded part up to the OLD maximum, which
// is what the modding forums warn about -- so the character's wounds, bandages
// and wear go with it.
//
// Returns how many parts were updated.
int ApplyAnatomyToCharacter(Character* character, RaceData* race);

// Whether a race has a body at all. Cheaper than reading every part, and it is
// what separates a race the engine can build a character from -- every real one
// has an anatomy -- from the stubs that fill the data.
bool HasAnatomy(RaceData* race);

// The name of the list the anatomy was found under, for the log. Empty until a
// race has been read.
const std::string& AnatomyListName();
} // namespace totaleditor
