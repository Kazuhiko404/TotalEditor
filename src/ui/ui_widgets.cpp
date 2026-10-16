#include "common.h"
#include "ui_widgets.h"
#include "log.h"

#include <cstdio>
#include <cstdlib>

#include <mygui/MyGUI_TabControl.h>
#include <mygui/MyGUI_TabItem.h>

namespace totaleditor
{
namespace
{
// Skin fallbacks. The Kenshi_* skins are registered by RE_Kenshi at runtime and
// do not live in the game's data folder, so their absence cannot be checked
// ahead of time -- only tried. The generic dark skins ship with MyGUI and keep
// the window usable if one is ever missing.
const char* const kLabelSkins[]  = { "Kenshi_TextboxStandardText", "TextBox_Dark", "TextBox" };
// Kenshi_GenericTextBox first, and Kenshi_EditBox only as a fallback: the edit
// box skin is the one-line input field, and it draws one line however the
// widget is configured. The report reaching it was 1422 characters over 50
// lines and came out as "BODY" -- the text was never the problem, the skin was.
// Shutori's status box uses the generic skin for the same reason.
const char* const kStaticSkins[] = { "Kenshi_GenericTextBox", "Kenshi_EditBox",
                                     "EditBox_Dark", "EditBox" };
const char* const kInputSkins[]  = { "Kenshi_EditBox", "EditBox_Dark", "EditBox" };
const char* const kButtonSkins[] = { "Kenshi_Button1", "Kenshi_Button", "Button_Dark", "Button" };
const char* const kComboSkins[]  = { "Kenshi_ComboBox", "ComboBox" };
const char* const kListSkins[]   = { "Kenshi_ListBox", "ListBox" };
const char* const kScrollSkins[] = { "Kenshi_ScrollerHorizontalSkin", "ScrollBarH", "ScrollBar" };
const char* const kTickSkins[]   = { "Kenshi_TickBoxSkin", "CheckBox", "Button" };
const char* const kTabSkins[]    = { "TabControl2_Dark", "TabControl", "TabControlEmpty" };
const char* const kPanelSkins[]  = { "PanelEmpty", "Panel1Skin_Dark", "Widget" };

const int kSkinCount = 4;

// Each creator is a pair: a plain function that may build UString and IntCoord
// temporaries freely, and a guarded one whose frame holds nothing but PODs.

template <typename T>
T* Create(MyGUI::Widget* parent, const char* skin, const char* name,
          int left, int top, int width, int height)
{
    return parent->createWidget<T>(
        skin, MyGUI::IntCoord(left, top, width, height),
        MyGUI::Align::Default, name);
}

template <typename T>
T* SafeCreate(MyGUI::Widget* parent, const char* skin, const char* name,
              int left, int top, int width, int height)
{
    if (parent == NULL)
    {
        return NULL;
    }
    TE_TRY { return Create<T>(parent, skin, name, left, top, width, height); }
    TE_CATCH { return NULL; }
}

// Tries each skin in turn and returns the first the engine accepts.
template <typename T>
T* CreateWithSkins(MyGUI::Widget* parent, const char* const* skins, int count,
                   const char* name, int left, int top, int width, int height)
{
    for (int i = 0; i < count; ++i)
    {
        if (skins[i] == NULL)
        {
            continue;
        }
        T* widget = SafeCreate<T>(parent, skins[i], name, left, top, width, height);
        if (widget != NULL)
        {
            return widget;
        }
    }
    return NULL;
}

MyGUI::Window* CreateRoot(MyGUI::Gui* gui, const char* skin, const char* name,
                          const char* layer, int left, int top,
                          int width, int height)
{
    return gui->createWidget<MyGUI::Window>(
        skin, MyGUI::IntCoord(left, top, width, height),
        MyGUI::Align::Default, layer, name);
}

MyGUI::Window* SafeCreateRoot(MyGUI::Gui* gui, const char* skin, const char* name,
                              const char* layer, int left, int top,
                              int width, int height)
{
    TE_TRY { return CreateRoot(gui, skin, name, layer, left, top, width, height); }
    TE_CATCH { return NULL; }
}

void ApplyCaption(MyGUI::TextBox* widget, const char* text)
{
    if (widget->getFontName().empty())
    {
        widget->setFontName("Kenshi_StandardFont_Medium");
        if (widget->getFontHeight() <= 0)
        {
            widget->setFontHeight(14);
        }
    }
    widget->setCaption(text);
}

void ApplyTextColour(MyGUI::TextBox* widget, const MyGUI::Colour& colour)
{
    widget->setTextColour(colour);
}

void ApplyTextShadow(MyGUI::TextBox* widget, bool enable, const MyGUI::Colour& shadowColour)
{
    widget->setTextShadow(enable);
    if (enable)
    {
        widget->setTextShadowColour(shadowColour);
    }
}

void ApplyOnlyText(MyGUI::EditBox* widget, const char* text)
{
    widget->setOnlyText(text);
}

// getCaption, not getOnlyText.
//
// An EditBox keeps the two apart: setCaption fills one and setOnlyText the
// other, and reading the wrong one gives an empty string that looks exactly
// like an empty box. Every input in this window is written with setOnlyText,
// but the caption is what MyGUI updates when the player types, so that is what
// has to be read back.
void CopyOnlyText(MyGUI::EditBox* widget, char* buffer, size_t size)
{
    strncpy_s(buffer, size, widget->getCaption().asUTF8_c_str(), _TRUNCATE);
}

bool SafeCopyOnlyText(MyGUI::EditBox* widget, char* buffer, size_t size)
{
    TE_TRY
    {
        CopyOnlyText(widget, buffer, size);
        return true;
    }
    TE_CATCH { return false; }
}

void ApplyComboItems(MyGUI::ComboBox* combo, const std::vector<std::string>& items,
                     int selected)
{
    combo->removeAllItems();
    for (size_t i = 0; i < items.size(); ++i)
    {
        combo->addItem(items[i]);
    }
    // setComboModeDrop makes it a dropdown rather than an editable field, which
    // is what a fixed list of races and skills wants.
    combo->setComboModeDrop(true);
    if (selected >= 0 && selected < (int)items.size())
    {
        combo->setIndexSelected((size_t)selected);
    }
}

void ApplyListItems(MyGUI::ListBox* list, const std::vector<std::string>& items)
{
    list->removeAllItems();
    for (size_t i = 0; i < items.size(); ++i)
    {
        list->addItem(items[i]);
    }
}
} // namespace

MyGUI::Window* MakeWindow(const char* const* skins, int skinCount,
                          const char* name, const char* layer,
                          int left, int top, int width, int height)
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (gui == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < skinCount; ++i)
    {
        MyGUI::Window* window =
            SafeCreateRoot(gui, skins[i], name, layer, left, top, width, height);
        if (window != NULL)
        {
            return window;
        }
    }
    return NULL;
}

