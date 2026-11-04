"""CSV tables with the numbers behind the charts of the plot_*.py scripts.

Each chart gets a .csv with the same name in the same folder. Quartiles use the
same method as matplotlib's box plots, so the table matches what the chart draws.
"""

import csv
import statistics

import numpy as np


def _us(value):
    return f"{value:.1f}"


def write_box_csv(path, system, x_label, xs, obj_values, arch_values, level, extra_columns=None):
    """Writes one row per storage and X value of a box plot.

    obj_values and arch_values line up with xs, and hold None where a run is
    missing. extra_columns maps a column title to a function of the X value.
    """
    extra_columns = extra_columns or {}
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "Measured scope",
                "Optimization level",
                x_label,
                *extra_columns,
                "Storage",
                "Frames measured",
                "Median (us per frame)",
                "1st quartile (us per frame)",
                "3rd quartile (us per frame)",
                "Fastest frame (us)",
                "Slowest frame (us)",
                "Median relative to Archetype (x times)",
            ]
        )
        for x, obj, arch in zip(xs, obj_values, arch_values):
            arch_median = statistics.median(arch) if arch else None
            for storage, samples in (("Object", obj), ("Archetype", arch)):
                if not samples:
                    continue
                median = statistics.median(samples)
                q1, q3 = np.percentile(samples, [25, 75])
                writer.writerow(
                    [
                        system,
                        f"O{level}" if level else "unknown",
                        x,
                        *(column(x) for column in extra_columns.values()),
                        storage,
                        len(samples),
                        _us(median),
                        _us(q1),
                        _us(q3),
                        _us(min(samples)),
                        _us(max(samples)),
                        f"{median / arch_median:.2f}" if arch_median else "",
                    ]
                )
    print(f"wrote {path}")


def write_frames_csv(path, obj_values, arch_values):
    """Writes one row per frame of a per-frame comparison plot."""
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["Frame", "Object (us)", "Archetype (us)"])
        for frame in range(max(len(obj_values), len(arch_values))):
            writer.writerow(
                [
                    frame,
                    _us(obj_values[frame]) if frame < len(obj_values) else "",
                    _us(arch_values[frame]) if frame < len(arch_values) else "",
                ]
            )
    print(f"wrote {path}")
