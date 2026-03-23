#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def run(args: list[str]) -> None:
    subprocess.run(args, cwd=ROOT, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate twister candidates, split shards, and materialize candidate folders.")
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--seed", type=int)
    parser.add_argument("--shard-size", type=int, default=500)
    parser.add_argument("--generated-dir", default="generated")
    parser.add_argument("--candidates-dir", default="generated/candidates")
    parser.add_argument("--clean-candidates", action="store_true")
    parser.add_argument("--skip-materialize", action="store_true")
    args = parser.parse_args()

    generated_dir = ROOT / args.generated_dir
    candidates_dir = ROOT / args.candidates_dir
    generated_dir.mkdir(parents=True, exist_ok=True)

    generate_cmd = [
        sys.executable,
        "tools/generate_twist_candidates.py",
        "generate",
        "--count",
        str(args.count),
        "--output-dir",
        str(generated_dir),
    ]
    if args.seed is not None:
        generate_cmd.extend(["--seed", str(args.seed)])
    run(generate_cmd)

    run(
        [
            sys.executable,
            "tools/split_twist_candidates.py",
            "--input",
            str(generated_dir / "twist_candidates_generated_verbose.cpp"),
            "--output-dir",
            str(generated_dir / "shards_cpp"),
            "--shard-size",
            str(args.shard_size),
            "--index-output",
            str(generated_dir / "shards_index.json"),
        ]
    )

    if not args.skip_materialize:
        if args.clean_candidates and candidates_dir.exists():
            shutil.rmtree(candidates_dir)

        run(
            [
                sys.executable,
                "tools/materialize_candidate_folders.py",
                "--index",
                str(generated_dir / "shards_index.json"),
                "--output-dir",
                str(candidates_dir),
            ]
        )

        print(f"candidate_folders={candidates_dir}")
    else:
        print("candidate_folders=skipped")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
