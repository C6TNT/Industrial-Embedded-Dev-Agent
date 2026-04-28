from __future__ import annotations

import json
import re
import subprocess
import sys
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable


TEXT_EXTENSIONS = {
    ".bat",
    ".c",
    ".cfg",
    ".cmake",
    ".cpp",
    ".csv",
    ".h",
    ".hpp",
    ".ini",
    ".json",
    ".jsonl",
    ".ld",
    ".log",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".toml",
    ".txt",
    ".xml",
    ".yaml",
    ".yml",
}

SKIP_DIRS = {
    ".git",
    ".pytest_cache",
    "__pycache__",
    "CMakeFiles",
    "ddr_debug",
    "ddr_release",
    "debug",
    "release",
}

NOISY_DIRS = {
    "reports",
    "replay_cases",
    "generated",
}

SEARCH_SCOPES = {
    "all": [],
    "source": [
        "mix_protocol/bsp",
        "mix_protocol/freertos",
        "mix_protocol/libtpr20pro",
        "mix_protocol/main_remote.c",
    ],
    "tools": [
        "mix_protocol/tools",
        "tools",
    ],
    "docs": [
        "README.md",
        "mix_protocol/.codex-memory.md",
        "mix_protocol/docs",
        "docs",
    ],
    "xml": [
        "assets/xml",
        "mix_protocol",
    ],
    "profiles": [
        "profiles",
        "tools/generated",
        "mix_protocol/tools/generated",
    ],
    "logs": [
        "logs",
        "mix_protocol/tools/generated",
    ],
}

RISK_ORDER = {
    "offline_ok": 0,
    "board_required": 1,
    "robot_motion_required": 2,
    "io_required": 3,
    "firmware_required": 4,
}


@dataclass(frozen=True)
class MaterialFile:
    path: str
    area: str
    kind: str
    size: int
    searchable: bool


@dataclass(frozen=True)
class MaterialSearchHit:
    path: str
    line: int
    text: str
    area: str
    kind: str


@dataclass(frozen=True)
class MaterialTool:
    path: str
    risk: str
    reason: str
    recommended_action: str


def resolve_material_workspace(root: Path, material_root: Path | None = None) -> Path:
    if material_root is not None:
        return material_root.resolve()
    config_path = root / ".planning" / "config.json"
    if config_path.exists():
        try:
            raw = json.loads(config_path.read_text(encoding="utf-8"))
            configured = raw.get("material_workspace")
            if configured:
                return Path(str(configured)).resolve()
        except json.JSONDecodeError:
            pass
    fallback = root.parent / "test"
    return fallback.resolve()


def build_material_inventory(
    root: Path,
    *,
    material_root: Path | None = None,
    include_files: bool = False,
    file_limit: int = 200,
) -> dict[str, object]:
    workspace = resolve_material_workspace(root, material_root)
    files = list(iter_material_files(workspace))
    area_counts = Counter(item.area for item in files)
    kind_counts = Counter(item.kind for item in files)
    searchable_count = sum(1 for item in files if item.searchable)
    total_size = sum(item.size for item in files)
    important = _important_inventory_files(files)
    payload: dict[str, object] = {
        "material_root": str(workspace),
        "exists": workspace.exists(),
        "total_files": len(files),
        "total_size_bytes": total_size,
        "searchable_files": searchable_count,
        "area_counts": dict(sorted(area_counts.items())),
        "kind_counts": dict(sorted(kind_counts.items())),
        "important_files": [asdict(item) for item in important[:file_limit]],
        "tool_risk_summary": summarize_material_tools(root, material_root=workspace)["risk_counts"],
    }
    if include_files:
        payload["files"] = [asdict(item) for item in files[:file_limit]]
        payload["files_truncated"] = len(files) > file_limit
    return payload