// Children parented to the frame itself land behind the caption bar and off the
// edges; the client area is the part that excludes the chrome.
MyGUI::Widget* ClientOf(MyGUI::Window* window)
{
    if (window == NULL)
    {
        return NULL;
    }
    TE_TRY
    {
        MyGUI::Widget* client = window->getClientWidget();
        return (client != NULL) ? client : window;
    }
    TE_CATCH { return NULL; }
}

MyGUI::TextBox* MakeLabel(MyGUI::Widget* parent, const char* name,
                          int left, int top, int width, int height)
{
    return CreateWithSkins<MyGUI::TextBox>(parent, kLabelSkins, 3, name,
                                           left, top, width, height);
}

MyGUI::EditBox* MakeStaticText(MyGUI::Widget* parent, const char* name,
                               int left, int top, int width, int height)
{
    MyGUI::EditBox* box = CreateWithSkins<MyGUI::EditBox>(
        parent, kStaticSkins,
        (int)(sizeof(kStaticSkins) / sizeof(kStaticSkins[0])),
        name, left, top, width, height);
    if (box == NULL)
    {
        return NULL;
    }

    // A TextBox draws one line and clips the rest, so wrapping text has to be
    // an EditBox with its editing switched off: a label that happens to be an
    // edit box.
    // Order matters. setEditStatic locks the box's editing behaviour, and
    // MyGUI applies multi-line and wrapping as part of that behaviour -- set
    // static first and the box keeps drawing a single line, which is why both
    // the warning and the details report stopped at their first newline.
    // Order matters, and the scrollbars have to come after setEditStatic
    // rather than before it. Setting them on a box that is not static yet left
    // the report drawing its first block and stopping -- the same truncation
    // showed in the warning and in every page's help line. Shutori's status
    // box does read-only, multi-line, static, caption, in that order, and
    // draws its whole text.
    TE_TRY
    {
        box->setEditReadOnly(true);
        box->setEditMultiLine(true);
        box->setEditWordWrap(true);
        box->setEditStatic(true);
        box->setNeedKeyFocus(false);
        // A vertical scrollbar rather than silent clipping: text that outgrows
        // the box has to stay reachable.
        box->setNeedMouseFocus(true);
        box->setVisibleVScroll(true);
        box->setVisibleHScroll(false);
    }
    TE_CATCH { }
    return box;
}

