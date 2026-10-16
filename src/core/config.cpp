#include "common.h"
#include "config.h"
#include "json.h"
#include "hotkey.h"
#include "log.h"

namespace totaleditor
{
namespace
{
Config g_config;

// Reads one key, keeping whatever is already there when the key is absent or of
// the wrong kind. Every setting goes through these, which is what makes a
// partial config.json harmless.
void ReadBool(const JsonValue& root, const char* path, bool* target)
{
    *target = root.At(path).AsBool(*target);
}

void ReadInt(const JsonValue& root, const char* path, int* target)
{
    *target = (int)root.At(path).AsNumber((double)*target);
}

void ReadFloat(const JsonValue& root, const char* path, float* target)
{
    *target = (float)root.At(path).AsNumber((double)*target);
}

// The toggle key, written either as a name a player can type or as the raw OIS
// code the first release accepted.
//
//   "key": "K"     what the readme documents
//   "key": 37      what older configs hold, still honoured
//
// A name the table does not know is reported and the default kept, rather than
// silently leaving the mod with no way to open.
void ReadHotkey(const JsonValue& root, const char* path, int* target)
{
    const JsonValue& value = root.At(path);

    const std::string name = value.AsString(std::string());
    if (!name.empty())
    {
        const int code = KeyCodeFromName(name);
        if (code != 0)
        {
            *target = code;
            return;
        }
        LogError("config.json: '" + name + "' is not a key name this mod "
                 "knows; keeping the default. See the readme for the list.");
        return;
    }

    *target = (int)value.AsNumber((double)*target);
}

// A range whose ends are crossed would make every slider unusable, and a zero
// step count would divide by zero when the widget converts back to a float.
// Rather than refuse to load, the mod reports it and keeps its own defaults.
void ValidateRanges()
{
    const Config defaults;

    if (g_config.bonusMax <= g_config.bonusMin)
    {
        LogError("config.json: bonus.max must be greater than bonus.min; "
                 "using defaults for both.");
        g_config.bonusMin = defaults.bonusMin;
        g_config.bonusMax = defaults.bonusMax;
    }

    if (g_config.statMax <= g_config.statMin)
    {
        LogError("config.json: stat.max must be greater than stat.min; "
                 "using defaults for both.");
        g_config.statMin = defaults.statMin;
        g_config.statMax = defaults.statMax;
    }

    if (g_config.bonusSteps < 1)
    {
        LogError("config.json: bonus.steps must be at least 1; using default.");
        g_config.bonusSteps = defaults.bonusSteps;
    }

    if (g_config.partHealthMin < 1 || g_config.partHealthMax <= g_config.partHealthMin)
    {
        LogError("config.json: partHealth.min must be at least 1 and below "
                 "partHealth.max; using defaults for both.");
        g_config.partHealthMin = defaults.partHealthMin;
        g_config.partHealthMax = defaults.partHealthMax;
    }

    if (g_config.windowWidth < 320 || g_config.windowHeight < 240)
    {
        LogError("config.json: window smaller than 320x240 would clip its own "
                 "controls; using defaults.");
        g_config.windowWidth = defaults.windowWidth;
        g_config.windowHeight = defaults.windowHeight;
    }
}
} // namespace

Config::Config()
    : enabled(true)
    , toggleKey(37)          // OIS::KC_K
    , toggleNeedsCtrl(true)
    , bonusMin(0.0f)
    , bonusMax(3.0f)
    , bonusSteps(200)
    , statMin(0.0f)
    , statMax(200.0f)
    , partHealthMin(1)
    , partHealthMax(500)
    , followSelection(true)
    , windowWidth(1280)
    , windowHeight(1020)
    , saveOnClose(true)
    , applyOnStart(true)
    , debugLog(false)
{
}

void LoadConfig(const std::string& modDirectory)
{
    g_config = Config();

    if (modDirectory.empty())
    {
        LogInfo("No mod directory; using built-in defaults.");
        return;
    }

    const std::string path = modDirectory + "\\config.json";
    std::string text;
    if (!ReadFile(path, &text))
    {
        LogInfo("No config.json found; using built-in defaults.");
        return;
    }

    JsonValue root;
    if (!ParseJson(text, &root) || !root.isObject())
    {
        LogError("config.json is malformed; using built-in defaults.");
        return;
    }

    ReadBool(root, "enabled", &g_config.enabled);
    ReadHotkey(root, "hotkey.key", &g_config.toggleKey);
    ReadBool(root, "hotkey.needsCtrl", &g_config.toggleNeedsCtrl);

    ReadFloat(root, "bonus.min", &g_config.bonusMin);
    ReadFloat(root, "bonus.max", &g_config.bonusMax);
    ReadInt(root, "bonus.steps", &g_config.bonusSteps);

    ReadFloat(root, "stat.min", &g_config.statMin);
    ReadFloat(root, "stat.max", &g_config.statMax);

    ReadInt(root, "partHealth.min", &g_config.partHealthMin);
    ReadInt(root, "partHealth.max", &g_config.partHealthMax);

    ReadBool(root, "followSelection", &g_config.followSelection);

    ReadInt(root, "window.width", &g_config.windowWidth);
    ReadInt(root, "window.height", &g_config.windowHeight);

    ReadBool(root, "store.saveOnClose", &g_config.saveOnClose);
    ReadBool(root, "store.applyOnStart", &g_config.applyOnStart);

    ReadBool(root, "debugLog", &g_config.debugLog);

    ValidateRanges();

    // The log gate is set from here rather than read through GetConfig at each
    // call: logging runs from inside __try frames and from paths that run
    // before the config is loaded, and a plain bool is safe in both.
    SetDebugLogging(g_config.debugLog);
    LogInfo("config.json loaded.");
}

const Config& GetConfig()
{
    return g_config;
}
} // namespace totaleditor
