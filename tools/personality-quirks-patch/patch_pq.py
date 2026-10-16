#!/usr/bin/env python3
"""Binary patch for the Kenshi Workshop plugin "Personal Quirks" (PersonalityQuirks.dll, x64).

Fixes (see README.md next to this script for the full rationale):
  A  WARM_KIND first aid: never targets enemies; own squad, allies and neutrals are accepted.
  B  WARM_KIND first aid: a target inside a building is accepted only if it is an ally.
  C  FEARFUL panic: player characters are not re-forced into RUN_AWAY_FORCED every second
     during the 4 s panic (the initial forced order still happens). NPCs unchanged.

Usage:  py -I patch_pq.py <path-to-PersonalityQuirks.dll> [--out <path>] [--dry-run]

Safety:
  * Every patch site is verified against the exact original bytes; a file that is neither fully
    original nor fully patched is refused (e.g. a different mod version after a Workshop update).
  * Idempotent: an already patched file is reported and left untouched.
  * Writes <dll>.orig next to the DLL before the first in-place write (never overwrites it).
  * Recomputes the PE OptionalHeader CheckSum.
"""
import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys

ORIGINAL_SHA256 = "ca8831d860f29c6804946312162fbf92ecdef65bce1411bbfbbe3fad7177a286"
PATCHED_SHA256 = "0bd02f926bbd511e4ea27d51bc8b02f540926baec9b362f14d9418fc388911e2"

TEXT_VSIZE_ORIG = 0x125D7
TEXT_VSIZE_NEW = 0x12600  # == SizeOfRawData; exposes the zero tail of .text used as the code cave

# (name, rva, original bytes, patched bytes)
PATCHES = [
    # FIX A + B: target filter in the WARM_KIND first-aid tick (fn 0x5FF0), 0x6490..0x64C8.
    # Original: isEnemy result discarded; isAlly(target) == true -> skip (with an NPC/player exception).
    # Patched:
    #   6490 test al,al ; jne 64c3            -> isEnemy(target,true) -> skip
    #   6494 mov rax,[r14] ; mov r8b,1 ; mov rdx,rsi ; mov rcx,r14 ; call [rax+3F0h]   isAlly(target,true)
    #   64a6 test al,al ; jne 64c8            -> ally (own squad / same faction) -> accept
    #   64aa mov rax,[rsi] ; mov rcx,rsi ; call [rax+1D8h]                              target->isIndoors()
    #   64b6 mov rcx,rax ; call [hand::isValid]
    #   64bf test al,al ; je 64c8             -> outdoors -> accept
    #   64c3 jmp 6627                         -> skip candidate
    ("A+B warm target filter", 0x6490,
     bytes.fromhex(
         "4d8b1e41b001488bd6498bce41ff93f0030000"  # mov r11,[r14] .. call [r11+3F0h]
         "807c2442007410"                          # cmp byte [rsp+42h],0 ; je 64ba
         "807c244000"                              # cmp byte [rsp+40h],0
         "7409"                                    # je 64ba
         "4584e4"                                  # test r12b,r12b
         "7404"                                    # je 64ba
         "b101eb02"                                # mov cl,1 ; jmp 64bc
         "32c9"                                    # xor cl,cl
         "84c07408"                                # test al,al ; je 64c8
         "84c90f845f010000"                        # test cl,cl ; je 6627
     ),
     bytes.fromhex(
         "84c0752f"                                # test al,al ; jne 64c3
         "498b0641b001488bd6498bceff90f0030000"    # isAlly via [rax+3F0h]
         "84c0751e"                                # test al,al ; jne 64c8
         "488b06488bceff90d8010000"                # isIndoors via [rax+1D8h]
         "488bc8ff15c1dd0000"                      # mov rcx,rax ; call [rip+0DDC1h] -> hand::isValid
         "84c07405"                                # test al,al ; je 64c8
         "e95f010000"                              # jmp 6627
     )),
    # FIX C: FEARFUL panic start (fn 0x6A80). Original 0x6F11: lea rax,[r15+3E8h] ; mov [r12+18h],rax
    # (next re-force time = now + 1 s). Patched: jump to a cave that stores now+1 s for NPCs but
    # now+4 s (= panic end, [r12+10h]) for player characters, so the re-force branch never fires.
    ("C panic re-force site", 0x6F11,
     bytes.fromhex("498d87e803000049894424 18".replace(" ", "")),
     bytes.fromhex("e9cac60000" "cccccccccccccc")),
    # Code cave at the end of .text (raw zero padding, RVA 0x135E0..0x135FD).
    #   lea rax,[r15+3E8h] ; test sil,sil (sil = isPlayerCharacter, set at 0x6E80) ; je +7
    #   lea rax,[r15+0FA0h] ; mov [r12+18h],rax ; jmp 6f1d
    ("C code cave", 0x135E0,
     bytes(29),
     bytes.fromhex("498d87e8030000" "4084f6" "7407" "498d87a00f0000" "4989442418" "e92039ffff")),
]


class PatchError(Exception):
    pass


