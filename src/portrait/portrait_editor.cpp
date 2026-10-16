#include "common.h"
#include "portrait_editor.h"
#include "log.h"
#include "ui/ui_palette.h"
#include "ui/ui_widgets.h"

#include <cmath>
#include <float.h>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>

#include <kenshi/InstanceID.h>
#include <kenshi/RenderToTextureotron.h>
#include <kenshi/gui/PortraitManager.h>
#include <kenshi/gui/MainBarGUI.h>
#include <mygui/MyGUI_IUnlinkWidget.h>
#include <mygui/MyGUI_WidgetManager.h>
#include <ogre/OgreImage.h>
#include <ogre/OgreTextureManager.h>
#include <ogre/OgreHardwarePixelBuffer.h>

namespace totaleditor
{
namespace PortraitEditor
{
namespace
{
bool g_pluginReady = false;

struct Settings
{
    float yaw, height, horizontal, zoom, r, g, b, brightness, opacity;
    int background;
    Settings() : yaw(0), height(0), horizontal(0), zoom(1), r(.25f), g(.25f), b(.25f), brightness(1), opacity(1), background(0) {}
};

std::map<std::string, Settings> g_settings;
std::map<std::string, Ogre::TexturePtr> g_composed;
std::map<std::string, DWORD> g_requested;
unsigned int g_textureSerial = 0;
std::map<std::string, hand> g_pendingComposition;
std::map<std::string, Ogre::TRect<float> > g_nativeUV;
std::map<std::string, std::string> g_nativeTextures;

struct Capture { hand character; MyGUI::ImageBox* widget; };
__declspec(thread) Capture* g_capturing = NULL;

std::string Key(Character* c)
{
    if (!c) return "";
    InstanceID* i = c->getInstanceID();
    if (i && !i->uid.empty()) return i->uid;
    const hand& h = c->getHandle();
    std::ostringstream key;
    key << "character_" << h.container << '_' << h.containerSerial << '_' << h.index << '_' << h.serial;
    return key.str();
}

void (*g_originalTexture)(MyGUI::ImageBox*, const std::string&) = NULL;
void TextureHook(MyGUI::ImageBox* image, const std::string& name)
{
    g_originalTexture(image, name);
    if (g_pluginReady && g_capturing && g_capturing->widget == image)
    {
        Character* c = g_capturing->character.getCharacter();
        if (c && name.find("PortraitEditorComposed_") != 0)
        {
            std::string k = Key(c);
            if (!k.empty()) g_nativeTextures[k] = name;
        }
    }
}

void (*g_originalUV)(MyGUI::ImageBox*, const MyGUI::FloatRect&) = NULL;
void UVHook(MyGUI::ImageBox* image, const MyGUI::FloatRect& rect)
{
    g_originalUV(image, rect);
    if (g_pluginReady && g_capturing && g_capturing->widget == image)
    {
        Character* c = g_capturing->character.getCharacter();
        if (c && rect.right > rect.left && rect.bottom > rect.top)
        {
            std::string k = Key(c);
            if (!k.empty()) g_nativeUV[k] = Ogre::TRect<float>(rect.left, rect.top, rect.right, rect.bottom);
        }
    }
}



void (*g_originalSetImage)(PortraitManager*, const hand&, MyGUI::ImageBox*, bool) = NULL;
void SetImageHook(PortraitManager* pm, const hand& character, MyGUI::ImageBox* image, bool force)
{
    if (!g_pluginReady)
    {
        g_originalSetImage(pm, character, image, force);
        return;
    }
    Capture capture;
    capture.character = character;
    capture.widget = image;
    Capture* previous = g_capturing;
    g_capturing = &capture;
    try { g_originalSetImage(pm, character, image, force); }
    catch (...) { g_capturing = previous; throw; }
    g_capturing = previous;
}

std::string g_resources, g_config, g_backgroundFolder;
std::vector<std::string> g_customBackgrounds;
std::map<std::string, std::vector<unsigned char> > g_backgroundPixels;

void ScanBackgrounds()
{
    WIN32_FIND_DATAA item;
    HANDLE search = FindFirstFileA((g_backgroundFolder + "/*").c_str(), &item);
    if (search == INVALID_HANDLE_VALUE) return;
    do
    {
        if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::string name = item.cFileName;
        size_t dot = name.find_last_of(".");
        if (dot == std::string::npos) continue;
        std::string ext = name.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (g_customBackgrounds.size() < 128 && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
            g_customBackgrounds.push_back(name);
    } while (FindNextFileA(search, &item));
    FindClose(search);
    std::sort(g_customBackgrounds.begin(), g_customBackgrounds.end());
}

size_t BackgroundCount() { return 6 + g_customBackgrounds.size(); }

MyGUI::Widget* g_window = NULL;
MyGUI::ImageBox* g_preview = NULL;
MyGUI::ImageBox* g_backdrop = NULL;
MyGUI::TextBox* g_title = NULL;
MyGUI::TextBox* g_bgLabel = NULL;
MyGUI::TextBox* g_status = NULL;
MyGUI::ScrollBar* g_rotation = NULL;
MyGUI::ScrollBar* g_height = NULL;
MyGUI::ScrollBar* g_horizontal = NULL;
MyGUI::ScrollBar* g_zoom = NULL;
MyGUI::ScrollBar* g_red = NULL;
MyGUI::ScrollBar* g_green = NULL;
MyGUI::ScrollBar* g_blue = NULL;
MyGUI::ScrollBar* g_brightness = NULL;
MyGUI::ScrollBar* g_opacity = NULL;
MyGUI::TextBox* g_redLabel = NULL;
MyGUI::TextBox* g_greenLabel = NULL;
MyGUI::TextBox* g_blueLabel = NULL;
MyGUI::TextBox* g_brightnessLabel = NULL;
MyGUI::TextBox* g_opacityLabel = NULL;
MyGUI::Button* g_resetButton = NULL;
MyGUI::Button* g_cancelButton = NULL;
MyGUI::Button* g_applyButton = NULL;

hand g_editing;
Settings g_draft;
bool g_opened = false;
bool g_updating = false;
bool g_dirty = false;
bool g_failed = false;
bool g_resourcesReady = false;

struct WidgetLifetime : MyGUI::IUnlinkWidget
{
    void _unlinkWidget(MyGUI::Widget* widget)
    {
        if (widget == g_window)
        {
            g_window = NULL;
            g_opened = false;
            g_editing.setNull();
            g_title = NULL; g_bgLabel = NULL; g_status = NULL;
            g_rotation = NULL; g_height = NULL; g_horizontal = NULL; g_zoom = NULL;
            g_red = NULL; g_green = NULL; g_blue = NULL; g_brightness = NULL; g_opacity = NULL;
            g_redLabel = NULL; g_greenLabel = NULL; g_blueLabel = NULL; g_brightnessLabel = NULL; g_opacityLabel = NULL;
            g_resetButton = NULL; g_cancelButton = NULL; g_applyButton = NULL;
        }
        if (widget == g_preview) { g_preview = NULL; g_opened = false; }
        if (widget == g_backdrop) g_backdrop = NULL;
    }
} g_widgetLifetime;

bool g_lifetimeRegistered = false;

void Resources()
{
    if (!g_resourcesReady)
    {
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(g_resources, "FileSystem", "General");
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(g_backgroundFolder, "FileSystem", "General");
        g_resourcesReady = true;
    }
}

DWORD g_changedAt = 0;
const char* g_bgNames[] = { "Default", "Teal", "Black", "Sand", "Custom colour", "Bordeaux" };


Settings Get(Character* c)
{
    std::string k = Key(c);
    if (k.empty()) return Settings();
    if (g_settings.count(k)) return g_settings[k];

    char value[128];
    GetPrivateProfileStringA(k.c_str(), "view", "0 0 1 0", value, 128, g_config.c_str());
    Settings s;
    std::istringstream in(value);
    in >> s.yaw >> s.height >> s.zoom >> s.background >> s.r >> s.g >> s.b >> s.brightness >> s.opacity >> s.horizontal;
    s.horizontal = std::max(-1.f, std::min(1.f, s.horizontal));
    s.brightness = std::max(0.f, std::min(2.f, s.brightness));
    s.opacity = std::max(0.f, std::min(1.f, s.opacity));
    s.yaw = std::max(-180.f, std::min(180.f, s.yaw));
    s.height = std::max(-1.f, std::min(1.f, s.height));
    s.zoom = std::max(.6f, std::min(1.8f, s.zoom));
    s.background = std::max(0, std::min(5, s.background));

    if (!_finite(s.yaw) || !_finite(s.height) || !_finite(s.horizontal) || !_finite(s.zoom) ||
        !_finite(s.r) || !_finite(s.g) || !_finite(s.b) || !_finite(s.brightness) || !_finite(s.opacity))
        s = Settings();

    s.r = std::max(0.f, std::min(1.f, s.r));
    s.g = std::max(0.f, std::min(1.f, s.g));
    s.b = std::max(0.f, std::min(1.f, s.b));

    char file[260];
    GetPrivateProfileStringA(k.c_str(), "background_file", "", file, 260, g_config.c_str());
    if (file[0])
    {
        s.background = 0;
        for (size_t i = 0; i < g_customBackgrounds.size(); ++i)
        {
            if (g_customBackgrounds[i] == file)
            {
                s.background = int(i) + 6;
                break;
            }
        }
    }
    g_settings[k] = s;
    return s;
}

Settings Effective(Character* c)
{
    return g_opened && g_editing.getCharacter() == c ? g_draft : Get(c);
}

void Store(Character* c, const Settings& s)
{
    std::string k = Key(c);
    if (k.empty()) return;
    g_settings[k] = s;
    std::ostringstream out;
    out << s.yaw << ' ' << s.height << ' ' << s.zoom << ' ' << s.background << ' '
        << s.r << ' ' << s.g << ' ' << s.b << ' ' << s.brightness << ' ' << s.opacity << ' ' << s.horizontal;
    WritePrivateProfileStringA(k.c_str(), "view", out.str().c_str(), g_config.c_str());
    WritePrivateProfileStringA(k.c_str(), "background_file",
        s.background >= 6 ? g_customBackgrounds[s.background - 6].c_str() : "", g_config.c_str());
}

MyGUI::TextBox* Text(MyGUI::Widget* p, int x, int y, int w, int h, const std::string& caption)
{
    MyGUI::TextBox* t = p->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText", MyGUI::IntCoord(x, y, w, h), MyGUI::Align::Default);
    t->setCaption(caption);
    t->setTextColour(kMuted);
    t->setNeedMouseFocus(false);
    return t;
}

const int kDlgWidth = 636;
const int kDlgHeight = 550;
const int kSliderPitch = 44;

void Background()
{
    if (g_bgLabel) g_bgLabel->setCaption(g_draft.background < 6 ? g_bgNames[g_draft.background] : g_customBackgrounds[g_draft.background - 6]);
    if (g_backdrop) g_backdrop->setVisible(false);

    bool colour = g_draft.background == 4;
    bool chosen = g_draft.background != 0;

    if (g_red) g_red->setVisible(colour);
    if (g_green) g_green->setVisible(colour);
    if (g_blue) g_blue->setVisible(colour);
    if (g_redLabel) g_redLabel->setVisible(colour);
    if (g_greenLabel) g_greenLabel->setVisible(colour);
    if (g_blueLabel) g_blueLabel->setVisible(colour);

    if (g_brightness) g_brightness->setVisible(chosen);
    if (g_opacity) g_opacity->setVisible(chosen);
    if (g_brightnessLabel) g_brightnessLabel->setVisible(chosen);
    if (g_opacityLabel) g_opacityLabel->setVisible(chosen);

    int row = colour ? 418 : 286;
    if (g_brightness) g_brightness->setPosition(380, row);
    if (g_brightnessLabel) g_brightnessLabel->setPosition(286, row + 4);
    if (g_opacity) g_opacity->setPosition(380, row + kSliderPitch);
    if (g_opacityLabel) g_opacityLabel->setPosition(286, row + kSliderPitch + 4);

    if (g_window)
    {
        g_window->setSize(kDlgWidth, kDlgHeight);
        MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos = g_window->getPosition();
        if (pos.top + g_window->getHeight() > view.height)
            g_window->setPosition(pos.left, std::max(0, view.height - g_window->getHeight()));
        if (pos.left + g_window->getWidth() > view.width)
            g_window->setPosition(std::max(0, view.width - g_window->getWidth()), pos.top);
    }
}

void Sync()
{
    g_updating = true;
    if (g_rotation) g_rotation->setScrollPosition(size_t(g_draft.yaw + 180));
    if (g_height) g_height->setScrollPosition(size_t((g_draft.height + 1) * 100));
    if (g_horizontal) g_horizontal->setScrollPosition(size_t((g_draft.horizontal + 1) * 100));
    if (g_zoom) g_zoom->setScrollPosition(size_t((g_draft.zoom - .6f) * 100));
    if (g_red) g_red->setScrollPosition(size_t(g_draft.r * 255));
    if (g_green) g_green->setScrollPosition(size_t(g_draft.g * 255));
    if (g_blue) g_blue->setScrollPosition(size_t(g_draft.b * 255));
    if (g_brightness) g_brightness->setScrollPosition(size_t(g_draft.brightness * 100));
    if (g_opacity) g_opacity->setScrollPosition(size_t(g_draft.opacity * 100));
    g_updating = false;
    Background();
}

void Changed()
{
    g_dirty = true;
    g_changedAt = GetTickCount();
}

void Slider(MyGUI::ScrollBar* sender, size_t position)
{
    if (g_updating) return;
    if (sender == g_rotation) g_draft.yaw = float(position) - 180;
    else if (sender == g_height) g_draft.height = float(position) / 100 - 1;
    else if (sender == g_horizontal) g_draft.horizontal = float(position) / 100 - 1;
    else if (sender == g_zoom) g_draft.zoom = .6f + float(position) / 100;
    else if (sender == g_red) g_draft.r = float(position) / 255;
    else if (sender == g_green) g_draft.g = float(position) / 255;
    else if (sender == g_blue) g_draft.b = float(position) / 255;
    else if (sender == g_brightness) g_draft.brightness = float(position) / 100;
    else g_draft.opacity = float(position) / 100;

    if (g_draft.background == 4) Background();
    Changed();
}

void BgNext(MyGUI::Widget*)
{
    g_draft.background = int((g_draft.background + 1) % BackgroundCount());
    Background();
    Changed();
}

void BgPrevious(MyGUI::Widget*)
{
    g_draft.background = int((g_draft.background + BackgroundCount() - 1) % BackgroundCount());
    Background();
    Changed();
}

void QueuePortrait()
{
    PortraitManager* pm = PortraitManager::getInstance();
    if (pm && g_editing.getCharacter())
    {
        PortraitData* pd = pm->getPortrait(g_editing);
        if (pd)
        {
            pd->reloadPortrait = true;
            pd->forceUpdate();
        }
    }
    if (ou && g_editing.getCharacter()) ou->addPortraitUpdate(g_editing);
}

void Reset(MyGUI::Widget*) { g_draft = Settings(); Sync(); Changed(); }
void Front(MyGUI::Widget*) { g_draft.yaw = 0; Sync(); Changed(); }
void ThreeQuarter(MyGUI::Widget*) { g_draft.yaw = 35; Sync(); Changed(); }
void Cancel(MyGUI::Widget*) { g_opened = false; if (g_window) g_window->setVisible(false); QueuePortrait(); }
void Apply(MyGUI::Widget*)
{
    Character* c = g_editing.getCharacter();
    if (c) Store(c, g_draft);
    g_opened = false;
    if (g_window) g_window->setVisible(false);
    QueuePortrait();
}

MyGUI::Button* OutlineBtn(MyGUI::Widget* p, int x, int y, int w, int h, const char* label,
                          const MyGUI::Colour& colour, void (*callback)(MyGUI::Widget*))
{
    OutlineButton ob;
    BuildOutlineButton(p, x, y, w, h, "", label, colour, &ob);
    if (ob.button != NULL && callback != NULL)
    {
        ob.button->eventMouseButtonClick += MyGUI::newDelegate(callback);
    }
    return ob.button;
}

MyGUI::ScrollBar* SliderRow(int y, const char* label, size_t range, MyGUI::TextBox** labelOut = NULL)
{
    MyGUI::TextBox* t = Text(g_window, 286, y + 4, 90, 24, label);
    SetTextColour(t, kMuted);
    if (labelOut) *labelOut = t;
    MyGUI::ScrollBar* s = g_window->createWidget<MyGUI::ScrollBar>("Kenshi_SliderHBig", MyGUI::IntCoord(380, y, 236, 32), MyGUI::Align::Default);
    MyGUI::Widget* start = s->findWidget("Start");
    MyGUI::Widget* end = s->findWidget("End");
    if (start)
    {
        start->changeWidgetSkin("Kenshi_Button1");
        MyGUI::Button* b = start->castType<MyGUI::Button>(false);
        if (b)
        {
            b->setFontName("Kenshi_StandardFont_Medium");
            b->setFontHeight(12);
            b->setTextAlign(MyGUI::Align::Center);
            b->setCaption("<");
            SetTextColour(b, kBrass);
        }
    }
    if (end)
    {
        end->changeWidgetSkin("Kenshi_Button1");
        MyGUI::Button* b = end->castType<MyGUI::Button>(false);
        if (b)
        {
            b->setFontName("Kenshi_StandardFont_Medium");
            b->setFontHeight(12);
            b->setTextAlign(MyGUI::Align::Center);
            b->setCaption(">");
            SetTextColour(b, kBrass);
        }
    }
    s->setScrollRange(range);
    s->setScrollPage(1);
    s->setMinTrackSize(36);
    s->setTrackSize(36);
    s->setMoveToClick(true);
    s->eventScrollChangePosition += MyGUI::newDelegate(&Slider);
    return s;
}

void CreateUI()
{
    if (!g_lifetimeRegistered)
    {
        MyGUI::WidgetManager::getInstance().registerUnlinker(&g_widgetLifetime);
        g_lifetimeRegistered = true;
    }
    Resources();

    MyGUI::Gui* g = MyGUI::Gui::getInstancePtr();
    if (!g) return;
    MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();

    g_window = g->createWidget<MyGUI::Widget>("Kenshi_GenericTextBoxSkin",
        MyGUI::IntCoord(std::max(0, (view.width - kDlgWidth) / 2), std::max(0, (view.height - kDlgHeight) / 2), kDlgWidth, kDlgHeight),
        MyGUI::Align::Default, "Window", "TotalEditorPortraitWindow");

    // Lacquer background covering entire window
    MakeFill(g_window, 0, 0, kDlgWidth, kDlgHeight, kLacquer);
    // Outer border frame
    MakeFrame(g_window, 0, 0, kDlgWidth, kDlgHeight, 1, kGoldDim);

    g_title = Text(g_window, 16, 12, 550, 26, "PORTRAIT");
    SetTextColour(g_title, kBrass);
    // 1px gold rule under the title
    MakeFill(g_window, 16, 42, 604, 1, kGoldDim);

    OutlineBtn(g_window, 594, 10, 26, 26, "X", kDanger, Cancel);

    // Frame around the square preview
    MakeFrame(g_window, 15, 53, 258, 258, 1, kGoldDim);
    MyGUI::Widget* square = g_window->createWidget<MyGUI::Widget>("PanelEmpty", MyGUI::IntCoord(16, 54, 256, 256), MyGUI::Align::Default);
    MakeFill(square, 0, 0, 256, 256, kBgDark);
    g_backdrop = square->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 256, 256), MyGUI::Align::Stretch);
    g_backdrop->setNeedMouseFocus(false);
    g_backdrop->setDepth(5);

