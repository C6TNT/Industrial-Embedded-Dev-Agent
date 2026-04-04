from __future__ import annotations

import re
from pathlib import Path

from .analysis import analyze_text
from .models import Citation, RagAnswer, SearchHit
from .retrieval import build_search_documents, search_documents


TOKEN_RE = re.compile(r"[A-Za-z0-9_]+")


def answer_with_rag(root: Path, question: str, *, limit: int = 5) -> RagAnswer:
    documents = build_search_documents(root)
    hits = search_documents(documents, question, limit=limit)
    diagnosis = analyze_text(question, mode="auto")
    citations = _select_citations(hits, question, limit=3)
    answer = _compose_answer(question, diagnosis, citations)
    return RagAnswer(
        question=question,
        answer=answer,
        citations=citations,
        structured_diagnosis=diagnosis,
    )


def _to_citation(hit: SearchHit, question: str) -> Citation:
    snippet = _extract_snippet(hit.content, question)
    return Citation(
        source_id=hit.source_id,
        source_type=hit.source_type,
        title=hit.title,
        score=hit.score,
        snippet=snippet,
    )


def _select_citations(hits: list[SearchHit], question: str, *, limit: int) -> list[Citation]:
    ranked = sorted(hits, key=lambda hit: (_citation_priority(hit), hit.score), reverse=True)
    citations: list[Citation] = []
    for hit in ranked:
        citation = _to_citation(hit, question)
        if not citation.snippet:
            continue
        citations.append(citation)
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


def _compose_answer(question: str, diagnosis, citations: list[Citation]) -> str:
    lead = diagnosis.summary
    if diagnosis.should_refuse:
        action_text = "、".join(diagnosis.action_labels) if diagnosis.action_labels else "保持只读诊断"
        return (
            f"{lead} 当前建议不要自动执行该动作。"
            f" 可先转为 {action_text} 这类低风险步骤，并保留人工确认。"
        )

    evidence_text = _build_evidence_text(citations)
    label_text = _build_label_text(diagnosis)
    if evidence_text:
        return f"{lead} {evidence_text} {label_text}".strip()
    return f"{lead} {label_text}".strip()


def _build_evidence_text(citations: list[Citation]) -> str:
    if not citations:
        return "当前没有检索到足够强的证据片段，建议补充更具体的对象字典、日志关键字或工具名。"
    top = citations[:2]
    parts = [f"{item.source_id} 提到“{item.snippet}”" for item in top if item.snippet]
    if not parts:
        return ""
    return "结合检索结果，" + "；".join(parts) + "。"


def _citation_priority(hit: SearchHit) -> tuple[int, int]:
    source_priority = {
        "benchmark": 3,
        "materials": 2,
        "taxonomy": 1,
    }.get(hit.source_type, 0)
    title_bonus = 1 if any(token in hit.title.lower() for token in ["log", "case", "knowledge", "tool"]) else 0
    return (source_priority, title_bonus)


def _build_label_text(diagnosis) -> str:
    tags = [diagnosis.issue_category, *diagnosis.cause_labels, *diagnosis.action_labels, diagnosis.risk_level]
    compact = [tag for tag in tags if tag]
    if not compact:
        return ""
    return "结构化标签: " + ", ".join(compact) + "。"


def _tokenize(text: str) -> list[str]:
    lowered = text.lower()
    ascii_tokens = TOKEN_RE.findall(lowered)
    cjk_tokens = [char for char in lowered if "\u4e00" <= char <= "\u9fff"]
    return ascii_tokens + cjk_tokens
