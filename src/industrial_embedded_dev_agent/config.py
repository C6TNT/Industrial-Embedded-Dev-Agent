from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ProjectPaths:
    root: Path
    data_dir: Path
    benchmark_dir: Path
    taxonomy_dir: Path
    materials_dir: Path
    chunks_dir: Path


def discover_project_root(start: Path | None = None) -> Path:
    current = (start or Path(__file__)).resolve()
    for candidate in [current, *current.parents]:
        if (candidate / "pyproject.toml").exists():
            return candidate
    raise FileNotFoundError("Could not locate project root from pyproject.toml")


def get_project_paths() -> ProjectPaths:
    root = discover_project_root()
    data_dir = root / "data"
    return ProjectPaths(
        root=root,
        data_dir=data_dir,
        benchmark_dir=data_dir / "benchmark",
        taxonomy_dir=data_dir / "taxonomy",
        materials_dir=data_dir / "materials",
        chunks_dir=data_dir / "chunks",
    )
