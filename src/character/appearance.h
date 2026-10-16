#pragma once

class Character;

namespace totaleditor
{
// Opens Kenshi's own character editor (the one from a new game) on a character,
// through PlayerInterface::activateCharacterEditMode. Copied from CharTweak 2's
// "Edit Appearance" button, including its rule that only the player's own
// characters may be edited: the game's editor assumes a squad member.

enum AppearanceResult
{
    APPEARANCE_OK,
    APPEARANCE_NO_CHARACTER,
    APPEARANCE_NOT_PLAYER,
    APPEARANCE_NO_PLAYER_INTERFACE,
    APPEARANCE_FAULT
};

// Checks the character can be edited, without opening anything.
AppearanceResult CanEditAppearance(Character* character);

// Opens the game's character editor on the character. The caller closes its
// own window first, as CharTweak does, so the editor is not drawn underneath.
AppearanceResult OpenAppearanceEditor(Character* character);
} // namespace totaleditor