MyGUI::EditBox* MakeInput(MyGUI::Widget* parent, const char* name,
                          int left, int top, int width, int height)
{
    MyGUI::EditBox* box = CreateWithSkins<MyGUI::EditBox>(
        parent, kInputSkins, 3, name, left, top, width, height);
    if (box == NULL)
    {
        return NULL;
    }

    TE_TRY
    {
        box->setEditStatic(false);
        box->setEditReadOnly(false);
        box->setEditMultiLine(false);
        box->setNeedKeyFocus(true);
    }
    TE_CATCH { }
    return box;
}

MyGUI::Button* MakeButton(MyGUI::Widget* parent, const char* name,
                          int left, int top, int width, int height)
{
    return CreateWithSkins<MyGUI::Button>(parent, kButtonSkins, kSkinCount, name,
                                          left, top, width, height);
}

MyGUI::Widget* MakeFillUnguarded(MyGUI::Widget* parent, int x, int y, int w, int h,
                                 const MyGUI::Colour& colour, const char* name)
{
    MyGUI::Widget* fill = parent->createWidget<MyGUI::Widget>(
        "WhiteSkin", MyGUI::IntCoord(x, y, w, h), MyGUI::Align::Default, name);
    if (fill != NULL)
    {
        fill->setNeedMouseFocus(false);
        fill->setColour(colour);
    }
    return fill;
}

MyGUI::Widget* MakeFill(MyGUI::Widget* parent, int x, int y, int w, int h,
                        const MyGUI::Colour& colour, const char* name)
{
    if (parent == NULL || w <= 0 || h <= 0)
    {
        return NULL;
    }
    TE_TRY { return MakeFillUnguarded(parent, x, y, w, h, colour, name); }
    TE_CATCH { return NULL; }
}

void MakeFrame(MyGUI::Widget* parent, int x, int y, int w, int h, int thickness,
               const MyGUI::Colour& colour)
{
    if (parent == NULL || w <= 0 || h <= 0 || thickness <= 0)
    {
        return;
    }
    MakeFill(parent, x, y, w, thickness, colour);
    MakeFill(parent, x, y + h - thickness, w, thickness, colour);
    MakeFill(parent, x, y, thickness, h, colour);
    MakeFill(parent, x + w - thickness, y, thickness, h, colour);
}

void MakeCard(MyGUI::Widget* parent, int x, int y, int w, int h,
              const MyGUI::Colour& fillColour, const MyGUI::Colour& borderColour)
{
    if (parent == NULL || w <= 0 || h <= 0)
    {
        return;
    }
    MakeFill(parent, x, y, w, h, fillColour);
    MakeFrame(parent, x, y, w, h, 1, borderColour);
}

void ApplyFontProperties(MyGUI::TextBox* widget, const char* font, int height)
{
    if (widget == NULL) return;
    widget->setFontName(font);
    if (height > 0) widget->setFontHeight(height);
}

MyGUI::Button* MakeFlatButton(MyGUI::Widget* parent, const char* name,
                              int left, int top, int width, int height)
{
    const char* const skins[] = { "Kenshi_TextBoxEmptySkin", "Kenshi_Button1", "Button" };
    MyGUI::Button* btn = CreateWithSkins<MyGUI::Button>(parent, skins, 3, name, left, top, width, height);
    if (btn != NULL)
    {
        int fontHeight = 14;
        if (height > 0 && height < 24)
        {
            fontHeight = height - 4;
        }
        if (fontHeight < 10) fontHeight = 10;
        ApplyFontProperties(btn, "Kenshi_StandardFont_Medium", fontHeight);
        btn->setTextAlign(MyGUI::Align::Center);
    }
    return btn;
}

