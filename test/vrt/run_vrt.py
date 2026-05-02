#!/usr/bin/env python3
"""run_vrt.py — drive the doomviz_vrt_runner across the test scenarios and
compare its output to committed baselines.

Bootstrap workflow:
1. First run on CI (no baselines committed yet): runner writes PPMs, this
   script reports "no baselines, skipping comparison" and exits 0. CI's
   upload-artifact step makes the run output downloadable.
2. Maintainer downloads the artifact, copies the PPMs into
   `test/vrt/baselines/<scenario>/`, commits.
3. Next CI run does byte-exact diff. Same toolchain → same bits → green.

Subsequent merges: byte-exact diff. Mismatches write a diff PPM and fail.

Layout:
  test/vrt/
    audio/<fixture>.wav         deterministic input(s)
    baselines/<scenario>/       committed baseline PPMs (LFS-tracked)
    output/<scenario>/          run output (gitignored)
    output/diffs/<scenario>/    per-frame visual diffs on failure
    output/vrt-results.xml      JUnit report
    output/vrt-report.html      contact-sheet for failing frames
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable
from xml.sax.saxutils import escape as xml_escape


# Each tuple is (scenario_name, scene_id, audio_fixture, optional config.yaml).
# The scenario name is what shows up in JUnit / HTML / baseline directory.
SCENARIOS: list[tuple[str, str, str, str | None]] = [
    ("killroom",  "killroom",  "test_dnb_20s.wav", "config/default_killroom.yaml"),
    ("analyzer",  "analyzer",  "test_dnb_20s.wav", None),
    ("spectrum2", "spectrum2", "test_dnb_20s.wav", None),
]


@dataclass
class ScenarioResult:
    name: str
    status: str            # "passed" | "failed" | "skipped" | "error"
    message: str = ""
    failed_frames: list[str] = field(default_factory=list)


def run_runner(runner: Path, scene: str, wav: Path, out_dir: Path,
               wad: Path, cfg: Path | None) -> tuple[int, str]:
    out_dir.mkdir(parents=True, exist_ok=True)
    cmd = [str(runner), scene, str(wav), str(out_dir), str(wad)]
    if cfg is not None:
        cmd.append(str(cfg))
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    except subprocess.TimeoutExpired:
        return 124, "runner timed out"
    return proc.returncode, (proc.stdout + proc.stderr).strip()


# Tolerance for "byte-exact-enough" diff. Captured from observation, not
# a guess. After a follow-up investigation, the per-frame noise is much
# smaller than we initially set: same-binary same-input runs produce at
# most 4 differing pixels per frame, *all on row y=83* — Doom's wall/sky
# horizon boundary in a 168-tall 3D view (`centery = viewheight/2 = 84`).
# The pixel at the very top of a wall column is a long-known fragility in
# linuxdoom's R_DrawColumn / R_FindPlane fixed-point boundary math; it can
# wobble by 1 row depending on accumulated visplane state across frames.
# Spectrum2 + Analyzer are bit-identical across runs.
#
# Tolerance set to 20 px / frame: above the observed worst case for
# local-vs-CI compares (frame_0016 was 12 px), tightly below what any
# real visual regression disturbs (a moved sprite shifts hundreds to
# thousands of pixels). Down from the initial guess of 50.
MAX_DIFFERING_PIXELS = 20


def diff_ppms(actual: Path, baseline: Path, diff_out: Path) -> tuple[bool, int]:
    """Tolerance-based diff. Returns (matched, num_differing_pixels).
    On mismatch, writes a per-pixel L1 diff PPM (visualization scaled ×8) to
    `diff_out`."""
    a = actual.read_bytes()
    b = baseline.read_bytes()
    if a == b:
        return True, 0

    def split(buf: bytes) -> tuple[bytes, bytes]:
        # Header: "P6\n<w> <h>\n255\n"
        nl = 0
        for _ in range(3):
            nl = buf.index(b"\n", nl) + 1
        return buf[:nl], buf[nl:]

    head_a, body_a = split(a)
    head_b, body_b = split(b)
    n = min(len(body_a), len(body_b))
    diff = bytearray(n)
    diff_pixels = 0
    for px in range(n // 3):
        i = px * 3
        if body_a[i:i+3] != body_b[i:i+3]:
            diff_pixels += 1
            for k in range(3):
                d = abs(body_a[i+k] - body_b[i+k])
                diff[i+k] = min(255, d * 8)

    matched = diff_pixels <= MAX_DIFFERING_PIXELS

    if not matched:
        diff_out.parent.mkdir(parents=True, exist_ok=True)
        diff_out.write_bytes(head_b + bytes(diff))

    return matched, diff_pixels


def compare_scenario(actual_dir: Path, baseline_dir: Path,
                     diff_dir: Path) -> tuple[int, int, list[str]]:
    """Compare every actual PPM to its baseline. Returns (matched, total, failed_names)."""
    actuals = sorted(actual_dir.glob("frame_*.ppm"))
    failed: list[str] = []
    matched = 0
    for actual in actuals:
        baseline = baseline_dir / actual.name
        if not baseline.exists():
            failed.append(actual.name + " (no baseline)")
            continue
        diff_path = diff_dir / actual.name
        ok, diff_px = diff_ppms(actual, baseline, diff_path)
        if ok:
            matched += 1
        else:
            failed.append(f"{actual.name} ({diff_px} px)")
    return matched, len(actuals), failed


def write_junit(results: list[ScenarioResult], out: Path) -> None:
    n_total = len(results)
    n_failed = sum(1 for r in results if r.status == "failed" or r.status == "error")
    n_skipped = sum(1 for r in results if r.status == "skipped")
    parts = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<testsuite name="vrt" tests="{n_total}" failures="{n_failed}" '
        f'skipped="{n_skipped}">',
    ]
    for r in results:
        msg = xml_escape(r.message)
        parts.append(f'  <testcase classname="vrt" name="{xml_escape(r.name)}">')
        if r.status == "skipped":
            parts.append(f'    <skipped message="{msg}"/>')
        elif r.status == "failed":
            parts.append(f'    <failure message="{msg}">{msg}</failure>')
        elif r.status == "error":
            parts.append(f'    <error message="{msg}">{msg}</error>')
        parts.append('  </testcase>')
    parts.append('</testsuite>')
    out.write_text("\n".join(parts))


def write_html_report(results: list[ScenarioResult], baseline_root: Path,
                      output_root: Path, diff_root: Path, out: Path) -> None:
    rows: list[str] = []
    for r in results:
        status_color = {"passed": "#1a4", "failed": "#c33",
                        "skipped": "#888", "error": "#c33"}.get(r.status, "#888")
        rows.append(
            f'<h2 style="color:{status_color}">{xml_escape(r.name)} — {r.status}</h2>'
            f'<p>{xml_escape(r.message)}</p>'
        )
        for frame_name in r.failed_frames:
            base = baseline_root / r.name / frame_name
            actual = output_root / r.name / frame_name
            diff = diff_root / r.name / frame_name
            rows.append(
                '<table><tr>'
                f'<td><div>baseline</div><img src="{base}"></td>'
                f'<td><div>actual</div><img src="{actual}"></td>'
                f'<td><div>diff (×8)</div><img src="{diff}"></td>'
                '</tr></table>'
            )
    out.write_text(
        "<!doctype html><html><head><meta charset='utf-8'>"
        "<style>body{font-family:monospace;background:#111;color:#ddd;}"
        "img{image-rendering:pixelated;width:320px;height:200px;}</style>"
        "<title>VRT report</title></head><body>"
        "<h1>DoomViz VRT report</h1>"
        + "".join(rows) +
        "</body></html>"
    )


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--runner", required=True, help="Path to doomviz_vrt_runner")
    ap.add_argument("--wad", required=True, help="Path to DOOM1.WAD")
    ap.add_argument("--audio-dir", required=True, help="Directory holding the WAV fixtures")
    ap.add_argument("--baseline-dir", required=True, help="Root of committed baselines")
    ap.add_argument("--output-dir", required=True, help="Run output root (gitignored)")
    ap.add_argument("--repo-root", required=True, help="Repo root (for resolving config paths)")
    args = ap.parse_args(argv)

    runner = Path(args.runner)
    wad = Path(args.wad)
    audio_root = Path(args.audio_dir)
    base_root = Path(args.baseline_dir)
    out_root = Path(args.output_dir)
    repo_root = Path(args.repo_root)

    if not runner.exists():
        print(f"Runner not built: {runner}", file=sys.stderr)
        return 2

    out_root.mkdir(parents=True, exist_ok=True)
    diff_root = out_root / "diffs"

    results: list[ScenarioResult] = []
    any_real_failure = False

    for name, scene_id, wav_name, cfg_rel in SCENARIOS:
        scenario_out = out_root / name
        scenario_baseline = base_root / name
        scenario_diff = diff_root / name
        wav_path = audio_root / wav_name
        cfg_path = (repo_root / cfg_rel) if cfg_rel else None

        rc, runner_log = run_runner(runner, scene_id, wav_path, scenario_out, wad, cfg_path)
        if rc != 0:
            results.append(ScenarioResult(name, "error",
                f"runner exited {rc}: {runner_log}"))
            any_real_failure = True
            continue

        # Comparison phase
        if not scenario_baseline.exists() or not any(scenario_baseline.glob("frame_*.ppm")):
            results.append(ScenarioResult(name, "skipped",
                f"no baselines under {scenario_baseline} — bootstrap pending"))
            continue

        matched, total, failed = compare_scenario(scenario_out, scenario_baseline,
                                                  scenario_diff)
        if total == 0:
            results.append(ScenarioResult(name, "error", "runner produced no frames"))
            any_real_failure = True
        elif failed:
            r = ScenarioResult(name, "failed",
                f"{matched}/{total} frames match; failed: {', '.join(failed[:6])}"
                f"{' …' if len(failed) > 6 else ''}",
                failed)
            results.append(r)
            any_real_failure = True
        else:
            results.append(ScenarioResult(name, "passed", f"{matched}/{total} frames match"))

    write_junit(results, out_root / "vrt-results.xml")
    write_html_report(results, base_root, out_root, diff_root,
                      out_root / "vrt-report.html")

    print()
    for r in results:
        print(f"  {r.status:8s} {r.name}: {r.message}")

    return 1 if any_real_failure else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
