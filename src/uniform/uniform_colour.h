#pragma once

#include <string>

namespace totaleditor
{
namespace UniformColour
{
struct Rgb
{
    int r;
    int g;
    int b;
};

bool ParseHex(const std::string& text, Rgb* out);
std::string ToHex(const Rgb& colour);
bool NormalizeText(const std::string& text, std::string* out);
void ToUnit(const Rgb& colour, float* r, float* g, float* b);
Rgb FromUnit(float r, float g, float b);
} // namespace UniformColour
} // namespace totaleditor
