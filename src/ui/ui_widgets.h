#pragma once

#include <string>
#include <vector>
#include "ui/ui_palette.h"

namespace MyGUI
{
class TabControl;
class TabItem;
class Button;
class ComboBox;
class EditBox;
class ListBox;
class ScrollBar;
class TextBox;
class Widget;
class Window;
}

namespace totaleditor
{
// Guarded wrappers around the MyGUI calls the editor makes.
//
// Two rules shape every function here. Engine-owned memory access-violates on a
// stale pointer rather than returning null, so each call is guarded; and MSVC
// forbids __try in any frame that also holds a C++ object needing unwinding --
// counting the whole function, not just the guarded block. So each wrapper is a
// pair: a plain function free to build UString and IntCoord temporaries, and a
// guarded one whose frame holds nothing but pointers and PODs.
//
// Collecting them here keeps that discipline in one place instead of repeating
// it beside every widget in the window.

// A skin name that does not exist yields no widget, so each creator takes a
// list and returns the first that works. The Kenshi_* skins are registered by
// RE_Kenshi at runtime rather than shipped in the game data, so a generic
// fallback is what keeps the window usable if one is ever missing.
MyGUI::Window*   MakeWindow(const char* const* skins, int skinCount,
                            const char* name, const char* layer,
                            int left, int top, int width, int height);
MyGUI::Widget*   ClientOf(MyGUI::Window* window);
MyGUI::TextBox*  MakeLabel(MyGUI::Widget* parent, const char* name,
                           int left, int top, int width, int height);
MyGUI::EditBox*  MakeStaticText(MyGUI::Widget* parent, const char* name,
                                int left, int top, int width, int height);
MyGUI::EditBox*  MakeInput(MyGUI::Widget* parent, const char* name,
                           int left, int top, int width, int height);
MyGUI::Button*   MakeButton(MyGUI::Widget* parent, const char* name,
                            int left, int top, int width, int height);
MyGUI::Button*   MakeFlatButton(MyGUI::Widget* parent, const char* name,
                                int left, int top, int width, int height);

// Solid colour fill and border frames (using WhiteSkin)
MyGUI::Widget*   MakeFill(MyGUI::Widget* parent, int x, int y, int w, int h,
                          const MyGUI::Colour& colour, const char* name = "");
void             MakeFrame(MyGUI::Widget* parent, int x, int y, int w, int h, int thickness,
                           const MyGUI::Colour& colour);
void             MakeCard(MyGUI::Widget* parent, int x, int y, int w, int h,
                          const MyGUI::Colour& fillColour = kBgDark,
                          const MyGUI::Colour& borderColour = kLine);

struct OutlineButton
{
    MyGUI::Widget* edge[4];
    MyGUI::Widget* fill;
    MyGUI::Button* button;
    OutlineButton() : fill(NULL), button(NULL) { for (int i = 0; i < 4; ++i) edge[i] = NULL; }
};

void             BuildOutlineButton(MyGUI::Widget* parent, int x, int y, int w, int h,
                                    const char* name, const std::string& caption,
                                    const MyGUI::Colour& colour, OutlineButton* out,
                                    const MyGUI::Colour& bgColour = kSumi);
void             SetOutlineButtonColour(OutlineButton* b, const MyGUI::Colour& colour);
void             SetOutlineButtonColour(OutlineButton* b, const MyGUI::Colour& borderColour,
                                        const MyGUI::Colour& textColour);
void             SetOutlineButtonColour(OutlineButton* b, const MyGUI::Colour& borderColour,
                                        const MyGUI::Colour& textColour,
                                        const MyGUI::Colour& bgColour);
void             SetOutlineButtonVisible(OutlineButton* b, bool visible);
void             SetOutlineButtonCaption(OutlineButton* b, const std::string& caption);
void             SetOutlineButtonEnabled(OutlineButton* b, bool enabled, const MyGUI::Colour& colour);

MyGUI::TextBox*  MakeHeadingWithRule(MyGUI::Widget* parent, int x, int y, int w, int h,
                                     const std::string& caption, const char* name = "");

MyGUI::ComboBox* MakeCombo(MyGUI::Widget* parent, const char* name,
                           int left, int top, int width, int height);
MyGUI::ListBox*  MakeList(MyGUI::Widget* parent, const char* name,
                          int left, int top, int width, int height);

// MyGUI::Slider exists as a header but exports no symbol in any of the three
// libraries, so a value slider is a ScrollBar. It works in whole numbers, which
// is why the callers scale a multiplier into steps and back.
MyGUI::ScrollBar* MakeScrollBar(MyGUI::Widget* parent, const char* name,
                                int left, int top, int width, int height);

// A real tick box, not a button whose caption fakes one.
//
// Kenshi_TickBoxSkin is a Button skin that draws a check mark and tracks its
// own selected state; KenshiLua's settings window uses it, which is the proof
// it exists at all -- the Kenshi_* skins are registered by RE_Kenshi at
// runtime rather than shipped in the game data.
MyGUI::Button* MakeTickBox(MyGUI::Widget* parent, const char* name,
                           int left, int top, int width, int height);

void SetTicked(MyGUI::Button* box, bool ticked);
bool IsTicked(MyGUI::Button* box);

// An invisible container to group a page's controls, so one call shows or
// hides the lot. PanelEmpty draws nothing and is what KenshiLua's own script
// editor parents its widgets to.
MyGUI::Widget* MakePanel(MyGUI::Widget* parent, const char* name,
                         int left, int top, int width, int height);

// Tabs, through the engine's own creator.
//
// createWidget<TabControl> with a guessed skin produced a control that linked,
// accepted the skin and drew nothing. ForgottenGUI::createTabPanel is how the
// game makes its own tab panels, and KenshiLua exposes it for the same reason.
// It takes relative coordinates (0..1 of the screen) and builds its own window,
// so it is not parented to anything.
MyGUI::TabControl* MakeTabPanel(const char* name, float top, float left,
                                float width, float height,
                                const char* layer, const char* skin);

// Adds a page and returns it. Content is parented to the returned TabItem, the
// way Shutori's DefensivePositions adds its widgets to the game's own MODS tab.
MyGUI::TabItem* AddTab(MyGUI::TabControl* tabs, const std::string& caption);

// The page at an index, for reaching a tab created by someone else.
MyGUI::TabItem* TabAt(MyGUI::TabControl* tabs, int index);
int TabCount(MyGUI::TabControl* tabs);

void SetText(MyGUI::TextBox* widget, const std::string& text);
void SetTextColour(MyGUI::TextBox* widget, const MyGUI::Colour& colour);
void SetTextShadow(MyGUI::TextBox* widget, bool enable, const MyGUI::Colour& shadowColour = MyGUI::Colour::Black);

// Multi-line body text for a static EditBox.
//
// setOnlyText hands the string to the edit buffer, which treats a newline as
// the end of the line being edited rather than as a break to draw -- so a
// report built from newline-separated lines came out as its first line and
// nothing else. setCaption goes through the text rendering path, which honours
// them.
void SetBodyText(MyGUI::EditBox* widget, const std::string& text);
void SetInputText(MyGUI::EditBox* widget, const std::string& text);
std::string GetInputText(MyGUI::EditBox* widget);

void SetCoord(MyGUI::Widget* widget, int left, int top, int width, int height);
void SetVisible(MyGUI::Widget* widget, bool visible);
void SetEnabled(MyGUI::Widget* widget, bool enabled);
bool ClientSize(MyGUI::Widget* widget, int* outWidth, int* outHeight);
void ViewSize(int* outWidth, int* outHeight);
void Destroy(MyGUI::Widget* widget);

// Replaces a combo's contents and selects one entry, which is the only way the
// editor ever changes a dropdown: rebuilding avoids the stale-index bugs that
// come from patching a list in place.
void FillCombo(MyGUI::ComboBox* combo, const std::vector<std::string>& items,
               int selected);
int  ComboSelection(MyGUI::ComboBox* combo);

void FillList(MyGUI::ListBox* list, const std::vector<std::string>& items);
int  ListSelection(MyGUI::ListBox* list);
void ClearListSelection(MyGUI::ListBox* list);

void SetScrollRange(MyGUI::ScrollBar* bar, int range);
void SetScrollPosition(MyGUI::ScrollBar* bar, int position);
int  ScrollPosition(MyGUI::ScrollBar* bar);

// Formats a float for display without the trailing zeroes that make a
// multiplier of 1.15 read as 1.150000.
std::string FormatFloat(float value, int decimals);

// Reads a float the player typed. Returns false on anything that is not a
// number, so a typo leaves the current value alone instead of zeroing it.
bool ParseFloat(const std::string& text, float* out);
} // namespace totaleditor
