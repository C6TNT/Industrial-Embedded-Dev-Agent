from __future__ import annotations

import re
from pathlib import Path

from .benchmarks import load_benchmark_items, summarize_benchmark
from .config import ProjectPaths
from .models import DatasetOverview
from .taxonomy import summarize_taxonomy


def _extract_material_counts(material_index_path: Path) -> dict[str, int]:
    content = material_index_path.read_text(encoding="utf-8")
    counts = {"documents": 0, "logs": 0, "cases": 0, "scripts": 0}
    section_to_key = {
        "## 1.": "documents",
        "## 2.": "logs",
        "## 3.": "cases",
        "## 4.": "scripts",
    }
    current_key: str | None = None

    for raw_line in content.splitlines():
        line = raw_line.strip()
        if line.startswith("## "):
            current_key = None
            for prefix, key in section_to_key.items():
                if prefix in line:
                    current_key = key
                    break
            continue
        if current_key and line.startswith("|") and not line.startswith("|---"):
            if re.match(r"^\|\s*ID\s*\|", line, flags=re.IGNORECASE):
                continue
            counts[current_key] += 1
    return counts


def build_dataset_overview(paths: ProjectPaths) -> DatasetOverview:
    benchmark_items = load_benchmark_items(paths.benchmark_dir / "benchmark_v1.jsonl")
    benchmark_summary = summarize_benchmark(benchmark_items)
    taxonomy_groups = summarize_taxonomy(paths.taxonomy_dir / "labels_v1.md")
    material_counts = _extract_material_counts(paths.materials_dir / "material_index_v1.md")
    return DatasetOverview(
        benchmark_total=benchmark_summary["total"],
        benchmark_by_type=benchmark_summary["by_type"],
        taxonomy_groups=taxonomy_groups,
        material_counts=material_counts,
    )
