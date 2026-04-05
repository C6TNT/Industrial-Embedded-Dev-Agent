from __future__ import annotations

import runpy
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: run_with_stub.py <script_path> [args...]", file=sys.stderr)
        return 2

    script_path = Path(sys.argv[1]).resolve()
    repo_root = Path(__file__).resolve().parents[2]
    sys.path.insert(0, str(repo_root / "scripts" / "wsl"))

    from librobot_stub_runtime import load_stub_profile, patch_ctypes_cdll

    profile = load_stub_profile(repo_root / ".ieda_stub_profile.json")
    patch_ctypes_cdll(scenario=profile["scenario"])
    sys.argv = [str(script_path), *sys.argv[2:]]
    runpy.run_path(str(script_path), run_name="__main__")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
