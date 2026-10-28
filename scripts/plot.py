"""Построение графиков из реальных CSV-замеров: python scripts/plot.py."""
import csv
import os
import sys
from collections import defaultdict
from pathlib import Path
from statistics import median

ROOT = Path(__file__).resolve().parent.parent
local_packages = ROOT / ".plot-deps"
if local_packages.is_dir():
    sys.path.insert(0, str(local_packages))
os.environ.setdefault("MPLCONFIGDIR", str(ROOT / "tmp" / "matplotlib"))

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

RESULTS = ROOT / "results"
plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 10})
ORDER_NAMES = {"ascending": "Возрастание", "descending": "Убывание", "random": "Случайный порядок"}


def read(name):
    with (RESULTS / name).open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source))


def series(rows, metric):
    groups = defaultdict(list)
    for row in rows:
        groups[int(row["n"])].append(float(row[metric]))
    sizes = sorted(groups)
    return sizes, [median(groups[n]) for n in sizes]


def save(fig, name):
    fig.savefig(RESULTS / f"{name}.png", dpi=170)
    fig.savefig(RESULTS / f"{name}.svg")
    plt.close(fig)


def main():
    rows = read("lab1.csv")
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.8), layout="constrained")
    for axis, metric, title in zip(axes, ["insert_ms", "find_ms", "erase_ms"],
                                   ["Вставка n элементов", "Поиск n элементов", "Удаление n элементов"]):
        for order, label in ORDER_NAMES.items():
            sizes, times = series([r for r in rows if r["order"] == order], metric)
            axis.plot(sizes, times, "o-", label=label)
        axis.set(xscale="log", yscale="log", xlabel="Число узлов n", ylabel="Время, мс", title=title)
        axis.grid(alpha=0.25)
    axes[0].legend(fontsize=8)
    repeats = len({r["repeat"] for r in rows})
    fig.suptitle(f"ЛР 1 · AVL-дерево · медиана {repeats} запусков", fontsize=14)
    save(fig, "lab1_performance")

    rows = read("lab2.csv")
    fig, axes = plt.subplots(2, 3, figsize=(15, 8), layout="constrained")
    for column, (order, title) in enumerate(ORDER_NAMES.items()):
        for row_index, metric, label in [(0, "build_ms", "Построение, мс"), (1, "search_ms", "100 000 запросов, мс")]:
            axis = axes[row_index, column]
            for backend, name in [("AVL", "AVL-дерево"), ("BinarySearch", "Двоичный поиск / массив")]:
                sizes, times = series([r for r in rows if r["order"] == order and r["backend"] == backend], metric)
                axis.plot(sizes, times, "o-", label=name)
            axis.set(xlabel="Число ненулевых элементов k", ylabel=label, title=title, xscale="log", yscale="log")
            axis.grid(alpha=0.25)
            if column == 0:
                axis.legend(fontsize=8)
    repeats = len({r["repeat"] for r in rows})
    fig.suptitle(f"ЛР 2 · Построение и поиск измерены отдельно · медиана {repeats} запусков", fontsize=14)
    save(fig, "lab2_performance")
    print("Created results/lab1_performance.{png,svg} and lab2_performance.{png,svg}")


if __name__ == "__main__":
    main()
