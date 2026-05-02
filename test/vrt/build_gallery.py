#!/usr/bin/env python3
"""Build a static VRT baseline gallery for GitHub Pages.

Walks `test/vrt/baselines/{scene}/frame_*.ppm`, converts each PPM to a PNG
under the output directory, and writes an `index.html` that lays the
thumbnails out per scene with the same Doom-HUD palette the docs/ page
uses.

Inputs are LFS-backed PPMs in the repo. Outputs are not committed —
GitHub Actions builds them on every push to main and ships them into the
Pages artifact.

Requires Pillow.

Usage:
    python3 build_gallery.py \\
        --baseline-dir test/vrt/baselines \\
        --output-dir docs/vrt
"""

import argparse
import html
import os
import subprocess
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.stderr.write("Pillow is required. Install with: pip install pillow\n")
    sys.exit(1)


SCENES = [
    ("killroom",  "Kill Room",  "Auto-walks E1M1, per-band sprite spawns."),
    ("analyzer",  "Analyzer",   "Eight FFT bars injected onto the wall texture."),
    ("spectrum2", "Spectrum",   "2D backgrounds + per-band sprite scaling."),
]


def repo_short_sha() -> str:
    """Return the short SHA of HEAD, or '' if not in a git repo."""
    try:
        sha = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL,
        )
        return sha.decode().strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return ""


def convert_ppm_to_png(src: Path, dst: Path) -> None:
    """Read a PPM, write a PNG. Pillow handles P6 binary PPMs natively."""
    dst.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(src) as img:
        img.save(dst, format="PNG", optimize=True)


def collect_scene_frames(baseline_dir: Path, scene: str) -> list[Path]:
    scene_dir = baseline_dir / scene
    if not scene_dir.is_dir():
        return []
    return sorted(scene_dir.glob("frame_*.ppm"))


def build_scene(baseline_dir: Path, output_dir: Path, scene: str) -> int:
    """Convert one scene's PPMs to PNGs. Returns frame count."""
    frames = collect_scene_frames(baseline_dir, scene)
    if not frames:
        sys.stderr.write(f"warning: no frames found for scene '{scene}'\n")
        return 0
    for src in frames:
        dst = output_dir / scene / (src.stem + ".png")
        convert_ppm_to_png(src, dst)
    return len(frames)


def render_html(scene_counts: dict[str, int], commit: str) -> str:
    """Lay out thumbnails per scene with the same palette docs/index.html uses."""
    sections: list[str] = []
    for slug, title, blurb in SCENES:
        count = scene_counts.get(slug, 0)
        if count == 0:
            continue
        thumbs: list[str] = []
        for i in range(count):
            name = f"frame_{i:04d}"
            thumbs.append(
                f'        <a href="{slug}/{name}.png" class="thumb"><img src="{slug}/{name}.png" alt="{name}"><span>{i:02d}</span></a>'
            )
        thumbs_html = "\n".join(thumbs)
        sections.append(f"""
    <section>
        <h2>{html.escape(title)} <span class="count">{count} frames</span></h2>
        <p class="blurb">{html.escape(blurb)}</p>
        <div class="grid">
{thumbs_html}
        </div>
    </section>""")

    commit_strip = (
        f'<p class="commit">Generated from <code>{html.escape(commit)}</code> on push to <code>main</code>.</p>'
        if commit else ""
    )

    body = "\n".join(sections)
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DoomViz — VRT baselines</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{
            background: #1a1a1a;
            color: #e0e0e0;
            font-family: 'Courier New', monospace;
            line-height: 1.6;
        }}
        .container {{ max-width: 1200px; margin: 0 auto; padding: 20px; }}
        a {{ color: #ff6644; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        h1 {{
            color: #ff4444;
            font-size: 2.4em;
            text-align: center;
            margin: 30px 0 8px;
            text-shadow: 0 0 20px rgba(255, 68, 68, 0.5);
        }}
        .subtitle {{
            text-align: center;
            color: #888;
            font-size: 1em;
            margin-bottom: 8px;
        }}
        .commit {{
            text-align: center;
            color: #555;
            font-size: 0.85em;
            margin-bottom: 30px;
        }}
        .commit code {{
            background: #2a2a2a;
            padding: 2px 6px;
            border-radius: 3px;
        }}
        section {{ margin-top: 40px; }}
        h2 {{
            color: #ff6644;
            margin-bottom: 6px;
            border-bottom: 1px solid #333;
            padding-bottom: 5px;
            font-size: 1.4em;
        }}
        h2 .count {{
            float: right;
            color: #555;
            font-size: 0.7em;
            font-weight: normal;
        }}
        .blurb {{ color: #888; margin-bottom: 16px; font-size: 0.9em; }}
        .grid {{
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
            gap: 8px;
        }}
        .thumb {{
            position: relative;
            display: block;
            border: 1px solid #444;
            background: #000;
            overflow: hidden;
        }}
        .thumb img {{
            display: block;
            width: 100%;
            image-rendering: pixelated;
        }}
        .thumb span {{
            position: absolute;
            left: 4px;
            bottom: 4px;
            background: rgba(0,0,0,0.7);
            color: #f0d878;
            padding: 1px 5px;
            font-size: 0.75em;
            font-weight: bold;
        }}
        .thumb:hover {{
            border-color: #ff4444;
        }}
        footer {{
            text-align: center;
            color: #555;
            margin-top: 60px;
            padding: 20px;
            border-top: 1px solid #333;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>VRT BASELINES</h1>
        <p class="subtitle">Per-frame baselines the visual regression suite asserts against.</p>
        {commit_strip}{body}
        <footer>
            <p><a href="../">&laquo; back to DoomViz</a></p>
        </footer>
    </div>
</body>
</html>
"""


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--baseline-dir", type=Path, required=True)
    p.add_argument("--output-dir",   type=Path, required=True)
    args = p.parse_args()

    if not args.baseline_dir.is_dir():
        sys.stderr.write(f"error: baseline dir not found: {args.baseline_dir}\n")
        return 1

    args.output_dir.mkdir(parents=True, exist_ok=True)

    scene_counts: dict[str, int] = {}
    for slug, _title, _blurb in SCENES:
        n = build_scene(args.baseline_dir, args.output_dir, slug)
        scene_counts[slug] = n
        print(f"  {slug}: converted {n} frames")

    index_path = args.output_dir / "index.html"
    index_path.write_text(render_html(scene_counts, repo_short_sha()))
    print(f"  wrote {index_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
