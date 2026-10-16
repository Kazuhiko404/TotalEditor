#include "common.h"
#include "uniform_dresser.h"
#include "log.h"
#include "uniform_schemes.h"

#include <kenshi/Appearance.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>

#ifndef NULL
#define NULL 0
#endif

namespace totaleditor
{
namespace UniformDresser
{
namespace
{
const int kMaxItems = 24;
const int kMaxServed = 256;

struct Served
{
    Character* character;
    GameData* scheme;
};

SRWLOCK g_servedLock = SRWLOCK_INIT;
Served g_served[kMaxServed];
int g_servedCount = 0;
volatile LONG g_passThrough = 0;

GameData* (*g_getUniformColorScheme_orig)(Character*) = NULL;

GameData* GetUniformColorScheme_hook(Character* self)
{
    if (self != NULL && g_passThrough == 0)
    {
        GameData* scheme = NULL;
        AcquireSRWLockShared(&g_servedLock);
        for (int i = 0; i < g_servedCount; ++i)
        {
            if (g_served[i].character == self)
            {
                scheme = g_served[i].scheme;
                break;
            }
        }
        ReleaseSRWLockShared(&g_servedLock);
        if (scheme != NULL)
            return scheme;
    }
    return g_getUniformColorScheme_orig(self);
}

struct ItemRead
{
    Item* item;
    GameData* colorise;
    ItemSnap id;
};

void ReadHandle(Item* item, ItemSnap* out)
{
    const hand& h = item->getHandle();
    out->type = static_cast<unsigned int>(h.type);
    out->container = h.container;
    out->containerSerial = h.containerSerial;
    out->index = h.index;
    out->serial = h.serial;
}

bool SafeReadItem(Item* item, ItemRead* out)
{
    out->item = item;
    TE_TRY
    {
        ReadHandle(item, &out->id);
        out->colorise = item->coloriseData;
        return true;
    }
    TE_CATCH { return false; }
}

bool SameItem(const ItemSnap& a, const ItemSnap& b)
{
    return a.index == b.index && a.serial == b.serial && a.type == b.type;
}

Item* SafeResolve(const ItemSnap& s)
{
    TE_TRY
    {
        const hand h(s.index, s.serial, static_cast<itemType>(s.type), s.container, s.containerSerial);
        return h.getItem();
    }
    TE_CATCH { return static_cast<Item*>(NULL); }
}

GameData* SafeColorise(Item* item)
{
    TE_TRY { return item != NULL ? item->coloriseData : static_cast<GameData*>(NULL); }
    TE_CATCH { return static_cast<GameData*>(NULL); }
}

bool SafeSetColorise(Item* item, GameData* scheme)
{
    TE_TRY
    {
        item->coloriseData = scheme;
        return true;
    }
    TE_CATCH { return false; }
}

AppearanceBase* SafeAppearance(Character* c)
{
    TE_TRY { return c->getAppearance(); }
    TE_CATCH { return static_cast<AppearanceBase*>(NULL); }
}

GameData* SafeAppearanceColour(AppearanceBase* appearance)
{
    TE_TRY { return appearance != NULL ? appearance->factionColor : static_cast<GameData*>(NULL); }
    TE_CATCH { return static_cast<GameData*>(NULL); }
}

bool SafeSetAppearanceColour(AppearanceBase* appearance, GameData* scheme, bool reload)
{
    TE_TRY
    {
        appearance->factionColor = scheme;
        if (reload)
            appearance->reload();
        return true;
    }
    TE_CATCH { return false; }
}

GameData* SafeOriginalScheme(Character* c)
{
    TE_TRY { return g_getUniformColorScheme_orig(c); }
    TE_CATCH { return static_cast<GameData*>(NULL); }
}

int CollectArmour(Character* c, Item** out, int max)
{
    Inventory* inventory = c->getInventory();
    if (inventory == NULL)
        return 0;
    lektor<Item*> equipped;
    inventory->getEquippedArmour(equipped);
    int n = 0;
    for (uint32_t i = 0; i < equipped.size() && n < max; ++i)
    {
        if (equipped[i] != NULL)
            out[n++] = equipped[i];
    }
    return n;
}

int SafeCollectArmour(Character* c, Item** out, int max)
{
    if (c == NULL)
        return 0;
    TE_TRY { return CollectArmour(c, out, max); }
    TE_CATCH { return 0; }
}

bool ReplayEquip(Character* c, Item* item)
{
    const std::string section(item->inventorySection);
    if (section.empty())
        return false;
    c->unequipItem(section, item);
    c->equipItem(section, item);
    return true;
}

bool SafeReplayEquip(Character* c, Item* item)
{
    TE_TRY { return ReplayEquip(c, item); }
    TE_CATCH { return false; }
}

void SafeRefreshInventoryGui(Character* c)
{
    TE_TRY
    {
        Inventory* inventory = c->getInventory();
        if (inventory != NULL)
            inventory->refreshGui();
    }
    TE_CATCH { }
}

int FindSnap(const Record& rec, const ItemSnap& id)
{
    for (size_t i = 0; i < rec.items.size(); ++i)
    {
        if (SameItem(rec.items[i], id))
            return static_cast<int>(i);
    }
    return -1;
}

int ReadEquipped(Character* c, ItemRead* out, int max)
{
    Item* items[kMaxItems];
    const int n = SafeCollectArmour(c, items, max < kMaxItems ? max : kMaxItems);
    int read = 0;
    for (int i = 0; i < n; ++i)
    {
        if (SafeReadItem(items[i], &out[read]))
            ++read;
    }
    return read;
}

bool IsEquipped(const ItemRead* equipped, int count, const ItemSnap& id)
{
    for (int i = 0; i < count; ++i)
    {
        if (SameItem(equipped[i].id, id))
            return true;
    }
    return false;
}

void Track(Record* rec, const ItemRead& item)
{
    if (FindSnap(*rec, item.id) >= 0)
        return;
    ItemSnap snap = item.id;
    snap.known = !UniformSchemes::IsOurs(item.colorise);
    snap.original = snap.known ? item.colorise : NULL;
    rec->items.push_back(snap);
}

GameData* BackColour(const ItemSnap& snap, GameData* vanilla)
{
    return snap.known ? snap.original : vanilla;
}

bool PutBackAppearance(Character* c, const Record& rec, GameData* vanilla, bool reload)
{
    AppearanceBase* appearance = SafeAppearance(c);
    if (appearance == NULL)
        return false;
    GameData* target = (rec.appearanceTaken && rec.appearance == appearance) ? rec.appearanceBefore : vanilla;
    return SafeSetAppearanceColour(appearance, target, reload);
}

void ReleaseAway(Record* rec, const ItemRead* equipped, int count, GameData* vanilla, Counts* counts)
{
    std::vector<ItemSnap> kept;
    for (size_t i = 0; i < rec->items.size(); ++i)
    {
        const ItemSnap& snap = rec->items[i];
        if (IsEquipped(equipped, count, snap))
        {
            kept.push_back(snap);
            continue;
        }
        Item* item = SafeResolve(snap);
        if (item == NULL)
        {
            ++counts->lost;
            continue;
        }
        if (UniformSchemes::IsOurs(SafeColorise(item)))
        {
            SafeSetColorise(item, BackColour(snap, vanilla));
            ++counts->away;
        }
    }
    rec->items.swap(kept);
}
} // namespace

bool InstallHook()
{
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Character::getUniformColorScheme),
            GetUniformColorScheme_hook, &g_getUniformColorScheme_orig))
    {
        LogError("[uniform] Failed to hook Character::getUniformColorScheme.");
        return false;
    }
    LogInfo("[uniform] Hooked Character::getUniformColorScheme.");
    return true;
}

