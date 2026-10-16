#include "uniform_colour.h"
#include <cstdio>
#include <cctype>

namespace totaleditor
{
namespace UniformColour
{
namespace
{
int HexDigitValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int Clamp255(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v;
}
} // namespace

bool ParseHex(const std::string& text, Rgb* out)
{
    if (out == NULL) return false;

    const char* p = text.c_str();
    if (*p == '#') ++p;

    int digits[6];
    for (int i = 0; i < 6; ++i)
    {
        const int val = HexDigitValue(p[i]);
        if (val < 0) return false;
        digits[i] = val;
    }
    if (p[6] != '\0') return false;

    out->r = (digits[0] << 4) | digits[1];
    out->g = (digits[2] << 4) | digits[3];
    out->b = (digits[4] << 4) | digits[5];
    return true;
}

std::string ToHex(const Rgb& colour)
{
    const int r = Clamp255(colour.r);
    const int g = Clamp255(colour.g);
    const int b = Clamp255(colour.b);

    char buf[16];
    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%02X%02X%02X", r, g, b);
    return std::string(buf);
}

bool NormalizeText(const std::string& text, std::string* out)
{
    if (out == NULL) return false;

    if (text.empty())
    {
        out->clear();
        return true;
    }

    Rgb rgb;
    if (!ParseHex(text, &rgb)) return false;

    *out = ToHex(rgb);
    return true;
}

void ToUnit(const Rgb& colour, float* r, float* g, float* b)
{
    if (r) *r = static_cast<float>(Clamp255(colour.r)) / 255.0f;
    if (g) *g = static_cast<float>(Clamp255(colour.g)) / 255.0f;
    if (b) *b = static_cast<float>(Clamp255(colour.b)) / 255.0f;
}

Rgb FromUnit(float r, float g, float b)
{
    Rgb result;
    result.r = Clamp255(static_cast<int>(r * 255.0f + 0.5f));
    result.g = Clamp255(static_cast<int>(g * 255.0f + 0.5f));
    result.b = Clamp255(static_cast<int>(b * 255.0f + 0.5f));
    return result;
}

} // namespace UniformColour
} // namespace totaleditor
