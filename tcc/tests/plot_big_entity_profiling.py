#!/usr/bin/env python3
"""Plot profiling results and compare the Object and Archetype big entity runs.

Discovers BigEntitySimulationObj-<N>.O<L>.json and
BigEntitySimulationArchetype-<N>.O<L>.json files (or accepts paths/globs) and
writes one box plot, built from the per-frame durations, for each read pass:

    BigEntity::readHot
    BigEntity::readAll

Files of other simulations are ignored. Repeated runs of the same count
(<N>.O<L>-1.json, ...) are merged, but different optimization levels are not:
pass only the files of one level.

Every chart also gets a .csv with the same name, holding the numbers behind it.

Examples:
    ./plot_big_entity_profiling.py
    ./plot_big_entity_profiling.py --out plots
    ./plot_big_entity_profiling.py --linear
    ./plot_big_entity_profiling.py --frames
    ./plot_big_entity_profiling.py some/dir/*.json
"""

import argparse
import glob
import json
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path

try:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:  # pragma: no cover
    sys.exit("matplotlib is required: pip install matplotlib")

from profiling_csv import write_box_csv, write_frames_csv

OBJ = "Obj"
ARCH = "Archetype"

SYSTEMS = ["BigEntity::readHot", "BigEntity::readAll"]

# <simulation>-<count>[.O<level>][-<repeat>], see SimulationProfiling.h
STEM = re.compile(
    rf"^BigEntitySimulation(?P<simulation>{OBJ}|{ARCH})"
    r"-(?P<count>\d+)(?:\.O(?P<level>\d))?(?:-\d+)?$"
)


def load_trace(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f).get("traceEvents", [])


def collect(paths):
    data = {OBJ: defaultdict(dict), ARCH: defaultdict(dict)}
    levels = set()
    for path in paths:
        match = STEM.match(Path(path).stem)
        if match is None:
            continue
        simulation = match["simulation"]
        count = int(match["count"])
        levels.add(match["level"])

        for event in load_trace(path):
            name = event.get("name")
            dur = event.get("dur")
            if name in SYSTEMS and dur is not None:
                data[simulation][count].setdefault(name, []).append(dur)
    return data, levels


def slug(name):
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def draw_boxes(ax, values, positions, width, label, color):
    """Draws one box per count that has samples, labeled with its median."""
    present = [(p, v) for p, v in zip(positions, values) if v]
    if not present:
        return None
    boxes = ax.boxplot(
        [v for _, v in present],
        positions=[p for p, _ in present],
        widths=width,
        patch_artist=True,
        manage_ticks=False,
        medianprops={"color": "black"},
        flierprops={"markersize": 3, "markeredgecolor": color},
    )
    for box in boxes["boxes"]:
        box.set_facecolor(color)
    # Above the highest sample, so the label doesn't cover the outliers
    for position, samples in present:
        ax.annotate(
            f"{statistics.median(samples):.1f}",
            xy=(position, max(samples)),
            xytext=(0, 3),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8,
            rotation=90,
        )
    return boxes["boxes"][0], label


