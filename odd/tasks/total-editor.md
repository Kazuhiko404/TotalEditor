# Feature: TotalEditor (rename + personality editor)

## Objective
Rename the mod RaceStatsRework to TotalEditor everywhere inside the repo (the user
renames the outer repo folder), and add a character personality editor copied 1:1
from the Workshop mod CharTweak 2 (id 3782510891), using RaceStatsRework's control
conventions.

## Reverse-engineering facts (CharTweak.dll, capstone disassembly)
- Field: `Character::stateBroadcast` (0x1A0) -> `StateBroadcastData::myPersonality`
  (0xD8, enum `PersonalityTags`). Verified in KENSHILIB_DIR headers
  (StateBroadcastData.h:62, Character.h:217).
- Options, in CharTweak order: 1 Honorable, 2 Traitorous, 5 Smart, 6 Dumb, 9 Brave,
  10 Fearful, 11 Warm / Kind, 12 Cold / Cruel, 13 Normal, 14 Maniacal.
- Display `"%s (%d)"`; 0 -> "None"; other values -> "Unknown".
- Prev/Next cycle modulo 10; a value outside the table jumps to Honorable.
- Edits the selected character; null stateBroadcast -> "Selected character has no
  personality state."; no selection -> "Select a character first."
- One direct int write; no side-effect calls; persists natively in the save
  ("Kenshi will save it normally"). No sidecar, no reapply hook.
- Log line: "personality changed %s (%d) -> %s (%d)".

## Placement decision
Character tab, left column, below the skill Apply button: heading PERSONALITY,
current-value label, `[combo (full width)] [APPLY]` (Previous/Next removed at user request).
Why: personality is per-character like skills, reuses the selected character and
followSelection, and avoids a 6th tab that narrows every tab button.
RSR convention: selection controls only stage; the Apply button writes.

## Scope / constraints
- Isolated modules (personality logic in its own src/personality.* files).
- Never overwrite a player's existing data: migration fallback reads
  `..\RaceStatsRework\races.json` / `config.json` when the TotalEditor folder has none.
- Build: MSBuild v100, Release|x64. Deploy to `Kenshi\mods\TotalEditor\` unless
  kenshi_x64 is running.

## Tasks
- [x] T1 Rename RaceStatsRework -> TotalEditor (namespace, files, vcxproj, mod folder,
      .mod, RE_Kenshi.json, widget prefixes, log prefix, messages title, README,
      STEAM_*.md, .engram) + legacy-data migration fallback. Route: delegated (2+ files).
- [x] T2 Personality editor in the Character tab. Route: delegated (2+ files).
- [x] T3 Build, deploy, md5 check. Route: delegated with T2.

## Acceptance criteria
- No "RaceStatsRework"/"Race Stats"/`racestats`/`RSR_` left in tracked files
  (except the migration fallback path and history notes).
- Release|x64 builds with zero errors.
- Personality block shows current value as "Label (n)", cycles in CharTweak order,
  Apply writes `myPersonality` and reports the change in the status line and log.

## Progress / evidence
- Branch: feat/total-editor-personality
- T1 (delegated writer): identifiers renamed with `sd` on word-bounded tokens
  (`racestats`, `RS_TRY/CATCH/BIND` incl. `RS_CATCH_LOG_*`, `"RSR_`), files via
  `git mv`; legacy import in `src/legacy_data.*` (CopyFileA with failIfExists,
  races.json then config.json, run before LoadConfig). Acceptance grep leaves only
  legacy_data.*, one TotalEditor.cpp comment, README/STEAM migration notes, and
  this doc. Build: MSBuild Release|x64 -> x64\Release\TotalEditor.dll, 0 errors
  (only pre-existing KenshiLib header warnings C4482/C4091). Commit: see git log
  `refactor: rename the mod to TotalEditor` (6c438c2).
- T2: logic in `src/personality.*` (option table, labels, Prev/Next index,
  guarded read/write of `stateBroadcast->myPersonality`); UI wiring only in
  editor_window.cpp (BuildPersonalityBlock, RefreshPersonality on open and on
  selection change, Prev/Next stage, Apply writes + LogDebug). New
  `personality.*` keys added to messages.json, no existing value changed.
- Layout deviation: page height at the default window is 607px (client
  1180x875 from the Kenshi_WindowCX skin: header 38 + borders 7; page =
  875 - 268). The skill Apply ends at y=502, leaving 105px; the specified
  four-row block needs 130px with zero gaps. Built as two rows instead:
  y=512..538 heading (1/3 width) + "Current: ..." (2/3), y=542..576
  `[< Previous 100][combo 195][Next > 100][Apply personality 140]`; 31px left
  below. No existing widget moved.
- Build: MSBuild Rebuild Release|x64: 0 errors, 75 warnings (all C4482/C4091 in
  KenshiLib Kenshi.h / MyGUI BaseLayout.h, pre-existing).
- Deploy (Kenshi not running): repo TotalEditor\ copied to
  Kenshi\mods\TotalEditor\ (config.json included: folder was new). md5
  cd02cb88dab2663a71c2f18488e1fa82 matches for x64 build, repo copy and
  deployed DLL. mods\RaceStatsRework untouched; its races.json will be imported
  on first start. Pending: add TotalEditor to mods.cfg / disable
  RaceStatsRework in the launcher, and in-game check of the personality block.
- T4 (inline, user request): removed Previous/Next and NextPersonalityIndex; row is
  `[combo width-144][APPLY 140]`, Apply caption "APPLY".
- T5 (inline, user request): Edit Appearance button copied from CharTweak 2 (fn 0xCA20:
  no selection / isPlayerCharacter guard, hide window, PlayerInterface::activateCharacterEditMode).
  Module src/appearance.*; button shares the "Edit this race" row in the Character tab.
