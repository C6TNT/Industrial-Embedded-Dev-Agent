from __future__ import annotations

from pathlib import Path


SECTION_TO_GROUP = {
    "1.": "issue_categories",
    "2.": "cause_labels",
    "3.": "action_labels",
    "4.": "risk_labels",
}


def parse_taxonomy_labels(path: Path) -> dict[str, list[str]]:
    content = path.read_text(encoding="utf-8")
    results: dict[str, list[str]] = {value: [] for value in SECTION_TO_GROUP.values()}
    current_group: str | None = None

    for raw_line in content.splitlines():
        line = raw_line.strip()
        if line.startswith("## "):
            current_group = None
            for prefix, group_name in SECTION_TO_GROUP.items():
                if prefix in line:
                    current_group = group_name
                    break
            continue
        if not current_group or not line.startswith("| `"):
            continue
        parts = [part.strip() for part in line.split("|")]
        if len(parts) < 3:
            continue
        label = parts[1].strip("` ")
        if label and label.lower() != "label":
            results[current_group].append(label)
    return results


def summarize_taxonomy(path: Path) -> dict[str, int]:
    groups = parse_taxonomy_labels(path)
    return {name: len(labels) for name, labels in groups.items()}
