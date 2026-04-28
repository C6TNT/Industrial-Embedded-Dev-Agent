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
from .board_diagnostics import (
    board_report,
    board_status,
    ethercat_query_readonly,
    m7_health,
    rpmsg_health,
)
from .chunking import build_chunks, load_chunk_documents, summarize_chunks
from .config import get_project_paths
from .datasets import build_dataset_overview
from .engineering import (
    audit_hardware_action,
    draft_material_fact,
    draft_project_fact,
    gsd_status,
    import_real_report,
    project_status,
    run_gsd_offline,
    run_pre_push_check,
    scan_secrets,
    summarize_fake_regression,
    write_regression_overview,
)
from .material_workspace import (
    SEARCH_SCOPES,
    build_material_inventory,
    build_material_status,
    plan_material_tool,
    run_material_tool,
    search_material_workspace,
    summarize_material_tools,
)
from .rag import answer_with_rag
from .retrieval import build_search_documents, search_documents
from .runner import run_benchmark, run_local_checks_with_options
from .taxonomy import parse_taxonomy_labels, summarize_taxonomy
from .tools import (
    build_bench_pack,
    disable_wsl_stub_environment,
    finish_real_bench,
    get_execution_mode,
    inspect_wsl_environment,
    kickoff_real_bench,
    list_stub_scenarios,
    list_tools,
    apply_formal_merge,
    candidate_quality_check,
    canonical_merge_preflight,
    canonical_merge_checklist,
    canonical_merge_report,
    canonical_patch_helper,
    canonical_merge_preview_bundle,
    current_project_baseline,
    prepare_formal_merge_assistant,
    plan_pending_merge,
    plan_tool_request,
    promote_finish_candidates,
    prepare_real_bench_package,
    review_finish_candidates,
    run_tool_request,
    setup_wsl_stub_environment,
)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="spindle", description="Spindle industrial engineering agent CLI")
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
    check_parser.add_argument("--no-write-summary", action="store_true", help="Do not write a Markdown regression overview under reports/")
    check_parser.add_argument(
        "--rag-type",
        choices=["knowledge_qa", "log_attribution", "tool_safety"],
        help="Optional benchmark item_type filter when --include-rag is enabled",
    )

    tools_parser = subparsers.add_parser("tools", help="Inspect and invoke the guarded tool layer")
    tools_subparsers = tools_parser.add_subparsers(dest="tools_command", required=True)
    tools_subparsers.add_parser("list", help="List registered tool specs")
    tools_subparsers.add_parser("stub-scenarios", help="List the available no-hardware stub scenarios")
    tools_subparsers.add_parser("project-baseline", help="Show the active EtherCAT Dynamic Profile Agent baseline")
    tools_subparsers.add_parser("gsd-status", help="Show guarded GSD automation readiness and safety boundaries")
    tools_gsd_run_parser = tools_subparsers.add_parser("gsd-offline-run", help="Run the guarded offline GSD automation gate and write a report")
    tools_gsd_run_parser.add_argument("--include-history", action="store_true", help="Also scan git history during the static gate")
    tools_gsd_run_parser.add_argument("--no-write-report", action="store_true", help="Do not write reports/gsd_runs output")
    tools_gsd_run_parser.add_argument("--no-update-state", action="store_true", help="Do not update .planning/STATE.md")
    tools_project_status_parser = tools_subparsers.add_parser("project-status", help="Show project health, data shape, git state, and hardware boundaries")
    tools_project_status_parser.add_argument("--run-checks", action="store_true", help="Also run the full local offline check bundle")
    tools_board_status_parser = tools_subparsers.add_parser("board-status", help="Plan or run board-only read diagnostics for SSH and required files")
    _add_board_args(tools_board_status_parser)
    tools_rpmsg_health_parser = tools_subparsers.add_parser("rpmsg-health", help="Plan or run board-only read diagnostics for RPMsg device state")
    _add_board_args(tools_rpmsg_health_parser)
    tools_m7_health_parser = tools_subparsers.add_parser("m7-health", help="Plan or run board-only read diagnostics for M7 logs")
    _add_board_args(tools_m7_health_parser)
    tools_query_readonly_parser = tools_subparsers.add_parser("ethercat-query-readonly", help="Plan or run a read-only a53_send_ec_profile --query")
    _add_board_args(tools_query_readonly_parser)
    tools_board_report_parser = tools_subparsers.add_parser("board-report", help="Create a board-only diagnostic report; dry-run by default")
    _add_board_args(tools_board_report_parser)
    tools_board_report_parser.add_argument("--output-dir", help="Optional output directory for report JSON/Markdown")
    tools_board_report_parser.add_argument("--no-write-report", action="store_true", help="Do not write report files")
    tools_inventory_parser = tools_subparsers.add_parser("material-inventory", help="Build a search-first inventory of the external engineering material workspace")
    tools_inventory_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_inventory_parser.add_argument("--include-files", action="store_true", help="Include a bounded file list in the output")
    tools_inventory_parser.add_argument("--file-limit", type=int, default=200, help="Maximum files to include when --include-files is used")
    tools_material_status_parser = tools_subparsers.add_parser("material-status", help="Show material workspace health, search shape, and tool boundary summary")
    tools_material_status_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_search_parser = tools_subparsers.add_parser("material-search", help="Search raw files in the engineering material workspace")
    tools_search_parser.add_argument("query", help="Literal text to search for")
    tools_search_parser.add_argument("--scope", choices=sorted(SEARCH_SCOPES), default="all", help="Workspace slice to search")
    tools_search_parser.add_argument("--limit", type=int, default=20, help="Maximum hits")
    tools_search_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_search_parser.add_argument("--case-sensitive", action="store_true", help="Use case-sensitive matching")
    tools_material_tools_parser = tools_subparsers.add_parser("material-tools", help="Classify existing material-workspace tools by safety scope")
    tools_material_tools_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_material_tools_parser.add_argument(
        "--risk",
        choices=["offline_ok", "board_required", "robot_motion_required", "io_required", "firmware_required"],
        help="Filter by risk scope",
    )
    tools_material_tool_plan_parser = tools_subparsers.add_parser("material-tool-plan", help="Plan whether one material-workspace tool may run autonomously")
    tools_material_tool_plan_parser.add_argument("tool_path", help="Tool path relative to the material workspace")
    tools_material_tool_plan_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_material_run_tool_parser = tools_subparsers.add_parser("material-run-tool", help="Execute one offline_ok material-workspace tool through the safety gate")
    tools_material_run_tool_parser.add_argument("tool_path", help="Tool path relative to the material workspace")
    tools_material_run_tool_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_material_run_tool_parser.add_argument("--timeout", type=int, default=30, help="Execution timeout in seconds")
    tools_material_run_tool_parser.add_argument("tool_args", nargs=argparse.REMAINDER, help="Optional arguments passed to the offline tool")
    tools_secret_scan_parser = tools_subparsers.add_parser("secret-scan", help="Scan current repository files for common secrets before publishing")
    tools_secret_scan_parser.add_argument("--include-history", action="store_true", help="Also scan git history with git grep")
    tools_pre_push_parser = tools_subparsers.add_parser("pre-push-check", help="Run secret scan, git diff check, and optional local regression before pushing")
    tools_pre_push_parser.add_argument("--include-history", action="store_true", help="Also scan git history")
    tools_pre_push_parser.add_argument("--skip-local-checks", action="store_true", help="Only run secret scan and git diff checks")
    tools_pre_push_parser.add_argument("--include-offline", action="store_true", help="Include offline stub regression samples")
    tools_pre_push_parser.add_argument("--include-rag", action="store_true", help="Include RAG benchmark in the local regression check")
    tools_fact_parser = tools_subparsers.add_parser("draft-fact", help="Create a reviewable draft for a new project fact without touching canonical data")
    tools_fact_parser.add_argument("text", help="New project fact or test result text")
    tools_fact_parser.add_argument("--title", help="Optional short title")
    tools_fact_parser.add_argument("--source", help="Optional source path or report name")
    tools_fact_parser.add_argument("--category", help="Optional category override")
    tools_fact_parser.add_argument("--output-dir", help="Optional output directory")
    tools_material_fact_parser = tools_subparsers.add_parser("material-draft-fact", help="Create a source-checked curated memory draft from the material workspace")
    tools_material_fact_parser.add_argument("text", help="New source-backed project fact")
    tools_material_fact_parser.add_argument("--source", required=True, help="Source path relative to the material workspace")
    tools_material_fact_parser.add_argument("--title", help="Optional short title")
    tools_material_fact_parser.add_argument("--material-root", help="Override the material workspace root")
    tools_material_fact_parser.add_argument("--output-dir", help="Optional output directory")
    tools_import_report_parser = tools_subparsers.add_parser("import-report", help="Convert a real JSON report into LOG/replay draft files")
    tools_import_report_parser.add_argument("report", help="Path to a JSON report")
    tools_import_report_parser.add_argument("--output-dir", help="Optional output directory")
    tools_fake_summary_parser = tools_subparsers.add_parser("summarize-fake-regression", help="Summarize fake harness regression JSON into reviewable material")
    tools_fake_summary_parser.add_argument("input", help="Path to a regression JSON file or directory")
    tools_fake_summary_parser.add_argument("--output-dir", help="Optional output directory")
    tools_audit_parser = tools_subparsers.add_parser("audit-hardware-action", help="Explain hardware boundary and required conditions for a requested action")
    tools_audit_parser.add_argument("request", help="Natural-language action request")
    tools_subparsers.add_parser("mode", help="Show the current WSL execution mode")
    tools_subparsers.add_parser("doctor", help="Inspect the local WSL execution environment")
    tools_prep_real_parser = tools_subparsers.add_parser("prep-real-bench", help="Generate a ready-to-fill real-bench prep pack with checklist and templates")
    tools_prep_real_parser.add_argument("--session-id", help="Stable identifier for the upcoming real bench session")
    tools_prep_real_parser.add_argument("--label", help="Optional human-friendly label for the prep pack")
    tools_prep_real_parser.add_argument("--output-dir", help="Optional output directory for the generated prep pack")
    tools_finish_real_parser = tools_subparsers.add_parser("finish-real-bench", help="Collect session-level outputs and write a closing summary back into the prep bundle")
    tools_finish_real_parser.add_argument("--session-id", required=True, help="Session identifier used by prep-real-bench and kickoff-real-bench")
    tools_finish_real_parser.add_argument("--prep-dir", help="Optional prep bundle directory if it is not under reports/real_bench_prep/<session_id>")
    tools_quality_candidates_parser = tools_subparsers.add_parser("candidate-quality-check", help="Run a lightweight machine-first quality screen on finish-pack candidate exports")
    tools_quality_candidates_parser.add_argument("--session-id", required=True, help="Session identifier used by finish-real-bench")
    tools_quality_candidates_parser.add_argument("--prep-dir", help="Optional prep bundle directory if it is not under reports/real_bench_prep/<session_id>")
    tools_review_candidates_parser = tools_subparsers.add_parser("review-finish-candidates", help="Review the draft case/log/benchmark candidates generated by finish-real-bench")
    tools_review_candidates_parser.add_argument("--session-id", required=True, help="Session identifier used by finish-real-bench")
    tools_review_candidates_parser.add_argument("--prep-dir", help="Optional prep bundle directory if it is not under reports/real_bench_prep/<session_id>")
    tools_promote_candidates_parser = tools_subparsers.add_parser("promote-finish-candidates", help="Promote reviewed finish-pack candidates into the pending dataset area")
    tools_promote_candidates_parser.add_argument("--session-id", required=True, help="Session identifier used by finish-real-bench")
    tools_promote_candidates_parser.add_argument("--prep-dir", help="Optional prep bundle directory if it is not under reports/real_bench_prep/<session_id>")
    tools_subparsers.add_parser("plan-pending-merge", help="Generate a merge plan for items currently staged under data/pending")
    tools_subparsers.add_parser("prepare-formal-merge", help="Generate draft bundles and append-ready files for curated formal dataset updates")
    tools_apply_formal_merge_parser = tools_subparsers.add_parser("apply-formal-merge", help="Prepare dry-run output or stage formal-merge patch files without touching canonical data")
    tools_apply_formal_merge_parser.add_argument("--dry-run", action="store_true", help="Generate review output only and do not create staging files")
    tools_apply_formal_merge_parser.add_argument("--execute", action="store_true", help="Write patch suggestions into a staging directory, without modifying canonical dataset files")
    tools_subparsers.add_parser("canonical-merge-preflight", help="Run read-only readiness checks before considering a canonical dataset merge")
    tools_subparsers.add_parser("canonical-patch-helper", help="Generate canonical patch files and a manifest without modifying canonical dataset files")
    tools_subparsers.add_parser("canonical-merge-preview", help="Generate preview files that show what canonical merge results could look like, without modifying canonical data")
    tools_subparsers.add_parser("canonical-merge-report", help="Generate one overview bundle that links the canonical preflight, patch, and preview outputs")
    tools_subparsers.add_parser("canonical-merge-checklist", help="Generate a final read-only checklist for whether manual canonical merge review can begin")
    tools_kickoff_real_parser = tools_subparsers.add_parser("kickoff-real-bench", help="Read a real-bench plan seed and create the first read-only bench-pack")
    tools_kickoff_real_parser.add_argument("seed", help="Path to a generated plan_seed.json")
    tools_kickoff_real_parser.add_argument("--execute", action="store_true", help="Actually execute the seeded read-only request")
    tools_kickoff_real_parser.add_argument("--render-first-run", action="store_true", help="Also render the generated bench-pack into a first-run Markdown draft")
    tools_kickoff_real_parser.add_argument("--render-session-review", action="store_true", help="Also render the generated bench-pack into a session-review Markdown draft")
    tools_kickoff_real_parser.add_argument("--render-all", action="store_true", help="Render both the first-run and session-review Markdown drafts")
    tools_kickoff_real_parser.add_argument("--timeout", type=int, default=20, help="Execution timeout in seconds")
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