def build_material_status(
    root: Path,
    *,
    material_root: Path | None = None,
) -> dict[str, object]:
    """Return a compact dashboard for the external engineering material workspace."""

    workspace = resolve_material_workspace(root, material_root)
    inventory = build_material_inventory(root, material_root=workspace, include_files=False, file_limit=20)
    tools = summarize_material_tools(root, material_root=workspace)
    risk_counts = tools["risk_counts"]
    offline_tools = int(risk_counts.get("offline_ok", 0))
    total_tools = int(tools["total_tools"])
    blocked_tools = max(total_tools - offline_tools, 0)
    return {
        "material_root": str(workspace),
        "exists": inventory["exists"],
        "total_files": inventory["total_files"],
        "searchable_files": inventory["searchable_files"],
        "top_areas": _top_counts(inventory["area_counts"]),
        "top_kinds": _top_counts(inventory["kind_counts"]),
        "tool_risk_counts": risk_counts,
        "offline_tool_count": offline_tools,
        "blocked_tool_count": blocked_tools,
        "status": "ready_for_offline_search" if inventory["exists"] else "material_workspace_missing",
        "recommended_commands": [
            "spindle tools material-search \"0x41F1\" --scope docs",
            "spindle tools material-tools --risk offline_ok",
            "spindle tools material-tool-plan <material-relative-tool-path>",
            "spindle tools material-run-tool <offline-ok-material-tool-path>",
            "spindle tools material-draft-fact \"<fact>\" --source <material-relative-source>",
        ],
        "hardware_boundary": {
            "autonomous_execution": "offline_ok only",
            "blocked_without_hardware_window": [
                "board_required",
                "robot_motion_required",
                "io_required",
                "firmware_required",
            ],
        },
    }


def search_material_workspace(
    root: Path,
    query: str,
    *,
    scope: str = "all",
    limit: int = 20,
    material_root: Path | None = None,
    case_sensitive: bool = False,
    max_file_bytes: int = 2_000_000,
) -> dict[str, object]:
    workspace = resolve_material_workspace(root, material_root)
    pattern = re.compile(re.escape(query), 0 if case_sensitive else re.IGNORECASE)
    hits: list[MaterialSearchHit] = []
    max_hits = max(limit * 5, limit)
    searched_files = 0
    skipped_files = 0

    for file_info in iter_material_files(workspace, scope=scope):
        if not file_info.searchable or file_info.size > max_file_bytes:
            skipped_files += 1
            continue
        searched_files += 1
        path = workspace / file_info.path
        text = _read_text_or_none(path)
        if text is None:
            skipped_files += 1
            continue
        for lineno, line in enumerate(text.splitlines(), 1):
            if not pattern.search(line):
                continue
            hits.append(
                MaterialSearchHit(
                    path=file_info.path,
                    line=lineno,
                    text=_trim_line(line),
                    area=file_info.area,
                    kind=file_info.kind,
                )
            )
            if len(hits) >= max_hits:
                return _search_payload(workspace, query, scope, searched_files, skipped_files, hits, limit=limit, truncated=True)

    return _search_payload(workspace, query, scope, searched_files, skipped_files, hits, limit=limit, truncated=False)


def summarize_material_tools(
    root: Path,
    *,
    material_root: Path | None = None,
    risk: str | None = None,
) -> dict[str, object]:
    workspace = resolve_material_workspace(root, material_root)
    tools_root = workspace / "mix_protocol" / "tools"
    tools: list[MaterialTool] = []
    if tools_root.exists():
        for path in sorted(tools_root.rglob("*")):
            if not path.is_file():
                continue
            if path.suffix.lower() not in {".py", ".cpp", ".h", ".bat", ".sh", ".ps1"}:
                continue
            rel = _relative_posix(path, workspace)
            tool = classify_material_tool(rel)
            if risk is None or tool.risk == risk:
                tools.append(tool)
    risk_counts = Counter(item.risk for item in tools)
    return {
        "material_root": str(workspace),
        "tools_root": str(tools_root),
        "total_tools": len(tools),
        "risk_counts": dict(sorted(risk_counts.items(), key=lambda item: RISK_ORDER.get(item[0], 99))),
        "tools": [asdict(item) for item in sorted(tools, key=lambda item: (RISK_ORDER.get(item.risk, 99), item.path))],
    }


