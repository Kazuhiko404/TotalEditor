#pragma once

#include <string>

namespace totaleditor
{
// Everything the mod writes goes to RE_Kenshi's log, which the player already
// has and which is shared with every other plugin they have loaded. That
// sharing is what decides the levels below: our noise is someone else's haystack.

// Something the player lost, or a step that did not happen. Always written.
void LogError(const std::string& message);

// What the mod did: loaded, built its window, copied a race. Always written,
// and kept to the handful of lines that place a later error in context.
void LogInfo(const std::string& message);

// The step-by-step trace. Off unless "debugLog": true is set in config.json,
// so a player can produce a full trace for a bug report without a special build.
void LogDebug(const std::string& message);

// Turns the detailed trace on or off. Called once, when config.json is read.
void SetDebugLogging(bool enabled);
bool DebugLogging();

// Reports a fault caught by a TE_CATCH, naming what was being attempted.
//
// Prefer TE_CATCH_LOG below to calling this directly: it is easy to add a catch
// and forget the log, and a silent catch turns a crash into a mod that quietly
// does nothing -- which is far harder to diagnose than the crash would have been.
void LogFault(const char* what);
} // namespace totaleditor

// Catches that report the fault instead of swallowing it.
//
//   TE_TRY { widget->setCaption(text); }
//   TE_CATCH_LOG_VOID("setting a caption")
//
//   TE_TRY { return Build(parent); }
//   TE_CATCH_LOG_RET("building a widget", NULL)
//
// Two forms rather than one open-ended block: an SEH handler body cannot be
// passed through a macro parameter cleanly, and in this codebase every catch
// either returns a fallback or returns nothing. Use a bare TE_CATCH only where
// a fault is expected and genuinely not worth reporting.
#define TE_CATCH_LOG_VOID(what) \
    TE_CATCH { totaleditor::LogFault(what); }

#define TE_CATCH_LOG_RET(what, fallback) \
    TE_CATCH { totaleditor::LogFault(what); return fallback; }
