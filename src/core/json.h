#pragma once

#include <map>
#include <string>
#include <vector>

namespace totaleditor
{
// A minimal JSON tree: objects, arrays, strings, numbers and booleans.
//
// A dependency would be a poor trade in a plugin that has to stay loadable, and
// the three files this mod reads -- config, messages and the override store --
// share one shape between them. One reader serves all three.
class JsonValue
{
public:
    enum Kind { NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT };

    JsonValue() : kind_(NUL), number_(0.0), boolean_(false) {}

    Kind kind() const { return kind_; }
    bool isObject() const { return kind_ == OBJECT; }
    bool isArray() const { return kind_ == ARRAY; }

    // Reading accessors. Each returns the fallback when this value is absent or
    // of another kind, so a hand-broken file costs a default and never a crash.
    double AsNumber(double fallback) const;
    bool AsBool(bool fallback) const;
    std::string AsString(const std::string& fallback) const;

    // Object member lookup by dotted path ("window.width"). Returns a null
    // value when any step is missing.
    const JsonValue& At(const std::string& path) const;

    // Direct child lookup, one key, no path splitting.
    const JsonValue& Child(const std::string& key) const;

    const std::vector<JsonValue>& Items() const { return array_; }

    // Object keys in insertion order, so a round-trip keeps the file readable.
    const std::vector<std::string>& Keys() const { return keys_; }

    // Building accessors, used to assemble the override store before saving.
    static JsonValue MakeObject();
    static JsonValue MakeArray();
    static JsonValue MakeNumber(double value);
    static JsonValue MakeString(const std::string& value);
    static JsonValue MakeBool(bool value);

    void Set(const std::string& key, const JsonValue& value);
    void Append(const JsonValue& value);

    // Serialises with two-space indentation, so a saved file stays hand-editable.
    std::string Serialise(int indent) const;

private:
    Kind kind_;
    double number_;
    bool boolean_;
    std::string string_;
    std::vector<JsonValue> array_;
    std::map<std::string, JsonValue> members_;
    std::vector<std::string> keys_;
};

// Parses text into a tree. Returns false on malformed input, leaving out
// untouched; the caller then keeps its defaults.
bool ParseJson(const std::string& text, JsonValue* out);

// Reads a whole file. Returns false when it cannot be opened.
bool ReadFile(const std::string& path, std::string* out);

// Writes text to a file, replacing it. Returns false when it cannot be written.
bool WriteFile(const std::string& path, const std::string& text);
} // namespace totaleditor
