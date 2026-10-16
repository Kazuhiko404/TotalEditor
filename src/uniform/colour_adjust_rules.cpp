#include "colour_adjust_rules.h"
#include "uniform_colour.h"
#include <cstdio>

namespace totaleditor
{
namespace ColourAdjustRules
{
namespace
{
const Preset kPresets[kPresetCount] = {
    { "Black", "191919" },
    { "White", "DCDCDC" },
    { "Grey", "696969" },
    { "Dark Red", "912823" },
    { "Orange", "B95A23" },
    { "Ochre", "AF8C2D" },
    { "Green", "2D6937" },
    { "Blue", "2D4682" },
    { "Purple", "64377D" },
    { "Brown", "5F3C23" },
    { "Bone", "B9AA87" },
};

int Clamp255(int value)
{
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}
} // namespace

const Preset& PresetAt(int index)
{
    const int i = index < 0 ? 0 : (index >= kPresetCount ? kPresetCount - 1 : index);
    return kPresets[i];
}

int PresetIndex(const std::string& hex)
{
    std::string norm;
    if (!UniformColour::NormalizeText(hex, &norm))
        return -1;
    for (int i = 0; i < kPresetCount; ++i)
    {
        if (norm == kPresets[i].hex)
            return i;
    }
    return -1;
}

std::string PresetName(const std::string& hex)
{
    const int idx = PresetIndex(hex);
    if (idx >= 0)
        return kPresets[idx].name;
    return "Custom";
}

std::string StepChannel(const std::string& currentHex, int channel, int delta)
{
    UniformColour::Rgb c;
    if (!UniformColour::ParseHex(currentHex, &c))
    {
        c.r = 0; c.g = 0; c.b = 0;
    }

    if (channel == 0) c.r = Clamp255(c.r + delta);
    else if (channel == 1) c.g = Clamp255(c.g + delta);
    else if (channel == 2) c.b = Clamp255(c.b + delta);

    return UniformColour::ToHex(c);
}

std::string CyclePreset(const std::string& currentHex, int direction)
{
    const int currentIdx = PresetIndex(currentHex);
    if (currentIdx < 0)
    {
        return direction > 0 ? kPresets[0].hex : kPresets[kPresetCount - 1].hex;
    }
    int next = currentIdx + (direction > 0 ? 1 : -1);
    if (next < 0) next = kPresetCount - 1;
    else if (next >= kPresetCount) next = 0;
    return kPresets[next].hex;
}

std::string Readout(const std::string& hex)
{
    UniformColour::Rgb c;
    if (!UniformColour::ParseHex(hex, &c))
        return "R   -   G   -   B   -";
    char buf[64];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "R %3d   G %3d   B %3d", Clamp255(c.r), Clamp255(c.g), Clamp255(c.b));
    return std::string(buf);
}

} // namespace ColourAdjustRules
} // namespace totaleditor
