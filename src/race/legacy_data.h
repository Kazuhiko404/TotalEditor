#pragma once

#include <string>

namespace totaleditor
{
// TotalEditor used to ship as RaceStatsRework, in its own mod folder. A player
// who switches keeps their saved race edits: when this mod's folder has no
// races.json (or no config.json) but the old RaceStatsRework folder next to it
// does, the file is copied over once. The old file is never moved or deleted,
// and a file already present in the TotalEditor folder is never overwritten.
//
// Must run before LoadConfig and LoadStore read the mod folder.
void ImportLegacyData(const std::string& modDirectory);
} // namespace totaleditor
