from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .bench_pack_render import (
    compare_bench_packs,
    compare_latest_bench_packs_in_session,
    render_bench_pack_markdown,
    render_session_bundle_markdown,
    render_sessions_index_markdown,
    summarize_bench_sessions,
)
from .benchmarks import filter_benchmark_items, load_benchmark_items, summarize_benchmark
from .chunking import build_chunks, load_chunk_documents, summarize_chunks
from .config import get_project_paths
from .datasets import build_dataset_overview
from .rag import answer_with_rag
from .retrieval import build_search_documents, search_documents
from .runner import run_benchmark, run_local_checks_with_options
from .taxonomy import parse_taxonomy_labels, summarize_taxonomy
from .tools import (
    build_bench_pack,
    disable_wsl_stub_environment,
    get_execution_mode,
    inspect_wsl_environment,
    list_stub_scenarios,
    list_tools,
    plan_tool_request,
    prepare_real_bench_package,
    run_tool_request,
    setup_wsl_stub_environment,
)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="ieda", description="Industrial Embedded Dev Agent MVP CLI")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("overview", help="Show a project dataset overview")

    benchmark_parser = subparsers.add_parser("benchmark", help="Inspect benchmark assets")
    benchmark_subparsers = benchmark_parser.add_subparsers(dest="benchmark_command", required=True)
    benchmark_subparsers.add_parser("summary", help="Summarize benchmark items")

    benchmark_run_parser = benchmark_subparsers.add_parser("run", help="Run a benchmark engine against the seed set")
    benchmark_run_parser.add_argument(
        "--engine",
        choices=["rules", "rag", "tools"],
        default="rules",
        help="Benchmark engine to execute",
    )
    benchmark_run_parser.add_argument("--type", dest="item_type", help="Filter benchmark run by item_type")
    benchmark_run_parser.add_argument("--tag", help="Filter benchmark run by tag")

    benchmark_list_parser = benchmark_subparsers.add_parser("list", help="List benchmark items")
    benchmark_list_parser.add_argument("--type", dest="item_type", help="Filter by item_type")
    benchmark_list_parser.add_argument("--tag", help="Filter by tag")

    taxonomy_parser = subparsers.add_parser("taxonomy", help="Inspect taxonomy labels")
    taxonomy_subparsers = taxonomy_parser.add_subparsers(dest="taxonomy_command", required=True)
    taxonomy_subparsers.add_parser("summary", help="Summarize taxonomy groups")

    taxonomy_list_parser = taxonomy_subparsers.add_parser("list", help="List taxonomy labels")
    taxonomy_list_parser.add_argument(
        "--group",
        choices=["issue_categories", "cause_labels", "action_labels", "risk_labels"],
        required=True,
        help="Select which taxonomy group to print",
    )

    search_parser = subparsers.add_parser("search", help="Search normalized project assets")
    search_parser.add_argument("query", help="Search query text")
    search_parser.add_argument("--limit", type=int, default=5, help="Maximum number of hits")

    analyze_parser = subparsers.add_parser("analyze", help="Produce structured baseline analysis")
    analyze_parser.add_argument("text", help="Input text to analyze")
    analyze_parser.add_argument("--mode", choices=["auto", "general", "log", "safety"], default="auto")

    ask_parser = subparsers.add_parser("ask", help="Answer a question with retrieval-augmented generation")
    ask_parser.add_argument("question", help="Question text")
    ask_parser.add_argument("--limit", type=int, default=5, help="Maximum number of retrieved references")
    ask_parser.add_argument(
        "--include-benchmark",
        action="store_true",
        help="Allow benchmark questions to participate in retrieval",
    )

    chunks_parser = subparsers.add_parser("chunks", help="Build and inspect document chunks")
    chunks_subparsers = chunks_parser.add_subparsers(dest="chunks_command", required=True)
    chunks_subparsers.add_parser("build", help="Build normalized document chunks under data/chunks")
    chunks_subparsers.add_parser("summary", help="Summarize the currently built chunks")

    check_parser = subparsers.add_parser("check", help="Run the local regression bundle: pytest, rules benchmark, and tool-safety benchmark")
    check_parser.add_argument("--include-rag", action="store_true", help="Also run the RAG benchmark as part of the local check bundle")
    check_parser.add_argument("--include-offline", action="store_true", help="Also validate the curated offline stub sample compare set")
    check_parser.add_argument(
        "--rag-type",
        choices=["knowledge_qa", "log_attribution", "tool_safety"],
        help="Optional benchmark item_type filter when --include-rag is enabled",
    )

    tools_parser = subparsers.add_parser("tools", help="Inspect and invoke the guarded tool layer")
    tools_subparsers = tools_parser.add_subparsers(dest="tools_command", required=True)
    tools_subparsers.add_parser("list", help="List registered tool specs")
    tools_subparsers.add_parser("stub-scenarios", help="List the available no-hardware stub scenarios")
    tools_subparsers.add_parser("mode", help="Show the current WSL execution mode")
    tools_subparsers.add_parser("doctor", help="Inspect the local WSL execution environment")
    tools_prep_real_parser = tools_subparsers.add_parser("prep-real-bench", help="Generate a ready-to-fill real-bench prep pack with checklist and templates")
    tools_prep_real_parser.add_argument("--session-id", help="Stable identifier for the upcoming real bench session")
    tools_prep_real_parser.add_argument("--label", help="Optional human-friendly label for the prep pack")
    tools_prep_real_parser.add_argument("--output-dir", help="Optional output directory for the generated prep pack")
    tools_setup_stub_parser = tools_subparsers.add_parser("setup-stub", help="Enable the no-hardware WSL stub mode for local testing")
    tools_setup_stub_parser.add_argument("--scenario", default="nominal", help="Stub scenario name, for example nominal or axis1_fault")
    tools_subparsers.add_parser("use-real", help="Disable stub mode and switch back to real-hardware execution")

    tools_pack_parser = tools_subparsers.add_parser("bench-pack", help="Capture mode, doctor, plan, and execution into one JSON bundle")
    tools_pack_parser.add_argument("request", help="Natural-language tool request")
    tools_pack_parser.add_argument("--tool", dest="tool_id", help="Force a specific tool_id")
    tools_pack_parser.add_argument("--session-id", help="Group this bench-pack under a stable session identifier")
    tools_pack_parser.add_argument("--label", help="Optional human-friendly label for the current bench session")
    tools_pack_parser.add_argument("--no-execute", action="store_true", help="Capture plan-only without actual execution")
    tools_pack_parser.add_argument("--timeout", type=int, default=20, help="Execution timeout in seconds")
    tools_pack_parser.add_argument("--output", help="Optional JSON output path")

    tools_render_parser = tools_subparsers.add_parser("render-pack", help="Render a bench-pack JSON into a Markdown draft")
    tools_render_parser.add_argument("input", help="Path to a bench-pack JSON file")
    tools_render_parser.add_argument("--template", choices=["first-run", "issue", "session-review"], required=True, help="Markdown draft type")
    tools_render_parser.add_argument("--output", help="Optional Markdown output path")

    tools_render_session_parser = tools_subparsers.add_parser("render-session", help="Render all bench-packs under one session_id into a consolidated Markdown review")
    tools_render_session_parser.add_argument("session_id", help="Session identifier created by tools bench-pack --session-id")
    tools_render_session_parser.add_argument("--output", help="Optional Markdown output path")

    tools_sessions_parser = tools_subparsers.add_parser("sessions", help="Summarize captured bench sessions")
    tools_sessions_parser.add_argument("--render-index", action="store_true", help="Also render a Markdown index page under reports/bench_packs/rendered")
    tools_sessions_parser.add_argument("--output", help="Optional Markdown output path when used with --render-index")

    tools_compare_parser = tools_subparsers.add_parser("compare-pack", help="Compare two bench-pack JSON files, or auto-compare the latest two packs in one session")
    tools_compare_parser.add_argument("left", nargs="?", help="Path to the earlier or baseline bench-pack JSON")
    tools_compare_parser.add_argument("right", nargs="?", help="Path to the later bench-pack JSON")
    tools_compare_parser.add_argument("--session-id", help="Auto-select the latest two bench packs under this session_id")
    tools_compare_parser.add_argument("--output", help="Optional Markdown output path")

    tools_plan_parser = tools_subparsers.add_parser("plan", help="Plan a tool call without executing it")
    tools_plan_parser.add_argument("request", help="Natural-language tool request")
    tools_plan_parser.add_argument("--tool", dest="tool_id", help="Force a specific tool_id")

    tools_run_parser = tools_subparsers.add_parser("run", help="Plan or execute a tool call")
    tools_run_parser.add_argument("request", help="Natural-language tool request")
    tools_run_parser.add_argument("--tool", dest="tool_id", help="Force a specific tool_id")
    tools_run_parser.add_argument("--execute", action="store_true", help="Actually execute the allowed tool")
    tools_run_parser.add_argument("--timeout", type=int, default=20, help="Execution timeout in seconds")
    return parser