void BuildOutlineButton(MyGUI::Widget* parent, int x, int y, int w, int h,
                        const char* name, const std::string& caption,
                        const MyGUI::Colour& colour, OutlineButton* out,
                        const MyGUI::Colour& bgColour)
{
    if (out == NULL || parent == NULL)
    {
        return;
    }

    out->fill = MakeFill(parent, x + 1, y + 1, w - 2, h - 2, bgColour);
    if (out->fill != NULL)
    {
        out->fill->setNeedMouseFocus(false);
    }

    out->edge[0] = MakeFill(parent, x, y, w, 1, colour);
    out->edge[1] = MakeFill(parent, x, y + h - 1, w, 1, colour);
    out->edge[2] = MakeFill(parent, x, y, 1, h, colour);
    out->edge[3] = MakeFill(parent, x + w - 1, y, 1, h, colour);
    for (int e = 0; e < 4; ++e)
    {
        if (out->edge[e] != NULL)
        {
            out->edge[e]->setNeedMouseFocus(false);
        }
    }
    out->button = MakeFlatButton(parent, name, x, y, w, h);
    if (out->button != NULL)
    {
        SetText(out->button, caption);
        SetTextColour(out->button, colour);
    }
}

void SetOutlineButtonColour(OutlineButton* b, const MyGUI::Colour& colour)
{
    if (b == NULL)
    {
        return;
    }
    for (int e = 0; e < 4; ++e)
    {
        if (b->edge[e] != NULL)
        {
            b->edge[e]->setColour(colour);
        }
    }
    if (b->button != NULL)
    {
        SetTextColour(b->button, colour);
    }
}

void SetOutlineButtonColour(OutlineButton* b, const MyGUI::Colour& borderColour,
                            const MyGUI::Colour& textColour)
{
    if (b == NULL)
    {
        return;
    }
    for (int e = 0; e < 4; ++e)
    {
        if (b->edge[e] != NULL)
        {
            b->edge[e]->setColour(borderColour);
        }
    }
    if (b->button != NULL)
    {
        SetTextColour(b->button, textColour);
    }
}

void SetOutlineButtonColour(OutlineButton* b, const MyGUI::Colour& borderColour,
                            const MyGUI::Colour& textColour,
                            const MyGUI::Colour& bgColour)
{
    if (b == NULL)
    {
        return;
    }
    if (b->fill != NULL)
    {
        b->fill->setColour(bgColour);
    }
    for (int e = 0; e < 4; ++e)
    {
        if (b->edge[e] != NULL)
        {
            b->edge[e]->setColour(borderColour);
        }
    }
    if (b->button != NULL)
    {
        SetTextColour(b->button, textColour);
    }
}

void SetOutlineButtonVisible(OutlineButton* b, bool visible)
{
    if (b == NULL)
    {
        return;
    }
    if (b->fill != NULL)
    {
        b->fill->setVisible(visible);
    }
    for (int e = 0; e < 4; ++e)
    {
        if (b->edge[e] != NULL)
        {
            b->edge[e]->setVisible(visible);
        }
    }
    if (b->button != NULL)
    {
        b->button->setVisible(visible);
    }
}

void SetOutlineButtonCaption(OutlineButton* b, const std::string& caption)
{
    if (b != NULL && b->button != NULL)
    {
        SetText(b->button, caption);
    }
}

void SetOutlineButtonEnabled(OutlineButton* b, bool enabled, const MyGUI::Colour& colour)
{
    if (b == NULL)
    {
        return;
    }
    const MyGUI::Colour& ink = enabled ? colour : kMuted;
    for (int e = 0; e < 4; ++e)
    {
        if (b->edge[e] != NULL)
        {
            b->edge[e]->setColour(ink);
        }
    }
    if (b->button != NULL)
    {
        b->button->setEnabled(enabled);
        SetTextColour(b->button, ink);
    }
}

