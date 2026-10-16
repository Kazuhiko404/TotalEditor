#pragma once

#include <string>

class GameData;

namespace totaleditor
{
namespace UniformSchemes
{
GameData* Ensure(const std::string& schemeId, const std::string& color1, const std::string& color2);
bool IsOurs(const GameData* scheme);
std::string IdOf(const GameData* scheme);
} // namespace UniformSchemes
} // namespace totaleditor
