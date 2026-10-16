#pragma once

#include <string>

class Character;

namespace totaleditor
{
namespace CharacterIdentity
{
bool GetOrCreate(Character* character, std::string& id);
} // namespace CharacterIdentity
} // namespace totaleditor
