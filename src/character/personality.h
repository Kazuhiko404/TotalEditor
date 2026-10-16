#pragma once

#include <string>
#include <vector>

class Character;

namespace totaleditor
{
// A character's personality is one enum on its state broadcast:
// Character::stateBroadcast -> StateBroadcastData::myPersonality. Dialogue and
// AI read it from there; Kenshi saves it with the character, so a change made
// here persists through the normal save with no sidecar file and no reapply.
//
// The choices, their order and their labels copy CharTweak 2, so a player who
// knows that mod sees the same list.

// The number of personalities offered, in display order.
int PersonalityCount();

// The enum value at a display index, or 0 when the index is out of range.
int PersonalityValueAt(int index);

// The display index of an enum value, or -1 when the value is not offered.
int PersonalityIndexOf(int value);

// The labels of every offered personality, in display order, for a dropdown.
std::vector<std::string> PersonalityLabels();

// "Honorable (1)" for an offered value, "None" for 0, "Unknown" otherwise.
std::string PersonalityLabel(int value);

enum PersonalityResult
{
    PERSONALITY_OK,
    PERSONALITY_NO_CHARACTER,
    PERSONALITY_NO_STATE,     // the character has no state broadcast
    PERSONALITY_FAULT         // reading the engine's memory faulted
};

PersonalityResult ReadPersonality(Character* character, int* out);

// One direct write of the enum, as CharTweak does: no side-effect calls.
PersonalityResult WritePersonality(Character* character, int value);
} // namespace totaleditor
