# Total Editor

Edit any race's bonuses, body part health, blood and traits from inside the
game — and any character's own skills — without opening the FCS.

Press **Ctrl + K**.

**Formerly Race Stats Rework.** Total Editor replaces it. Your saved race edits
are picked up from the old mod's folder automatically the first time you start
the game (they are copied, the old file is left alone). Disable the old Race
Stats Rework mod — running both at once means two editors on the same key.

---

## Read this first

**Race bonuses are shared by every character of that race.** Give Shek +20%
Strength and you have given it to your squad, to town guards, and to every Shek
bandit who attacks you. Kenshi stores one bonus table per race and every
character points at it; there is no per-character version of it.

**Bonuses are multipliers, not added points.** `1.15` means fifteen percent
more, `1.0` means unchanged.

The one page that edits a single person — **Character** — is labelled as such,
and those skills are the engine's own 0–200 values.

---

## The six pages

**Race overview** — two races side by side. Pick one to copy FROM on the left
and one to change on the right; both columns list everything that race carries,
so you can see the trade before you make it. Copy sits under the source, revert
under the target.

**Skill bonuses** — the selected race's bonuses on the right. Click one to load
it into the controls on the left, then change or remove it.

**Body parts** — every part with its health and hit weight, plus the whole-body
blood range. Hit weight is relative, not a percentage: raising one part lowers
every other part's share.

**Equipment and nature** — sixteen switches, each read as a permission. Ticked means
the race CAN do it.

**Character** — the selected character's own skills, listed on the right. Click
one to edit it on the left. Below them, the character's **personality**
(Honorable, Traitorous, Smart, Dumb, Brave, Fearful, Warm / Kind, Cold / Cruel,
Normal, Maniacal): pick one from the dropdown, then press
APPLY. **Edit Appearance** opens the game's own character editor on the
selection (your squad only).

**Uniform & Portrait** — gear styling and photography for the selected character.
Tweak primary and secondary armor colors with live 3D preview on the character,
cycle presets, step RGB channels, or restore vanilla gear colors. Click "Edit portrait
camera" to open the portrait studio with camera rotation, elevation, zoom, and custom
backgrounds (teal, black, sand, custom RGB, bordeaux, or custom background images).

---

## What reaches characters already alive

Most of what you edit is the race's template, which Kenshi reads when it BUILDS
a character. Change it and nobody already walking around is affected.

These are pushed onto the selected character immediately:

- Character skills (they are not a race property at all)
- Character personality
- Blood range
- Equipment slots
- Body part health, after a copy or a manual edit

Everything else applies to characters born after the change.

---

## Undoing things

- **Revert race** puts one race back to what it shipped with, including its
  body parts.
- **Deleting `races.json`** (next to the mod's DLL) drops every edit.
- **Unsubscribing** restores vanilla races in old and new saves alike — none of
  this is written into your save file.

The exception is character skills and personality. Those live in your save,
like any other skill your character earned.

---

## Configuration

`config.json`, next to the mod's DLL.

**Changing the hotkey** — write the key by name:

```json
"hotkey": { "key": "F5", "needsCtrl": false }
```

Accepted: `A`–`Z`, `0`–`9`, `F1`–`F12`, `HOME`, `END`, `INSERT`, `DELETE`,
`PGUP`, `PGDOWN`, `SPACE`, `ENTER`, `TAB`, `BACKSPACE`, `ESCAPE`, `GRAVE`,
`MINUS`, `EQUALS`, `LBRACKET`, `RBRACKET`, `SEMICOLON`, `APOSTROPHE`,
`BACKSLASH`, `COMMA`, `PERIOD`, `SLASH`.

Other useful keys:

| Key | What it does |
|---|---|
| `window.width` / `window.height` | Window size in pixels |
| `stat.max` | Skill ceiling on the Character page, default 200 |
| `partHealth.max` | Body part health ceiling, default 500 |
| `store.applyOnStart` | Reapply saved edits on load. `false` to play vanilla without deleting them |
| `followSelection` | Window follows the character you click. `false` if you suspect it of anything |
| `debugLog` | Detailed log for bug reports, see below |

A missing or broken `config.json` is not fatal — the defaults are used.

---

## Requirements

- Kenshi **1.0.65**
- [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/906)

---

## Something went wrong?

Please include the log. Without it a report is a guess.

1. Open `config.json` and set `"debugLog": true`
2. Start the game and do the thing that fails
3. Attach `RE_Kenshi_log.txt` from your Kenshi folder

The log names the mod version and the key that opens the window, which answers
most reports on its own. Set it back to `false` afterwards — it is shared with
every other plugin you have loaded.

**The window will not open.** Another mod may have taken Ctrl+K. Change the key
as shown above.

---

## Known limits

- A race added by a mod you later disable keeps its entry in `races.json`
  rather than losing your edit — it is simply not applied while that mod is
  off, and comes back when it returns.
- Body part health is pushed onto the character you have selected. Others of
  that race already in the world keep theirs until they are next built.
