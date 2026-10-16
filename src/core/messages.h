#pragma once

#include <string>

namespace totaleditor
{
// Every string the player reads, loaded from messages.json so the text can be
// rewritten, translated or extended without touching the plugin.
//
// The mod ships a working default for every key it asks for, so a missing or
// hand-broken file costs nothing: the window still opens and still reads.
void LoadMessages(const std::string& modDirectory);

// Looks up a dotted path ("window.title"). A key the file does not have falls
// back to the default passed here, which is what makes an incomplete
// translation harmless rather than fatal.
std::string Text(const std::string& path, const std::string& fallback);

// Replaces {name}, {value} and {count} in a line already looked up. Keeping
// substitution separate from lookup lets one line serve several call sites.
std::string Fill(const std::string& text, const std::string& name,
                 const std::string& value, int count);

// Logs, once, every key the code asked for that messages.json did not have.
// Called on shutdown, so an incomplete file names itself instead of hiding
// behind the fallbacks.
void ReportMissingKeys();
} // namespace totaleditor
