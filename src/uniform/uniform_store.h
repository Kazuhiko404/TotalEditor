#pragma once

#include <string>

namespace totaleditor
{
namespace UniformStore
{
void Set(const std::string& characterId, const std::string& primary, const std::string& secondary);
bool Get(const std::string& characterId, std::string* primary, std::string* secondary);
void Remove(const std::string& characterId);
void Clear();

void SetCurrentSaveFolder(const std::string& folder);
const std::string& CurrentSaveFolder();

bool LoadFromFolder(const std::string& saveFolder);
bool SaveToFolder(const std::string& saveFolder);

bool InstallHooks();
} // namespace UniformStore
} // namespace totaleditor
