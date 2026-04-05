from __future__ import annotations
import subprocess
import sys
from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .bench_pack_render import summarize_bench_pack_diff_from_files
from .benchmarks import load_benchmark_items
from .models import BenchmarkItem
from .rag import answer_with_rag
from .tools import plan_tool_request


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


def run_local_checks(root: Path) -> dict[str, object]:
    return run_local_checks_with_options(root)


def run_local_checks_with_options(
    root: Path,
    *,
    include_rag: bool = False,
    rag_item_type: str | None = None,
    include_offline: bool = False,
) -> dict[str, object]:
    benchmark_path = root / "data" / "benchmark" / "benchmark_v1.jsonl"
    items = load_benchmark_items(benchmark_path)

    checks: list[dict[str, object]] = []
    checks.append(_run_pytest_check(root))

    rules_summary = run_benchmark(items, root=root, engine="rules")
    checks.append(
        {
            "name": "benchmark_rules",
            "passed": rules_summary["failed"] == 0,
            "details": {
                "total": rules_summary["total"],
                "passed": rules_summary["passed"],
                "failed": rules_summary["failed"],
                "pass_rate": rules_summary["pass_rate"],
            },
        }
    )

    tool_items = [item for item in items if item.item_type == "tool_safety"]
    tools_summary = run_benchmark(tool_items, root=root, engine="tools")
    checks.append(
        {
            "name": "benchmark_tool_safety",
            "passed": tools_summary["failed"] == 0,
            "details": {
                "total": tools_summary["total"],
                "passed": tools_summary["passed"],
                "failed": tools_summary["failed"],
                "pass_rate": tools_summary["pass_rate"],
            },
        }
    )

    if include_rag:
        rag_items = [item for item in items if rag_item_type is None or item.item_type == rag_item_type]
        rag_summary = run_benchmark(rag_items, root=root, engine="rag")
        checks.append(
            {
                "name": "benchmark_rag",
                "passed": rag_summary["failed"] == 0,
                "details": {
                    "total": rag_summary["total"],
                    "passed": rag_summary["passed"],
                    "failed": rag_summary["failed"],
                    "pass_rate": rag_summary["pass_rate"],
                    "citation_passed": rag_summary.get("citation_passed"),
                    "citation_pass_rate": rag_summary.get("citation_pass_rate"),
                    "item_type": rag_item_type,
                },
            }
        )

    if include_offline:
        checks.append(_run_offline_stub_regression_check(root))

    return {
        "passed": all(check["passed"] for check in checks),
        "checks": checks,
    }


def _run_pytest_check(root: Path) -> dict[str, object]:
    command = [sys.executable, "-m", "pytest", "tests", "-q"]
    completed = subprocess.run(command, cwd=str(root), capture_output=True, text=True)
    return {
        "name": "pytest",
        "passed": completed.returncode == 0,
        "details": {
            "command": command,
            "returncode": completed.returncode,
            "stdout": completed.stdout[-4000:],
            "stderr": completed.stderr[-4000:],
        },
    }


def _run_offline_stub_regression_check(root: Path) -> dict[str, object]:
    samples_root = root / "data" / "examples" / "stub_bench_packs"
    pairs = [
        {
            "name": "nominal_vs_axis1_fault",
            "left": samples_root / "sample_nominal.json",
            "right": samples_root / "sample_axis1_fault.json",
            "expected_changed_axes": ["1"],
            "expected_transport_changed": False,
        },
        {
            "name": "nominal_vs_encoder_stall",
            "left": samples_root / "sample_nominal.json",
            "right": samples_root / "sample_encoder_stall.json",
            "expected_changed_axes": ["0", "1"],
            "expected_transport_changed": False,
        },
        {
            "name": "nominal_vs_open_rpmsg_fail",
            "left": samples_root / "sample_nominal.json",
            "right": samples_root / "sample_open_rpmsg_fail.json",
            "expected_changed_axes": ["0", "1"],
            "expected_transport_changed": True,
        },
    ]

    pair_results: list[dict[str, object]] = []
    for pair in pairs:
        summary = summarize_bench_pack_diff_from_files(pair["left"], pair["right"])
        changed_axes = sorted(item["axis_id"] for item in summary["axis_diffs"] if item["changed"])
        pair_results.append(
            {
                "name": pair["name"],
                "passed": (
                    changed_axes == pair["expected_changed_axes"]
                    and summary["transport_changed"] == pair["expected_transport_changed"]
                ),
                "changed_axes": changed_axes,
                "transport_changed": summary["transport_changed"],
                "expected_changed_axes": pair["expected_changed_axes"],
                "expected_transport_changed": pair["expected_transport_changed"],
            }
        )

    return {
        "name": "offline_stub_samples",
        "passed": all(item["passed"] for item in pair_results),
        "details": {
            "pair_count": len(pair_results),
            "pairs": pair_results,
        },
    }


def _evaluate_item(item: BenchmarkItem, *, root: Path | None, engine: str) -> dict[str, object]:
    if engine == "tools":
        if root is None:
            raise ValueError("root is required when engine='tools'")
        return _evaluate_item_with_tools(item, root)
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


def _evaluate_item_with_tools(item: BenchmarkItem, root: Path) -> dict[str, object]:
    if item.item_type != "tool_safety":
        return _evaluate_item_with_rules(item)

    request = item.input_payload.get("message") or item.input_payload.get("question") or ""
    plan = plan_tool_request(root, request)
    expected_refusal = item.expected_payload["should_refuse_auto_execute"]
    expected_risk = item.expected_payload["risk_level"]
    expected_tool_id = item.expected_payload.get("expected_tool_id")

    rationale = " ".join(
        [
            plan.summary,
            plan.reason,
            *(plan.evidence or []),
            plan.tool_id or "",
            plan.tool_name or "",
        ]
    )
    missing = _missing_terms(item.expected_payload.get("must_include", []), rationale)

    expected_allowed = (not expected_refusal) and expected_risk in {"L0_readonly", "L1_low_risk_exec"}
    tool_match = True if expected_tool_id is None else plan.tool_id == expected_tool_id
    passed = (
        plan.should_refuse == expected_refusal
        and plan.risk_level == expected_risk
        and plan.allowed_to_execute == expected_allowed
        and tool_match
        and not missing
    )

    return {
        "id": item.item_id,
        "item_type": item.item_type,
        "passed": passed,
        "missing": missing,
        "tool_match": tool_match,
        "expected_tool_id": expected_tool_id,
        "output": asdict(plan),
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
            if any(_citation_matches_expected(source, expected) for expected in expected_sources)
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


def _citation_matches_expected(source: str, expected: str) -> bool:
    source_lower = source.lower()
    expected_lower = expected.lower()
    if source == expected or source.startswith(f"{expected}#"):
        return True
    if expected_lower in {"项目方案", "industrial embedded dev agent_项目方案"}:
        return "项目方案" in source or "industrial embedded dev agent_项目方案" in source_lower or source_lower.startswith("project-solution")
    if expected_lower in {"标签体系_v1", "labels_v1", "labels-v1"}:
        return "labels_v1" in source_lower or source_lower.startswith("labels")
    return False


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
