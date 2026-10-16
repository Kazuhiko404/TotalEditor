#pragma once

class Character;
namespace MyGUI { class ImageBox; }

namespace totaleditor
{
namespace PortraitEditor
{
// Binds the character's portrait into the widget, using the composed
// background when the player has set one.
void Decorate(Character* c, MyGUI::ImageBox* image);
bool Install();
void Tick();
void Open(Character* c);
void Close();
bool IsOpen();
} // namespace PortraitEditor
} // namespace totaleditor