def pe_layout(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise PatchError("not a PE file")
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    optsz = struct.unpack_from("<H", data, pe + 20)[0]
    opt = pe + 24
    if struct.unpack_from("<H", data, opt)[0] != 0x20B:
        raise PatchError("not a PE32+ (x64) image")
    secs = []
    so = opt + optsz
    for i in range(nsec):
        name = data[so:so + 8].rstrip(b"\0").decode("ascii", "replace")
        vsz, va, rsz, rp = struct.unpack_from("<IIII", data, so + 8)
        secs.append({"name": name, "hdr": so, "va": va, "vsz": vsz, "rp": rp, "rsz": rsz})
        so += 40
    return {"opt": opt, "secs": secs}


def rva_to_off(layout, rva, size):
    for s in layout["secs"]:
        if s["va"] <= rva and rva + size <= s["va"] + s["rsz"]:
            return rva - s["va"] + s["rp"]
    raise PatchError("RVA 0x%X not inside raw section data" % rva)


def pe_checksum(data, checksum_off):
    total = 0
    n = len(data)
    padded = data + (b"\0" if n % 2 else b"")
    for i in range(0, len(padded), 2):
        if checksum_off <= i < checksum_off + 4:
            continue
        total += padded[i] | (padded[i + 1] << 8)
        total = (total & 0xFFFF) + (total >> 16)
    total = (total & 0xFFFF) + (total >> 16)
    return (total + n) & 0xFFFFFFFF


def state_of(data):
    """Return ('original'|'patched', layout) or raise PatchError on a mixed/unknown file."""
    lay = pe_layout(data)
    text = next((s for s in lay["secs"] if s["name"] == ".text"), None)
    if text is None:
        raise PatchError(".text section not found")
    states = set()
    if text["vsz"] == TEXT_VSIZE_ORIG:
        states.add("original")
    elif text["vsz"] == TEXT_VSIZE_NEW:
        states.add("patched")
    else:
        raise PatchError(".text VirtualSize 0x%X unexpected (different mod version?)" % text["vsz"])
    for name, rva, orig, new in PATCHES:
        assert len(orig) == len(new), name
        off = rva_to_off(lay, rva, len(orig))
        cur = data[off:off + len(orig)]
        if cur == orig:
            states.add("original")
        elif cur == new:
            states.add("patched")
        else:
            raise PatchError("site '%s' @RVA 0x%X has unexpected bytes %s" % (name, rva, cur.hex()))
    if len(states) != 1:
        raise PatchError("file is partially patched; restore the .orig backup first")
    return states.pop(), lay


def build_patched(data):
    state, lay = state_of(data)
    if state == "patched":
        return None
    out = bytearray(data)
    text = next(s for s in lay["secs"] if s["name"] == ".text")
    struct.pack_into("<I", out, text["hdr"] + 8, TEXT_VSIZE_NEW)
    for name, rva, orig, new in PATCHES:
        off = rva_to_off(lay, rva, len(orig))
        out[off:off + len(new)] = new
    ck_off = lay["opt"] + 64
    struct.pack_into("<I", out, ck_off, pe_checksum(bytes(out), ck_off))
    return bytes(out)


def kenshi_running():
    if os.name != "nt":
        return False
    try:
        r = subprocess.run(["tasklist", "/FI", "IMAGENAME eq kenshi_x64.exe"],
                           capture_output=True, text=True, timeout=15)
        return "kenshi_x64.exe" in r.stdout.lower()
    except Exception:
        return False


def sha256(b):
    return hashlib.sha256(b).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dll")
    ap.add_argument("--out", help="write the patched DLL here instead of patching in place (no backup)")
    ap.add_argument("--dry-run", action="store_true", help="only verify and report")
    a = ap.parse_args()

    data = open(a.dll, "rb").read()
    digest = sha256(data)
    print("input   %s\nsha256  %s" % (a.dll, digest))
    if digest != ORIGINAL_SHA256 and digest != PATCHED_SHA256:
        print("note: SHA-256 differs from the known build; relying on exact per-site byte checks")
    try:
        patched = build_patched(data)
    except PatchError as e:
        print("REFUSED: %s" % e)
        return 2
    if patched is None:
        print("already patched - nothing to do")
        return 0
    print("patched sha256 %s" % sha256(patched))
    if a.dry_run:
        print("dry run - nothing written")
        return 0
    if a.out:
        open(a.out, "wb").write(patched)
        print("wrote %s" % a.out)
        return 0
    if kenshi_running():
        print("REFUSED: kenshi_x64.exe is running; close the game first")
        return 3
    backup = a.dll + ".orig"
    if not os.path.exists(backup):
        shutil.copy2(a.dll, backup)
        print("backup  %s" % backup)
    else:
        print("backup exists, kept: %s" % backup)
    tmp = a.dll + ".tmp"
    open(tmp, "wb").write(patched)
    os.replace(tmp, a.dll)
    if sha256(open(a.dll, "rb").read()) != sha256(patched):
        print("ERROR: verification after write failed")
        return 4
    print("patched %s" % a.dll)
    return 0


if __name__ == "__main__":
    sys.exit(main())
