#pragma once

// Shared includes and small helpers for TotalEditor.
//
// Include order matters here. Ogre defines Ogre::min/max as functions taking
// const float&, which the min/max macros in <Windows.h> destroy, so the engine
// headers must be seen first. NOMINMAX keeps them suppressed regardless.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Kenshi.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
// The player's current selection lives on ForgottenGUI, which PlayerInterface
// only forward-declares.
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/RaceData.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/RootObject.h>
#include <kenshi/Enums.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

// MyGUI_DelegateImplement.h is an internal header the public ones expand
// several times with different macros; including it directly breaks the build.
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_ComboBox.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_Macros.h>
#include <mygui/MyGUI_MouseButton.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ScrollBar.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_Window.h>

#include <Windows.h>

namespace totaleditor
{
// The only Kenshi build whose reversed addresses this plugin is validated
// against. Hooking a different build would resolve to the wrong functions.
const char* const kSupportedGameVersion = "1.0.65";

// Written to the log at startup. A bug report that does not say which build it
// came from cannot be acted on, and the player has no other way to tell.
const char* const kModVersion = "1.0.0";
} // namespace totaleditor

// Reading a RootObject* the engine already freed access-violates rather than
// returning null, so every dereference of engine-owned memory is guarded.
#define TE_TRY __try
#define TE_CATCH __except (EXCEPTION_EXECUTE_HANDLER)
