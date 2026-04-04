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


def _tokenize(text: str) -> list[str]:
    lowered = text.lower()
    ascii_tokens = TOKEN_RE.findall(lowered)
    cjk_tokens = [char for char in lowered if "\u4e00" <= char <= "\u9fff"]
    return ascii_tokens + cjk_tokens


def build_search_documents(root: Path) -> list[SearchDocument]:
    docs: list[SearchDocument] = []
    docs.extend(_markdown_sections(root / "data" / "materials" / "material_index_v1.md", "materials"))
    docs.extend(_markdown_sections(root / "data" / "taxonomy" / "labels_v1.md", "taxonomy"))
    docs.extend(_benchmark_documents(root / "data" / "benchmark" / "benchmark_v1.jsonl"))
    return docs


def _markdown_sections(path: Path, source_type: str) -> list[SearchDocument]:
    content = path.read_text(encoding="utf-8")
    sections: list[SearchDocument] = []
    current_title = path.stem
    current_lines: list[str] = []
    index = 0

    def flush() -> None:
        nonlocal current_lines, index
        body = "\n".join(current_lines).strip()
        if body:
            sections.append(
                SearchDocument(
                    source_id=f"{path.stem}:{index}",
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
    return sections


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
        score = 0.0
        for term in query_terms:
            if term not in token_counter:
                continue
            idf = math.log((1 + total_docs) / (1 + doc_freq[term])) + 1.0
            score += (token_counter[term] / norm) * idf
        if score > 0:
            hits.append(
                SearchHit(
                    source_id=doc.source_id,
                    source_type=doc.source_type,
                    title=doc.title,
                    content=doc.content[:300],
                    score=round(score, 4),
                )
            )
    return sorted(hits, key=lambda item: item.score, reverse=True)[:limit]
