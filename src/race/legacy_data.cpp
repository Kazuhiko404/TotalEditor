#include "common.h"
#include "legacy_data.h"
#include "log.h"

namespace totaleditor
{
namespace
{
// The folder the mod was published under before the rename, as a sibling of
// this mod's own folder under Kenshi\mods.
const char* const kLegacyFolder = "RaceStatsRework";

bool FileExists(const std::string& path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::string ParentDirectory(const std::string& directory)
{
    const size_t slash = directory.find_last_of("\\/");
    if (slash == std::string::npos)
    {
        return std::string();
    }
    return directory.substr(0, slash);
}

// Copies <legacy>\<file> into <modDirectory>\<file> when only the legacy one
// exists. CopyFile with failIfExists set is the last guard against ever
// overwriting a file the player already has in the new folder.
void ImportFile(const std::string& modDirectory, const std::string& legacyDirectory,
                const char* file)
{
    const std::string target = modDirectory + "\\" + file;
    if (FileExists(target))
    {
        return;
    }

    const std::string source = legacyDirectory + "\\" + file;
    if (!FileExists(source))
    {
        return;
    }

    if (CopyFileA(source.c_str(), target.c_str(), TRUE))
    {
        LogInfo(std::string("Imported ") + file + " from the old " + kLegacyFolder +
                " mod folder; the original is left in place.");
    }
    else
    {
        LogError(std::string("Could not copy ") + file + " from the old " +
                 kLegacyFolder + " mod folder (error " +
                 std::to_string((long long)GetLastError()) + ").");
    }
}
} // namespace

void ImportLegacyData(const std::string& modDirectory)
{
    if (modDirectory.empty())
    {
        return;
    }

    const std::string modsDirectory = ParentDirectory(modDirectory);
    if (modsDirectory.empty())
    {
        return;
    }

    const std::string legacyDirectory = modsDirectory + "\\" + kLegacyFolder;
    ImportFile(modDirectory, legacyDirectory, "races.json");
    // The mod ships its own config.json, so in practice this only fires when a
    // player deleted it; then their old settings beat the defaults.
    ImportFile(modDirectory, legacyDirectory, "config.json");
}
} // namespace totaleditor
