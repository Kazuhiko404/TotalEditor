#include "common.h"
#include "messages.h"
#include "json.h"
#include "log.h"

#include <cstdio>
#include <map>
#include <set>

namespace totaleditor
{
namespace
{
// Flattened at load time: "window.title" -> the line. Flattening keeps every
// lookup a single map hit, which matters because these run while the window is
// being laid out.
std::map<std::string, std::string> g_lines;

// Keys the code asked for and the file did not have. A set, reported once,
// because per-call logging would drown the log it is meant to help.
std::set<std::string> g_missing;

// Walks the tree and records every string leaf under its dotted path. Anything
// that is not a string is skipped rather than rejected, so a file carrying
// extra structure still loads the lines it does have.
void Flatten(const JsonValue& node, const std::string& prefix)
{
    if (!node.isObject())
    {
        return;
    }

    const std::vector<std::string>& keys = node.Keys();
    for (size_t i = 0; i < keys.size(); ++i)
    {
        const std::string path = prefix.empty() ? keys[i] : prefix + "." + keys[i];
        const JsonValue& child = node.Child(keys[i]);

        if (child.kind() == JsonValue::STRING)
        {
            g_lines[path] = child.AsString("");
        }
        else if (child.isObject())
        {
            Flatten(child, path);
        }
    }
}

// Replaces every occurrence of one token. Written out rather than using a
// regex because this runs during layout and the patterns are three fixed
// literals.
std::string Replace(const std::string& text, const std::string& token,
                    const std::string& value)
{
    std::string out = text;
    size_t at = out.find(token);
    while (at != std::string::npos)
    {
        out.replace(at, token.size(), value);
        at = out.find(token, at + value.size());
    }
    return out;
}
} // namespace

void LoadMessages(const std::string& modDirectory)
{
    g_lines.clear();
    g_missing.clear();

    if (modDirectory.empty())
    {
        LogInfo("No mod directory; every line falls back to its default.");
        return;
    }

    const std::string path = modDirectory + "\\messages.json";
    std::string text;
    if (!ReadFile(path, &text))
    {
        LogInfo("No messages.json found; every line falls back to its default.");
        return;
    }

    JsonValue root;
    if (!ParseJson(text, &root) || !root.isObject())
    {
        LogError("messages.json is malformed; every line falls back to its default.");
        return;
    }

    Flatten(root, "");

    char count[16] = { 0 };
    _snprintf_s(count, sizeof(count), _TRUNCATE, "%d", (int)g_lines.size());
    LogInfo("messages.json loaded (" + std::string(count) + " lines).");
}

std::string Text(const std::string& path, const std::string& fallback)
{
    std::map<std::string, std::string>::const_iterator found = g_lines.find(path);
    if (found != g_lines.end())
    {
        return found->second;
    }

    g_missing.insert(path);
    return fallback;
}

std::string Fill(const std::string& text, const std::string& name,
                 const std::string& value, int count)
{
    char number[16] = { 0 };
    _snprintf_s(number, sizeof(number), _TRUNCATE, "%d", count);

    std::string out = Replace(text, "{name}", name);
    out = Replace(out, "{value}", value);
    out = Replace(out, "{count}", number);
    return out;
}

void ReportMissingKeys()
{
    if (g_missing.empty())
    {
        return;
    }

    std::string joined;
    for (std::set<std::string>::const_iterator it = g_missing.begin();
         it != g_missing.end(); ++it)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += *it;
    }

    LogInfo("messages.json is missing these keys (defaults were used): " + joined);
}
} // namespace totaleditor
