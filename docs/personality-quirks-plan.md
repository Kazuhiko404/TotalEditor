# Personality Quirks in TotalEditor — design plan

Status: **planned, not implemented.** This document captures the reverse-engineering
of the Workshop mod *Personal Quirks — Beta* (id 3792221410, `PersonalityQuirks.dll`)
and the plan to rebuild it inside TotalEditor, with a new tab that turns every
bonus and quirk ON or OFF.

## 1. Goal

- Give Kenshi's existing personalities (`StateBroadcastData::myPersonality`, edited
  from the Character tab) the same passive effects and quirks as Personal Quirks.
- Every single effect can be switched ON/OFF from a new **Quirks** tab, separately
  for player characters and NPCs.
- Fix the original mod's bugs instead of copying them (see §4).
- Add nothing Kenshi does not already have: every behaviour issues a vanilla order
  (`FIRST_AID_ORDER`, `RUN_AWAY_FORCED`) or adjusts a value the game already reads.

## 2. How the original works (confirmed by disassembly)

### 2.1 Hooks

| Target | Purpose |
|---|---|
| `CharStats::getStat` | skill buffs, frenzy Precision Shooting penalty |
| `CharStats::getMeleeAttack` / `getMeleeDefence` | melee buffs |
| `CharStats::getStealthSkill01` | Traitorous stealth |
| `GunClass::calculateReloadTime` | Fearful and frenzy reload |
| `CraftingBuilding::calculateCriticalChance` | Smart quirk |
| `MedicalSystem::applyFirstAid` / `applyDoctoring` | Warm / Kind +10% |
| `AnimationClass::chooseStumbleAnim` | frenzy flinch suppression |
| `GameWorld::_NV_mainLoop_GPUSensitiveStuff` | per-frame quirk ticks |
| `TitleScreen::_CONSTRUCTOR` | creates its windows |
| `Character::addOrder`, `OrdersReceiver::addOrder/addJob`, `Character::_startStumble` | debug logging only |

### 2.2 Passive buffs

Every buff hook calls the original, then changes only the returned value:
`v + |v|·k` (or `v - |v|·k`). Nothing is written to the save, so switching a buff
off reverts it instantly. `CharStats+0x10` is the owning `Character*`.
"Precision Shooting" is `STAT_FRIENDLY_FIRE` (36).

| Personality | Effect | Hook / stat |
|---|---|---|
| Honorable | +5% Melee Defence | getMeleeDefence |
| Traitorous | +10% Stealth (clamped to 1.0) | getStealthSkill01 |
| Smart | +10% Science, -5% Labouring, -5% Farming | getStat 4 / 3 / 12 |
| Smart (quirk) | crafting crit chance x1.2 (relative, clamped to 1.0) | calculateCriticalChance |
| Dumb | +10% Labouring | getStat 3 |
| Brave | +10% Melee Defence | getMeleeDefence |
| Fearful | +10% Athletics, reload time /1.05 | getStat 17, calculateReloadTime |
| Warm / Kind | +10% First Aid & Doctoring skill, +5% Precision Shooting, -5% Assassination | applyFirstAid/applyDoctoring, getStat 36 / 22 |
| Cold / Cruel | +5% Melee Attack | getMeleeAttack |
| Maniacal | +10% Melee Attack, -10% Melee Defence | getMeleeAttack / getMeleeDefence |
| Normal / None | nothing | — |

`getStat` buffs apply only when its bool argument is false; `getStealthSkill01`
only when its bool is true (to be confirmed against KenshiLib parameter names).

### 2.3 Maniacal — Frenzy

- Per character (map keyed by `hand`): while `isInCombatMode(true,true)`, 10 s of
  frenzy, then 20 s of recovery; it cycles for the whole fight.
- `chooseStumbleAnim`: during frenzy, a non-`bigone` stumble returns no animation
  (light flinch suppressed); heavy stumbles still play. Character from
  `AnimationClass+0x2D8`.
