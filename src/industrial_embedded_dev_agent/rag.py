from __future__ import annotations

import re
from pathlib import Path

from .analysis import analyze_text
from .models import Citation, RagAnswer, SearchHit
from .retrieval import build_search_documents, search_documents


TOKEN_RE = re.compile(r"[A-Za-z0-9_]+")


def answer_with_rag(
    root: Path,
    question: str,
    *,
    limit: int = 5,
    include_benchmark: bool = False,
) -> RagAnswer:
    primary_hits = _search(root, question, include_benchmark=include_benchmark, limit=limit)
    hits = primary_hits
    if not hits and not include_benchmark:
        hits = _search(root, question, include_benchmark=True, limit=limit)

    diagnosis = analyze_text(question, mode="auto")
    citations = _select_citations(hits, question, limit=3)
    answer = _compose_answer(diagnosis, citations)
    return RagAnswer(
        question=question,
        answer=answer,
        citations=citations,
        structured_diagnosis=diagnosis,
    )


def _search(root: Path, question: str, *, include_benchmark: bool, limit: int) -> list[SearchHit]:
    documents = build_search_documents(root, include_benchmark=include_benchmark)
    return search_documents(documents, question, limit=limit)


def _select_citations(hits: list[SearchHit], question: str, *, limit: int) -> list[Citation]:
    citations: list[Citation] = []
    for hit in hits:
        snippet = _extract_snippet(hit.content, question)
        if not snippet:
            continue
        citations.append(
            Citation(
                source_id=hit.source_id,
                source_type=hit.source_type,
                title=hit.title,
                score=hit.score,
                snippet=snippet,
            )
        )
        if len(citations) >= limit:
            break
    return citations


def _extract_snippet(content: str, question: str) -> str:
    chunks = [chunk.strip() for chunk in re.split(r"[。！？\n]", content) if chunk.strip()]
    if not chunks:
        return content[:180]

    query_terms = _tokenize(question)
    best_chunk = chunks[0]
    best_score = -1
    for chunk in chunks:
        chunk_terms = set(_tokenize(chunk))
        score = sum(1 for term in query_terms if term in chunk_terms)
        if score > best_score:
            best_chunk = chunk
            best_score = score
    return best_chunk[:180]


def _compose_answer(diagnosis, citations: list[Citation]) -> str:
    lead = diagnosis.summary
    evidence_text = _build_evidence_text(citations)
    label_text = _build_label_text(diagnosis)

    if diagnosis.should_refuse:
        action_text = "、".join(diagnosis.action_labels) if diagnosis.action_labels else "保持只读诊断"
        return (
            f"{lead} 当前建议不要自动执行该动作。"
            f" 可先转为 {action_text} 这类低风险步骤，并保留人工确认。"
            f" {evidence_text} {label_text}"
        ).strip()

    return f"{lead} {evidence_text} {label_text}".strip()


def _build_evidence_text(citations: list[Citation]) -> str:
    if not citations:
        return "当前没有检索到足够强的文档片段，建议补充更具体的对象字典、日志关键字或工具名。"

    parts = [f"{item.source_id} 提到“{item.snippet}”" for item in citations[:2]]
    return "结合文档片段，" + "；".join(parts) + "。"


def _build_label_text(diagnosis) -> str:
    tags = [diagnosis.issue_category, *diagnosis.cause_labels, *diagnosis.action_labels, diagnosis.risk_level]
    compact = [tag for tag in tags if tag]
    return f"结构化标签: {', '.join(compact)}。" if compact else ""


def _tokenize(text: str) -> list[str]:
    lowered = text.lower()
    ascii_tokens = TOKEN_RE.findall(lowered)
    cjk_tokens = [char for char in lowered if "\u4e00" <= char <= "\u9fff"]
    return ascii_tokens + cjk_tokens
