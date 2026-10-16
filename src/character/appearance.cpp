#include "common.h"
#include "appearance.h"

namespace totaleditor
{
namespace
{
// The guarded frames hold only PODs: __try may not share a frame with
// anything that needs unwinding.
bool SafeIsPlayerCharacter(Character* character, bool* out)
{
    TE_TRY
    {
        *out = character->isPlayerCharacter();
        return true;
    }
    TE_CATCH { return false; }
}

bool SafePlayerInterface(PlayerInterface** out)
{
    TE_TRY
    {
        *out = (ou != NULL) ? ou->player : NULL;
        return true;
    }
    TE_CATCH { return false; }
}

bool SafeActivateEditMode(PlayerInterface* player, Character* character)
{
    TE_TRY
    {
        player->activateCharacterEditMode(character);
        return true;
    }
    TE_CATCH { return false; }
}
} // namespace

AppearanceResult CanEditAppearance(Character* character)
{
    if (character == NULL)
    {
        return APPEARANCE_NO_CHARACTER;
    }
    bool isPlayer = false;
    if (!SafeIsPlayerCharacter(character, &isPlayer))
    {
        return APPEARANCE_FAULT;
    }
    return isPlayer ? APPEARANCE_OK : APPEARANCE_NOT_PLAYER;
}

AppearanceResult OpenAppearanceEditor(Character* character)
{
    const AppearanceResult allowed = CanEditAppearance(character);
    if (allowed != APPEARANCE_OK)
    {
        return allowed;
    }

    PlayerInterface* player = NULL;
    if (!SafePlayerInterface(&player))
    {
        return APPEARANCE_FAULT;
    }
    if (player == NULL)
    {
        return APPEARANCE_NO_PLAYER_INTERFACE;
    }
    return SafeActivateEditMode(player, character) ? APPEARANCE_OK : APPEARANCE_FAULT;
}
} // namespace totaleditor
