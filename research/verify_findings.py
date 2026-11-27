#!/usr/bin/env python3
"""Offline regression checks for the researched ELF bytes, not a mod/ABI test.

These checks require the two supplied, unmodified binaries. They do not establish
that a hook is safe at runtime or that the port is behaviorally equivalent.
"""
import hashlib
import mmap
import os
from pathlib import Path
import re
import struct
import unittest

APOLLON = Path(os.environ.get("APOLLON_ELF", "/home/user/research_resources/libReverseSigma.so"))
GAME = Path(os.environ.get("GAME_ELF", "/home/user/binaries/libminecraftpe.so"))


def read(path, offset, size):
    with path.open("rb") as f:
        f.seek(offset)
        data = f.read(size)
    if len(data) != size:
        raise ValueError(f"Short read at {offset:#x} from {path}")
    return data


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)
    return h.hexdigest()


class VerifiedBytes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for path in (APOLLON, GAME):
            if not path.is_file():
                raise FileNotFoundError(f"Input binary not available: {path}")

    def test_input_hashes(self):
        self.assertEqual(sha256(APOLLON), "c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f")
        self.assertEqual(sha256(GAME), "4e7f97ecf1989edfaa7ca4805acc907a63bd4c56f1b1965079b2c47f63e3a7c4")

    def test_apollon_config_string_and_encrypted_ui_label(self):
        self.assertEqual(read(APOLLON, 0x3f931, 12), b"noJumpDelay\0")
        cipher = read(APOLLON, 0x38b10, 8) + read(APOLLON, 0x393a8, 4) + bytes.fromhex("aea5")
        key = read(APOLLON, 0x38230, 8) + read(APOLLON, 0x38bf0, 8)[::2] + bytes.fromhex("d7a5")
        self.assertEqual(bytes(a ^ b for a, b in zip(cipher, key)), b"No Jump Delay\0")
        self.assertEqual(read(APOLLON, 0xa4568, 28).hex(), "e0c305910d5101942f510194811500f021903b9150850294d1d20294")

    def test_apollon_conditional_component_write(self):
        self.assertEqual(read(APOLLON, 0xae2c8, 40).hex(), "481500d00841433968020037481500b008917b391f05007181000054e0031caae1031f2aab0c0094")
        self.assertEqual(read(APOLLON, 0xb1650, 28).hex(), "286968f808fdffb4491940928a0280522821aa9b011100b9c0035fd6")

    def test_game_component_type_and_accessors(self):
        self.assertEqual(read(GAME, 0x24cc02d, 17), b"minecraft:health\0")
        self.assertEqual(read(GAME, 0x1018e97c, 40).hex(), "e119f9d021b40091e00300914b7fcd94400202d000203d91e1030091e2031f2a2300805248680094")
        self.assertEqual(read(GAME, 0x101a9ad0, 8).hex(), "007c40bdc0035fd6")
        self.assertEqual(read(GAME, 0x25a32ba, 18), b"MobJumpComponent]\0")
        self.assertEqual(read(GAME, 0xc0e89e8, 28).hex(), "09ef915289a5b87208240029c825fbf008e90a910902805208a400a9")
        self.assertEqual(read(GAME, 0x10192090, 20).hex(), "8a028052291940922821aa9b001140b9fd7bc1a8")
        self.assertEqual(read(GAME, 0x10192238, 24).hex(), "494540928a028052291940922821aa9b011100b9fd7bc1a8")
        self.assertEqual(read(GAME, 0xf4290cc, 48).hex(), "e00313aac2a33594a0000034e0031f2af44f41a9fd7bc2a8c0035fd6e00314aa59741994e00313aa4101805223a43594")

    def test_local_player_vtable_relocation_and_prologue(self):
        # BedrockTools NormalTick signature, commit 56d974c3c178c66d6f2494693cb1d121f2aef032.
        # Uniqueness supports the identification but does not prove an ABI.
        words = ("? ? ? FC ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 "
                 "? ? ? A9 ? ? ? 91 ? ? ? D1 54 D0 3B D5 F3 03 00 AA "
                 "? ? ? F9 ? ? ? F8 ? ? ? 39").split()
        pattern = re.compile(b"".join(b"." if w == "?" else re.escape(bytes([int(w, 16)])) for w in words), re.DOTALL)
        with GAME.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
            matches = [match.start() for match in pattern.finditer(data, 0x6834bc0, 0x135d3388)]
        self.assertEqual(matches, [0xaf5e460])
        # .eh_frame_hdr uses data-relative signed 32-bit initial-location entries.
        hdr = 0x40f7c60
        self.assertEqual(read(GAME, hdr, 4), bytes.fromhex("011b033b"))
        count = struct.unpack("<I", read(GAME, hdr + 8, 4))[0]
        lo, hi = 0, count
        with GAME.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
            while lo < hi:
                mid = (lo + hi) // 2
                addr = hdr + struct.unpack_from("<i", data, hdr + 12 + 8 * mid)[0]
                if addr < 0xaf5e460:
                    lo = mid + 1
                else:
                    hi = mid
            starts = [hdr + struct.unpack_from("<i", data, hdr + 12 + 8 * n)[0] for n in (lo, lo + 1)]
        self.assertEqual(starts, [0xaf5e460, 0xaf5f580])
        self.assertEqual(read(GAME, 0xaf5e460, 32).hex(), "e80f19fcfd7b01a9fc6f02a9fa6703a9f85f04a9f65705a9f44f06a9fd430091")
        # ELF .rela.dyn entry, R_AARCH64_RELATIVE -> virtual entry at RVA 0x139f6f68.
        target, info, addend = struct.unpack("<QQq", read(GAME, 0x11c4168, 24))
        self.assertEqual((target, info, addend), (0x139f6f68, 0x403, 0xaf5e460))
        self.assertEqual(read(GAME, 0x2e3bbdb, 14), b"11LocalPlayer\0")


if __name__ == "__main__":
    unittest.main(verbosity=2)