    g_preview = square->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 256, 256), MyGUI::Align::Stretch);
    g_preview->setNeedMouseFocus(false);
    g_preview->setDepth(3);

    OutlineBtn(g_window, 16, 324, 124, 32, "Front", kBrass, Front);
    OutlineBtn(g_window, 148, 324, 124, 32, "3/4 view", kBrass, ThreeQuarter);

    g_resetButton = OutlineBtn(g_window, 16, 370, 76, 36, "Reset", kMuted, Reset);
    g_cancelButton = OutlineBtn(g_window, 98, 370, 86, 36, "Cancel", kDanger, Cancel);
    g_applyButton = OutlineBtn(g_window, 190, 370, 82, 36, "Apply", kBrass, Apply);

    g_status = Text(g_window, 16, 512, 604, 24, "Preview updates when you release a control.");
    SetTextColour(g_status, kMuted);

    g_rotation = SliderRow(54, "Rotation", 361);
    g_height = SliderRow(98, "Camera height", 201);
    g_horizontal = SliderRow(142, "Horizontal", 201);
    g_zoom = SliderRow(186, "Zoom", 121);

    MyGUI::TextBox* bgTitle = Text(g_window, 286, 240, 90, 24, "Background");
    SetTextColour(bgTitle, kMuted);
    OutlineBtn(g_window, 380, 236, 32, 30, "<", kBrass, BgPrevious);
    g_bgLabel = Text(g_window, 418, 240, 160, 24, "");
    OutlineBtn(g_window, 584, 236, 32, 30, ">", kBrass, BgNext);