def _print_json(payload: object) -> None:
    text = json.dumps(payload, ensure_ascii=False, indent=2)
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass
    try:
        print(text)
    except UnicodeEncodeError:
        sys.stdout.buffer.write(text.encode("utf-8"))
        sys.stdout.buffer.write(b"\n")


def _benchmark_path(root: Path) -> Path:
    return root / "data" / "benchmark" / "benchmark_v1.jsonl"


def _taxonomy_path(root: Path) -> Path:
    return root / "data" / "taxonomy" / "labels_v1.md"


def main() -> None:
    parser = _build_parser()
    args = parser.parse_args()
    paths = get_project_paths()

    if args.command == "overview":
        overview = build_dataset_overview(paths)
        _print_json(
            {
                "benchmark_total": overview.benchmark_total,
                "benchmark_by_type": overview.benchmark_by_type,
                "taxonomy_groups": overview.taxonomy_groups,
                "material_counts": overview.material_counts,
            }
        )
        return

    if args.command == "benchmark":
        items = load_benchmark_items(_benchmark_path(paths.root))
        if args.benchmark_command == "summary":
            _print_json(summarize_benchmark(items))
            return
        if args.benchmark_command == "run":
            filtered = filter_benchmark_items(items, item_type=args.item_type, tag=args.tag)
            _print_json(run_benchmark(filtered, root=paths.root, engine=args.engine))
            return
        if args.benchmark_command == "list":
            filtered = filter_benchmark_items(items, item_type=args.item_type, tag=args.tag)
            _print_json(
                [
                    {
                        "id": item.item_id,
                        "item_type": item.item_type,
                        "difficulty": item.difficulty,
                        "tags": item.tags,
                    }
                    for item in filtered
                ]
            )
            return

    if args.command == "taxonomy":
        taxonomy_path = _taxonomy_path(paths.root)
        if args.taxonomy_command == "summary":
            _print_json(summarize_taxonomy(taxonomy_path))
            return
        if args.taxonomy_command == "list":
            groups = parse_taxonomy_labels(taxonomy_path)
            _print_json(groups[args.group])
            return

    if args.command == "search":
        documents = build_search_documents(paths.root)
        hits = search_documents(documents, args.query, limit=args.limit)
        _print_json([asdict(hit) for hit in hits])
        return

    if args.command == "analyze":
        diagnosis = analyze_text(args.text, mode=args.mode)
        _print_json(asdict(diagnosis))
        return

    if args.command == "ask":
        result = answer_with_rag(
            paths.root,
            args.question,
            limit=args.limit,
            include_benchmark=args.include_benchmark,
        )
        _print_json(asdict(result))
        return

    if args.command == "chunks":
        chunk_output = paths.chunks_dir / "doc_chunks_v1.jsonl"
        if args.chunks_command == "build":
            chunks = build_chunks(paths.root, output_path=chunk_output)
            _print_json(summarize_chunks(chunks))
            return
        if args.chunks_command == "summary":
            chunks = load_chunk_documents(chunk_output)
            _print_json(summarize_chunks(chunks))
            return

    if args.command == "check":
        _print_json(
            run_local_checks_with_options(
                paths.root,
                include_rag=args.include_rag,
                rag_item_type=args.rag_type,
                include_offline=args.include_offline,
            )
        )
        return

    if args.command == "tools":
        if args.tools_command == "list":
            _print_json(list_tools(paths.root))
            return
        if args.tools_command == "stub-scenarios":
            _print_json(list_stub_scenarios())
            return
        if args.tools_command == "mode":
            _print_json(get_execution_mode(paths.root))
            return
        if args.tools_command == "doctor":
            _print_json(inspect_wsl_environment(paths.root))
            return
        if args.tools_command == "prep-real-bench":
            _print_json(
                prepare_real_bench_package(
                    paths.root,
                    session_id=args.session_id,
                    label=args.label,
                    output_dir=Path(args.output_dir) if args.output_dir else None,
                )
            )
            return
        if args.tools_command == "setup-stub":
            _print_json(setup_wsl_stub_environment(paths.root, scenario=args.scenario))
            return
        if args.tools_command == "use-real":
            _print_json(disable_wsl_stub_environment(paths.root))
            return
        if args.tools_command == "bench-pack":
            _print_json(
                build_bench_pack(
                    paths.root,
                    args.request,
                    tool_id=args.tool_id,
                    session_id=args.session_id,
                    label=args.label,
                    execute=not args.no_execute,
                    timeout_seconds=args.timeout,
                    output_path=Path(args.output) if args.output else None,
                )
            )
            return
        if args.tools_command == "render-pack":
            _print_json(
                render_bench_pack_markdown(
                    paths.root,
                    Path(args.input),
                    template=args.template,
                    output_path=Path(args.output) if args.output else None,
                )
            )
            return
        if args.tools_command == "render-session":
            _print_json(
                render_session_bundle_markdown(
                    paths.root,
                    args.session_id,
                    output_path=Path(args.output) if args.output else None,
                )
            )
            return
        if args.tools_command == "sessions":
            if args.render_index:
                _print_json(
                    render_sessions_index_markdown(
                        paths.root,
                        output_path=Path(args.output) if args.output else None,
                    )
                )
                return
            _print_json(summarize_bench_sessions(paths.root))
            return
        if args.tools_command == "compare-pack":
            if args.session_id:
                _print_json(
                    compare_latest_bench_packs_in_session(
                        paths.root,
                        args.session_id,
                        output_path=Path(args.output) if args.output else None,
                    )
                )
                return
            if not args.left or not args.right:
                parser.error("tools compare-pack requires either --session-id or both left and right paths")
            _print_json(
                compare_bench_packs(
                    paths.root,
                    Path(args.left),
                    Path(args.right),
                    output_path=Path(args.output) if args.output else None,
                )
            )
            return
        if args.tools_command == "plan":
            _print_json(asdict(plan_tool_request(paths.root, args.request, tool_id=args.tool_id)))
            return
        if args.tools_command == "run":
            _print_json(
                run_tool_request(
                    paths.root,
                    args.request,
                    tool_id=args.tool_id,
                    execute=args.execute,
                    timeout_seconds=args.timeout,
                )
            )
            return
