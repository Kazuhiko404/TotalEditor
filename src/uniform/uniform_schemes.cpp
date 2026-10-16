#include "common.h"
#include "uniform_schemes.h"
#include "log.h"
#include "uniform_colour.h"

#include <map>
#include <ogre/OgreVector3.h>

#ifndef NULL
#define NULL 0
#endif

namespace totaleditor
{
namespace UniformSchemes
{
namespace
{
struct Entry
{
    GameData* scheme;
    std::string color1;
    std::string color2;
    Entry() : scheme(NULL) {}
};

std::map<std::string, Entry>& Schemes()
{
    static std::map<std::string, Entry> schemes;
    return schemes;
}

struct Unit
{
    float v[3];
};

bool UnitOf(const std::string& hex, Unit* out)
{
    UniformColour::Rgb rgb = { 0, 0, 0 };
    if (!UniformColour::ParseHex(hex, &rgb))
        return false;
    UniformColour::ToUnit(rgb, &out->v[0], &out->v[1], &out->v[2]);
    return true;
}

void SetColours(GameData* d, const Unit& primary, const Unit& secondary)
{
    const Ogre::Vector3 c1(primary.v[0], primary.v[1], primary.v[2]);
    const Ogre::Vector3 c2(secondary.v[0], secondary.v[1], secondary.v[2]);
    d->add(std::string("color 1"), c1);
    d->add(std::string("color 2"), c2);
    d->activeValues[std::string("color 1")] = true;
    d->activeValues[std::string("color 2")] = true;
    d->setColor(std::string("color 1"), c1);
    d->setColor(std::string("color 2"), c2);
}

GameData* Build(const char* schemeId, const Unit& primary, const Unit& secondary)
{
    GameDataCopyStandalone* d = new GameDataCopyStandalone();
    d->initialise(COLOR_DATA, true);
    d->name = std::string("TotalEditor uniform ") + schemeId;
    d->stringID = schemeId;
    SetColours(d, primary, secondary);
    return d;
}

GameData* SafeBuild(const char* schemeId, const Unit& primary, const Unit& secondary)
{
    TE_TRY { return Build(schemeId, primary, secondary); }
    TE_CATCH { return static_cast<GameData*>(NULL); }
}

bool SafeSetColours(GameData* d, const Unit& primary, const Unit& secondary)
{
    TE_TRY { SetColours(d, primary, secondary); return true; }
    TE_CATCH { return false; }
}
} // namespace

GameData* Ensure(const std::string& schemeId, const std::string& color1, const std::string& color2)
{
    Unit primary;
    Unit secondary;
    if (schemeId.empty() || !UnitOf(color1, &primary) || !UnitOf(color2, &secondary))
    {
        LogError("[uniform] scheme '" + schemeId + "': bad colours '" + color1 + "' / '" + color2 + "'.");
        return NULL;
    }
    Entry& entry = Schemes()[schemeId];
    if (entry.scheme == NULL)
    {
        entry.scheme = SafeBuild(schemeId.c_str(), primary, secondary);
        if (entry.scheme == NULL)
        {
            Schemes().erase(schemeId);
            LogError("[uniform] scheme '" + schemeId + "': allocation failed.");
            return NULL;
        }
        entry.color1 = color1;
        entry.color2 = color2;
        return entry.scheme;
    }
    if (entry.color1 != color1 || entry.color2 != color2)
    {
        if (SafeSetColours(entry.scheme, primary, secondary))
        {
            entry.color1 = color1;
            entry.color2 = color2;
        }
        else
        {
            LogError("[uniform] scheme '" + schemeId + "': update in place failed.");
        }
    }
    return entry.scheme;
}

bool IsOurs(const GameData* scheme)
{
    if (scheme == NULL)
        return false;
    for (std::map<std::string, Entry>::const_iterator it = Schemes().begin(); it != Schemes().end(); ++it)
    {
        if (it->second.scheme == scheme)
            return true;
    }
    return false;
}

std::string IdOf(const GameData* scheme)
{
    if (scheme == NULL)
        return "";
    for (std::map<std::string, Entry>::const_iterator it = Schemes().begin(); it != Schemes().end(); ++it)
    {
        if (it->second.scheme == scheme)
            return it->first;
    }
    return "";
}

} // namespace UniformSchemes
} // namespace totaleditor
