#include "common.h"
#include "log.h"

namespace totaleditor
{
namespace
{
const char* const kLogPrefix = "[TotalEditor] ";

// A plain bool rather than a read through GetConfig(): this is checked from
// inside __try frames and from paths that run before config.json is read, and
// it has to be safe in both. False until told otherwise, so a failure to load
// the config leaves the log quiet rather than noisy.
bool g_debug = false;
}

void SetDebugLogging(bool enabled)
{
    g_debug = enabled;
}

bool DebugLogging()
{
    return g_debug;
}

void LogInfo(const std::string& message)
{
    DebugLog(std::string(kLogPrefix) + message);
}

void LogError(const std::string& message)
{
    ErrorLog(std::string(kLogPrefix) + message);
}

void LogDebug(const std::string& message)
{
    if (!g_debug)
    {
        return;
    }
    DebugLog(std::string(kLogPrefix) + message);
}

void LogFault(const char* what)
{
    // An error, not a debug line: reaching here means an engine call faulted,
    // and whatever the player asked for did not happen. Silence here is what
    // makes a mod look like it is ignoring clicks.
    ErrorLog(std::string(kLogPrefix) + "Faulted while " + what +
             ". That step did not complete.");
}
} // namespace totaleditor
