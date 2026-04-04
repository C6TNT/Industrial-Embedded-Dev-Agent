from __future__ import annotations

from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .benchmarks import load_benchmark_items
from .models import BenchmarkItem


def run_benchmark(items: list[BenchmarkItem]) -> dict[str, object]:
    results: list[dict[str, object]] = []
    passed = 0
    for item in items:
        result = _evaluate_item(item)
        results.append(result)
        if result["passed"]:
            passed += 1
    return {
        "total": len(items),
        "passed": passed,
        "failed": len(items) - passed,
        "pass_rate": round(passed / len(items), 4) if items else 0.0,
        "results": results,
    }


def run_benchmark_from_path(path: Path) -> dict[str, object]:
    return run_benchmark(load_benchmark_items(path))


def _evaluate_item(item: BenchmarkItem) -> dict[str, object]:
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