void Serve(Character* c, GameData* scheme)
{
    if (c == NULL || scheme == NULL)
        return;
    AcquireSRWLockExclusive(&g_servedLock);
    bool done = false;
    for (int i = 0; i < g_servedCount && !done; ++i)
    {
        if (g_served[i].character == c)
        {
            g_served[i].scheme = scheme;
            done = true;
        }
    }
    if (!done && g_servedCount < kMaxServed)
    {
        g_served[g_servedCount].character = c;
        g_served[g_servedCount].scheme = scheme;
        ++g_servedCount;
        done = true;
    }
    ReleaseSRWLockExclusive(&g_servedLock);
    if (!done)
        LogError("[uniform] the served table is full; uniform shows only after re-dress.");
}

void StopServing(Character* c)
{
    AcquireSRWLockExclusive(&g_servedLock);
    for (int i = 0; i < g_servedCount; ++i)
    {
        if (g_served[i].character == c)
        {
            g_served[i] = g_served[g_servedCount - 1];
            --g_servedCount;
            break;
        }
    }
    ReleaseSRWLockExclusive(&g_servedLock);
}

void StopServingAll()
{
    AcquireSRWLockExclusive(&g_servedLock);
    g_servedCount = 0;
    ReleaseSRWLockExclusive(&g_servedLock);
}

void SetPassThrough(bool on)
{
    InterlockedExchange(&g_passThrough, on ? 1 : 0);
}

