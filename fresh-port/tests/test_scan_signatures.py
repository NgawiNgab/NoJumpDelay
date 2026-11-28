import importlib.util
from pathlib import Path
import struct
import unittest

path = Path(__file__).resolve().parents[1] / 'tools' / 'scan_signatures.py'
spec = importlib.util.spec_from_file_location('scan_signatures', path)
scan = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scan)


class ScanTests(unittest.TestCase):
    def test_wildcards_and_overlap(self):
        p = scan.parse_pattern('41 ? 41')
        self.assertEqual(list(scan.matches(b'ABABA', p)), [0, 2])
        self.assertEqual(list(scan.matches(b'ABABA', p, 1, 4)), [])

    def test_boundaries_and_anchor(self):
        self.assertEqual(list(scan.matches(b'XXABCDXXABCD', scan.parse_pattern('? 42 43 ?'), 2, 6)), [2])
        self.assertEqual(list(scan.matches(b'ABCD', scan.parse_pattern('41 42 43 44 45'))), [])

    def test_bad_patterns(self):
        for pattern in ('', '? ??', 'abc', 'GG'):
            with self.assertRaises(ValueError):
                scan.parse_pattern(pattern)

    def test_elf_segment_and_bounds(self):
        data = bytearray(200)
        data[:6] = b'\x7fELF\x02\x01'
        struct.pack_into('<H', data, 18, 183)
        struct.pack_into('<Q', data, 32, 64)
        struct.pack_into('<HH', data, 54, 56, 1)
        struct.pack_into('<IIQQQQQQ', data, 64, 1, 5, 120, 0x2000, 0, 80, 80, 0x4000)
        self.assertEqual(list(scan.executable_segments(data)), [(120, 0x2000, 80)])
        struct.pack_into('<Q', data, 64 + 32, 81)
        with self.assertRaises(ValueError):
            list(scan.executable_segments(data))


if __name__ == '__main__':
    unittest.main()
