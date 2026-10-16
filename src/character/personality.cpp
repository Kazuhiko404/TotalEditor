#include "common.h"
#include "personality.h"

#include <kenshi/StateBroadcastData.h>

namespace totaleditor
{
namespace
{
struct PersonalityOption
{
    PersonalityTags value;
    const char* label;
};

// CharTweak 2's list, in its order. The enum's gaps (3, 4, 7, 8) are unnamed
// in the game data and are not offered.
const PersonalityOption kOptions[] =
{
    { PT_HONORABLE,  "Honorable" },
    { PT_TRAITOROUS, "Traitorous" },
    { PT_SMART,      "Smart" },
    { PT_DUMB,       "Dumb" },
    { PT_BRAVE,      "Brave" },
    { PT_FEARFUL,    "Fearful" },
    { PT_WARM_KIND,  "Warm / Kind" },
    { PT_COLD_CRUEL, "Cold / Cruel" },
    { PT_NORMAL,     "Normal" },
    { PT_MANIACAL,   "Maniacal" }
};
const int kOptionCount = sizeof(kOptions) / sizeof(kOptions[0]);

// The guarded frames hold only PODs: __try may not share a frame with
// anything that needs unwinding.
bool SafeReadState(Character* character, StateBroadcastData** out)
{
    TE_TRY
    {
        *out = character->stateBroadcast;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeReadValue(StateBroadcastData* state, int* out)
{
    TE_TRY
    {
        *out = (int)state->myPersonality;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeWriteValue(StateBroadcastData* state, int value)
{
    TE_TRY
    {
        state->myPersonality = (PersonalityTags)value;
        return true;
    }
    TE_CATCH { return false; }
}

// Resolves the character's state broadcast, or says why there is none.
PersonalityResult StateOf(Character* character, StateBroadcastData** out)
{
    if (character == NULL)
    {
        return PERSONALITY_NO_CHARACTER;
    }
    if (!SafeReadState(character, out))
    {
        return PERSONALITY_FAULT;
    }
    return (*out != NULL) ? PERSONALITY_OK : PERSONALITY_NO_STATE;
}
} // namespace

int PersonalityCount()
{
    return kOptionCount;
}

int PersonalityValueAt(int index)
{
    if (index < 0 || index >= kOptionCount)
    {
        return 0;
    }
    return (int)kOptions[index].value;
}

int PersonalityIndexOf(int value)
{
    for (int i = 0; i < kOptionCount; ++i)
    {
        if ((int)kOptions[i].value == value)
        {
            return i;
        }
    }
    return -1;
}

std::vector<std::string> PersonalityLabels()
{
    std::vector<std::string> labels;
    for (int i = 0; i < kOptionCount; ++i)
    {
        labels.push_back(PersonalityLabel((int)kOptions[i].value));
    }
    return labels;
}

std::string PersonalityLabel(int value)
{
    if (value == (int)PT_NONE)
    {
        return "None";
    }

    const int index = PersonalityIndexOf(value);
    if (index < 0)
    {
        return "Unknown";
    }

    char text[64] = { 0 };
    _snprintf_s(text, sizeof(text), _TRUNCATE, "%s (%d)", kOptions[index].label, value);
    return text;
}

PersonalityResult ReadPersonality(Character* character, int* out)
{
    StateBroadcastData* state = NULL;
    const PersonalityResult result = StateOf(character, &state);
    if (result != PERSONALITY_OK)
    {
        return result;
    }
    return SafeReadValue(state, out) ? PERSONALITY_OK : PERSONALITY_FAULT;
}

PersonalityResult WritePersonality(Character* character, int value)
{
    StateBroadcastData* state = NULL;
    const PersonalityResult result = StateOf(character, &state);
    if (result != PERSONALITY_OK)
    {
        return result;
    }
    return SafeWriteValue(state, value) ? PERSONALITY_OK : PERSONALITY_FAULT;
}
} // namespace totaleditor
