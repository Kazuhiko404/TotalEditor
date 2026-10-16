#pragma once

namespace totaleditor
{
// Watches the player's selection while the editor is open.
//
// The window read the selected character once, when it opened, so clicking a
// different person in the world left it editing the first one -- with their name
// still on screen. There is no selection-changed callback in the SDK, so this
// hooks the GUI's own per-frame update, which is what TotalRecruit and Shutori
// both do for the same reason.
bool InstallFrameTick();
void ShutdownFrameTick();
} // namespace totaleditor
