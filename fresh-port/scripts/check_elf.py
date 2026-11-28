#!/usr/bin/env python3
"""Check Android arm64 ELF load alignment and the required exported entrypoint."""
import argparse
import re
import subprocess
from pathlib import Path


def check(path: Path):
    header = subprocess.check_output(['readelf', '-hW', str(path)], text=True)
    if not re.search(r'Machine:\s+AArch64\b', header):
        raise ValueError('not an AArch64 ELF')
    program = subprocess.check_output(['readelf', '-lW', str(path)], text=True)
    loads = [line.split() for line in program.splitlines() if line.strip().startswith('LOAD ')]
    if not loads:
        raise ValueError('no PT_LOAD segments')
    for fields in loads:
        offset, address, alignment = int(fields[1], 16), int(fields[2], 16), int(fields[-1], 16)
        if alignment < 0x4000 or (address - offset) % 0x4000:
            raise ValueError('non-16-KiB-aligned PT_LOAD: ' + ' '.join(fields))
    symbols = subprocess.check_output(['readelf', '--dyn-syms', '-W', str(path)], text=True)
    if not any('PLGetModRegistration' in line and 'UND' not in line for line in symbols.splitlines()):
        raise ValueError('PLGetModRegistration missing or undefined')
    if any(re.search(r'\bJNI_OnLoad\b', line) for line in symbols.splitlines()):
        raise ValueError('JNI_OnLoad must not be present')
    if 'NativeMod7current' in symbols:
        raise ValueError('unexpected NativeMod::current dependency')
    if any(re.search(r'\bUND\b', line) and '__hash_memory' in line
           for line in symbols.splitlines()):
        raise ValueError('undefined __hash_memory symbol (incompatible libc++ runtime)')
    dynamic = subprocess.check_output(['readelf', '-dW', str(path)], text=True)
    if any('(NEEDED)' in line and re.search(r'\[libc\+\+_shared\.so\]', line)
           for line in dynamic.splitlines()):
        raise ValueError('direct dependency on libc++_shared.so')
    return program


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('library', type=Path)
    args = parser.parse_args()
    print(check(args.library))
    print('AArch64, 16 KiB PT_LOAD alignment, exports and C++ runtime dependency checks passed.')


if __name__ == '__main__':
    main()
