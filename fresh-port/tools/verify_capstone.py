#!/usr/bin/env python3
"""Read-only Capstone evidence for the supplied AArch64 ELFs.

A disassembly is not a semantic ABI proof. Struct offsets in Sigma code are
reported separately from target code, with base registers left intact.
"""
import argparse
import hashlib
from pathlib import Path
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
from capstone.arm64_const import ARM64_OP_MEM
from elftools.elf.elffile import ELFFile

TARGETS = {
    'tick': 0x0af5e460, 'rotation detour target': 0x10383480,
    'level vector thunk': 0x10b16414, 'actor manager vector': 0x103b4754,
    'attack internal': 0x1009cba4, 'swing': 0x0af60d78,
    'component': 0x0fdabbfc, 'name candidate': 0x0fd881d4,
    'classifier A': 0x0fd8e120, 'classifier B': 0x101c2d7c,
    'native helper A': 0x09b6bbd4, 'native helper B': 0x09b94c98,
}
OFFSETS = (0x1f0, 0xa08, 0xca8, 0x478, 0x228, 0x230, 0x238, 0x1b4, 0xb28, 0x7c)


def read_vaddr(elf, vaddr, size):
    for segment in elf.iter_segments():
        if segment['p_type'] != 'PT_LOAD':
            continue
        delta = vaddr - segment['p_vaddr']
        if 0 <= delta and delta + size <= segment['p_filesz']:
            elf.stream.seek(segment['p_offset'] + delta)
            return elf.stream.read(size)
    raise ValueError(f'RVA {vaddr:#x}+{size:#x} has no file-backed PT_LOAD')


def open_elf(path):
    f = open(path, 'rb')
    elf = ELFFile(f)
    if elf.elfclass != 64 or not elf.little_endian or elf['e_machine'] != 'EM_AARCH64':
        f.close()
        raise ValueError(f'{path}: expected little-endian AArch64 ELF64')
    return f, elf


def sha(path):
    with open(path, 'rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game', type=Path, default=Path('/home/user/re_workspace/mc_libs/1.26.60.4/libminecraftpe.so'))
    p.add_argument('--sigma', type=Path, default=Path('/home/user/re_workspace/targets/libReverseSigma.so'))
    args = p.parse_args()
    print('Game SHA-256:', sha(args.game))
    print('Sigma SHA-256:', sha(args.sigma))
    md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
    md.detail = True
    game_file, game = open_elf(args.game)
    sigma_file, sigma = open_elf(args.sigma)
    try:
        print('\n=== Target game disassembly (first 64 bytes per RVA) ===')
        for name, rva in TARGETS.items():
            data = read_vaddr(game, rva, 64)
            print(f'\n{name} RVA {rva:#x} bytes16={data[:16].hex(" ")}')
            for ins in md.disasm(data, rva):
                print(f'  {ins.address:09x}  {ins.mnemonic:8s} {ins.op_str}')
        print('\n=== Sigma field-displacement samples in tick/rotation detours ===')
        print('These are encoded displacements; register provenance and target-layout identity require separate proof.')
        samples = {n: [] for n in OFFSETS}
        for label, begin, size in [('rotation', 0xab7d4, 0x35c), ('combat', 0xabb30, 0x2cb0)]:
            for ins in md.disasm(read_vaddr(sigma, begin, size), begin):
                for op in ins.operands:
                    if op.type == ARM64_OP_MEM and op.mem.disp in samples and len(samples[op.mem.disp]) < 4:
                        samples[op.mem.disp].append(f'{label} {ins.address:#x}: {ins.mnemonic} {ins.op_str}')
        for off, hits in samples.items():
            print(f'{off:#x}:')
            for hit in hits:
                print('  ' + hit)
            if not hits:
                print('  no direct memory displacement in scanned ranges (may use add/address calculation)')
    finally:
        game_file.close()
        sigma_file.close()


if __name__ == '__main__':
    main()
