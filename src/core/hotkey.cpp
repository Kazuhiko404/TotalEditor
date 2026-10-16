#include "common.h"
#include "hotkey.h"
#include "config.h"

#include <cctype>

namespace totaleditor
{
namespace
{
struct KeyName
{
    const char* name;
    int code;
};

// The keys worth binding a window to, with the OIS codes taken from
// OISKeyboard.h. Deliberately not the whole keyboard: modifiers, the numpad and
// the media keys are either already spoken for by the game or a poor choice for
// a toggle, and a shorter table is a shorter thing to document.
const KeyName kKeys[] =
{
    { "A", 0x1E }, { "B", 0x30 }, { "C", 0x2E }, { "D", 0x20 },
    { "E", 0x12 }, { "F", 0x21 }, { "G", 0x22 }, { "H", 0x23 },
    { "I", 0x17 }, { "J", 0x24 }, { "K", 0x25 }, { "L", 0x26 },
    { "M", 0x32 }, { "N", 0x31 }, { "O", 0x18 }, { "P", 0x19 },
    { "Q", 0x10 }, { "R", 0x13 }, { "S", 0x1F }, { "T", 0x14 },
    { "U", 0x16 }, { "V", 0x2F }, { "W", 0x11 }, { "X", 0x2D },
    { "Y", 0x15 }, { "Z", 0x2C },

    { "0", 0x0B }, { "1", 0x02 }, { "2", 0x03 }, { "3", 0x04 },
    { "4", 0x05 }, { "5", 0x06 }, { "6", 0x07 }, { "7", 0x08 },
    { "8", 0x09 }, { "9", 0x0A },

    { "F1",  0x3B }, { "F2",  0x3C }, { "F3",  0x3D }, { "F4",  0x3E },
    { "F5",  0x3F }, { "F6",  0x40 }, { "F7",  0x41 }, { "F8",  0x42 },
    { "F9",  0x43 }, { "F10", 0x44 }, { "F11", 0x57 }, { "F12", 0x58 },

    { "HOME",   0xC7 }, { "END",    0xCF }, { "INSERT", 0xD2 },
    { "DELETE", 0xD3 }, { "PGUP",   0xC9 }, { "PGDOWN", 0xD1 },

    { "SPACE",      0x39 }, { "ENTER",     0x1C }, { "TAB",       0x0F },
    { "BACKSPACE",  0x0E }, { "ESCAPE",    0x01 }, { "GRAVE",     0x29 },
    { "MINUS",      0x0C }, { "EQUALS",    0x0D }, { "LBRACKET",  0x1A },
    { "RBRACKET",   0x1B }, { "SEMICOLON", 0x27 }, { "APOSTROPHE",0x28 },
    { "BACKSLASH",  0x2B }, { "COMMA",     0x33 }, { "PERIOD",    0x34 },
    { "SLASH",      0x35 },
};

const int kKeyCount = (int)(sizeof(kKeys) / sizeof(kKeys[0]));

std::string ToUpper(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
    {
        out[i] = (char)toupper((unsigned char)out[i]);
    }
    return out;
}
} // namespace

int KeyCodeFromName(const std::string& name)
{
    const std::string wanted = ToUpper(name);
    for (int i = 0; i < kKeyCount; ++i)
    {
        if (wanted == kKeys[i].name)
        {
            return kKeys[i].code;
        }
    }
    return 0;
}

std::string KeyNameFromCode(int code)
{
    for (int i = 0; i < kKeyCount; ++i)
    {
        if (kKeys[i].code == code)
        {
            return kKeys[i].name;
        }
    }
    return std::string();
}

std::string HotkeyDescription()
{
    const Config& config = GetConfig();

    std::string key = KeyNameFromCode(config.toggleKey);
    if (key.empty())
    {
        // A code with no name in the table above: report the number, which is
        // still what the player wrote and still tells us what they bound.
        char buffer[32] = { 0 };
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "key code %d",
                    config.toggleKey);
        key = buffer;
    }

    return config.toggleNeedsCtrl ? ("Ctrl + " + key) : key;
}
} // namespace totaleditor