def plan_material_tool(
    root: Path,
    tool_path: str,
    *,
    material_root: Path | None = None,
) -> dict[str, object]:
    workspace = resolve_material_workspace(root, material_root)
    normalized = tool_path.replace("\\", "/")
    if normalized.startswith(str(workspace).replace("\\", "/")):
        try:
            normalized = Path(tool_path).resolve().relative_to(workspace).as_posix()
        except ValueError:
            pass
    tool = classify_material_tool(normalized)
    full_path = workspace / normalized
    allowed = tool.risk == "offline_ok"
    command = _suggest_tool_command(normalized, full_path) if allowed else []
    return {
        "material_root": str(workspace),
        "tool": asdict(tool),
        "exists": full_path.exists(),
        "allowed_to_execute": allowed,
        "requires_manual_confirmation": not allowed,
        "command": command,
        "next_action": "run_with_offline_gate" if allowed else tool.recommended_action,
        "safety_note": (
            "Offline tool: may run after normal repo checks."
            if allowed
            else "Blocked from autonomous execution because it crosses board, robot, IO, or firmware scope."
        ),
    }


def run_material_tool(
    root: Path,
    tool_path: str,
    *,
    material_root: Path | None = None,
    tool_args: list[str] | None = None,
    timeout_seconds: int = 30,
) -> dict[str, object]:
    """Execute one material-workspace tool only when its scope is offline_ok."""

    workspace = resolve_material_workspace(root, material_root)
    plan = plan_material_tool(root, tool_path, material_root=workspace)
    if not plan["allowed_to_execute"]:
        return {
            "executed": False,
            "passed": False,
            "plan": plan,
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "reason": "tool_not_offline_ok",
        }

    command = list(plan["command"])
    if not command:
        return {
            "executed": False,
            "passed": False,
            "plan": plan,
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "reason": "no_command_available",
        }

    execution_command = _execution_command(command, tool_args or [])
    try:
        completed = subprocess.run(
            execution_command,
            cwd=str(workspace),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_seconds,
        )
        return {
            "executed": True,
            "passed": completed.returncode == 0,
            "plan": plan,
            "command": execution_command,
            "returncode": completed.returncode,
            "stdout": completed.stdout[-4000:],
            "stderr": completed.stderr[-4000:],
            "reason": "completed",
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "executed": True,
            "passed": False,
            "plan": plan,
            "command": execution_command,
            "returncode": None,
            "stdout": (exc.stdout or "")[-4000:] if isinstance(exc.stdout, str) else "",
            "stderr": (exc.stderr or "")[-4000:] if isinstance(exc.stderr, str) else "",
            "reason": "timeout",
        }


def classify_material_tool(rel_path: str) -> MaterialTool:
    rel_lower = rel_path.lower().replace("\\", "/")
    name = Path(rel_lower).name

    firmware_tokens = ["m7_hot_reload", "remoteproc", "firmware", "flash", "deploy", "build_and_deploy"]
    if any(token in rel_lower for token in firmware_tokens):
        return _tool(rel_path, "firmware_required", "firmware or M7 lifecycle token", "generate firmware window checklist")

    io_tokens = ["io_output", "weld", "limit_output"]
    if any(token in rel_lower for token in io_tokens):
        return _tool(rel_path, "io_required", "IO/welding/limit output token", "generate IO safety audit")

    robot_tokens = [
        "driver_motor_lib_verify",
        "motion",
        "position_probe",
        "robot_axis",
        "robot_dual",
        "robot_mixed",
        "robot_six",
        "robot_triple",
        "takeover_regression",
    ]
    if any(token in rel_lower for token in robot_tokens):
        return _tool(rel_path, "robot_motion_required", "robot/servo motion token", "generate robot motion checklist")

    board_tokens = [
        "a53_",
        "board",
        "pdo_diag_collect",
        "readonly_snapshot",
        "rpmsg",
        "runtime_axis_verify",
        "runtime_readback",
        "snapshot_readback",
        "stop_start",
    ]
    if any(token in rel_lower for token in board_tokens):
        return _tool(rel_path, "board_required", "board/RPMsg/EtherCAT access token", "generate board read-only audit")

    offline_tokens = [
        "analyze_",
        "build_",
        "check_",
        "compare_",
        "fake_ecat_harness",
        "profile_to_topology",
        "run_offline",
        "run_static",
        "validate_",
        "verify_driver_profile",
        "xml_to_ec_profile",
    ]
    if any(token in rel_lower for token in offline_tokens) or name.startswith("run_xml"):
        return _tool(rel_path, "offline_ok", "offline generation/validation token", "safe to run in offline automation")

    return _tool(rel_path, "board_required", "unclassified engineering script", "audit before execution")


