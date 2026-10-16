#include "common.h"
#include "uniform_engine.h"
#include "log.h"
#include "character_identity.h"
#include "uniform_store.h"
#include "uniform_schemes.h"
#include "uniform_dresser.h"

#include <kenshi/SaveManager.h>
#include <kenshi/Character.h>
#include <kenshi/PlayerInterface.h>
#include <vector>

namespace totaleditor
{
namespace UniformEngine
{
namespace
{
struct WornEntry
{
    std::string uid;
    Character* character;
    std::string primary;
    std::string secondary;
    UniformDresser::Record record;
    WornEntry() : character(NULL) {}
};

std::vector<WornEntry> g_worn;
bool g_installed = false;
bool g_dirty = false;
bool g_postLoad = true;
unsigned long g_squadSeenAt = 0;
unsigned long g_lastPoll = 0;

// Preview tracking state
bool g_previewActive = false;
Character* g_previewTarget = NULL;
bool g_previewHadInitial = false;
std::string g_previewInitialP;
std::string g_previewInitialS;

void (*g_saveOriginal)(SaveManager*, const std::string&, bool) = NULL;
void (*g_updateAutoSaveOriginal)(SaveManager*) = NULL;

WornEntry* FindWorn(const std::string& uid)
{
    for (size_t i = 0; i < g_worn.size(); ++i)
    {
        if (g_worn[i].uid == uid)
            return &g_worn[i];
    }
    return NULL;
}

void EraseWorn(const std::string& uid)
{
    for (size_t i = 0; i < g_worn.size(); ++i)
    {
        if (g_worn[i].uid == uid)
        {
            g_worn[i] = g_worn.back();
            g_worn.pop_back();
            return;
        }
    }
}

bool AnySquadMember()
{
    TE_TRY
    {
        return ou != NULL && ou->player != NULL && ou->player->getAllPlayerCharacters().size() > 0;
    }
    TE_CATCH { return false; }
}

bool IsHumanSquadMember(Character* c)
{
    if (c == NULL) return false;
    bool playerOwned = false;
    bool animal = false;
    TE_TRY
    {
        playerOwned = c->isPlayerCharacter();
        animal = (c->isAnimal() != NULL);
    }
    TE_CATCH { return false; }
    return playerOwned && !animal;
}

const int kMaxSquad = 256;
int CollectSquadCharacters(Character** out, int maxCount)
{
    TE_TRY
    {
        if (ou == NULL || ou->player == NULL)
            return 0;
        const lektor<Character*>& list = ou->player->getAllPlayerCharacters();
        int count = 0;
        for (unsigned int i = 0; i < list.size() && count < maxCount; ++i)
        {
            if (list[i] != NULL)
                out[count++] = list[i];
        }
        return count;
    }
    TE_CATCH { return 0; }
}

void SyncSquad()
{
    Character* squad[kMaxSquad];
    const int count = CollectSquadCharacters(squad, kMaxSquad);

    for (int i = 0; i < count; ++i)
    {
        Character* c = squad[i];
        if (!IsHumanSquadMember(c))
            continue;

        std::string uid;
        if (!CharacterIdentity::GetOrCreate(c, uid))
            continue;

        std::string p, s;
        if (UniformStore::Get(uid, &p, &s))
        {
            Apply(c, p, s);
        }
    }
}

void SaveHook(SaveManager* self, const std::string& name, bool autosave)
{
    BeforeSave();
    g_saveOriginal(self, name, autosave);
    AfterSave();
}

void UpdateAutoSaveHook(SaveManager* self)
{
    BeforeSave();
    g_updateAutoSaveOriginal(self);
    AfterSave();
}

} // namespace

bool Install()
{
    if (g_installed)
        return true;

    if (!UniformStore::InstallHooks())
    {
        LogError("[uniform] Failed to install UniformStore hooks.");
        return false;
    }

    if (!UniformDresser::InstallHook())
    {
        LogError("[uniform] Failed to install UniformDresser hook.");
        return false;
    }

    SaveManager* sm = SaveManager::getSingleton();
    if (sm != NULL)
    {
        if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::save),
                               &SaveHook, &g_saveOriginal) != KenshiLib::SUCCESS)
        {
            LogError("[uniform] Failed to hook SaveManager::save.");
        }

        if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::updateAutoSave),
                               &UpdateAutoSaveHook, &g_updateAutoSaveOriginal) != KenshiLib::SUCCESS)
        {
            LogError("[uniform] Failed to hook SaveManager::updateAutoSave.");
        }
    }

    g_installed = true;
    LogInfo("[uniform] UniformEngine installed successfully.");
    return true;
}

void Tick()
{
    if (!g_installed) return;

    const unsigned long now = GetTickCount();
    const unsigned long kPollMs = 500;
    const unsigned long kPostLoadDelayMs = 5000;

    if (g_postLoad)
    {
        if (now - g_lastPoll < kPollMs) return;
        g_lastPoll = now;

        if (!AnySquadMember())
        {
            g_squadSeenAt = 0;
            return;
        }

        if (g_squadSeenAt == 0)
        {
            g_squadSeenAt = now == 0 ? 1 : now;
            return;
        }

        if (now - g_squadSeenAt < kPostLoadDelayMs)
            return;

        g_postLoad = false;
        g_dirty = true;
        LogInfo("[uniform] Post-load squad ready: redressing uniforms.");
    }

    if (g_dirty)
    {
        g_dirty = false;
        SyncSquad();
    }
}

