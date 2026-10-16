# Personal Quirks binary patch

`patch_pq.py` patches the closed-source Workshop plugin **Personal Quirks**
(`PersonalityQuirks.dll`, Workshop item 3792221410, x64) in place. It fixes two
behaviour bugs and changes nothing else.

Known build: original SHA-256 `ca8831d860f29c6804946312162fbf92ecdef65bce1411bbfbbe3fad7177a286`,
patched SHA-256 `0bd02f926bbd511e4ea27d51bc8b02f540926baec9b362f14d9418fc388911e2`.

## What each fix does

| Fix | Behaviour before | Behaviour after | Site (RVA) |
| --- | --- | --- | --- |
| A | WARM_KIND first aid ignored `isEnemy` and skipped every **ally**, so it healed only neutrals and enemies | Enemies (`Character::isEnemy(target, true)`, vtable +0x3E8) are skipped; allies, own squad and neutrals are accepted | `0x6490`–`0x64C8` in fn `0x5FF0` |
| B | A healer walked into buildings to treat anyone | A target inside a building (`isValid(target->isIndoors())`, RootObject vtable +0x1D8) is accepted only if `Character::isAlly(target, true)` (vtable +0x3F0, true for the same faction) | same block |
| C | FEARFUL panic re-issued `forceOrderInsert(RUN_AWAY_FORCED)` every 1 s for 4 s, which overrode player orders | Player characters get only the initial forced run-away. Their next re-force time is set to the panic end, so the re-force branch never fires. NPCs keep the 1 s re-force | `0x6F11` in fn `0x6A80`, jumps to a code cave at `0x135E0` |

Fix C needs the `.text` VirtualSize raised from `0x125D7` to `0x12600`, which equals
its SizeOfRawData. This maps the zero padding that holds the cave. The PE CheckSum
is recomputed.

The NPC two-pass logic and the player-target preference roll are unchanged. The old
"NPC healer may treat an allied player character" exception is no longer needed,
because allies are now always accepted.

## Usage

Close Kenshi first. The script refuses to run while `kenshi_x64.exe` is running.

```
py -I patch_pq.py "D:\SteamLibrary\steamapps\workshop\content\233860\3792221410\PersonalityQuirks.dll"
py -I patch_pq.py <dll> --dry-run      # verify only
py -I patch_pq.py <dll> --out <file>   # write a patched copy, leave the input alone
```

- The script checks the exact original bytes at every site. It refuses a file that is
  partially patched or has unexpected bytes.
- It is idempotent: an already patched DLL is reported and left untouched.
- The first in-place run writes `PersonalityQuirks.dll.orig` next to the DLL. An
  existing backup is never overwritten. To restore, copy the `.orig` file back.

## After a Workshop update

Steam can replace the DLL when the mod updates or when you verify files.

1. Run the script again.
2. If it reports `REFUSED: ... unexpected bytes`, the author shipped a new build.
   The offsets no longer apply, so the patch must be redone against the new
   disassembly. Do not force it.
3. Delete or refresh the old `.orig` backup after an update, so the backup matches
   the new original.
