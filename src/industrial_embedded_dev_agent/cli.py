from __future__ import annotations

import argparse
import json
from dataclasses import asdict
from pathlib import Path

from .analysis import analyze_text
from .benchmarks import filter_benchmark_items, load_benchmark_items, summarize_benchmark
from .chunking import build_chunks, summarize_chunks
from .config import get_project_paths
from .datasets import build_dataset_overview
from .rag import answer_with_rag
from .retrieval import build_search_documents, search_documents
from .runner import run_benchmark
from .taxonomy import parse_taxonomy_labels, summarize_taxonomy


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="ieda", description="Industrial Embedded Dev Agent MVP CLI")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("overview", help="Show a project dataset overview")

    benchmark_parser = subparsers.add_parser("benchmark", help="Inspect benchmark assets")
    benchmark_subparsers = benchmark_parser.add_subparsers(dest="benchmark_command", required=True)
    benchmark_subparsers.add_parser("summary", help="Summarize benchmark items")

    benchmark_run_parser = benchmark_subparsers.add_parser("run", help="Run the rule-based benchmark baseline")
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
    return parser


def _print_json(payload: object) -> None:
    print(json.dumps(payload, ensure_ascii=False, indent=2))


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
            _print_json(run_benchmark(filtered))
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
        chunk_output = paths.root / "data" / "chunks" / "doc_chunks_v1.jsonl"
        if args.chunks_command == "build":
            chunks = build_chunks(paths.root, output_path=chunk_output)
            _print_json(summarize_chunks(chunks))
            return
        if args.chunks_command == "summary":
            from .chunking import load_chunk_documents

            chunks = load_chunk_documents(chunk_output)
            _print_json(summarize_chunks(chunks))
            return