Counts Dress(Character* c, GameData* scheme, Record* rec)
{
    Counts counts;
    if (c == NULL || scheme == NULL || rec == NULL)
        return counts;

    AppearanceBase* appearance = SafeAppearance(c);
    if (!rec->appearanceTaken && appearance != NULL)
    {
        GameData* current = SafeAppearanceColour(appearance);
        if (!UniformSchemes::IsOurs(current))
        {
            rec->appearanceTaken = true;
            rec->appearance = appearance;
            rec->appearanceBefore = current;
        }
    }
    Serve(c, scheme);
    rec->scheme = scheme;

    ItemRead equipped[kMaxItems];
    const int count = ReadEquipped(c, equipped, kMaxItems);
    GameData* vanilla = SafeOriginalScheme(c);
    ReleaseAway(rec, equipped, count, vanilla, &counts);
    for (int i = 0; i < count; ++i)
    {
        Track(rec, equipped[i]);
        const int snap = FindSnap(*rec, equipped[i].id);
        if (snap >= 0 && !rec->items[static_cast<size_t>(snap)].known)
            ++counts.noSnapshot;
        if (!SafeSetColorise(equipped[i].item, scheme))
            LogError("[uniform] an item's coloriseData write failed.");
        ++counts.items;
    }
    if (appearance != NULL)
        counts.appearance = SafeSetAppearanceColour(appearance, scheme, true);
    for (int i = 0; i < count; ++i)
    {
        if (SafeReplayEquip(c, equipped[i].item))
            ++counts.replayed;
    }
    SafeRefreshInventoryGui(c);
    return counts;
}

Counts Undress(Character* c, Record* rec)
{
    Counts counts;
    if (rec == NULL)
        return counts;
    if (c != NULL)
        StopServing(c);

    ItemRead equipped[kMaxItems];
    const int count = c != NULL ? ReadEquipped(c, equipped, kMaxItems) : 0;
    GameData* vanilla = c != NULL ? SafeOriginalScheme(c) : NULL;
    if (c != NULL)
        counts.appearance = PutBackAppearance(c, *rec, vanilla, true);

    for (int i = 0; i < count; ++i)
    {
        const int snap = FindSnap(*rec, equipped[i].id);
        if (snap >= 0)
        {
            SafeSetColorise(equipped[i].item, BackColour(rec->items[static_cast<size_t>(snap)], vanilla));
            if (!rec->items[static_cast<size_t>(snap)].known)
                ++counts.noSnapshot;
        }
        else if (UniformSchemes::IsOurs(equipped[i].colorise))
        {
            SafeSetColorise(equipped[i].item, vanilla);
            ++counts.noSnapshot;
        }
        ++counts.items;
    }
    ReleaseAway(rec, equipped, count, vanilla, &counts);
    for (int i = 0; i < count; ++i)
    {
        if (SafeReplayEquip(c, equipped[i].item))
            ++counts.replayed;
    }
    if (c != NULL)
        SafeRefreshInventoryGui(c);
    *rec = Record();
    return counts;
}

Counts StripData(Character* c, Record* rec)
{
    Counts counts;
    if (rec == NULL)
        return counts;
    ItemRead equipped[kMaxItems];
    const int count = c != NULL ? ReadEquipped(c, equipped, kMaxItems) : 0;
    GameData* vanilla = c != NULL ? SafeOriginalScheme(c) : NULL;
    for (int i = 0; i < count; ++i)
        Track(rec, equipped[i]);
    for (size_t i = 0; i < rec->items.size(); ++i)
    {
        const ItemSnap& snap = rec->items[i];
        Item* item = SafeResolve(snap);
        if (item == NULL)
        {
            ++counts.lost;
            continue;
        }
        if (!UniformSchemes::IsOurs(SafeColorise(item)))
            continue;
        SafeSetColorise(item, BackColour(snap, vanilla));
        ++counts.items;
        if (!snap.known)
            ++counts.noSnapshot;
    }
    if (c != NULL)
        counts.appearance = PutBackAppearance(c, *rec, vanilla, false);
    return counts;
}

Counts RedressData(Character* c, Record* rec)
{
    Counts counts;
    if (c == NULL || rec == NULL || rec->scheme == NULL)
        return counts;
    ItemRead equipped[kMaxItems];
    const int count = ReadEquipped(c, equipped, kMaxItems);
    std::vector<ItemSnap> kept;
    for (size_t i = 0; i < rec->items.size(); ++i)
    {
        if (IsEquipped(equipped, count, rec->items[i]))
            kept.push_back(rec->items[i]);
        else
            ++counts.away;
    }
    rec->items.swap(kept);
    for (int i = 0; i < count; ++i)
    {
        Track(rec, equipped[i]);
        SafeSetColorise(equipped[i].item, rec->scheme);
        ++counts.items;
    }
    AppearanceBase* appearance = SafeAppearance(c);
    if (appearance != NULL)
        counts.appearance = SafeSetAppearanceColour(appearance, rec->scheme, false);
    return counts;
}

} // namespace UniformDresser
} // namespace totaleditor
