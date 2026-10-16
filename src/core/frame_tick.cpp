#include "common.h"
#include "frame_tick.h"
#include "editor_window.h"
#include "log.h"

#include <kenshi/gui/ForgottenGUI.h>

namespace totaleditor
{
namespace
{
void (*g_forgottenGuiUpdate_orig)(ForgottenGUI*) = NULL;

// Kept out of the hook body so a fault in the plugin's own work can never take
// the game's GUI update down with it.
void SafeTick()
{
    TE_TRY { EditorTick(); }
    TE_CATCH { }
}

void ForgottenGUI_update_hook(ForgottenGUI* thisptr)
{
    // The game's own update runs first and unconditionally: whatever the plugin
    // does afterwards, the GUI has already been drawn.
    if (g_forgottenGuiUpdate_orig != NULL)
    {
        g_forgottenGuiUpdate_orig(thisptr);
    }

    SafeTick();
}
} // namespace

bool InstallFrameTick()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ForgottenGUI::update),
            ForgottenGUI_update_hook, &g_forgottenGuiUpdate_orig))
    {
        LogError("Could not hook ForgottenGUI::update; the window will not "
                 "notice when you select a different character.");
        return false;
    }

    LogInfo("Frame tick installed.");
    return true;
}

void ShutdownFrameTick()
{
    g_forgottenGuiUpdate_orig = NULL;
}
} // namespace totaleditor
