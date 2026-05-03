#!/usr/bin/env python3
"""Build the bundled DoomViz default sample set (.dvset) from extracted
shareware Doom sounds.

The default kit is a 9-pad selection picked for percussive variety:
    1. dspistol  — Pistol (snare-like crack)
    2. dsbarexp  — Barrel / rocket explosion (kick / boom)
    3. dsdoropn  — Door open creak
    4. dsdorcls  — Door slam
    5. dsoof     — Player oof (vocal punch)
    6. dsitemup  — Item pickup chime
    7. dsclaw    — Imp claw slap
    8. dsfirxpl  — Fireball explode
    9. dspldeth  — Player death groan

The .dvset format is XML with base64-encoded 16-bit signed PCM at the
WAV's native sample rate. The plugin resamples to host SR on load.

This script lives next to extract_doom_sounds.py and runs once during
build / repo bootstrap. Re-run when the kit selection changes.

Usage:
    python3 scripts/build_default_sample_set.py \\
        --sounds-dir resources/sounds \\
        --output resources/sets/default.dvset

Stdlib only — wave, base64, pathlib, struct.
"""

from __future__ import annotations

import argparse
import base64
import struct
import sys
import wave
from pathlib import Path
from xml.sax.saxutils import escape

# (filename, display name) — index = pad index (0-based).
KIT = [
    ("dspistol.wav", "DSPISTOL.WAV"),
    ("dsbarexp.wav", "DSBAREXP.WAV"),
    ("dsdoropn.wav", "DSDOROPN.WAV"),
    ("dsdorcls.wav", "DSDORCLS.WAV"),
    ("dsoof.wav",    "DSOOF.WAV"),
    ("dsitemup.wav", "DSITEMUP.WAV"),
    ("dsclaw.wav",   "DSCLAW.WAV"),
    ("dsfirxpl.wav", "DSFIRXPL.WAV"),
    ("dspldeth.wav", "DSPLDETH.WAV"),
]


def read_wav_as_int16(path: Path) -> tuple[int, bytes]:
    """Return (sample_rate, raw_int16_le_bytes) for a mono WAV file.

    The extract_doom_sounds.py output is mono 16-bit PCM, so this is a
    direct read. If the file is multi-channel, raise — we don't expect
    that for the bundled Doom sounds.
    """
    with wave.open(str(path), "rb") as w:
        n_channels = w.getnchannels()
        sample_width = w.getsampwidth()
        sample_rate = w.getframerate()
        n_frames = w.getnframes()

        if n_channels != 1:
            raise SystemExit(
                f"{path}: expected mono, got {n_channels} channels")
        if sample_width != 2:
            raise SystemExit(
                f"{path}: expected 16-bit PCM, got {sample_width * 8}-bit")

        raw = w.readframes(n_frames)
        return sample_rate, raw


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sounds-dir", required=True,
                    help="Directory containing the extracted ds*.wav files")
    ap.add_argument("--output", required=True,
                    help="Path to the output .dvset file")
    args = ap.parse_args()

    sounds_dir = Path(args.sounds_dir)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    if len(KIT) != 9:
        raise SystemExit(f"KIT must have exactly 9 entries, got {len(KIT)}")

    pads_xml = []
    for idx, (filename, display) in enumerate(KIT):
        wav_path = sounds_dir / filename
        if not wav_path.exists():
            raise SystemExit(f"Missing required WAV: {wav_path}")

        sr, raw = read_wav_as_int16(wav_path)
        n_samples = len(raw) // 2
        encoded = base64.b64encode(raw).decode("ascii")

        # XML layout matches VisualizerState's <Pad> element so the
        # decode path is shared (decodeSamplePCM → vector<float>).
        pads_xml.append(
            f'  <Pad index="{idx}"\n'
            f'       name="{escape(display)}"\n'
            f'       sourceSampleRate="{float(sr)}"\n'
            f'       startSample="0"\n'
            f'       endSample="{n_samples}">\n'
            f'    <SamplePCM>{encoded}</SamplePCM>\n'
            f'  </Pad>'
        )

    xml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<DoomVizSampleSet version="1" name="Default Doom Kit">\n'
        + "\n".join(pads_xml) +
        "\n</DoomVizSampleSet>\n"
    )

    output.write_text(xml, encoding="utf-8")
    size_kb = len(xml.encode("utf-8")) / 1024
    print(f"Wrote {output} ({size_kb:.1f} KB, 9 pads)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
