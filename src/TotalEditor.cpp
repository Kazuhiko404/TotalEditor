// TotalEditor - an in-game editor for race bonuses and character stats.
//
// Two things about Kenshi's data model shape this whole mod:
//
//   * Race bonuses are GLOBAL. RaceData::AllRaces holds one object per race and
//     Character::myRace is a pointer into it, so every Greenlander in the world
//     shares one bonus table. There is no "edit only my character" without
//     cloning the race, so this is a global race editor and says so.
//
//   * Race bonuses are MULTIPLIERS, not added points. The engine applies them
//     as `stat *= bonus`, which is why the bonus control spans a range around
//     1.0 while the character-stat control spans 0..200.
//
// Neither survives a reload: RaceData is rebuilt from the FCS data every start
// and AllRaces is only a runtime cache. So the mod keeps its own races.json,
// holding both the edits to reapply and the values each race shipped with.

#include "common.h"
#include "catalog.h"
#include "config.h"
#include "editor_window.h"
#include "frame_tick.h"
#include "hotkey.h"
#include "legacy_data.h"
#include "log.h"
#include "messages.h"
#include "store.h"
#include "uniform_engine.h"
#include "portrait_editor.h"

#include <kenshi/InputHandler.h>
#include <ois/OISKeyboard.h>

namespace totaleditor
{
// Set by DllMain before startPlugin runs, so it cannot live in an anonymous
// namespace.
HMODULE g_module = NULL;

namespace
{
void (*g_keyDownEvent_orig)(InputHandler*, OIS::KeyCode) = NULL;

// Directory the DLL was loaded from, where config.json, messages.json and
// races.json live.
std::string ModuleDirectory()
{
    if (g_module == NULL)
    {
        return std::string();
    }

    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(g_module, path, MAX_PATH) == 0)
    {
        return std::string();
    }

    std::string full(path);
    const size_t slash = full.find_last_of("\\/");
    if (slash == std::string::npos)
    {
        return std::string();
    }
    return full.substr(0, slash);
}

void InputHandler_keyDownEvent_hook(InputHandler* thisptr, OIS::KeyCode key)
{
    // The game's own handling runs first and unconditionally, so a failure here
    // can never cost the player their normal keyboard.
    if (g_keyDownEvent_orig != NULL)
    {
        g_keyDownEvent_orig(thisptr, key);
    }

    const Config& config = GetConfig();
    if (!config.enabled || config.toggleKey == 0)
    {
        return;
    }
    if ((int)key != config.toggleKey)
    {
        return;
    }

    // The modifier is read off the handler rather than from GetAsyncKeyState,
    // because this is the state the game itself is acting on.
    bool ctrlHeld = false;
    TE_TRY { ctrlHeld = (thisptr != NULL) && thisptr->ctrl; }
    TE_CATCH { return; }

    if (config.toggleNeedsCtrl && !ctrlHeld)
    {
        return;
    }

    ToggleEditor();
}

bool InstallInputHook()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&InputHandler::keyDownEvent),
            InputHandler_keyDownEvent_hook,
            &g_keyDownEvent_orig))
    {
        LogError("Failed to hook InputHandler::keyDownEvent.");
        return false;
    }
    return true;
}
} // namespace
} // namespace totaleditor

__declspec(dllexport) void startPlugin()
{
    using namespace totaleditor;

    // First line of the log, before anything can fail, so a plugin that loaded
    // and broke is always distinguishable from one that never loaded. The
    // version goes here because a bug report that does not name the build
    // cannot be acted on.
    LogInfo("TotalEditor " + std::string(kModVersion) + " starting.");

    // A version mismatch is reported but not fatal: RE_Kenshi runs a patched
    // binary whose reported version can differ, and refusing outright would
    // make the plugin silently do nothing.
    KenshiLib::BinaryVersion versionInfo = KenshiLib::GetKenshiVersion();
    const std::string version = versionInfo.GetVersion();
    LogInfo("Detected Kenshi version: '" + version + "' (" +
            versionInfo.GetPlatformStr() + ").");

    if (version != kSupportedGameVersion)
    {
        LogError("Version is not " + std::string(kSupportedGameVersion) +
                 "; hooks may resolve to the wrong functions. Continuing anyway.");
    }

    const std::string directory = ModuleDirectory();
    // Before anything reads the folder: carries over a player's saved races
    // from the RaceStatsRework folder this mod was renamed from.
    ImportLegacyData(directory);
    LoadConfig(directory);
    LoadMessages(directory);
    LoadStore(directory);

    if (!GetConfig().enabled)
    {
        LogInfo("Disabled by config; no hooks installed.");
        return;
    }

    if (!InstallInputHook())
    {
        LogError("No keyboard hook; the editor cannot be opened.");
        return;
    }

    // What the player actually has to press. "It does not open" is the most
    // common report a hotkey mod gets, and most of the time the answer is in
    // their own config.
    LogInfo("Editor opens with " + HotkeyDescription() + ".");

    // Frame tick handles selection following and automatic override application on world load.
    InstallFrameTick();
    UniformEngine::Install();
    PortraitEditor::Install();
    if (!GetConfig().followSelection)
    {
        LogInfo("followSelection is off; selection tracking disabled, but world load monitor active.");
    }

    LogInfo("startPlugin complete.");
}

__declspec(dllexport) void stopPlugin()
{
    using namespace totaleditor;
    LogInfo("stopPlugin called.");

    ShutdownFrameTick();
    ShutdownEditor();
    ReportMissingKeys();
    ClearCatalogCache();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        totaleditor::g_module = module;
        DisableThreadLibraryCalls(module);
        break;
    case DLL_PROCESS_DETACH:
        // Only clean up on an explicit unload; on process exit the engine's
        // widgets are already gone.
        if (reserved == NULL)
        {
            stopPlugin();
        }
        break;
    default:
        break;
    }
    return TRUE;
}
