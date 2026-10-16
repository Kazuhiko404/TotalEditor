#include "common.h"
#include "json.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace totaleditor
{
namespace
{
// Returned by every accessor that finds nothing, so callers never see a null
// pointer and can chain lookups without checking each step.
const JsonValue& NullValue()
{
    static JsonValue empty;
    return empty;
}

void SkipSpace(const std::string& text, size_t* pos)
{
    while (*pos < text.size() && isspace((unsigned char)text[*pos]))
    {
        ++(*pos);
    }
}

bool ParseValue(const std::string& text, size_t* pos, JsonValue* out);

// Reads one JSON string starting at the opening quote. Handles the escapes a
// translator would actually type; everything else passes through as written.
bool ParseString(const std::string& text, size_t* pos, std::string* out)
{
    if (*pos >= text.size() || text[*pos] != '"')
    {
        return false;
    }
    ++(*pos);

    std::string value;
    while (*pos < text.size() && text[*pos] != '"')
    {
        if (text[*pos] == '\\' && (*pos + 1) < text.size())
        {
            const char next = text[*pos + 1];
            if (next == 'n')      { value += '\n'; }
            else if (next == 't') { value += '\t'; }
            else if (next == 'r') { value += '\r'; }
            else                  { value += next; }
            *pos += 2;
            continue;
        }
        value += text[*pos];
        ++(*pos);
    }
    if (*pos >= text.size())
    {
        return false;
    }

    ++(*pos);
    *out = value;
    return true;
}

bool ParseObjectBody(const std::string& text, size_t* pos, JsonValue* out)
{
    *out = JsonValue::MakeObject();
    ++(*pos); // consume the opening brace

    SkipSpace(text, pos);
    if (*pos < text.size() && text[*pos] == '}')
    {
        ++(*pos);
        return true;
    }

    for (;;)
    {
        SkipSpace(text, pos);
        std::string key;
        if (!ParseString(text, pos, &key))
        {
            return false;
        }

        SkipSpace(text, pos);
        if (*pos >= text.size() || text[*pos] != ':')
        {
            return false;
        }
        ++(*pos);

        JsonValue child;
        if (!ParseValue(text, pos, &child))
        {
            return false;
        }
        out->Set(key, child);

        SkipSpace(text, pos);
        if (*pos >= text.size())
        {
            return false;
        }
        if (text[*pos] == ',')
        {
            ++(*pos);
            continue;
        }
        if (text[*pos] == '}')
        {
            ++(*pos);
            return true;
        }
        return false;
    }
}

bool ParseArrayBody(const std::string& text, size_t* pos, JsonValue* out)
{
    *out = JsonValue::MakeArray();
    ++(*pos); // consume the opening bracket

    SkipSpace(text, pos);
    if (*pos < text.size() && text[*pos] == ']')
    {
        ++(*pos);
        return true;
    }

    for (;;)
    {
        JsonValue child;
        if (!ParseValue(text, pos, &child))
        {
            return false;
        }
        out->Append(child);

        SkipSpace(text, pos);
        if (*pos >= text.size())
        {
            return false;
        }
        if (text[*pos] == ',')
        {
            ++(*pos);
            continue;
        }
        if (text[*pos] == ']')
        {
            ++(*pos);
            return true;
        }
        return false;
    }
}

bool ParseLiteral(const std::string& text, size_t* pos,
                  const char* literal, const JsonValue& value, JsonValue* out)
{
    const size_t length = strlen(literal);
    if (text.compare(*pos, length, literal) != 0)
    {
        return false;
    }
    *pos += length;
    *out = value;
    return true;
}

bool ParseNumber(const std::string& text, size_t* pos, JsonValue* out)
{
    const size_t start = *pos;
    if (*pos < text.size() && (text[*pos] == '-' || text[*pos] == '+'))
    {
        ++(*pos);
    }
    while (*pos < text.size() &&
           (isdigit((unsigned char)text[*pos]) || text[*pos] == '.' ||
            text[*pos] == 'e' || text[*pos] == 'E' ||
            text[*pos] == '-' || text[*pos] == '+'))
    {
        ++(*pos);
    }
    if (*pos == start)
    {
        return false;
    }

    *out = JsonValue::MakeNumber(atof(text.substr(start, *pos - start).c_str()));
    return true;
}

bool ParseValue(const std::string& text, size_t* pos, JsonValue* out)
{
    SkipSpace(text, pos);
    if (*pos >= text.size())
    {
        return false;
    }

    const char c = text[*pos];
    if (c == '{') { return ParseObjectBody(text, pos, out); }
    if (c == '[') { return ParseArrayBody(text, pos, out); }
    if (c == '"')
    {
        std::string value;
        if (!ParseString(text, pos, &value))
        {
            return false;
        }
        *out = JsonValue::MakeString(value);
        return true;
    }
    if (c == 't') { return ParseLiteral(text, pos, "true",  JsonValue::MakeBool(true),  out); }
    if (c == 'f') { return ParseLiteral(text, pos, "false", JsonValue::MakeBool(false), out); }
    if (c == 'n') { return ParseLiteral(text, pos, "null",  JsonValue(),                out); }
    return ParseNumber(text, pos, out);
}

std::string EscapeString(const std::string& text)
{
    std::string out;
    for (size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        if      (c == '"')  { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else if (c == '\n') { out += "\\n"; }
        else if (c == '\t') { out += "\\t"; }
        else if (c == '\r') { out += "\\r"; }
        else                { out += c; }
    }
    return out;
}

// Trailing zeroes make a hand-edited file harder to read, and a multiplier
// written as 1.15 should come back as 1.15 rather than 1.150000.
std::string FormatNumber(double value)
{
    char buffer[64] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%.6g", value);
    return std::string(buffer);
}

std::string Indent(int depth)
{
    return std::string((size_t)depth * 2, ' ');
}
} // namespace

double JsonValue::AsNumber(double fallback) const
{
    return (kind_ == NUMBER) ? number_ : fallback;
}

bool JsonValue::AsBool(bool fallback) const
{
    return (kind_ == BOOL) ? boolean_ : fallback;
}

std::string JsonValue::AsString(const std::string& fallback) const
{
    return (kind_ == STRING) ? string_ : fallback;
}

const JsonValue& JsonValue::Child(const std::string& key) const
{
    if (kind_ != OBJECT)
    {
        return NullValue();
    }
    std::map<std::string, JsonValue>::const_iterator found = members_.find(key);
    return (found != members_.end()) ? found->second : NullValue();
}

const JsonValue& JsonValue::At(const std::string& path) const
{
    const JsonValue* current = this;
    size_t start = 0;
    for (;;)
    {
        const size_t dot = path.find('.', start);
        const std::string key = (dot == std::string::npos)
            ? path.substr(start)
            : path.substr(start, dot - start);

        current = &current->Child(key);
        if (current->kind() == NUL || dot == std::string::npos)
        {
            return *current;
        }
        start = dot + 1;
    }
}

JsonValue JsonValue::MakeObject()
{
    JsonValue value;
    value.kind_ = OBJECT;
    return value;
}

JsonValue JsonValue::MakeArray()
{
    JsonValue value;
    value.kind_ = ARRAY;
    return value;
}

JsonValue JsonValue::MakeNumber(double number)
{
    JsonValue value;
    value.kind_ = NUMBER;
    value.number_ = number;
    return value;
}

JsonValue JsonValue::MakeString(const std::string& text)
{
    JsonValue value;
    value.kind_ = STRING;
    value.string_ = text;
    return value;
}

JsonValue JsonValue::MakeBool(bool boolean)
{
    JsonValue value;
    value.kind_ = BOOL;
    value.boolean_ = boolean;
    return value;
}

void JsonValue::Set(const std::string& key, const JsonValue& value)
{
    if (kind_ != OBJECT)
    {
        *this = MakeObject();
    }
    if (members_.find(key) == members_.end())
    {
        keys_.push_back(key);
    }
    members_[key] = value;
}

void JsonValue::Append(const JsonValue& value)
{
    if (kind_ != ARRAY)
    {
        *this = MakeArray();
    }
    array_.push_back(value);
}

std::string JsonValue::Serialise(int indent) const
{
    switch (kind_)
    {
    case NUL:    return "null";
    case BOOL:   return boolean_ ? "true" : "false";
    case NUMBER: return FormatNumber(number_);
    case STRING: return "\"" + EscapeString(string_) + "\"";

    case ARRAY:
    {
        if (array_.empty())
        {
            return "[]";
        }
        std::string out = "[\n";
        for (size_t i = 0; i < array_.size(); ++i)
        {
            out += Indent(indent + 1) + array_[i].Serialise(indent + 1);
            out += (i + 1 < array_.size()) ? ",\n" : "\n";
        }
        out += Indent(indent) + "]";
        return out;
    }

    case OBJECT:
    {
        if (keys_.empty())
        {
            return "{}";
        }
        std::string out = "{\n";
        for (size_t i = 0; i < keys_.size(); ++i)
        {
            std::map<std::string, JsonValue>::const_iterator found =
                members_.find(keys_[i]);
            if (found == members_.end())
            {
                continue;
            }
            out += Indent(indent + 1) + "\"" + EscapeString(keys_[i]) + "\": ";
            out += found->second.Serialise(indent + 1);
            out += (i + 1 < keys_.size()) ? ",\n" : "\n";
        }
        out += Indent(indent) + "}";
        return out;
    }
    }
    return "null";
}

bool ParseJson(const std::string& text, JsonValue* out)
{
    size_t pos = 0;
    JsonValue parsed;
    if (!ParseValue(text, &pos, &parsed))
    {
        return false;
    }
    *out = parsed;
    return true;
}

bool ReadFile(const std::string& path, std::string* out)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    *out = buffer.str();
    return true;
}

bool WriteFile(const std::string& path, const std::string& text)
{
    std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        return false;
    }
    file << text;
    return file.good();
}
} // namespace totaleditor