    g_red = SliderRow(286, "Red", 256, &g_redLabel);
    g_green = SliderRow(330, "Green", 256, &g_greenLabel);
    g_blue = SliderRow(374, "Blue", 256, &g_blueLabel);

    g_brightness = SliderRow(418, "Brightness", 201, &g_brightnessLabel);
    g_opacity = SliderRow(462, "Opacity", 101, &g_opacityLabel);

    g_window->setVisible(false);
}

void Compose(Character* c, Ogre::SharedPtr<Ogre::Texture> atlas, const iVector2& tileSize, const Ogre::TRect<float>& rect)
{
    Settings s = Effective(c);
    if (s.background == 0 || atlas.isNull()) return;
    std::string key = Key(c);
    if (key.empty()) return;
    try
    {
        size_t w = atlas->getWidth(), h = atlas->getHeight();
        if (!w || !h || w > 4096 || h > 4096 || !_finite(rect.left) || !_finite(rect.top) || !_finite(rect.right) || !_finite(rect.bottom)) return;
        if (g_composed.size() >= 256 && !g_composed.count(key)) return;
        if (rect.left < 0 || rect.top < 0 || rect.right > 1 || rect.bottom > 1 || rect.right <= rect.left || rect.bottom <= rect.top) return;

        size_t left = size_t(std::max(0.f, rect.left) * w + .5f), top = size_t(std::max(0.f, rect.top) * h + .5f);
        size_t right = size_t(std::min(1.f, rect.right) * w + .5f), bottom = size_t(std::min(1.f, rect.bottom) * h + .5f);
        if (right <= left || bottom <= top || right > w || bottom > h || right - left > 512 || bottom - top > 512) return;

        size_t tw = right - left, th = bottom - top;
        std::vector<unsigned char> pixels(tw * th * 4);
        Ogre::PixelBox box(tw, th, 1, Ogre::PF_BYTE_RGBA, &pixels[0]);
        std::vector<unsigned char> sheet(w * h * 4);
        Ogre::PixelBox fullSheet(w, h, 1, Ogre::PF_BYTE_RGBA, &sheet[0]);
        atlas->getBuffer()->blitToMemory(fullSheet);

        for (size_t y = 0; y < th; ++y)
            std::copy(sheet.begin() + ((top + y) * w + left) * 4, sheet.begin() + ((top + y) * w + right) * 4, pixels.begin() + y * tw * 4);

        float r = s.background == 1 ? 32.f / 255 : s.background == 2 ? 0 : s.background == 3 ? 139.f / 255 : s.background == 5 ? 93.f / 255 : s.r;
        float g = s.background == 1 ? 91.f / 255 : s.background == 2 ? 0 : s.background == 3 ? 116.f / 255 : s.background == 5 ? 36.f / 255 : s.g;
        float b = s.background == 1 ? 88.f / 255 : s.background == 2 ? 0 : s.background == 3 ? 84.f / 255 : s.background == 5 ? 52.f / 255 : s.b;
        const std::vector<unsigned char>* scenery = NULL;

        if (s.background >= 6)
        {
            if (size_t(s.background - 6) >= g_customBackgrounds.size()) return;
            Resources();
            std::string filename = g_customBackgrounds[s.background - 6];
            std::ostringstream cacheName; cacheName << filename << ':' << tw << 'x' << th;
            std::vector<unsigned char>& cached = g_backgroundPixels[cacheName.str()];
            if (cached.empty())
            {
                Ogre::Image image; image.load(filename, "General");
                size_t iw = image.getWidth(), ih = image.getHeight();
                if (!iw || !ih || iw > 2048 || ih > 2048) return;
                size_t side = std::min(iw, ih), ox = (iw - side) / 2, oy = (ih - side) / 2;
                cached.resize(tw * th * 3);
                for (size_t y = 0; y < th; ++y)
                {
                    for (size_t x = 0; x < tw; ++x)
                    {
                        Ogre::ColourValue colour = image.getColourAt(ox + x * side / tw, oy + y * side / th, 0);
                        size_t n = (y * tw + x) * 3;
                        cached[n] = static_cast<unsigned char>(colour.r * 255);
                        cached[n + 1] = static_cast<unsigned char>(colour.g * 255);
                        cached[n + 2] = static_cast<unsigned char>(colour.b * 255);
                    }
                }
            }
            scenery = &cached;
        }

        for (size_t n = 0; n < pixels.size(); n += 4)
        {
            float characterAlpha = pixels[n + 3] / 255.f;
            size_t bn = n / 4 * 3;
            float background[3] = { scenery ? float((*scenery)[bn]) : r * 255, scenery ? float((*scenery)[bn + 1]) : g * 255, scenery ? float((*scenery)[bn + 2]) : b * 255 };
            float visibleBackground = s.opacity * (1 - characterAlpha);
            float outputAlpha = characterAlpha + visibleBackground;
            for (int channel = 0; channel < 3; ++channel)
            {
                float colour = background[channel];
                colour = s.brightness <= 1 ? colour * s.brightness : colour + (255 - colour) * (s.brightness - 1);
                float output = outputAlpha > 0 ? (pixels[n + channel] * characterAlpha + colour * visibleBackground) / outputAlpha : 0;
                pixels[n + channel] = static_cast<unsigned char>(std::max(0.f, std::min(255.f, output)));
            }
            pixels[n + 3] = static_cast<unsigned char>(outputAlpha * 255 + .5f);
        }

        Ogre::TexturePtr& texture = g_composed[key];
        if (texture.isNull())
        {
            std::ostringstream name; name << "PortraitEditorComposed_" << ++g_textureSerial;
            texture = Ogre::TextureManager::getSingleton().createManual(name.str(), "General", Ogre::TEX_TYPE_2D, tw, th, 0, Ogre::PF_BYTE_RGBA, Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
        }
        if (texture->getWidth() != tw || texture->getHeight() != th) return;
        texture->getBuffer()->blitFromMemory(box);
    }
    catch (...) {}
}

void ComposeRegistered(Character* c)
{
    if (!c) return;
    hand saved = c->getHandle();
    PortraitManager* pm = PortraitManager::getInstance();
    if (!pm || pm->texture.isNull()) return;

    std::string key = Key(c);
    std::map<std::string, Ogre::TRect<float> >::iterator uv = g_nativeUV.find(key);
    if (uv == g_nativeUV.end())
    {
        if (g_opened && g_editing.getCharacter() == c && g_preview) pm->setImageWidget(g_editing, g_preview, true);
        c = saved.getCharacter();
        if (!c) return;
        pm = PortraitManager::getInstance();
        if (!pm || pm->texture.isNull()) return;
        uv = g_nativeUV.find(key);
        if (uv == g_nativeUV.end()) return;
    }
    Ogre::TexturePtr source = pm->texture;
    std::map<std::string, std::string>::iterator textureName = g_nativeTextures.find(key);
    if (textureName != g_nativeTextures.end())
    {
        Ogre::TexturePtr actual = Ogre::TextureManager::getSingleton().getByName(textureName->second);
        if (!actual.isNull()) source = actual;
    }
    Compose(c, source, pm->texturePortraitSize, uv->second);
}

void SetComposed(Character* c, MyGUI::ImageBox* image)
{
    if (!c || !image || Effective(c).background == 0) return;
    std::string key = Key(c);
    std::map<std::string, Ogre::TexturePtr>::iterator i = g_composed.find(key);
    if (i != g_composed.end() && !i->second.isNull())
    {
        image->setImageInfo(i->second->getName(), MyGUI::IntCoord(0, 0, i->second->getWidth(), i->second->getHeight()), MyGUI::IntSize(i->second->getWidth(), i->second->getHeight()));
        image->setImageIndex(0);
    }
    else if (ou && !key.empty() && GetTickCount() - g_requested[key] > 1000)
    {
        g_requested[key] = GetTickCount();
        ou->addPortraitUpdate(c->getHandle());
    }
}

void (*g_originalCell)(PortraitMainCellView*, const MyGUI::IBDrawItemInfo&, PortraitData*) = NULL;
void CellHook(PortraitMainCellView* cell, const MyGUI::IBDrawItemInfo& info, PortraitData* data)
{
    if (!cell) return;
    g_originalCell(cell, info, data);
    if (g_failed || !data) return;
    Character* c = data->getCharacter();
    if (c && c->isPlayerCharacter()) SetComposed(c, cell->imagePortrait);
}

struct RenderContext
{
    Character* character;
    Settings settings;
    Ogre::Camera* camera;
    Ogre::Vector3 position, focus;
    Ogre::Quaternion orientation;
    Ogre::Radian fov;
    bool adjusted, hasFocus;
    RenderContext() : character(NULL), camera(NULL), adjusted(false), hasFocus(false) {}
};

__declspec(thread) RenderContext* g_active = NULL;
__declspec(thread) bool g_modifying = false;

struct ModifyScope
{
    bool previous;
    ModifyScope() : previous(g_modifying) { g_modifying = true; }
    ~ModifyScope() { g_modifying = previous; }
};

void (*g_originalLook)(Ogre::Camera*, const Ogre::Vector3&) = NULL;
void (*g_originalLookXYZ)(Ogre::Camera*, float, float, float) = NULL;
const Ogre::Matrix4& (*g_originalView)(const Ogre::Camera*) = NULL;
const Ogre::Matrix4& (*g_originalViewOwn)(const Ogre::Camera*, bool) = NULL;

void LookHook(Ogre::Camera* camera, const Ogre::Vector3& target)
{
    g_originalLook(camera, target);
    if (g_active && !g_modifying && !g_active->adjusted)
    {
        g_active->camera = camera;
        g_active->focus = target;
        g_active->hasFocus = true;
    }
}

void LookXYZHook(Ogre::Camera* camera, float x, float y, float z)
{
    g_originalLookXYZ(camera, x, y, z);
    if (g_active && !g_modifying && !g_active->adjusted)
    {
        g_active->camera = camera;
        g_active->focus = Ogre::Vector3(x, y, z);
        g_active->hasFocus = true;
    }
}

void Adjust(const Ogre::Camera* source)
{
    RenderContext* ctx = g_active;
    if (!g_pluginReady || !ctx || g_modifying || ctx->adjusted || !source) return;
    Ogre::Camera* camera = const_cast<Ogre::Camera*>(source);
    if (ctx->camera && ctx->camera != camera) return;
    ctx->camera = camera;
    ctx->position = camera->getPosition();
    ctx->orientation = camera->getOrientation();
    ctx->fov = camera->getFOVy();
    ctx->adjusted = true;

    Ogre::Vector3 focus = ctx->hasFocus ? ctx->focus : ctx->position + camera->getDirection() * 10.f;
    Ogre::Quaternion turn(Ogre::Degree(ctx->settings.yaw), Ogre::Vector3::UNIT_Y);
    Ogre::Vector3 right = turn * (ctx->orientation * Ogre::Vector3::UNIT_X);
    Ogre::Vector3 offset = Ogre::Vector3(0, ctx->settings.height, 0) + right * ctx->settings.horizontal;

    ModifyScope scope;
    camera->setPosition(focus + turn * (ctx->position - focus) + offset);
    g_originalLook(camera, focus + offset);
    camera->setFOVy(Ogre::Radian(ctx->fov.valueRadians() / ctx->settings.zoom));
}

const Ogre::Matrix4& ViewHook(const Ogre::Camera* camera)
{
    Adjust(camera);
    return g_originalView(camera);
}

const Ogre::Matrix4& ViewOwnHook(const Ogre::Camera* camera, bool own)
{
    Adjust(camera);
    return g_originalViewOwn(camera, own);
}

void Restore(RenderContext& ctx)
{
    if (ctx.adjusted && ctx.camera)
    {
        ModifyScope scope;
        ctx.camera->setPosition(ctx.position);
        ctx.camera->setOrientation(ctx.orientation);
        ctx.camera->setFOVy(ctx.fov);
    }
}

void (*g_originalRender)(RenderToTextureotron*, Character*, Ogre::SharedPtr<Ogre::Texture>, const iVector2&, const Ogre::TRect<float>&) = NULL;
void RenderHook(RenderToTextureotron* machine, Character* c, Ogre::SharedPtr<Ogre::Texture> texture, const iVector2& size, const Ogre::TRect<float>& rect)
{
    if (!g_pluginReady || !c || !c->isPlayerCharacter())
    {
        g_originalRender(machine, c, texture, size, rect);
        return;
    }
    hand saved = c->getHandle();
    RenderContext ctx;
    ctx.character = c;
    ctx.settings = Effective(c);
    RenderContext* previous = g_active;
    g_active = &ctx;
    try { g_originalRender(machine, c, texture, size, rect); }
    catch (...) { g_active = previous; Restore(ctx); throw; }
    g_active = previous;
    Restore(ctx);

    c = saved.getCharacter();
    if (!c) return;
    if (Effective(c).background != 0) g_pendingComposition[Key(c)] = c->getHandle();
}

} // namespace