def iter_material_files(workspace: Path, *, scope: str = "all") -> Iterable[MaterialFile]:
    if not workspace.exists():
        return []
    roots = _scope_roots(workspace, scope)
    return _iter_material_files_from_roots(workspace, roots)


def _iter_material_files_from_roots(workspace: Path, roots: list[Path]) -> Iterable[MaterialFile]:
    for root in roots:
        if root.is_file():
            candidates = [root]
        elif root.exists():
            candidates = [path for path in root.rglob("*") if path.is_file()]
        else:
            candidates = []
        for path in candidates:
            rel = _relative_posix(path, workspace)
            if _should_skip(rel):
                continue
            try:
                size = path.stat().st_size
            except OSError:
                continue
            yield MaterialFile(
                path=rel,
                area=_classify_area(rel),
                kind=_classify_kind(path),
                size=size,
                searchable=path.suffix.lower() in TEXT_EXTENSIONS,
            )


def _scope_roots(workspace: Path, scope: str) -> list[Path]:
    if scope not in SEARCH_SCOPES:
        raise ValueError(f"Unsupported material search scope: {scope}")
    entries = SEARCH_SCOPES[scope]
    if not entries:
        return [workspace]
    roots = []
    for entry in entries:
        path = workspace / entry
        if scope == "xml" and path == workspace / "mix_protocol":
            roots.extend(workspace.glob("mix_protocol/*.xml"))
            continue
        roots.append(path)
    return roots


def _should_skip(rel: str) -> bool:
    parts = set(rel.split("/"))
    return bool(parts & SKIP_DIRS)


def _classify_area(rel: str) -> str:
    rel_lower = rel.lower()
    if rel_lower.startswith("mix_protocol/tools/fake_ecat_harness"):
        return "fake_harness"
    if rel_lower.startswith("mix_protocol/tools/generated"):
        return "generated_profiles_reports"
    if rel_lower.startswith("mix_protocol/tools"):
        return "engineering_tools"
    if rel_lower.startswith("mix_protocol/docs"):
        return "project_docs"
    if rel_lower.startswith("mix_protocol/bsp") or rel_lower.startswith("mix_protocol/freertos"):
        return "m7_rtos_source"
    if rel_lower.startswith("mix_protocol/libtpr20pro"):
        return "robot_library_source"
    if rel_lower.startswith("mix_protocol/armgcc"):
        return "build_system"
    if rel_lower.startswith("mix_protocol/tests"):
        return "tests"
    if rel_lower.startswith("assets/xml"):
        return "xml_esi"
    if rel_lower.startswith("docs/manuals"):
        return "manuals"
    if rel_lower.startswith("docs/vendor_guides"):
        return "vendor_guides"
    if rel_lower.startswith("logs"):
        return "logs"
    if rel_lower.startswith("board"):
        return "board_artifacts"
    if rel_lower.startswith("profiles"):
        return "profile_snapshots"
    if rel_lower.startswith("archive"):
        return "archive"
    return "workspace_root"


