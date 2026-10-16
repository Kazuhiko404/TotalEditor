#pragma once

#include <string>

namespace totaleditor
{
// Turns a key written in config.json into the OIS code the input hook compares
// against, and back again for the log.
//
// config.json accepts either form:
//
//   "key": "K"      the name, which is what a player can actually write
//   "key": 37       the raw OIS code, which is what older configs hold
//
// Both are kept because the numeric form shipped first and a config that
// already works must keep working. Names are matched case-insensitively.

// The OIS code for a key name, or 0 when the name is not one this build knows.
int KeyCodeFromName(const std::string& name);

// The name for an OIS code, or an empty string for a code with no name here.
// Used to tell the player which key they have bound, in the words they used.
std::string KeyNameFromCode(int code);

// The whole binding as one readable phrase, e.g. "Ctrl + K". Written to the log
// at startup, because "the window does not open" is usually a key the player
// changed and then forgot.
std::string HotkeyDescription();
} // namespace totaleditor
