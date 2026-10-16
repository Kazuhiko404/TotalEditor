#pragma once

#include <string>

class Character;

namespace totaleditor
{
namespace UniformEngine
{
bool Install();
void Tick();
void ForgetForLoad();

// Live Preview API
void BeginPreview(Character* c);
void PreviewColours(Character* c, const std::string& primary, const std::string& secondary);
void CancelPreview(Character* c);

// Persistence & Vanilla API
bool SaveColours(Character* c, const std::string& primary, const std::string& secondary);
bool RestoreVanilla(Character* c);

// Direct application
bool Apply(Character* c, const std::string& primary, const std::string& secondary);

void BeforeSave();
void AfterSave();
} // namespace UniformEngine
} // namespace totaleditor
