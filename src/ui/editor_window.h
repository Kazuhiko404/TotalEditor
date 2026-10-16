#pragma once

namespace totaleditor
{
// The editor window: race bonuses on the left, the selected character's own
// stats on the right.
//
// The split is not cosmetic. A race bonus is shared by every character of that
// race in the world -- the player's squad, town guards, bandits alike -- and a
// character stat belongs to one person. The window says so in as many words,
// because the two panels look alike and mean very different things.

// Called once per frame while the game's GUI updates.
//
// The SDK has no selection-changed callback, so noticing that the player
// clicked someone else means asking every frame. This compares against the
// character it last drew and does nothing at all when they match, which is the
// common case: the cost of running is one pointer comparison.
void EditorTick();

void ToggleEditor();
void HideEditor();
bool IsEditorOpen();

// Applies stored race overrides if they haven't been applied for the current game session yet.
void ApplyStoredOverridesIfNeeded();

// Destroys the widgets and flushes any pending edits to disk.
void ShutdownEditor();
} // namespace totaleditor
