import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

MODULE = Path(__file__).resolve().parents[1] / 'scripts' / 'check_elf.py'
spec = importlib.util.spec_from_file_location('check_elf', MODULE)
check_elf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_elf)

HEADER = '  Machine:                           AArch64\n'
PROGRAM = '  LOAD 0x000000 0x000000 0x000000 0x000400 0x000400 R E 0x4000\n'
SYMBOLS = '   1: 0000000000010000 12 FUNC GLOBAL DEFAULT 12 PLGetModRegistration\n'
DYNAMIC = ' 0x0000000000000001 (NEEDED) Shared library: [libpreloader.so]\n'


def run_check(symbols=SYMBOLS, dynamic=DYNAMIC):
    def readelf(args, text):
        assert text
        return {'-hW': HEADER, '-lW': PROGRAM, '--dyn-syms': symbols,
                '-dW': dynamic}[args[1]]
    with patch.object(check_elf.subprocess, 'check_output', side_effect=readelf):
        return check_elf.check(Path('fake.so'))


class ElfChecks(unittest.TestCase):
    def test_clean_runtime(self):
        self.assertEqual(run_check(), PROGRAM)

    def test_rejects_undefined_hash_memory(self):
        line = '  2: 0000000000000000 0 FUNC GLOBAL DEFAULT UND _ZNSt6__ndk113__hash_memoryEPKvm\n'
        with self.assertRaisesRegex(ValueError, 'undefined __hash_memory'):
            run_check(symbols=SYMBOLS + line)

    def test_rejects_versioned_undefined_hash_memory(self):
        line = '  2: 0000000000000000 0 FUNC GLOBAL DEFAULT UND foo__hash_memory@LIBCXX\n'
        with self.assertRaisesRegex(ValueError, 'undefined __hash_memory'):
            run_check(symbols=SYMBOLS + line)

    def test_defined_hash_memory_is_not_an_undefined_dependency(self):
        self.assertEqual(run_check(symbols=SYMBOLS + '  2: 0000000000020000 8 FUNC GLOBAL DEFAULT 12 __hash_memory\n'), PROGRAM)

    def test_rejects_shared_libcxx_needed(self):
        line = ' 0x0000000000000001 (NEEDED) Shared library: [libc++_shared.so]\n'
        with self.assertRaisesRegex(ValueError, 'libc\\+\\+_shared'):
            run_check(dynamic=DYNAMIC + line)

    def test_does_not_reject_unrelated_dynamic_tag(self):
        self.assertEqual(run_check(dynamic=DYNAMIC + ' (SONAME) Library soname: [libc++_shared.so]\n'), PROGRAM)


if __name__ == '__main__':
    unittest.main()
