#!/usr/bin/env python3
"""Build a demo MIDI file that drives pad 1 of the DoomViz default kit.

Generates `examples/e1m1_demo.mid` — a short single-channel pattern at
165 BPM (E1M1's tempo) that varispeed-pitches DSPISTOL across an
approximation of the E1M1 main riff. The user drags this single MIDI
clip onto one instrument track in their DAW, the track is set to
output MIDI on channel 1 (Bitwig's default), the channel-per-pad 1.0
routing fires pad 1, and the pistol crack becomes a chuggy lead.

Why one channel: Bitwig (and most DAWs) imports an MIDI file's events
onto a single track using the track's configured MIDI output channel,
discarding the original per-event channel. So a file authored across
channels 1-9 gets flattened to a single channel anyway. To make the
demo "just work" on a fresh DoomViz instance, we author for the most
common path: track on channel 1 → pad 1 (DSPISTOL).

The E1M1 riff approximation is in E (low-octave chugs with periodic
ascending pickups), eighth-note feel, 8 bars. With pad 1's DSPISTOL
varispeed-pitched it sounds like a malfunctioning pistol-bass.

Stdlib only — struct, pathlib, argparse.

Usage:
    python3 scripts/build_e1m1_demo_midi.py --output examples/e1m1_demo.mid
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


# --- Tiny MIDI writer (format-0, single track) -----------------------

def vlq(n: int) -> bytes:
    """Variable-length quantity for delta times."""
    if n < 0:
        raise ValueError(f"VLQ requires non-negative int, got {n}")
    if n == 0:
        return b"\x00"
    out = []
    while n > 0:
        out.append(n & 0x7F)
        n >>= 7
    out.reverse()
    return bytes((b | 0x80) for b in out[:-1]) + bytes([out[-1]])


def event_note(delta: int, status: int, channel: int, note: int, vel: int) -> bytes:
    """status = 0x90 (NoteOn) or 0x80 (NoteOff). channel 0..15 (so MIDI ch 1 = 0)."""
    return vlq(delta) + bytes([status | (channel & 0x0F), note & 0x7F, vel & 0x7F])


def event_tempo(delta: int, bpm: float) -> bytes:
    us_per_quarter = int(round(60_000_000 / bpm))
    payload = struct.pack(">I", us_per_quarter)[1:]
    return vlq(delta) + b"\xff\x51" + bytes([3]) + payload


def event_track_name(delta: int, name: str) -> bytes:
    raw = name.encode("utf-8")
    return vlq(delta) + b"\xff\x03" + vlq(len(raw)) + raw


def event_end_of_track(delta: int) -> bytes:
    return vlq(delta) + b"\xff\x2f\x00"


def build_track(events: bytes) -> bytes:
    return b"MTrk" + struct.pack(">I", len(events)) + events


def build_file(track: bytes, division: int) -> bytes:
    header = (
        b"MThd"
        + struct.pack(">I", 6)
        + struct.pack(">H", 0)         # format 0 = single track, single channel
        + struct.pack(">H", 1)         # ntracks
        + struct.pack(">H", division)
    )
    return header + track


# --- Pattern ---------------------------------------------------------

PPQN = 480
BEATS_PER_BAR = 4
BARS = 8
CHANNEL = 0   # MIDI ch 1 (zero-indexed → pad 1 in the channel-per-pad routing)

# E1M1-flavoured riff. 16 eighth-notes per bar would be too dense for
# DSPISTOL's transient; we use 8 quarter-note chugs per bar with a
# couple of off-beat pickups for shape.
#
# Notes (MIDI numbers, in low E-minor territory):
#     E2 = 40, F#2 = 42, G2 = 43, A2 = 45, B2 = 47, E3 = 52
#
# Each entry is (offset_in_8ths, midi_note, velocity). An 8th note is
# PPQN // 2 ticks; a bar is 16 8ths.

# Eight-bar phrase. Bars 0-3 are one half-phrase, bars 4-7 vary it.
RIFF: list[tuple[int, int, int]] = [
    # Bar 0: chug on E with a G accent.
    (0,  40, 110), (2,  40, 100), (4,  40, 105), (6,  43, 110),
    (8,  40, 100), (10, 40, 100), (12, 40, 105), (14, 43, 110),
    # Bar 1: walk up through G to A.
    (16, 40, 110), (18, 40, 100), (20, 43, 105), (22, 45, 110),
    (24, 40, 100), (26, 40, 100), (28, 43, 105), (30, 47, 110),
    # Bar 2: chug + E3 octave answer.
    (32, 40, 110), (34, 40, 100), (36, 40, 105), (38, 52, 115),
    (40, 40, 100), (42, 40, 100), (44, 40, 105), (46, 52, 115),
    # Bar 3: tension build to bar-4 downbeat.
    (48, 40, 105), (50, 42, 105), (52, 43, 110), (54, 45, 110),
    (56, 47, 115), (58, 47, 110), (60, 47, 105), (62, 47, 105),
    # Bar 4: same chug as bar 0.
    (64, 40, 110), (66, 40, 100), (68, 40, 105), (70, 43, 110),
    (72, 40, 100), (74, 40, 100), (76, 40, 105), (78, 43, 110),
    # Bar 5: G then A.
    (80, 40, 110), (82, 40, 100), (84, 43, 105), (86, 45, 110),
    (88, 40, 100), (90, 40, 100), (92, 43, 105), (94, 47, 110),
    # Bar 6: octave hits + sustain.
    (96,  40, 110), (98,  52, 115), (100, 40, 100), (102, 52, 115),
    (104, 40, 105), (106, 52, 110), (108, 47, 110), (110, 45, 110),
    # Bar 7: tag — descend back to E.
    (112, 47, 115), (114, 45, 110), (116, 43, 105), (118, 42, 100),
    (120, 40, 100), (122, 40,  95), (124, 40,  90), (126, 40,  85),
]

EIGHTH = PPQN // 2
HIT_LEN = int(EIGHTH * 0.85)  # slight gap so hits read as articulated


def build_events() -> bytes:
    events: list[tuple[int, int, int, int, int]] = []  # (abs_t, prio, status, note, vel)

    for off_eighths, note, vel in RIFF:
        t = off_eighths * EIGHTH
        events.append((t,           0, 0x90, note, vel))
        events.append((t + HIT_LEN, 1, 0x80, note, 0))

    events.sort(key=lambda e: (e[0], e[1]))

    stream = bytearray()
    stream += event_track_name(0, "DoomViz E1M1 demo (drives pad 1)")
    stream += event_tempo(0, 165.0)

    last_t = 0
    for abs_t, _, status, note, vel in events:
        delta = abs_t - last_t
        last_t = abs_t
        stream += event_note(delta, status, CHANNEL, note, vel)

    final_t = (BARS * BEATS_PER_BAR + 1) * PPQN
    stream += event_end_of_track(max(0, final_t - last_t))
    return bytes(stream)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--output", required=True,
                    help="Path to the output .mid file")
    args = ap.parse_args()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    midi_bytes = build_file(build_track(build_events()), division=PPQN)
    output.write_bytes(midi_bytes)

    print(f"Wrote {output} ({len(midi_bytes)} bytes, "
          f"{BARS} bars at 165 BPM, single channel → pad 1)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
