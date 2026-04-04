from __future__ import annotations

import json
import math
import re
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

from .models import SearchHit


TOKEN_RE = re.compile(r"[A-Za-z0-9_]+")


@dataclass(frozen=True)
class SearchDocument:
    source_id: str
    source_type: str
    title: str
    content: str


def build_search_documents(root: Path, *, include_benchmark: bool = True) -> list[SearchDocument]:
    docs: list[SearchDocument] = []
    docs.extend(_markdown_sections(root / "Industrial Embedded Dev Agent_项目方案.md", "project"))
    docs.extend(_markdown_sections(root / "data" / "materials" / "material_index_v1.md", "materials"))
    docs.extend(_markdown_sections(root / "data" / "taxonomy" / "labels_v1.md", "taxonomy"))

    if include_benchmark:
        docs.extend(_benchmark_documents(root / "data" / "benchmark" / "benchmark_v1.jsonl"))
    return docs


def search_documents(documents: list[SearchDocument], query: str, *, limit: int = 5) -> list[SearchHit]:
    query_terms = _tokenize(query)
    if not query_terms:
        return []

    doc_freq: Counter[str] = Counter()
    tokenized_docs: list[tuple[SearchDocument, list[str]]] = []
    for doc in documents:
        tokens = _tokenize(" ".join([doc.title, doc.content]))
        tokenized_docs.append((doc, tokens))
        for token in set(tokens):
            doc_freq[token] += 1

    total_docs = max(len(tokenized_docs), 1)
    hits: list[SearchHit] = []
    for doc, tokens in tokenized_docs:
        token_counter = Counter(tokens)
        norm = math.sqrt(sum(count * count for count in token_counter.values())) or 1.0
        lexical_score = 0.0
        for term in query_terms:
            if term not in token_counter:
                continue
            idf = math.log((1 + total_docs) / (1 + doc_freq[term])) + 1.0
            lexical_score += (token_counter[term] / norm) * idf

        if lexical_score <= 0:
            continue

        weighted_score = lexical_score * _source_weight(doc.source_type)
        hits.append(
            SearchHit(
                source_id=doc.source_id,
                source_type=doc.source_type,
                title=doc.title,
                content=doc.content[:400],
                score=round(weighted_score, 4),
            )
        )

    return sorted(hits, key=lambda item: item.score, reverse=True)[:limit]


def _markdown_sections(path: Path, source_type: str) -> list[SearchDocument]:
    content = path.read_text(encoding="utf-8")
    docs: list[SearchDocument] = []

    docs.extend(_heading_chunks(path, source_type, content))
    docs.extend(_table_row_chunks(path, source_type, content))
    return docs


def _heading_chunks(path: Path, source_type: str, content: str) -> list[SearchDocument]:
    docs: list[SearchDocument] = []
    current_title = path.stem
    current_lines: list[str] = []
    index = 0

    def flush() -> None:
        nonlocal current_lines, index
        body = "\n".join(current_lines).strip()
        if body:
            docs.append(
                SearchDocument(
                    source_id=f"{path.stem}:section:{index}",
                    source_type=source_type,
                    title=current_title,
                    content=body,
                )
            )
            index += 1
        current_lines = []

    for line in content.splitlines():
        if line.startswith("#"):
            flush()
            current_title = line.lstrip("#").strip() or current_title
        else:
            current_lines.append(line)
    flush()
    return docs


def _table_row_chunks(path: Path, source_type: str, content: str) -> list[SearchDocument]:
    docs: list[SearchDocument] = []
    current_title = path.stem
    headers: list[str] = []
    row_index = 0

    for raw_line in content.splitlines():
        line = raw_line.strip()
        if line.startswith("#"):
            current_title = line.lstrip("#").strip() or current_title
            headers = []
            continue
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        if not any(cells):
            continue
        if all(set(cell) <= {"-"} for cell in cells):
            continue
        if not headers:
            headers = cells
            continue

        row_data = []
        for index, value in enumerate(cells):
            header = headers[index] if index < len(headers) else f"col_{index}"
            if value:
                row_data.append(f"{header}: {value}")
        if not row_data:
            continue

        source_id = _row_source_id(path.stem, cells, row_index)
        docs.append(
            SearchDocument(
                source_id=source_id,
                source_type=source_type,
                title=current_title,
                content="；".join(row_data),
            )
        )
        row_index += 1
    return docs


def _row_source_id(stem: str, cells: list[str], row_index: int) -> str:
    if cells and re.fullmatch(r"[A-Z]+-\d+", cells[0]):
        return cells[0]
    return f"{stem}:row:{row_index}"


def _benchmark_documents(path: Path) -> list[SearchDocument]:
    docs: list[SearchDocument] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        raw = json.loads(line)
        content = raw["input"].get("question") or raw["input"].get("message") or raw["input"].get("log_text", "")
        docs.append(
            SearchDocument(
                source_id=raw["id"],
                source_type="benchmark",
                title=raw["item_type"],
                content=content,
            )
        )
    return docs


def _source_weight(source_type: str) -> float:
    return {
        "project": 1.35,
        "materials": 1.25,
        "taxonomy": 1.1,
        "benchmark": 0.7,
    }.get(source_type, 1.0)


def _tokenize(text: str) -> list[str]:
    lowered = text.lower()
    ascii_tokens = TOKEN_RE.findall(lowered)
    cjk_tokens = [char for char in lowered if "\u4e00" <= char <= "\u9fff"]
    return ascii_tokens + cjk_tokens