def _add_board_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--host", default="192.168.3.33", help="Board IP or host name")
    parser.add_argument("--user", default="root", help="SSH user")
    parser.add_argument("--port", type=int, default=22, help="SSH port")
    parser.add_argument("--timeout", type=int, default=8, help="SSH/connect timeout in seconds")
    parser.add_argument("--execute", action="store_true", help="Actually run the read-only board diagnostic command")


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
        check_payload = run_local_checks_with_options(
            paths.root,
            include_rag=args.include_rag,
            rag_item_type=args.rag_type,
            include_offline=args.include_offline,
        )
        if not args.no_write_summary:
            check_payload["regression_overview"] = write_regression_overview(paths.root, check_payload)
        _print_json(check_payload)
        return

    if args.command == "tools":
        if args.tools_command == "list":
            _print_json(list_tools(paths.root))
            return
        if args.tools_command == "stub-scenarios":
            _print_json(list_stub_scenarios())
            return
        if args.tools_command == "project-baseline":
            _print_json(current_project_baseline(paths.root))
            return
        if args.tools_command == "gsd-status":
            _print_json(gsd_status(paths.root))
            return
        if args.tools_command == "gsd-offline-run":
            _print_json(
                run_gsd_offline(
                    paths.root,
                    include_history=args.include_history,
                    write_report=not args.no_write_report,
                    update_state=not args.no_update_state,
                )
            )
            return
        if args.tools_command == "project-status":
            _print_json(project_status(paths.root, run_checks=args.run_checks))
            return
        if args.tools_command == "board-status":
            _print_json(
                board_status(
                    host=args.host,
                    user=args.user,
                    port=args.port,
                    execute=args.execute,
                    timeout_seconds=args.timeout,
                )
            )
            return
        if args.tools_command == "rpmsg-health":
            _print_json(
                rpmsg_health(
                    host=args.host,
                    user=args.user,
                    port=args.port,
                    execute=args.execute,
                    timeout_seconds=args.timeout,
                )
            )
            return
        if args.tools_command == "m7-health":
            _print_json(
                m7_health(
                    host=args.host,
                    user=args.user,
                    port=args.port,
                    execute=args.execute,
                    timeout_seconds=args.timeout,
                )
            )
            return
        if args.tools_command == "ethercat-query-readonly":
            _print_json(
                ethercat_query_readonly(
                    host=args.host,
                    user=args.user,
                    port=args.port,
                    execute=args.execute,
                    timeout_seconds=args.timeout,
                )
            )
            return
        if args.tools_command == "board-report":
            _print_json(
                board_report(
                    paths.root,
                    host=args.host,
                    user=args.user,
                    port=args.port,
                    execute=args.execute,
                    timeout_seconds=args.timeout,
                    output_dir=Path(args.output_dir) if args.output_dir else None,
                    write_report=not args.no_write_report,
                )
            )
            return
        if args.tools_command == "material-inventory":
            _print_json(
                build_material_inventory(
                    paths.root,
                    material_root=Path(args.material_root) if args.material_root else None,
                    include_files=args.include_files,
                    file_limit=args.file_limit,
                )
            )
            return
        if args.tools_command == "material-status":
            _print_json(
                build_material_status(
                    paths.root,
                    material_root=Path(args.material_root) if args.material_root else None,
                )
            )
            return
        if args.tools_command == "material-search":
            _print_json(
                search_material_workspace(
                    paths.root,
                    args.query,
                    scope=args.scope,
                    limit=args.limit,
                    material_root=Path(args.material_root) if args.material_root else None,
                    case_sensitive=args.case_sensitive,
                )
            )
            return
        if args.tools_command == "material-tools":
            _print_json(
                summarize_material_tools(
                    paths.root,
                    material_root=Path(args.material_root) if args.material_root else None,
                    risk=args.risk,
                )
            )
            return
        if args.tools_command == "material-tool-plan":
            _print_json(
                plan_material_tool(
                    paths.root,
                    args.tool_path,
                    material_root=Path(args.material_root) if args.material_root else None,
                )
            )
            return
        if args.tools_command == "material-run-tool":
            tool_args = list(args.tool_args)
            if tool_args and tool_args[0] == "--":
                tool_args = tool_args[1:]
            _print_json(
                run_material_tool(
                    paths.root,
                    args.tool_path,
                    material_root=Path(args.material_root) if args.material_root else None,
                    tool_args=tool_args,
                    timeout_seconds=args.timeout,
                )
            )
            return
        if args.tools_command == "secret-scan":
            _print_json(scan_secrets(paths.root, include_history=args.include_history))
            return
        if args.tools_command == "pre-push-check":
            _print_json(
                run_pre_push_check(
                    paths.root,
                    include_history=args.include_history,
                    run_checks=not args.skip_local_checks,
                    include_offline=args.include_offline,
                    include_rag=args.include_rag,
                )
            )
            return
        if args.tools_command == "draft-fact":
            _print_json(
                draft_project_fact(
                    paths.root,
                    args.text,
                    title=args.title,
                    source=args.source,
                    category=args.category,
                    output_dir=Path(args.output_dir) if args.output_dir else None,
                )
            )
            return
        if args.tools_command == "material-draft-fact":
            _print_json(
                draft_material_fact(
                    paths.root,
                    args.text,
                    source_path=args.source,
                    title=args.title,
                    material_root=Path(args.material_root) if args.material_root else None,
                    output_dir=Path(args.output_dir) if args.output_dir else None,
                )
            )
            return
        if args.tools_command == "import-report":
            _print_json(
                import_real_report(
                    paths.root,
                    Path(args.report),
                    output_dir=Path(args.output_dir) if args.output_dir else None,
                )
            )
            return
        if args.tools_command == "summarize-fake-regression":
            _print_json(
                summarize_fake_regression(
                    paths.root,
                    Path(args.input),
                    output_dir=Path(args.output_dir) if args.output_dir else None,
                )
            )
            return
        if args.tools_command == "audit-hardware-action":
            _print_json(audit_hardware_action(paths.root, args.request))
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
        if args.tools_command == "finish-real-bench":
            _print_json(
                finish_real_bench(
                    paths.root,
                    session_id=args.session_id,
                    prep_dir=Path(args.prep_dir) if args.prep_dir else None,
                )
            )
            return
        if args.tools_command == "candidate-quality-check":
            _print_json(
                candidate_quality_check(
                    paths.root,
                    session_id=args.session_id,
                    prep_dir=Path(args.prep_dir) if args.prep_dir else None,
                )
            )
            return
        if args.tools_command == "review-finish-candidates":
            _print_json(
                review_finish_candidates(
                    paths.root,
                    session_id=args.session_id,
                    prep_dir=Path(args.prep_dir) if args.prep_dir else None,
                )
            )
            return
        if args.tools_command == "promote-finish-candidates":
            _print_json(
                promote_finish_candidates(
                    paths.root,
                    session_id=args.session_id,
                    prep_dir=Path(args.prep_dir) if args.prep_dir else None,
                )
            )
            return
        if args.tools_command == "plan-pending-merge":
            _print_json(plan_pending_merge(paths.root))
            return
        if args.tools_command == "prepare-formal-merge":
            _print_json(prepare_formal_merge_assistant(paths.root))
            return
        if args.tools_command == "apply-formal-merge":
            _print_json(apply_formal_merge(paths.root, dry_run=not args.execute))
            return
        if args.tools_command == "canonical-merge-preflight":
            _print_json(canonical_merge_preflight(paths.root))
            return
        if args.tools_command == "canonical-patch-helper":
            _print_json(canonical_patch_helper(paths.root))
            return
        if args.tools_command == "canonical-merge-preview":
            _print_json(canonical_merge_preview_bundle(paths.root))
            return
        if args.tools_command == "canonical-merge-report":
            _print_json(canonical_merge_report(paths.root))
            return
        if args.tools_command == "canonical-merge-checklist":
            _print_json(canonical_merge_checklist(paths.root))
            return
        if args.tools_command == "kickoff-real-bench":
            render_first_run = args.render_first_run or args.render_all
            render_session_review = args.render_session_review or args.render_all
            _print_json(
                kickoff_real_bench(
                    paths.root,
                    Path(args.seed),
                    execute=args.execute,
                    render_first_run=render_first_run,
                    render_session_review=render_session_review,
                    timeout_seconds=args.timeout,
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