MyGUI::TextBox* MakeHeadingWithRule(MyGUI::Widget* parent, int x, int y, int w, int h,
                                    const std::string& caption, const char* name)
{
    if (parent == NULL)
    {
        return NULL;
    }
    MyGUI::TextBox* label = MakeLabel(parent, name, x, y, w, h);
    if (label != NULL)
    {
        SetText(label, caption);
        SetTextColour(label, kBrass);
    }
    MakeFill(parent, x, y + h, w, 1, kGoldDim);
    return label;
}


MyGUI::ComboBox* MakeCombo(MyGUI::Widget* parent, const char* name,
                           int left, int top, int width, int height)
{
    MyGUI::ComboBox* combo = CreateWithSkins<MyGUI::ComboBox>(
        parent, kComboSkins, 2, name, left, top, width, height);
    if (combo == NULL)
    {
        return NULL;
    }
    TE_TRY
    {
        combo->setComboModeDrop(true);
        // Kenshi ships nearly two thousand race definitions, and a dropdown
        // that tries to draw all of them fills the screen and scrolls past its
        // own window. Capping the drop height makes it scroll instead.
        combo->setMaxListLength(320);
    }
    TE_CATCH { }
    return combo;
}

MyGUI::ListBox* MakeList(MyGUI::Widget* parent, const char* name,
                         int left, int top, int width, int height)
{
    return CreateWithSkins<MyGUI::ListBox>(parent, kListSkins, 2, name,
                                           left, top, width, height);
}

MyGUI::ScrollBar* MakeScrollBar(MyGUI::Widget* parent, const char* name,
                                int left, int top, int width, int height)
{
    MyGUI::ScrollBar* bar = CreateWithSkins<MyGUI::ScrollBar>(
        parent, kScrollSkins, 3, name, left, top, width, height);
    if (bar == NULL)
    {
        return NULL;
    }

    TE_TRY
    {
        // Clicking the track should jump to that value: dragging the handle is
        // fine for scrolling a list, but this bar is a value control.
        bar->setMoveToClick(true);
    }
    TE_CATCH { }
    return bar;
}

MyGUI::Button* MakeTickBox(MyGUI::Widget* parent, const char* name,
                           int left, int top, int width, int height)
{
    return CreateWithSkins<MyGUI::Button>(parent, kTickSkins, 3, name,
                                          left, top, width, height);
}

void SetTicked(MyGUI::Button* box, bool ticked)
{
    if (box == NULL) return;
    TE_TRY { box->setStateSelected(ticked); }
    TE_CATCH { }
}

bool IsTicked(MyGUI::Button* box)
{
    if (box == NULL)
    {
        return false;
    }
    TE_TRY { return box->getStateSelected(); }
    TE_CATCH { return false; }
}

MyGUI::Widget* MakePanel(MyGUI::Widget* parent, const char* name,
                         int left, int top, int width, int height)
{
    MyGUI::Widget* panel = CreateWithSkins<MyGUI::Widget>(
        parent, kPanelSkins, 3, name, left, top, width, height);
    if (panel == NULL)
    {
        return NULL;
    }

    // A panel must not take the mouse itself, only pass it to its children.
    //
    // PanelEmpty draws nothing and still claims the pointer over its whole
    // area, so a page competes with the controls inside it. Clearing mouse
    // focus stops that -- but on its own it also makes the page's children
    // unpickable, which is the half that kept being missed. MyGUI says so in
    // MyGUI_WidgetInput.h: setInheritsPick "makes all child widgets pickable
    // even if widget don't need mouse focus", and it is false by default.
    //
    // Both calls together are what a container wants: the panel itself takes
    // no clicks, and everything inside it still does. Shutori's
    // CharacterInspector needs only the first because its PanelEmpty holds
    // text and nothing else -- its one clickable widget sits outside the panel
    // with mouse focus switched on.
    TE_TRY
    {
        panel->setNeedMouseFocus(false);
        panel->setInheritsPick(true);
    }
    TE_CATCH { }
    return panel;
}

MyGUI::TabControl* CreateTabPanel(const char* name, float top, float left,
                                  float width, float height,
                                  const char* layer, const char* skin)
{
    return gui->createTabPanel(name, top, left, width, height, layer, skin);
}

MyGUI::TabControl* MakeTabPanel(const char* name, float top, float left,
                                float width, float height,
                                const char* layer, const char* skin)
{
    if (gui == NULL)
    {
        return NULL;
    }
    TE_TRY { return CreateTabPanel(name, top, left, width, height, layer, skin); }
    TE_CATCH { return NULL; }
}