- Crossbows: reload time /1.2; Precision Shooting -20%. Damage is not reduced.
- Player characters only: "<name> enters a frenzy!" through
  `GameWorld::showPlayerAMessage`, once per frenzy.

### 2.4 Fearful — Panic

- "Badly injured": in bloodloss trauma, blood below 40% of max, or a non-robotic
  torso/head part below 50% flesh. A leg below 50% blocks the panic (cannot run).
- Gate: in combat, not already fleeing, not already running away, cooldown over.
- Roll: players 25% every 30 s, NPCs 33% every 10 s.
- Panic: `AI::forceOrderInsert(RUN_AWAY_FORCED = 280)` for 4 s, then
  `removeGoal(280)`; 60 s cooldown. "<name> panics!" for player characters.

### 2.5 Warm / Kind — Medical assistance

- Every 5 s per healer, outside combat, with no enemies nearby, not busy with a
  medical task, 30 s cooldown after each aid.
- Candidates: every loaded character within 50 m who is down, unconscious, not in
  combat, needs first aid, reachable, and the healer has a kit.
- Order: `Character::addOrder(nullptr, FIRST_AID_ORDER, target, false, true, 0)`,
  the same order a player gives with a right-click.

## 3. Player report this design answers

> "...my character ran off on their own to heal random people ... they ran off in
> town to heal some bandit who got beaten up inside a shop after closing hours, and
> then that made the healer an intruder and got them thrown into the slammer."

Root cause, confirmed in the disassembly:

1. **Inverted faction filter.** `isEnemy(target)` is called and its result thrown
   away; `isAlly(target)` is called and the candidate is **skipped when it is an
   ally**. The quirk heals only neutrals and enemies — never the healer's own squad.
2. **No location check.** Nothing tests whether the target is inside a building,
   so the healer walks into a closed shop and is jailed for trespassing.

## 4. Fixes in our version

| Original problem | Our behaviour |
|---|---|
| Heals neutrals and enemies, never the squad | Own squad, allies and neutrals only. Enemies never. |
| Walks into closed buildings | A target inside a building is accepted only if it is an ally (own squad counts). Neutrals indoors are skipped. |
| Panic re-forces the run-away order every 1 s, overriding the player | For player characters the run-away order is issued once; the player can take control back. NPCs unchanged. |
| Per-character maps never cleaned | Cleared on game load / reset and pruned of characters that leave the update list. |
| Polls 255 keys with `GetAsyncKeyState` every frame (debug window) | No debug window, no key polling. |
| Raw offsets everywhere | KenshiLib members where they exist; offsets only where the SDK has none, each documented. |

The same three bugs are fixed in place in the original DLL by
`tools/personality-quirks-patch/` for players who keep using that mod.

## 5. TotalEditor design

### 5.1 Modules (one responsibility each)

| File | Responsibility |
|---|---|
| `src/quirk_settings.h/.cpp` | the ON/OFF switches, defaults, load/save of `quirks.json` |
| `src/quirk_buffs.h/.cpp` | the return-value hooks for the passive buffs and Smart crit |
| `src/quirk_frenzy.h/.cpp` | Maniacal frenzy state, stumble and reload hooks, message |
| `src/quirk_panic.h/.cpp` | Fearful panic state and orders |
| `src/quirk_aid.h/.cpp` | Warm / Kind target filter and first-aid order |
| `src/quirk_tick.h/.cpp` | one pass over the update list per frame feeding frenzy/panic/aid |
| `src/personality.*` (existing) | personality read, reused by all of the above |

The per-frame tick reuses the existing `frame_tick.cpp` hook instead of adding a
second main-loop hook.

### 5.2 Settings file

Switches live in `quirks.json` next to the DLL — **not** in `config.json`, which
players edit by hand and the mod must never rewrite. The Quirks tab writes
`quirks.json`; unknown keys are kept; a malformed file is not overwritten.

