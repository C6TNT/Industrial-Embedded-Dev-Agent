from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass(frozen=True)
class BenchmarkItem:
    item_id: str
    item_type: str
    input_payload: dict[str, Any]
    expected_payload: dict[str, Any]
    tags: list[str] = field(default_factory=list)
    difficulty: str = "unknown"


@dataclass(frozen=True)
class LabelGroup:
    name: str
    labels: list[str]


@dataclass(frozen=True)
class DatasetOverview:
    benchmark_total: int
    benchmark_by_type: dict[str, int]
    taxonomy_groups: dict[str, int]
    material_counts: dict[str, int]


@dataclass(frozen=True)
class SearchHit:
    source_id: str
    source_type: str
    title: str
    content: str
    score: float


@dataclass(frozen=True)
class StructuredDiagnosis:
    summary: str
    issue_category: str
    cause_labels: list[str]
    action_labels: list[str]
    risk_level: str
    should_refuse: bool
    evidence: list[str]


@dataclass(frozen=True)
class Citation:
    source_id: str
    source_type: str
    title: str
    score: float
    snippet: str


@dataclass(frozen=True)
class RagAnswer:
    question: str
    answer: str
    citations: list[Citation]
    structured_diagnosis: StructuredDiagnosis
