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


@dataclass(frozen=True)
class DocumentChunk:
    chunk_id: str
    source_id: str
    source_type: str
    title: str
    source_path: str
    text: str
    ordinal: int
    section_title: str = ""
    content_kind: str = "text"


@dataclass(frozen=True)
class ToolSpec:
    tool_id: str
    name: str
    description: str
    risk_level: str
    source_script: str
    executor: str
    default_args: list[str] = field(default_factory=list)
    default_env: dict[str, str] = field(default_factory=dict)
    tags: list[str] = field(default_factory=list)


@dataclass(frozen=True)
class ToolPlan:
    request: str
    summary: str
    tool_id: str | None
    tool_name: str | None
    risk_level: str
    allowed_to_execute: bool
    requires_confirmation: bool
    should_refuse: bool
    command_preview: list[str] = field(default_factory=list)
    reason: str = ""
    evidence: list[str] = field(default_factory=list)


@dataclass(frozen=True)
class ToolExecutionResult:
    tool_id: str
    command: list[str]
    executed: bool
    returncode: int | None
    stdout: str
    stderr: str
    risk_level: str
    parsed_output: dict[str, Any] = field(default_factory=dict)