Every effect has its own switch, and each switch has a player and an NPC value:

```json
{
  "buffs": {
    "honorable.meleeDefence":   { "player": true, "npc": false },
    "traitorous.stealth":       { "player": true, "npc": false },
    "smart.science":            { "player": true, "npc": false },
    "smart.labouring":          { "player": true, "npc": false },
    "smart.farming":            { "player": true, "npc": false },
    "dumb.labouring":           { "player": true, "npc": false },
    "brave.meleeDefence":       { "player": true, "npc": false },
    "fearful.athletics":        { "player": true, "npc": false },
    "fearful.reload":           { "player": true, "npc": false },
    "warmKind.medic":           { "player": true, "npc": false },
    "warmKind.precision":       { "player": true, "npc": false },
    "warmKind.assassination":   { "player": true, "npc": false },
    "coldCruel.meleeAttack":    { "player": true, "npc": false },
    "maniacal.meleeAttack":     { "player": true, "npc": false },
    "maniacal.meleeDefence":    { "player": true, "npc": false }
  },
  "quirks": {
    "smart.methodical":         { "player": true, "npc": true },
    "maniacal.frenzy":          { "player": true, "npc": true },
    "fearful.panic":            { "player": true, "npc": true },
    "warmKind.firstAid":        { "player": true, "npc": true }
  }
}
```

Defaults mirror the original INI (`buffs_player=1, buffs_npc=0, quirks_player=1,
quirks_npc=1`). Every hook checks its switch on each call, so a change in the tab
applies at once with no restart.

### 5.3 The Quirks tab (6th tab)

- `kPageCount` 5 -> 6; tab buttons shrink from ~229 px to ~191 px at the default
  1180 px window, still wide enough for their captions.
- Built like the existing flag page (`BuildFlagPage`): one row per effect, grouped
  under a heading per personality, with two tick boxes per row — **Player** and
  **NPC** — and the effect text ("+10% Melee Defence").
- Quirk rows show a one-line description (e.g. "Panic: 25% every 30 s when badly
  injured, 4 s, 60 s cooldown").
- Header buttons: **All ON**, **All OFF**, **Defaults**.
- Ticking a box writes `quirks.json` and sets the status line.
- 19 rows do not fit the 607 px page at the current row height; the page uses a
  two-column layout (buffs left, quirks right) — confirm with a mockup before
  building.

### 5.4 Compatibility

- At start, if `PersonalityQuirks.dll` is loaded in the process, log a warning and
  show it in the Quirks tab: running both doubles every buff.
- Personality changes made in the Character tab take effect immediately, because
  every hook reads the personality on each call.

## 6. Tasks and estimate

| Task | Content | Effort | Lines (approx.) |
|---|---|---|---|
| Q1 | `quirk_settings` + `quirks.json` + Quirks tab with ON/OFF (mockup first) | low-medium | ~400 |
| Q2 | passive buffs + Smart crit hooks | low | ~300 |
| Q3 | Maniacal frenzy | low-medium | ~250 |
| Q4 | Fearful panic | medium | ~300 |
| Q5 | Warm / Kind first aid with the fixed filter | medium | ~300 |

Total ~1,500 lines, about 3 sessions, plus in-game testing of Q4 and Q5 (they
drive vanilla AI and need tuning in play).

## 7. In-game test plan

- Buffs: change a squad member's personality in the Character tab and watch the
  stat in the Character Stats window move; toggle its switch OFF and see it revert.
- Smart: compare crit rate at a crafting bench with the switch ON/OFF.
- Frenzy: Maniacal fighter takes light hits without flinching for 10 s, then
  flinches normally for 20 s; heavy hits still knock down.
- Panic: Fearful squad member below 40% blood flees once for ~4 s; a new order from
  the player is not overridden.
- First aid: Warm / Kind member heals a downed squadmate, an ally and a neutral in
  the open; ignores a downed enemy; ignores a downed neutral inside a shop.
