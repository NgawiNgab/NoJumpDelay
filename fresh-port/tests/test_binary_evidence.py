"""Optional evidence checks against the supplied files; never installs hooks."""
import hashlib
import importlib.util
import mmap
from pathlib import Path
import unittest

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('scan_signatures', root / 'tools/scan_signatures.py')
scan = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scan)
game = Path('/home/user/re_workspace/mc_libs/1.26.60.4/libminecraftpe.so')
defs = Path('/home/user/re_workspace/BedrockTools/src/core/memory/Signatures.cpp')
expected_hash = '4e7f97ecf1989edfaa7ca4805acc907a63bd4c56f1b1965079b2c47f63e3a7c4'


@unittest.skipUnless(game.is_file() and defs.is_file(), 'target ELF / reference signatures not present')
class BinaryEvidenceTests(unittest.TestCase):
    def test_candidates_against_recorded_binary(self):
        with game.open('rb') as f:
            h = hashlib.file_digest(f, 'sha256').hexdigest()
            self.assertEqual(h, expected_hash, 'wrong target binary')
            f.seek(0)
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
                segments = list(scan.executable_segments(data))
                patterns = scan.definitions(defs)
                expected = {'NormalTick': 0x0af5e460, 'ActorManagerList': 0x103b4754,
                            'GameModeAttackInternal': 0x1009cba4, 'LocalPlayerSwing': 0x0af60d78}
                for name, rva in expected.items():
                    with self.subTest(signature=name):
                        hits = []
                        for off, va, size in segments:
                            hits.extend(va + hit - off for hit in scan.matches(
                                data, scan.parse_pattern(patterns[name]), off, off + size))
                        self.assertEqual(hits, [rva])
                # The actor-list thunk's first two instructions are a load at
                # +0x478 and a branch; this alone is not a callable C++ ABI.
                self.assertEqual(data[0x10b16414:0x10b1641c], bytes.fromhex('00 3c 42 f9 cf 78 e2 17'))


if __name__ == '__main__':
    unittest.main()