MyGUI::TabItem* TabAt(MyGUI::TabControl* tabs, int index)
{
    if (tabs == NULL || index < 0)
    {
        return NULL;
    }
    TE_TRY { return tabs->getItemAt((size_t)index); }
    TE_CATCH { return NULL; }
}

int TabCount(MyGUI::TabControl* tabs)
{
    if (tabs == NULL)
    {
        return 0;
    }
    TE_TRY { return (int)tabs->getItemCount(); }
    TE_CATCH { return 0; }
}

MyGUI::TabItem* AddTabItem(MyGUI::TabControl* tabs, const std::string& caption)
{
    return tabs->addItem(caption);
}

MyGUI::TabItem* AddTab(MyGUI::TabControl* tabs, const std::string& caption)
{
    if (tabs == NULL)
    {
        return NULL;
    }
    TE_TRY { return AddTabItem(tabs, caption); }
    TE_CATCH { return NULL; }
}

// Body text that should be many lines has been arriving on screen as its first
// line only. This says what was handed over, so the next look can tell a report
// that was never built from one the widget refused to draw.
void LogBodyText(const std::string& text)
{
    int lines = 1;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\n') { ++lines; }
    }

    char detail[96] = { 0 };
    _snprintf_s(detail, sizeof(detail), _TRUNCATE,
                "%d chars, %d lines", (int)text.size(), lines);
    LogDebug("Body text set: " + std::string(detail) + ".");
}

void SetText(MyGUI::TextBox* widget, const std::string& text)
{
    if (widget == NULL) return;
    TE_TRY { ApplyCaption(widget, text.c_str()); }
    TE_CATCH_LOG_VOID("setting a label's text")
}

void SetTextColour(MyGUI::TextBox* widget, const MyGUI::Colour& colour)
{
    if (widget == NULL) return;
    TE_TRY { ApplyTextColour(widget, colour); }
    TE_CATCH_LOG_VOID("setting a widget's text colour")
}

void SetTextShadow(MyGUI::TextBox* widget, bool enable, const MyGUI::Colour& shadowColour)
{
    if (widget == NULL) return;
    TE_TRY { ApplyTextShadow(widget, enable, shadowColour); }
    TE_CATCH_LOG_VOID("setting a widget's text shadow")
}

// setOnlyText rather than setCaption, so markup characters in a race name are
// shown rather than parsed.
void SetInputText(MyGUI::EditBox* widget, const std::string& text)
{
    if (widget == NULL) return;
    TE_TRY { ApplyOnlyText(widget, text.c_str()); }
    TE_CATCH_LOG_VOID("setting an input box's text")
}

void SetBodyText(MyGUI::EditBox* widget, const std::string& text)
{
    if (widget == NULL) return;
    LogBodyText(text);
    TE_TRY { ApplyCaption(widget, text.c_str()); }
    TE_CATCH_LOG_VOID("setting a text panel's contents")
}

std::string GetInputText(MyGUI::EditBox* widget)
{
    if (widget == NULL)
    {
        return std::string();
    }

    // The std::string built on the way out counts as an object in this frame,
    // and C2712 counts the whole frame rather than just the guarded block. So
    // the guard lives in its own POD-only function and only the buffer crosses.
    char buffer[256] = { 0 };
    if (!SafeCopyOnlyText(widget, buffer, sizeof(buffer)))
    {
        return std::string();
    }
    return std::string(buffer);
}

void SetCoord(MyGUI::Widget* widget, int left, int top, int width, int height)
{
    if (widget == NULL) return;
    TE_TRY { widget->setCoord(left, top, width, height); }
    TE_CATCH_LOG_VOID("moving a widget")
}

void SetVisible(MyGUI::Widget* widget, bool visible)
{
    if (widget == NULL) return;
    TE_TRY { widget->setVisible(visible); }
    TE_CATCH_LOG_VOID("showing or hiding a widget")
}

void SetEnabled(MyGUI::Widget* widget, bool enabled)
{
    if (widget == NULL) return;
    TE_TRY { widget->setEnabled(enabled); }
    TE_CATCH_LOG_VOID("enabling or disabling a widget")
}