def _classify_kind(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix in {".c", ".h", ".cpp", ".hpp"}:
        return "source"
    if suffix == ".py":
        return "python"
    if suffix in {".md", ".txt"}:
        return "document"
    if suffix == ".xml":
        return "xml"
    if suffix in {".json", ".jsonl"}:
        return "json"
    if suffix in {".log"}:
        return "log"
    if suffix in {".pdf", ".docx"}:
        return "manual"
    if suffix in {".bat", ".sh", ".ps1", ".cmake", ".ld"}:
        return "script_or_build"
    if suffix in {".csv", ".tsv"}:
        return "table"
    if suffix in {".bin", ".elf", ".dtb", ".map"}:
        return "binary_or_build_artifact"
    return "other"


def _important_inventory_files(files: list[MaterialFile]) -> list[MaterialFile]:
    priority_areas = {
        "engineering_tools",
        "project_docs",
        "xml_esi",
        "profile_snapshots",
        "generated_profiles_reports",
        "fake_harness",
        "logs",
    }
    return sorted(
        [item for item in files if item.area in priority_areas],
        key=lambda item: (item.area not in priority_areas, item.area, item.path),
    )


def _search_payload(
    workspace: Path,
    query: str,
    scope: str,
    searched_files: int,
    skipped_files: int,
    hits: list[MaterialSearchHit],
    *,
    limit: int,
    truncated: bool,
) -> dict[str, object]:
    ranked_hits = _rank_search_hits(query, hits)[:limit]
    return {
        "material_root": str(workspace),
        "query": query,
        "scope": scope,
        "searched_files": searched_files,
        "skipped_files": skipped_files,
        "hit_count": len(ranked_hits),
        "total_hit_count_before_limit": len(hits),
        "truncated": truncated,
        "hits": [asdict(hit) for hit in ranked_hits],
    }


def _rank_search_hits(query: str, hits: list[MaterialSearchHit]) -> list[MaterialSearchHit]:
    query_lower = query.lower()
    area_priority = {
        "project_docs": 0,
        "engineering_tools": 1,
        "xml_esi": 2,
        "profile_snapshots": 3,
        "source": 4,
        "m7_rtos_source": 4,
        "robot_library_source": 4,
        "logs": 5,
        "fake_harness": 6,
        "generated_profiles_reports": 7,
        "archive": 8,
    }
    kind_priority = {
        "document": 0,
        "source": 1,
        "python": 2,
        "xml": 3,
        "json": 4,
        "log": 5,
    }

    def score(hit: MaterialSearchHit) -> tuple[int, int, int, str, int]:
        path_lower = hit.path.lower()
        filename = Path(path_lower).name
        filename_bonus = -3 if query_lower in filename else 0
        text_bonus = -1 if hit.text.lower().startswith(query_lower) else 0
        noisy_penalty = 2 if any(part in path_lower for part in NOISY_DIRS) else 0
        return (
            area_priority.get(hit.area, 9) + filename_bonus + text_bonus + noisy_penalty,
            kind_priority.get(hit.kind, 9),
            len(path_lower),
            hit.path,
            hit.line,
        )

    return sorted(hits, key=score)[: len(hits)]


def _top_counts(counts: object, limit: int = 8) -> dict[str, int]:
    if not isinstance(counts, dict):
        return {}
    items = sorted(((str(key), int(value)) for key, value in counts.items()), key=lambda item: (-item[1], item[0]))
    return dict(items[:limit])


def _tool(rel_path: str, risk: str, reason: str, recommended_action: str) -> MaterialTool:
    return MaterialTool(
        path=rel_path,
        risk=risk,
        reason=reason,
        recommended_action=recommended_action,
    )


def _suggest_tool_command(rel_path: str, full_path: Path) -> list[str]:
    suffix = full_path.suffix.lower()
    if suffix == ".py":
        return ["python", rel_path]
    if suffix == ".sh":
        return ["sh", rel_path]
    if suffix == ".bat":
        return [rel_path]
    return []


def _execution_command(command: list[str], tool_args: list[str]) -> list[str]:
    executable = sys.executable if command and command[0] == "python" else command[0]
    return [executable, *command[1:], *tool_args]


def _read_text_or_none(path: Path) -> str | None:
    for encoding in ("utf-8", "utf-8-sig", "gbk"):
        try:
            return path.read_text(encoding=encoding)
        except UnicodeDecodeError:
            continue
        except OSError:
            return None
    return None


def _trim_line(line: str, max_chars: int = 240) -> str:
    compact = re.sub(r"\s+", " ", line).strip()
    if len(compact) <= max_chars:
        return compact
    return compact[: max_chars - 3] + "..."


def _relative_posix(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()
