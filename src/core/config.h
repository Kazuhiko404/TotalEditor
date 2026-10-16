#pragma once

#include <string>

namespace totaleditor
{
// Every tunable the mod has, loaded from config.json. Nothing here is a magic
// number in the code: the ranges the sliders span, the window it draws and the
// key that opens it are all the player's to change.
struct Config
{
    // Turning this off installs no hooks at all, which is the honest way to
    // disable a plugin the launcher still loads.
    bool enabled;

    // OIS::KeyCode for the key that opens the editor, held with Ctrl.
    // 37 is OIS::KC_K.
    int toggleKey;

    // Whether the key needs Ctrl held. Off makes it a bare hotkey.
    bool toggleNeedsCtrl;

    // Race bonuses are multipliers the engine applies to a character's stats,
    // so this range is around 1.0, not around 100.
    float bonusMin;
    float bonusMax;

    // How many steps the bonus scrollbar divides its range into. The widget
    // works in integers, so this is what gives the multiplier its decimals:
    // 200 steps over 0..3 lands on 0.015 precision.
    int bonusSteps;

    // Character stats are the engine's 0..100 skill values. The ceiling goes
    // past 100 because the engine itself does, and the user asked for 200.
    float statMin;
    float statMax;

    // Body-part health, the per-part number the medical panel shows. A minimum
    // of 1 rather than 0: a part with no health at all is a part the engine can
    // never stop destroying.
    int partHealthMin;
    int partHealthMax;

    // Whether the window follows the player's selection while it is open.
    //
    // This costs a hook on the GUI's per-frame update. Turning it off is the
    // first thing to try if the window ever stops responding: it isolates the
    // hook from the rest of the mod without rebuilding anything.
    bool followSelection;

    // Window geometry, in pixels.
    int windowWidth;
    int windowHeight;

    // Writing the override store on every keystroke would hit the disk far too
    // often; edits are flushed when the window closes instead. Turning this off
    // saves immediately, which is useful when diagnosing a crash.
    bool saveOnClose;

    // Reapply the stored overrides when the plugin starts. Off makes the mod
    // read-only against a fresh game, which is how to test vanilla values
    // without deleting the store.
    bool applyOnStart;

    // Whether the detailed trace goes to RE_Kenshi's log.
    //
    // Off by default: the log is shared with every other plugin the player has
    // loaded, and a line per click would bury everyone else's output. Errors
    // and the session header are always written -- this adds the step-by-step
    // that turns "it did nothing" into a specific failure.
    //
    // Ask a player to set it to true and send the log; it needs no special
    // build.
    bool debugLog;

    Config();
};

// Reads config.json from the mod directory. Every key is optional: a missing,
// truncated or hand-broken file leaves the defaults in place, because a config
// is never a reason to fail to load.
void LoadConfig(const std::string& modDirectory);

const Config& GetConfig();
} // namespace totaleditor
