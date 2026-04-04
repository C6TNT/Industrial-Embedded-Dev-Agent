from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

from .models import BenchmarkItem


def load_benchmark_items(path: Path) -> list[BenchmarkItem]:
    items: list[BenchmarkItem] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        raw = json.loads(line)
        items.append(
            BenchmarkItem(
                item_id=raw["id"],
                item_type=raw["item_type"],
                input_payload=raw["input"],
                expected_payload=raw["expected"],
                tags=raw.get("tags", []),
                difficulty=raw.get("difficulty", "unknown"),
            )
        )
    return items


def summarize_benchmark(items: list[BenchmarkItem]) -> dict[str, dict[str, int] | int]:
    return {
        "total": len(items),
        "by_type": dict(Counter(item.item_type for item in items)),
        "by_difficulty": dict(Counter(item.difficulty for item in items)),
        "tag_frequency": dict(Counter(tag for item in items for tag in item.tags).most_common()),
    }


def filter_benchmark_items(
    items: list[BenchmarkItem],
    *,
    item_type: str | None = None,
    tag: str | None = None,
) -> list[BenchmarkItem]:
    filtered = items
    if item_type:
        filtered = [item for item in filtered if item.item_type == item_type]
    if tag:
        filtered = [item for item in filtered if tag in item.tags]
    return filtered