void ForgetForLoad()
{
    g_worn.clear();
    UniformDresser::StopServingAll();
    UniformDresser::SetPassThrough(false);
    g_previewActive = false;
    g_previewTarget = NULL;
    g_dirty = false;
    g_postLoad = true;
    g_squadSeenAt = 0;
    LogInfo("[uniform] Load boundary: cleared uniforms, will re-dress after squad load.");
}

void BeforeSave()
{
    UniformDresser::SetPassThrough(true);
    for (size_t i = 0; i < g_worn.size(); ++i)
    {
        if (g_worn[i].character != NULL)
        {
            UniformDresser::StripData(g_worn[i].character, &g_worn[i].record);
        }
    }
    LogDebug("[uniform] Stripped uniforms before save.");
}

void AfterSave()
{
    UniformDresser::SetPassThrough(false);
    for (size_t i = 0; i < g_worn.size(); ++i)
    {
        if (g_worn[i].character != NULL)
        {
            UniformDresser::RedressData(g_worn[i].character, &g_worn[i].record);
        }
    }
    LogDebug("[uniform] Redressed uniforms after save.");
}

void BeginPreview(Character* c)
{
    if (c == NULL) return;

    std::string uid;
    if (!CharacterIdentity::GetOrCreate(c, uid))
        return;

    g_previewActive = true;
    g_previewTarget = c;
    g_previewHadInitial = UniformStore::Get(uid, &g_previewInitialP, &g_previewInitialS);
}

void PreviewColours(Character* c, const std::string& primary, const std::string& secondary)
{
    if (c == NULL) return;

    std::string uid;
    if (!CharacterIdentity::GetOrCreate(c, uid)) return;

    const std::string schemeId = "uc-char-" + uid;
    GameData* scheme = UniformSchemes::Ensure(schemeId, primary, secondary);
    if (scheme == NULL) return;

    WornEntry* entry = FindWorn(uid);
    if (entry == NULL)
    {
        WornEntry newEntry;
        newEntry.uid = uid;
        newEntry.character = c;
        newEntry.primary = primary;
        newEntry.secondary = secondary;
        g_worn.push_back(newEntry);
        entry = &g_worn.back();
    }
    else
    {
        entry->character = c;
        entry->primary = primary;
        entry->secondary = secondary;
    }

    // Live dress: updates material color maps in real-time on armor!
    UniformDresser::Dress(c, scheme, &entry->record);
}

void CancelPreview(Character* c)
{
    if (!g_previewActive || c == NULL) return;

    std::string uid;
    if (CharacterIdentity::GetOrCreate(c, uid))
    {
        if (g_previewHadInitial)
        {
            PreviewColours(c, g_previewInitialP, g_previewInitialS);
        }
        else
        {
            WornEntry* entry = FindWorn(uid);
            if (entry != NULL)
            {
                UniformDresser::Undress(c, &entry->record);
                EraseWorn(uid);
            }
        }
    }

    g_previewActive = false;
    g_previewTarget = NULL;
}

bool SaveColours(Character* c, const std::string& primary, const std::string& secondary)
{
    if (c == NULL) return false;

    std::string uid;
    if (!CharacterIdentity::GetOrCreate(c, uid)) return false;

    UniformStore::Set(uid, primary, secondary);
    if (!UniformStore::CurrentSaveFolder().empty())
    {
        UniformStore::SaveToFolder(UniformStore::CurrentSaveFolder());
    }

    g_previewHadInitial = true;
    g_previewInitialP = primary;
    g_previewInitialS = secondary;

    return Apply(c, primary, secondary);
}

bool RestoreVanilla(Character* c)
{
    if (c == NULL) return false;

    std::string uid;
    if (!CharacterIdentity::GetOrCreate(c, uid)) return false;

    UniformStore::Remove(uid);
    if (!UniformStore::CurrentSaveFolder().empty())
    {
        UniformStore::SaveToFolder(UniformStore::CurrentSaveFolder());
    }

    WornEntry* entry = FindWorn(uid);
    if (entry != NULL)
    {
        UniformDresser::Undress(c, &entry->record);
        EraseWorn(uid);
    }

    g_previewHadInitial = false;
    g_previewInitialP.clear();
    g_previewInitialS.clear();

    LogInfo("[uniform] Restored character " + uid + " to vanilla colours.");
    return true;
}

bool Apply(Character* c, const std::string& primary, const std::string& secondary)
{
    if (c == NULL) return false;

    std::string uid;
    if (!CharacterIdentity::GetOrCreate(c, uid)) return false;

    const std::string schemeId = "uc-char-" + uid;
    GameData* scheme = UniformSchemes::Ensure(schemeId, primary, secondary);
    if (scheme == NULL) return false;

    WornEntry* entry = FindWorn(uid);
    if (entry == NULL)
    {
        WornEntry newEntry;
        newEntry.uid = uid;
        newEntry.character = c;
        newEntry.primary = primary;
        newEntry.secondary = secondary;
        g_worn.push_back(newEntry);
        entry = &g_worn.back();
    }
    else
    {
        entry->character = c;
        entry->primary = primary;
        entry->secondary = secondary;
    }

    UniformDresser::Dress(c, scheme, &entry->record);
    return true;
}

} // namespace UniformEngine
} // namespace totaleditor