bool Install()
{
    if (g_pluginReady) return true;

    char local[MAX_PATH];
    DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", local, MAX_PATH);
    if (!length || length >= MAX_PATH)
    {
        LogError("Portrait Editor: cannot find settings folder");
        return false;
    }
    std::string folder = std::string(local) + "/kenshi";
    CreateDirectoryA(folder.c_str(), NULL);
    g_config = folder + "/portrait_editor.ini";

    // Set paths for gui and backgrounds
    g_resources = "mods/TotalEditor/gui";
    g_backgroundFolder = "mods/TotalEditor/backgrounds";
    ScanBackgrounds();

    HMODULE ogre = GetModuleHandleA("OgreMain_x64.dll");
    const char* exports[] = {
        "?lookAt@Camera@Ogre@@QEAAXAEBVVector3@2@@Z",
        "?lookAt@Camera@Ogre@@QEAAXMMM@Z",
        "?getViewMatrix@Camera@Ogre@@UEBAAEBVMatrix4@2@XZ",
        "?getViewMatrix@Camera@Ogre@@QEBAAEBVMatrix4@2@_N@Z"
    };
    void* targets[4];
    for (int i = 0; i < 4; ++i)
    {
        targets[i] = ogre ? reinterpret_cast<void*>(GetProcAddress(ogre, exports[i])) : NULL;
        if (!targets[i])
        {
            LogError("Portrait Editor: camera export not found.");
            return false;
        }
    }

    if (KenshiLib::AddHook(targets[0], &LookHook, &g_originalLook) != KenshiLib::SUCCESS) g_failed = true;
    if (KenshiLib::AddHook(targets[1], &LookXYZHook, &g_originalLookXYZ) != KenshiLib::SUCCESS) g_failed = true;
    if (KenshiLib::AddHook(targets[2], &ViewHook, &g_originalView) != KenshiLib::SUCCESS) g_failed = true;
    if (KenshiLib::AddHook(targets[3], &ViewOwnHook, &g_originalViewOwn) != KenshiLib::SUCCESS) g_failed = true;

    HMODULE mygui = GetModuleHandleA("MyGUIEngine_x64.dll");
    void* uvTarget = mygui ? reinterpret_cast<void*>(GetProcAddress(mygui, "?_setUVSet@ImageBox@MyGUI@@AEAAXAEBU?$TRect@M@types@2@@Z")) : NULL;
    void* textureTarget = mygui ? reinterpret_cast<void*>(GetProcAddress(mygui, "?setImageTexture@ImageBox@MyGUI@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z")) : NULL;

    if (!textureTarget || KenshiLib::AddHook(textureTarget, &TextureHook, &g_originalTexture) != KenshiLib::SUCCESS) g_failed = true;
    if (!uvTarget || KenshiLib::AddHook(uvTarget, &UVHook, &g_originalUV) != KenshiLib::SUCCESS) g_failed = true;

    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&PortraitManager::setImageWidget), &SetImageHook, &g_originalSetImage) != KenshiLib::SUCCESS) g_failed = true;
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&RenderToTextureotron::renderCharacter), &RenderHook, &g_originalRender) != KenshiLib::SUCCESS) g_failed = true;
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&PortraitMainCellView::update), &CellHook, &g_originalCell) != KenshiLib::SUCCESS) g_failed = true;

    if (g_failed)
    {
        LogError("Portrait Editor: failed to install hooks.");
        return false;
    }

    g_pluginReady = true;
    LogInfo("Portrait Editor: hooks installed successfully.");
    return true;
}

