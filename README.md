# TotalEditor

[![Language: English](https://img.shields.io/badge/Language-English-blue)](#) [Español](README.es.md)

An in-game editor for Kenshi race bonuses, character stats, uniform armour colours, and portrait photography, built as an RE_Kenshi C++ plugin. Press **Ctrl+K** to open it.

This file is technical documentation for developers and modders. For player-facing release summaries, see `STEAM_EN.md` and `STEAM_ES.md`.

---

## Formerly RaceStatsRework

TotalEditor replaces RaceStatsRework; it is the same mod under an expanded architecture and a new folder (`mods/TotalEditor`). On startup, if the TotalEditor folder has no `races.json` but the old `mods/RaceStatsRework` folder does, it is copied over once (`race/legacy_data.cpp`) so saved race edits carry on seamlessly. The old file is copied, never moved or deleted. The old RaceStatsRework mod must be disabled in the launcher: running both simultaneously causes hotkey and memory hook collisions.

---

## The Six Pages

1. **Race overview** — a side-by-side comparison read left to right. The source race on the left and the target on the right, each with a full report of traits, bonuses, and body values, so a copy can be verified before committing. Copy sits under the source it reads from; Revert sits under the target it restores.
2. **Skill bonuses** — the target race's multipliers on the right, and the controls that modify one on the left. Selecting a list entry stages it for editing.
3. **Body parts** — per-part health and hit weights, plus the whole-body blood range.
4. **Equipment and nature** — sixteen toggles controlling race permissions (hats, shirts, boots, swimming, carrying, robot flag, vampirism, etc.).
5. **Character** — edits the individual character currently selected in the world:
   - **Skills**: individual attributes and combat skills on the engine's 0–200 scale.
   - **Personality**: sets the AI personality state (`personality.cpp`), choosing from the 10 game personalities (Honorable, Traitorous, Smart, Dumb, Brave, Fearful, Warm/Kind, Cold/Cruel, Normal, Maniacal).
   - **Edit Appearance**: button opens Kenshi's native character creation window on the selected player character (`appearance.cpp`).
6. **Uniform & Portrait** — integrated personal styling and photography:
   - **Armour Colours**: customize primary and secondary armor tint hex codes with real-time in-game preview on the live character model. Includes RGB delta buttons (±15), preset cycling, and live color swatch indicators. Changes are preview-only until pressing **Save**; pressing **Cancel** or closing the window reverts all unsaved changes instantly; **Vanilla** restores original gear tints.
   - **Portrait Camera & Background**: open the dedicated portrait photography editor to adjust rotation, camera elevation, horizontal framing, zoom (FOV), and custom background backdrops (Teal, Black, Sand, Custom RGB, Bordeaux, or user image files).

---

## What It Edits, and What That Means

The window operates across multiple architectural scopes:

- **Race bonuses are global and shared.** `RaceData::AllRaces` contains one instance per race definition, and `Character::myRace` points directly into it. Modifying Greenlander multipliers modifies every Greenlander in the world: player squad members, town guards, and hostile bandits alike. Bonuses are **multipliers** (`stat *= bonus`), where `1.0` means unchanged and `1.15` means +15%.
- **Character stats are individual.** Modifying skills on the Character page writes directly to the selected character's memory structure on Kenshi's `0..200` scale and forces immediate engine recalculation (encumbrance, attack speed).
- **Uniform colours are character-bound.** Gear color schemes are persisted per character unique identifier (`CharacterIdentity`), overriding faction cloth/metal tints without affecting other squad members or faction NPCs.

---

## Race Template vs Living Character

Almost all race data is a blueprint read when the engine creates or loads an entity. Modifying the blueprint does not retroactively alter already-spawned characters unless explicitly synchronized:

| Edit | On the selected character in the world |
|---|---|
| Equipment slots (hats, shirts, boots) | Inventory slots dynamically toggle enabled/disabled |
| Blood range | Recalculated and interpolated across character age |
| Body-part health | Applied directly onto living anatomy (`ApplyAnatomyToCharacter`) |
| Character skills | Written directly to character instance |
| Uniform colours | Applied live via temporary preview scheme, persisted on Save |

---

## Uniform and Portrait Pipeline

### Uniform Engine (`src/uniform/`)
- **Session Preview**: When switching characters or tuning colors, `UniformEngine::BeginPreview` creates a temporary in-memory `GameData` scheme.
- **Live Updates**: Color changes push immediate updates through `UniformEngine::PreviewColours` and `UniformDresser::RefreshEquipped`, dynamically repainting the equipped meshes.
- **Persistence**: Saved palettes are stored in `uniform_colours.json` mapped by character UID (`uniform_store.cpp`). Unsaved edits are rolled back on discard or window close via `CancelPreview`.

### Portrait Editor (`src/portrait/`)
- **Camera Hooks**: Intercepts `Ogre::Camera::getViewMatrix` and `RenderToTextureotron::renderCharacter` to manipulate camera angle, height, position, and focal length without modifying world geometry.
- **Atlas Blitting & Composition**: Intercepts `PortraitManager::setImageWidget` and `PortraitMainCellView::update`. Renders the character to texture, composites transparency over chosen background color/textures, and blits the resulting composite directly into the UI portrait cache.

---

## Persistence Architecture

TotalEditor maintains separate persistence stores beside its DLL:

1. **`races.json`**:
   - `originals` / `originalTraits` / `originalParts`: Captured the first time each race is loaded; acts as the immutable fallback for the Revert action.
   - `overrides` / `overrideTraits` / `overrideParts`: Active user modifications reapplied on game launch.
2. **`uniform_colours.json`**:
   - Maps persistent character UIDs to primary and secondary hex color codes.
3. **Save Game**:
   - Individual character skills, attributes, and personalities reside directly inside Kenshi's `.pl_save` files.

---

## Modular Codebase Structure

The codebase is organized into domain-driven modules:

```text
TotalEditor/
├── src/
│   ├── TotalEditor.cpp            Plugin entry points (startPlugin, shutdown)
│   ├── common.h                   Shared definitions, macros, and SEH helpers
│   ├── core/                      Infrastructure and engine integration
│   │   ├── config.cpp/.h          Configuration loader (config.json)
│   │   ├── frame_tick.cpp/.h      Main loop and game initialization hooks
│   │   ├── hotkey.cpp/.h          OIS keyboard hook and key mapping
│   │   ├── json.cpp/.h            Lightweight JSON serialization/deserialization
│   │   ├── log.cpp/.h             Three-tier logging (LogError, LogInfo, LogDebug)
│   │   └── messages.cpp/.h        Localization dictionary (messages.json)
│   ├── race/                      Race definitions and global stats
│   │   ├── anatomy.cpp/.h         Body part health, hit weights, and blood ranges
│   │   ├── catalog.cpp/.h         GameData race and stat enumeration
│   │   ├── legacy_data.cpp/.h     Migration from legacy RaceStatsRework store
│   │   ├── race_edit.cpp/.h       Memory readers and writers for race bonuses
│   │   └── store.cpp/.h           races.json persistence and revert baselines
│   ├── character/                 Per-character modifications
│   │   ├── appearance.cpp/.h      In-game character appearance editor bridge
│   │   ├── character_identity.cpp/.h Character UID generation and resolution
│   │   └── personality.cpp/.h     AI personality state reading and writing
│   ├── uniform/                   Armour tinting and clothing schemes
│   │   ├── colour_adjust_rules.cpp/.h Channel stepping (±15) and preset cyclers
│   │   ├── uniform_colour.cpp/.h  RGB/Hex conversions and validation
│   │   ├── uniform_dresser.cpp/.h Armour slot detection and material recoloring
│   │   ├── uniform_engine.cpp/.h  Preview state machine, save, and restore
│   │   ├── uniform_schemes.cpp/.h In-memory GameData scheme generation
│   │   └── uniform_store.cpp/.h   uniform_colours.json persistence
│   ├── portrait/                  Portrait camera and composite rendering
│   │   └── portrait_editor.cpp/.h Ogre camera hooks, background blitting, and popup UI
│   └── ui/                        MyGUI presentation layer
│       ├── editor_window.cpp/.h   Main 6-tab editor window and layout
│       └── ui_widgets.cpp/.h      Guarded MyGUI factory and styling primitives
└── TotalEditor/                   Deployed mod distribution files
    ├── TotalEditor.mod
    ├── TotalEditor.dll
    ├── RE_Kenshi.json
    ├── config.json
    ├── messages.json
    ├── backgrounds/
    └── gui/
```

---

## Configuration Reference (`config.json`)

| Setting | Default | Description |
|---|---|---|
| `enabled` | `true` | Enables or disables all plugin hooks |
| `hotkey.key` | `"K"` | Key name or OIS scancode to toggle the editor |
| `hotkey.needsCtrl` | `true` | Requires Ctrl modifier when true |
| `bonus.min` / `bonus.max` | `0.1` / `5.0` | Multiplier bounds for skill bonuses |
| `bonus.steps` | `98` | Slider granularity across the multiplier range |
| `stat.min` / `stat.max` | `0.0` / `200.0` | Range for individual character skills |
| `partHealth.min` / `partHealth.max` | `1.0` / `500.0` | Allowed HP range for anatomical parts |
| `window.width` / `window.height` | `1180` / `920` | Editor window dimensions in pixels |
| `store.saveOnClose` | `true` | Automatically writes modified data on window close |
| `store.applyOnStart` | `true` | Applies saved overrides when loading into a world |
| `followSelection` | `true` | Automatically inspects selected character on game click |
| `debugLog` | `false` | Enables verbose step-by-step diagnostic logging |

---

## Building and Deployment

### Requirements
- Visual Studio (MSVC) with **Platform Toolset v100** (Visual Studio 2010 compatibility toolchain)
- KenshiLib SDK (headers and libraries) pointed to by `KENSHILIB_DIR`
- Boost 1.60+ headers pointed to by `BOOST_INCLUDE_PATH`

### Build Command
Run MSBuild targeting the 64-bit Release configuration:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" TotalEditor.vcxproj /p:Configuration=Release /p:Platform=x64
```

The Post-Build event automatically deploys `TotalEditor.dll` to `D:\SteamLibrary\steamapps\common\Kenshi\mods\TotalEditor\TotalEditor.dll` if the folder exists.

> [!WARNING]
> Kenshi locks `TotalEditor.dll` in memory while running. Ensure Kenshi is closed before compiling to avoid write permission errors.

---

## Technical Notes & SDK Lessons

- **MyGUI WhiteSkin Hierarchy**: Do not parent `WhiteSkin` widgets directly inside another `WhiteSkin` widget. In Kenshi's MyGUI version, parent SubSkins can obscure nested children. Always instantiate border and fill swatches as sibling widgets on the parent panel.
- **`MyGUI::Slider` Symbol Absences**: `MyGUI::Slider` methods are unexported in Kenshi's engine binaries. Value sliders are constructed using `MyGUI::ScrollBar` with `setMoveToClick(true)`.
- **Virtual Functions**: Never declare matching `virtual` member functions against Kenshi engine classes in plugin code, as MSVC will emit an independent vtable. Call the corresponding non-virtual `_NV_` thunks provided by KenshiLib.
- **Anatomy Storage**: Combat anatomy HP and hit chances reside inside the race's `GameData` list (`TripleInt`), not directly within `RaceData`.
- **Exception Boundary Discipline**: Never write bare empty catch blocks (`TE_CATCH { }`) in event loops or binding routines. An unhandled exception during `BindEvents` unwinds the entire function, leaving remaining controls unresponsive and mimicking mouse input bugs.
