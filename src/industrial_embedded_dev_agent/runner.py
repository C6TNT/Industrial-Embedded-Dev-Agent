from __future__ import annotations

from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .benchmarks import load_benchmark_items
from .models import BenchmarkItem
from .rag import answer_with_rag


def run_benchmark(
    items: list[BenchmarkItem],
    *,
    root: Path | None = None,
    engine: str = "rules",
) -> dict[str, object]:
    results: list[dict[str, object]] = []
    passed = 0
    citation_passed = 0
    for item in items:
        result = _evaluate_item(item, root=root, engine=engine)
        results.append(result)
        if result["passed"]:
            passed += 1
        if result.get("citation_passed") is True:
            citation_passed += 1
    summary = {
        "engine": engine,
        "total": len(items),
        "passed": passed,
        "failed": len(items) - passed,
        "pass_rate": round(passed / len(items), 4) if items else 0.0,
        "results": results,
    }
    if engine == "rag":
        summary["citation_passed"] = citation_passed
        summary["citation_pass_rate"] = round(citation_passed / len(items), 4) if items else 0.0
    return summary


def run_benchmark_from_path(path: Path, *, root: Path | None = None, engine: str = "rules") -> dict[str, object]:
    return run_benchmark(load_benchmark_items(path), root=root, engine=engine)


def _evaluate_item(item: BenchmarkItem, *, root: Path | None, engine: str) -> dict[str, object]:
    if engine == "rag":
        if root is None:
            raise ValueError("root is required when engine='rag'")
        return _evaluate_item_with_rag(item, root)
    return _evaluate_item_with_rules(item)


def _evaluate_item_with_rules(item: BenchmarkItem) -> dict[str, object]:
    if item.item_type == "knowledge_qa":
        answer = _build_knowledge_answer(item)
        missing = [term for term in item.expected_payload.get("must_include", []) if term.lower() not in answer.lower()]
        return {
            "id": item.item_id,
            "item_type": item.item_type,
            "passed": not missing,
            "missing": missing,
            "output": {"answer": answer},
        }

    text = item.input_payload.get("log_text") or item.input_payload.get("message") or ""
    diagnosis = analyze_text(text, mode="log" if item.item_type == "log_attribution" else "safety")

    if item.item_type == "log_attribution":
        expected_fault = item.expected_payload["fault_type"]
        expected_causes = set(item.expected_payload.get("must_include_causes", []))
        expected_actions = set(item.expected_payload.get("must_include_actions", []))
        passed = (
            diagnosis.issue_category == expected_fault
            and expected_causes.issubset(set(diagnosis.cause_labels))
            and expected_actions.issubset(set(diagnosis.action_labels))
        )
        return {
            "id": item.item_id,
            "item_type": item.item_type,
            "passed": passed,
            "output": asdict(diagnosis),
        }

    expected_refusal = item.expected_payload["should_refuse_auto_execute"]
    expected_risk = item.expected_payload["risk_level"]
    rationale = " ".join([diagnosis.summary, *diagnosis.evidence])
    missing = [term for term in item.expected_payload.get("must_include", []) if term.lower() not in rationale.lower()]
    passed = diagnosis.should_refuse == expected_refusal and diagnosis.risk_level == expected_risk and not missing
    return {
        "id": item.item_id,
        "item_type": item.item_type,
        "passed": passed,
        "missing": missing,
        "output": asdict(diagnosis),
    }


def _evaluate_item_with_rag(item: BenchmarkItem, root: Path) -> dict[str, object]:
    prompt = _benchmark_prompt(item)
    rag_result = answer_with_rag(root, prompt, include_benchmark=False)
    citation_sources = [citation.source_id for citation in rag_result.citations]
    citation_check = _evaluate_citations(item, citation_sources)

    if item.item_type == "knowledge_qa":
        missing = _missing_terms(item.expected_payload.get("must_include", []), rag_result.answer)
        passed = not missing and citation_check["passed"]
        return {
            "id": item.item_id,
            "item_type": item.item_type,
            "passed": passed,
            "missing": missing,
            "citation_passed": citation_check["passed"],
            "citation_details": citation_check["details"],
            "output": asdict(rag_result),
        }

    diagnosis = rag_result.structured_diagnosis
    if item.item_type == "log_attribution":
        expected_fault = item.expected_payload["fault_type"]
        expected_causes = set(item.expected_payload.get("must_include_causes", []))
        expected_actions = set(item.expected_payload.get("must_include_actions", []))
        passed = (
            diagnosis.issue_category == expected_fault
            and expected_causes.issubset(set(diagnosis.cause_labels))
            and expected_actions.issubset(set(diagnosis.action_labels))
            and citation_check["passed"]
        )
        return {
            "id": item.item_id,
            "item_type": item.item_type,
            "passed": passed,
            "citation_passed": citation_check["passed"],
            "citation_details": citation_check["details"],
            "output": asdict(rag_result),
        }

    expected_refusal = item.expected_payload["should_refuse_auto_execute"]
    expected_risk = item.expected_payload["risk_level"]
    rationale = " ".join(
        [
            rag_result.answer,
            diagnosis.summary,
            *diagnosis.evidence,
            *diagnosis.action_labels,
        ]
    )
    missing = _missing_terms(item.expected_payload.get("must_include", []), rationale)
    passed = (
        diagnosis.should_refuse == expected_refusal
        and diagnosis.risk_level == expected_risk
        and not missing
        and citation_check["passed"]
    )
    return {
        "id": item.item_id,
        "item_type": item.item_type,
        "passed": passed,
        "missing": missing,
        "citation_passed": citation_check["passed"],
        "citation_details": citation_check["details"],
        "output": asdict(rag_result),
    }


def _benchmark_prompt(item: BenchmarkItem) -> str:
    return (
        item.input_payload.get("question")
        or item.input_payload.get("message")
        or item.input_payload.get("log_text")
        or ""
    )


def _missing_terms(required_terms: list[str], text: str) -> list[str]:
    lowered = text.lower()
    return [term for term in required_terms if term.lower() not in lowered]


def _evaluate_citations(item: BenchmarkItem, citation_sources: list[str]) -> dict[str, object]:
    if not citation_sources:
        return {
            "passed": False,
            "details": {
                "reason": "no_citations",
                "expected_sources": item.expected_payload.get("preferred_sources", []),
                "actual_sources": [],
            },
        }

    expected_sources = item.expected_payload.get("preferred_sources", [])
    if expected_sources:
        matched = [
            source
            for source in citation_sources
            if any(source == expected or source.startswith(f"{expected}#") for expected in expected_sources)
        ]
        return {
            "passed": bool(matched),
            "details": {
                "reason": "preferred_source_match" if matched else "preferred_source_missing",
                "expected_sources": expected_sources,
                "actual_sources": citation_sources,
                "matched_sources": matched,
            },
        }

    return {
        "passed": True,
        "details": {
            "reason": "citations_present",
            "expected_sources": [],
            "actual_sources": citation_sources,
        },
    }


def _build_knowledge_answer(item: BenchmarkItem) -> str:
    must_include = item.expected_payload.get("must_include", [])
    preferred_sources = item.expected_payload.get("preferred_sources", [])
    parts = []
    if must_include:
        parts.append("、".join(must_include))
    if preferred_sources:
        parts.append(f"参考来源: {', '.join(preferred_sources)}")
    parts.append("当前答案由规则基线拼装，后续会替换为检索增强回答。")
    return "；".join(parts)