void Tick()
{
    if (g_failed || !g_pluginReady) return;

    try
    {
        std::map<std::string, hand> queued;
        queued.swap(g_pendingComposition);
        for (std::map<std::string, hand>::iterator i = queued.begin(); i != queued.end(); ++i)
        {
            Character* c = i->second.getCharacter();
            if (c) ComposeRegistered(c);
        }

        if (!g_opened || !g_preview) return;
        Character* c = g_editing.getCharacter();
        if (!c) { Cancel(NULL); return; }

        if (g_dirty && GetTickCount() - g_changedAt > 180)
        {
            g_dirty = false;
            QueuePortrait();
        }

        PortraitManager* pm = PortraitManager::getInstance();
        if (pm) pm->setImageWidget(g_editing, g_preview, true);

        c = g_editing.getCharacter();
        if (g_opened && c && g_preview) SetComposed(c, g_preview);
    }
    catch (...) {}
}

void Decorate(Character* c, MyGUI::ImageBox* image)
{
    if (!c || !image) return;
    PortraitManager* pm = PortraitManager::getInstance();
    if (!pm) return;
    hand h = c->getHandle();
    pm->setImageWidget(h, image, true);
    c = h.getCharacter();
    if (c) SetComposed(c, image);
}

void Open(Character* c)
{
    if (!c) return;
    if (!g_window) CreateUI();
    if (g_opened) Cancel(NULL);

    g_editing = c->getHandle();
    g_draft = Get(c);
    g_opened = true;

    if (g_window)
    {
        g_window->setVisible(true);
        if (g_title) g_title->setCaption("PORTRAIT - " + c->getName());
        Sync();
    }

    PortraitManager* pm = PortraitManager::getInstance();
    if (pm) pm->setImageWidget(g_editing, g_preview, true);

    Changed();
}

void Close()
{
    Cancel(NULL);
}

bool IsOpen()
{
    return g_opened;
}

} // namespace PortraitEditor
} // namespace totaleditor
