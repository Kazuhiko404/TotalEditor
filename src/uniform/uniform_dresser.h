#pragma once

#include <vector>

class Character;
class GameData;
class Item;

namespace totaleditor
{
namespace UniformDresser
{
struct ItemSnap
{
    unsigned int type;
    unsigned int container;
    unsigned int containerSerial;
    unsigned int index;
    unsigned int serial;
    GameData* original;
    bool known;

    ItemSnap() : type(0), container(0), containerSerial(0), index(0), serial(0), original(0), known(false) {}
};

struct Record
{
    GameData* scheme;
    bool appearanceTaken;
    const void* appearance;
    GameData* appearanceBefore;
    std::vector<ItemSnap> items;

    Record() : scheme(0), appearanceTaken(false), appearance(0), appearanceBefore(0) {}
};

struct Counts
{
    int items;
    int replayed;
    int noSnapshot;
    int away;
    int lost;
    bool appearance;

    Counts() : items(0), replayed(0), noSnapshot(0), away(0), lost(0), appearance(false) {}
};

bool InstallHook();

void Serve(Character* c, GameData* scheme);
void StopServing(Character* c);
void StopServingAll();
void SetPassThrough(bool on);

Counts Dress(Character* c, GameData* scheme, Record* rec);
Counts Undress(Character* c, Record* rec);

Counts StripData(Character* c, Record* rec);
Counts RedressData(Character* c, Record* rec);
} // namespace UniformDresser
} // namespace totaleditor