bool ClientSize(MyGUI::Widget* widget, int* outWidth, int* outHeight)
{
    if (widget == NULL)
    {
        return false;
    }
    TE_TRY
    {
        *outWidth = widget->getWidth();
        *outHeight = widget->getHeight();
        return true;
    }
    TE_CATCH { return false; }
}

void ViewSize(int* outWidth, int* outHeight)
{
    *outWidth = 0;
    *outHeight = 0;
    TE_TRY
    {
        MyGUI::RenderManager* manager = MyGUI::RenderManager::getInstancePtr();
        if (manager != NULL)
        {
            *outWidth = manager->getViewSize().width;
            *outHeight = manager->getViewSize().height;
        }
    }
    TE_CATCH { }
}

void Destroy(MyGUI::Widget* widget)
{
    if (widget == NULL) return;
    TE_TRY
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            gui->destroyWidget(widget);
        }
    }
    TE_CATCH { }
}

void FillCombo(MyGUI::ComboBox* combo, const std::vector<std::string>& items,
               int selected)
{
    if (combo == NULL) return;
    TE_TRY { ApplyComboItems(combo, items, selected); }
    TE_CATCH_LOG_VOID("filling a dropdown")
}

int ComboSelection(MyGUI::ComboBox* combo)
{
    if (combo == NULL)
    {
        return -1;
    }
    TE_TRY
    {
        const size_t index = combo->getIndexSelected();
        // ITEM_NONE is a saturated size_t, which would become a negative int
        // by accident; comparing before the cast says "nothing selected" on
        // purpose.
        return (index == MyGUI::ITEM_NONE) ? -1 : (int)index;
    }
    TE_CATCH { return -1; }
}

void FillList(MyGUI::ListBox* list, const std::vector<std::string>& items)
{
    if (list == NULL) return;
    TE_TRY { ApplyListItems(list, items); }
    TE_CATCH_LOG_VOID("filling a list")
}

int ListSelection(MyGUI::ListBox* list)
{
    if (list == NULL)
    {
        return -1;
    }
    TE_TRY
    {
        const size_t index = list->getIndexSelected();
        return (index == MyGUI::ITEM_NONE) ? -1 : (int)index;
    }
    TE_CATCH { return -1; }
}

void ClearListSelection(MyGUI::ListBox* list)
{
    if (list == NULL) return;
    TE_TRY { list->clearIndexSelected(); }
    TE_CATCH_LOG_VOID("clearing a list selection")
}

void SetScrollRange(MyGUI::ScrollBar* bar, int range)
{
    if (bar == NULL || range < 1) return;
    TE_TRY
    {
        // The range is a count of positions, so the last reachable value is
        // range - 1; passing the step count directly would put the maximum one
        // step out of reach.
        bar->setScrollRange((size_t)(range + 1));
        bar->setScrollPage(1);
        bar->setScrollViewPage(1);
    }
    TE_CATCH { }
}

void SetScrollPosition(MyGUI::ScrollBar* bar, int position)
{
    if (bar == NULL || position < 0) return;
    TE_TRY { bar->setScrollPosition((size_t)position); }
    TE_CATCH { }
}

int ScrollPosition(MyGUI::ScrollBar* bar)
{
    if (bar == NULL)
    {
        return 0;
    }
    TE_TRY { return (int)bar->getScrollPosition(); }
    TE_CATCH { return 0; }
}

std::string FormatFloat(float value, int decimals)
{
    char format[16] = { 0 };
    _snprintf_s(format, sizeof(format), _TRUNCATE, "%%.%df", decimals);

    char buffer[64] = { 0 };
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, value);
    return std::string(buffer);
}

bool ParseFloat(const std::string& text, float* out)
{
    if (text.empty())
    {
        return false;
    }

    char* end = NULL;
    const double value = strtod(text.c_str(), &end);
    if (end == text.c_str())
    {
        return false;
    }

    // Trailing spaces are what a player actually types; anything else is a typo
    // and leaving the current value alone beats writing a zero.
    while (end != NULL && *end != '\0')
    {
        if (!isspace((unsigned char)*end))
        {
            return false;
        }
        ++end;
    }

    *out = (float)value;
    return true;
}
} // namespace totaleditor