def plot_system(system, data, out_dir, linear_scale, suffix, level):
    counts = sorted({c for sim in (OBJ, ARCH) for c in data[sim] if system in data[sim][c]})
    if not counts:
        return None

    obj_values = [data[OBJ].get(c, {}).get(system) for c in counts]
    arch_values = [data[ARCH].get(c, {}).get(system) for c in counts]

    x = range(len(counts))
    width = 0.38

    fig, ax = plt.subplots(figsize=(max(7, 1.1 * len(counts) + 4), 5))
    obj_pos = [i - width / 2 for i in x]
    arch_pos = [i + width / 2 for i in x]

    legend = [
        entry
        for entry in (
            draw_boxes(ax, obj_values, obj_pos, width * 0.9, "Object", "#d1495b"),
            draw_boxes(ax, arch_values, arch_pos, width * 0.9, "Archetype", "#2e86ab"),
        )
        if entry
    ]

    ax.set_xticks(list(x))
    ax.set_xticklabels([str(c) for c in counts])
    ax.set_xlim(-0.6, len(counts) - 0.4)
    ax.set_xlabel("Entities")
    ax.set_ylabel("Wall duration per frame (us), labeled with the median")
    title = f"{system}: Object vs Archetype{suffix}"
    if len(legend) < 2:
        title += " (missing series)"
    ax.set_title(title)
    ax.legend(*zip(*legend), loc="upper left")
    ax.grid(axis="y", alpha=0.3)

    if not linear_scale:
        ax.set_yscale("log")
    # Headroom for the labels above the tallest boxes
    ax.margins(y=0.15)

    fig.tight_layout()
    path = out_dir / f"{slug(system)}.png"
    fig.savefig(path, dpi=150)
    plt.close(fig)
    write_box_csv(
        path.with_suffix(".csv"), system, "Entities", counts, obj_values, arch_values, level
    )
    return path


def plot_frames(system, data, out_dir, suffix):
    pairs = sorted(
        set(data[OBJ]) & set(data[ARCH]),
    )
    written = []
    for count in pairs:
        obj_vals = data[OBJ][count].get(system)
        arch_vals = data[ARCH][count].get(system)
        if not obj_vals or not arch_vals:
            continue
        fig, ax = plt.subplots(figsize=(9, 4.5))
        ax.plot(range(len(obj_vals)), obj_vals, label="Object", color="#d1495b")
        ax.plot(range(len(arch_vals)), arch_vals, label="Archetype", color="#2e86ab")
        ax.set_xlabel("Frame")
        ax.set_ylabel("Wall duration (us)")
        ax.set_title(f"{system}: per-frame comparison ({count} entities){suffix}")
        ax.legend()
        ax.grid(alpha=0.3)
        fig.tight_layout()
        path = out_dir / "frames" / f"{slug(system)}_{count}.png"
        path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(path, dpi=150)
        plt.close(fig)
        written.append(path)
        write_frames_csv(path.with_suffix(".csv"), obj_vals, arch_vals)
    return written


def expand(paths):
    files = []
    for path in paths:
        if Path(path).is_dir():
            files.extend(str(p) for p in Path(path).glob("*.json"))
            continue
        matches = glob.glob(path)
        if matches:
            files.extend(matches)
        else:
            files.append(path)
    return sorted(set(files))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "paths",
        nargs="*",
        help="JSON files, globs or directories (default: script directory)",
    )
    parser.add_argument("--out", default="plots", help="Output directory (default: plots)")
    parser.add_argument(
        "--linear", action="store_true", help="Use a linear Y axis instead of a log one"
    )
    parser.add_argument(
        "--frames", action="store_true", help="Also write per-frame comparison plots"
    )
    args = parser.parse_args(argv)

    paths = args.paths or [str(Path(__file__).resolve().parent)]
    files = expand(paths)
    if not files:
        parser.error("no JSON files found")

    data, levels = collect(files)
    if not any(data[s] for s in (OBJ, ARCH)):
        parser.error("no BigEntitySimulation traces found")
    if len(levels) > 1:
        found = ", ".join(sorted(f"O{level}" if level else "unlabeled" for level in levels))
        parser.error(f"found several optimization levels ({found}), pass only one")
    level = next(iter(levels))
    suffix = f" (-O{level})" if level else ""

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    written = []
    for system in SYSTEMS:
        for simulation in (OBJ, ARCH):
            if not any(system in data[simulation][c] for c in data[simulation]):
                print(
                    f"warning: no {simulation} data for '{system}'",
                    file=sys.stderr,
                )
        path = plot_system(system, data, out_dir, args.linear, suffix, level)
        if path:
            written.append(path)
            print(f"wrote {path}")

    if args.frames:
        for system in SYSTEMS:
            for path in plot_frames(system, data, out_dir, suffix):
                written.append(path)
                print(f"wrote {path}")

    if not written:
        parser.error("no comparable system data found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
