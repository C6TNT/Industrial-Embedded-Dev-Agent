from __future__ import annotations

import re

from .benchmarks import load_benchmark_items, summarize_benchmark
from .config import ProjectPaths
from .models import DatasetOverview
from .taxonomy import summarize_taxonomy


def _extract_material_counts(material_index_path) -> dict[str, int]:
    content = material_index_path.read_text(encoding="utf-8")
    patterns = {
        "documents": r"已整理的\s+(\d+)\s+份调试文档",
        "logs": r"已整理的\s+(\d+)\s+份日志样本",
        "cases": r"已整理的\s+(\d+)\s+个历史问题 case",
        "scripts": r"已整理的\s+(\d+)\s+个常用脚本",
    }
    counts: dict[str, int] = {}
    for key, pattern in patterns.items():
        matched = re.search(pattern, content)
        counts[key] = int(matched.group(1)) if matched else 0
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
