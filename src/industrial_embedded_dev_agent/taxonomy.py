from __future__ import annotations

from collections import OrderedDict
from pathlib import Path


SECTION_TO_GROUP = OrderedDict(
    [
        ("## 1. 问题大类标签", "issue_categories"),
        ("## 2. 原因标签", "cause_labels"),
        ("## 3. 建议动作标签", "action_labels"),
        ("## 4. 风险等级标签", "risk_labels"),
    ]
)


def parse_taxonomy_labels(path: Path) -> dict[str, list[str]]:
    content = path.read_text(encoding="utf-8")
    results: dict[str, list[str]] = {value: [] for value in SECTION_TO_GROUP.values()}

    current_group: str | None = None
    for raw_line in content.splitlines():
        line = raw_line.strip()
        if line in SECTION_TO_GROUP:
            current_group = SECTION_TO_GROUP[line]
            continue
        if not current_group or not line.startswith("| `"):
            continue
        parts = [part.strip() for part in line.split("|")]
        if len(parts) < 3:
            continue
        label = parts[1].strip("` ")
        if label and label != "标签":
            results[current_group].append(label)
    return results


def summarize_taxonomy(path: Path) -> dict[str, int]:
    groups = parse_taxonomy_labels(path)
    return {name: len(labels) for name, labels in groups.items()}
