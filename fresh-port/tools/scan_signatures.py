#!/usr/bin/env python3
"""Scan AArch64 ELF executable PT_LOAD segments for IDA-style byte patterns.

Patterns are candidates, not proof of a function's ABI or semantics. The script
prints every occurrence (including ambiguous patterns); it never installs hooks.
"""
import argparse
import mmap
import re
import struct
from pathlib import Path


def parse_pattern(pattern):
    result = []
    for token in pattern.split():
        if token in ('?', '??'):
            result.append(None)
        elif re.fullmatch(r'[0-9a-fA-F]{2}', token):
            result.append(int(token, 16))
        else:
            raise ValueError(f'invalid pattern token: {token}')
    if not result or all(b is None for b in result):
        raise ValueError('pattern must contain at least one fixed byte')
    return result


def matches(data, pattern, start=0, end=None):
    """Yield overlapping matches wholly inside [start, end)."""
    end = len(data) if end is None else end
    if start < 0 or end > len(data) or start > end:
        raise ValueError('invalid search range')
    chunks = []
    i = 0
    while i < len(pattern):
        if pattern[i] is None:
            i += 1
            continue
        j = i
        while j < len(pattern) and pattern[j] is not None:
            j += 1
        chunks.append((i, bytes(pattern[i:j])))
        i = j
    if not chunks:
        raise ValueError('pattern must contain a fixed byte')
    offset, anchor = max(chunks, key=lambda c: len(c[1]))
    cursor = start + offset
    last = end - len(pattern)
    while cursor <= last + offset:
        pos = data.find(anchor, cursor, end)
        if pos < 0:
            break
        candidate = pos - offset
        if candidate <= last and all(
            byte is None or data[candidate + n] == byte
            for n, byte in enumerate(pattern)
        ):
            yield candidate
        cursor = pos + 1


def executable_segments(data):
    if len(data) < 64 or data[:6] != b'\x7fELF\x02\x01':
        raise ValueError('expected little-endian ELF64')
    if struct.unpack_from('<H', data, 18)[0] != 183:
        raise ValueError('expected AArch64 ELF')
    phoff = struct.unpack_from('<Q', data, 32)[0]
    phentsize, phnum = struct.unpack_from('<HH', data, 54)
    if phentsize < 56 or phoff + phentsize * phnum > len(data):
        raise ValueError('invalid ELF program headers')
    for i in range(phnum):
        p_type, flags, offset, vaddr, _, filesz, _, _ = struct.unpack_from(
            '<IIQQQQQQ', data, phoff + phentsize * i)
        if p_type == 1 and flags & 1:
            if offset + filesz > len(data):
                raise ValueError('segment extends past end of file')
            yield offset, vaddr, filesz


def definitions(path):
    text = Path(path).read_text(encoding='utf-8')
    return dict(re.findall(r'SignatureDefinition\{SignatureId::(\w+),\s*"([?0-9A-Fa-f\s]+)"\}', text))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('elf', type=Path)
    ap.add_argument('definitions', type=Path, help='BedrockTools Signatures.cpp')
    ap.add_argument('names', nargs='+', help='SignatureId names')
    args = ap.parse_args()
    patterns = definitions(args.definitions)
    missing = set(args.names) - patterns.keys()
    if missing:
        ap.error('unknown signatures: ' + ', '.join(sorted(missing)))
    with args.elf.open('rb') as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
        segments = list(executable_segments(data))
        for name in args.names:
            pattern = parse_pattern(patterns[name])
            hits = []
            for off, va, size in segments:
                hits.extend(va + x - off for x in matches(data, pattern, off, off + size))
            print(f'{name}: {len(hits)} hit(s)' + ''.join(f' 0x{x:x}' for x in hits))


if __name__ == '__main__':
    main()
