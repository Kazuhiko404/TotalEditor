#pragma once

#include <string>

namespace totaleditor
{
namespace ColourAdjustRules
{
struct Preset
{
    const char* name;
    const char* hex;
};

const int kPresetCount = 11;

const Preset& PresetAt(int index);
int PresetIndex(const std::string& hex);
std::string PresetName(const std::string& hex);
std::string StepChannel(const std::string& currentHex, int channel, int delta);
std::string CyclePreset(const std::string& currentHex, int direction);
std::string Readout(const std::string& hex);
} // namespace ColourAdjustRules
} // namespace totaleditor
