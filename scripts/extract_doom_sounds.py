#!/usr/bin/env python3
"""Extract DS* sound effects from a Doom WAD into standard WAV files.

Reads a Doom WAD's lump directory, finds every lump named DS*, parses each
as a DMX sound (the format Doom shipped: 8-byte header + 16 bytes of
padding + 8-bit unsigned PCM samples), and writes them out as 16-bit
signed PCM mono WAV files.

The bundled DOOM1.WAD shareware ships ~60 of these (player sfx, weapon
sounds, monster grunts, doors, lifts, item pickups). They become the
default sample bank for the BFG-SP404 sample player.

Native sample rate is preserved per lump — the C++ playback path
resamples at runtime, so resampling here would just throw away accuracy.

Usage:
    python3 scripts/extract_doom_sounds.py \\
        --wad resources/DOOM1.WAD \\
        --output-dir resources/sounds

Stdlib only — struct, wave, pathlib, argparse.
"""

from __future__ import annotations

import argparse
import struct
import sys
import wave
from dataclasses import dataclass
from pathlib import Path


@dataclass
class LumpEntry:
    """One row of the WAD's lump directory."""
    name: str
    filepos: int
    size: int


def read_wad_directory(wad: Path) -> list[LumpEntry]:
    """Parse a WAD's 12-byte header + 16-byte-per-entry directory."""
    data = wad.read_bytes()
    magic, numlumps, infotableofs = struct.unpack_from("<4sII", data, 0)
    if magic not in (b"IWAD", b"PWAD"):
        raise ValueError(f"Not a WAD (got magic {magic!r})")

    entries: list[LumpEntry] = []
    for i in range(numlumps):
        off = infotableofs + i * 16
        filepos, size, raw_name = struct.unpack_from("<II8s", data, off)
        # Lump names are zero-padded; trim and decode as ASCII.
        name = raw_name.split(b"\x00", 1)[0].decode("ascii", errors="replace")
        entries.append(LumpEntry(name=name, filepos=filepos, size=size))
    return entries


def parse_dmx_lump(blob: bytes) -> tuple[int, bytes] | None:
    """Decode a Doom DMX sound lump.

    Format (from the Doom Wiki / linuxdoom source):
        bytes 0..1   uint16 LE format id (always 0x0003)
        bytes 2..3   uint16 LE sample rate
        bytes 4..7   uint32 LE sample count (this number includes the 32
                     bytes of padding around the actual audio: 16 bytes
                     of leading + 16 bytes of trailing padding... or
                     8 + 8 depending on the writer; both are seen in the
                     wild)
        bytes 8..15  8 bytes of leading padding (skip)
        bytes 16..   raw 8-bit unsigned PCM mono audio
        last 8 bytes 8 bytes of trailing padding

    The audio body length is `sample_count - 32` per the canonical
    interpretation, but some lumps have trailing-pad mismatches. If
    `sample_count` reaches past the end of the lump, we fall back to
    using `len(blob) - 24` (header + leading pad + trailing pad).

    Returns (sample_rate_hz, audio_bytes) or None if the lump isn't a
    DMX sound (wrong format id, too short, etc).
    """
    if len(blob) < 24:
        return None

    fmt_id, sample_rate, sample_count = struct.unpack_from("<HHI", blob, 0)
    if fmt_id != 0x0003:
        return None

    # Canonical body length per the spec.
    body_len = sample_count - 32
    body_start = 8 + 8                 # header + leading pad
    body_end = body_start + body_len

    # Some lumps in the wild have a sample_count that overshoots the
    # lump bytes available. Fall back to whatever's actually present
    # between the leading + trailing 8-byte pads.
    if body_end > len(blob) - 8:
        body_end = len(blob) - 8

    if body_end <= body_start:
        return None

    return sample_rate, blob[body_start:body_end]


def write_wav(path: Path, sample_rate: int, u8_pcm: bytes) -> int:
    """Convert 8-bit unsigned mono PCM to 16-bit signed and write a WAV.

    `(s - 128) * 256` lifts u8's 0..255 (centred at 128) into s16's
    -32768..32767 (centred at 0). 256× scale uses the full s16 range
    without introducing any non-zero quantisation noise that wasn't
    already in the source.

    Returns bytes written.
    """
    s16 = bytearray(len(u8_pcm) * 2)
    for i, b in enumerate(u8_pcm):
        v = (b - 128) * 256
        s16[i * 2]     = v & 0xFF
        s16[i * 2 + 1] = (v >> 8) & 0xFF

    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)            # 16-bit
        w.setframerate(sample_rate)
        w.writeframes(bytes(s16))

    return path.stat().st_size


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--wad", type=Path, default=Path("resources/DOOM1.WAD"),
                   help="Path to the Doom WAD to extract from.")
    p.add_argument("--output-dir", type=Path, default=Path("resources/sounds"),
                   help="Directory to write the WAV files into.")
    args = p.parse_args(argv)

    if not args.wad.is_file():
        sys.stderr.write(f"WAD not found: {args.wad}\n")
        return 1

    entries = read_wad_directory(args.wad)
    ds_lumps = [e for e in entries if e.name.startswith("DS") and e.size > 0]
    if not ds_lumps:
        sys.stderr.write(f"No DS* lumps in {args.wad} — wrong file?\n")
        return 1

    args.output_dir.mkdir(parents=True, exist_ok=True)

    blob = args.wad.read_bytes()
    written = 0
    skipped: list[str] = []
    total_bytes = 0

    for lump in ds_lumps:
        data = blob[lump.filepos : lump.filepos + lump.size]
        parsed = parse_dmx_lump(data)
        if parsed is None:
            skipped.append(f"{lump.name} (size={lump.size}, not DMX)")
            continue
        sample_rate, audio = parsed
        out = args.output_dir / f"{lump.name.lower()}.wav"
        total_bytes += write_wav(out, sample_rate, audio)
        written += 1

    print(f"Found  {len(ds_lumps)} DS* lumps in {args.wad}")
    print(f"Wrote  {written} WAVs to {args.output_dir}")
    print(f"Total  {total_bytes / 1024:.1f} KiB")
    if skipped:
        print("Skipped:")
        for s in skipped:
            print(f"  - {s}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
