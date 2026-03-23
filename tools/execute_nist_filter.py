#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def run_env(env: dict[str, str], script: str) -> str:
    merged_env = {**os.environ, **env}
    result = subprocess.run(
        ["bash", script],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
        env=merged_env,
    )
    return result.stdout


def parse_key_value_output(text: str) -> dict[str, str]:
    data: dict[str, str] = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def candidate_dirs(input_dir: Path) -> list[Path]:
    return sorted(path for path in input_dir.iterdir() if path.is_dir() and path.name.startswith("candidate_"))


def parse_size_spec(size_spec: str) -> tuple[int, str]:
    text = size_spec.strip().lower()
    if not text:
        raise ValueError("empty size_spec")

    block_bytes = 7680
    named_sizes = {
        "l1": 261120,
        "block_size_l1": 261120,
        "l2": 522240,
        "block_size_l2": 522240,
        "l3": 1044480,
        "block_size_l3": 1044480,
    }
    if text in named_sizes:
        total_bytes = named_sizes[text]
        return total_bytes, text.upper().replace("BLOCK_SIZE_", "")

    unit_multipliers = {
        "b": 1,
        "byte": 1,
        "bytes": 1,
        "blk": block_bytes,
        "block": block_bytes,
        "blocks": block_bytes,
        "kb": 1024,
        "k": 1024,
        "kib": 1024,
        "mb": 1024 * 1024,
        "m": 1024 * 1024,
        "mib": 1024 * 1024,
        "gb": 1024 * 1024 * 1024,
        "g": 1024 * 1024 * 1024,
        "gib": 1024 * 1024 * 1024,
    }

    digits = []
    suffix = []
    for ch in text:
        if ch.isdigit() and not suffix:
            digits.append(ch)
        elif ch in " _":
            continue
        else:
            suffix.append(ch)

    if not digits:
        raise ValueError(f"invalid size_spec: {size_spec}")

    value = int("".join(digits))
    unit = "".join(suffix) or "mb"
    if unit not in unit_multipliers:
        raise ValueError(f"unsupported size unit in size_spec: {size_spec}")

    total_bytes = value * unit_multipliers[unit]
    if total_bytes <= 0:
        raise ValueError(f"size_spec must be positive: {size_spec}")

    if total_bytes % block_bytes == 0 and unit in {"blk", "block", "blocks"}:
        stage_tag = f"{total_bytes // block_bytes:03d}BLK"
    elif total_bytes % (1024 * 1024) == 0:
        stage_tag = f"{total_bytes // (1024 * 1024):03d}MB"
    elif total_bytes % 1024 == 0:
        stage_tag = f"{total_bytes // 1024:03d}KB"
    else:
        stage_tag = f"{total_bytes:03d}B"

    return total_bytes, stage_tag


def main() -> int:
    parser = argparse.ArgumentParser(description="Execute an NIST filter stage over candidate folders.")
    parser.add_argument("input_dir")
    parser.add_argument("output_dir")
    parser.add_argument("password_text")
    parser.add_argument("size_spec")
    parser.add_argument("--max-fail-rows", type=int, default=0)
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    if not input_dir.is_dir():
        raise SystemExit(f"missing input_dir: {input_dir}")

    size_bytes, stage_tag = parse_size_spec(args.size_spec)
    summary_csv = output_dir / f"summary_{args.password_text}_{stage_tag}.csv"
    passed_txt = output_dir / "passed_candidates.txt"
    failed_txt = output_dir / "failed_candidates.txt"
    build_dir = output_dir / ".build"
    stage_root = output_dir / ".stage"

    output_dir.mkdir(parents=True, exist_ok=True)
    build_dir.mkdir(parents=True, exist_ok=True)
    stage_root.mkdir(parents=True, exist_ok=True)

    if not args.resume:
        for path in output_dir.glob("candidate_*"):
            if path.is_dir():
                shutil.rmtree(path)

    if not summary_csv.exists() or not args.resume:
        summary_csv.write_text(
            "folder_name,candidate_id,function_name,status,pass_rows,fail_rows,total_rows,score_file,stream_file,final_report\n",
            encoding="utf-8",
        )
    if not passed_txt.exists() or not args.resume:
        passed_txt.write_text("", encoding="utf-8")
    if not failed_txt.exists() or not args.resume:
        failed_txt.write_text("", encoding="utf-8")

    compiled_bins: dict[str, Path] = {}

    for candidate_dir in candidate_dirs(input_dir):
        metadata_path = candidate_dir / "metadata.json"
        if not metadata_path.exists():
            continue
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        folder_name = candidate_dir.name
        final_dir = output_dir / folder_name
        score_name = f"scores_{args.password_text}_{stage_tag}.txt"
        stream_name = f"{args.password_text}_byte_stream_{stage_tag}.bin"

        if args.resume and (final_dir / score_name).exists():
            print(f"skipping {folder_name} (already passed into output)")
            continue

        shard_path = metadata.get("source_shard")
        if not shard_path:
            print(f"skipping {folder_name} (missing source_shard)")
            continue
        shard_path_obj = Path(shard_path)
        shard_name = metadata.get("source_shard_name", shard_path_obj.stem)
        candidate_id = str(metadata["candidate_id"])
        function_name = metadata.get("function_name", "")

        if shard_name not in compiled_bins:
            bin_path = build_dir / f"{shard_name}_stream"
            env = {
                "INPUT_CPP": str(shard_path_obj),
                "OUTPUT_BIN": str(bin_path),
            }
            subprocess.run(
                ["bash", "./run_build_twist_registered_stream.sh"],
                cwd=ROOT,
                check=True,
                env={**os.environ, **env},
                capture_output=True,
                text=True,
            )
            compiled_bins[shard_name] = bin_path

        tmp_dir = stage_root / folder_name
        if tmp_dir.exists():
            shutil.rmtree(tmp_dir)
        tmp_dir.mkdir(parents=True, exist_ok=True)

        score_file = tmp_dir / score_name
        stream_file = tmp_dir / stream_name
        report_root = tmp_dir / "nist_reports"
        run_label = f"{folder_name}_{args.password_text}_{stage_tag}"

        env = {
            "INPUT_CPP": str(shard_path_obj),
            "OUTPUT_BIN": str(compiled_bins[shard_name]),
            "CANDIDATE_ID": candidate_id,
            "PASSWORD_TEXT": args.password_text,
            "STREAM_BYTES": str(size_bytes),
            "STREAM_KIB": "0",
            "STREAM_MIB": "0",
            "STREAM_BITS": "0",
            "RUN_LABEL": run_label,
            "OUTPUT_ROOT": str(report_root),
            "OUTPUT_STREAM_PATH": str(stream_file),
            "SCORE_FILE": str(score_file),
            "KEEP_STREAM": "1",
            "KEEP_REPORT": "1",
        }
        output = subprocess.run(
            ["bash", "./run_registered_candidate_nist_once.sh"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
            env={**os.environ, **env},
        ).stdout
        parsed = parse_key_value_output(output)
        pass_rows = int(parsed.get("pass_rows", "0"))
        fail_rows = int(parsed.get("fail_rows", "0"))
        total_rows = int(parsed.get("total_rows", "0"))
        final_report = parsed.get("final_report", "")

        if total_rows <= 0:
            stage_status = "NO_REPORT"
            passes_stage = False
        elif fail_rows <= args.max_fail_rows:
            stage_status = "PASS"
            passes_stage = True
        else:
            stage_status = "TOO_MANY_FAILS"
            passes_stage = False

        if passes_stage:
            if final_dir.exists():
                shutil.rmtree(final_dir)
            shutil.copytree(candidate_dir, final_dir)
            shutil.copy2(score_file, final_dir / score_name)
            shutil.copy2(stream_file, final_dir / stream_name)
            report_source = report_root / run_label
            if report_source.exists():
                shutil.copytree(report_source, final_dir / f"nist_{args.password_text}_{stage_tag}", dirs_exist_ok=True)
            final_score_file = final_dir / score_name
            final_stream_file = final_dir / stream_name
            final_report_path = final_dir / f"nist_{args.password_text}_{stage_tag}" / "AlgorithmTesting" / "finalAnalysisReport.txt"
            with passed_txt.open("a", encoding="utf-8") as handle:
                handle.write(f"{folder_name}\n")
            print(
                f"PASS {folder_name} candidate_id={candidate_id} "
                f"pass_rows={pass_rows} fail_rows={fail_rows} total_rows={total_rows} "
                f"max_fail_rows={args.max_fail_rows}"
            )
        else:
            final_score_file = Path("")
            final_stream_file = Path("")
            final_report_path = Path("")
            with failed_txt.open("a", encoding="utf-8") as handle:
                handle.write(f"{folder_name}\n")
            print(
                f"FAIL {folder_name} candidate_id={candidate_id} reason={stage_status} "
                f"pass_rows={pass_rows} fail_rows={fail_rows} total_rows={total_rows} "
                f"max_fail_rows={args.max_fail_rows}"
            )

        with summary_csv.open("a", encoding="utf-8", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(
                [
                    folder_name,
                    candidate_id,
                    function_name,
                    stage_status,
                    pass_rows,
                    fail_rows,
                    total_rows,
                    str(final_score_file),
                    str(final_stream_file),
                    str(final_report_path),
                ]
            )

        shutil.rmtree(tmp_dir, ignore_errors=True)

    print(f"summary_csv={summary_csv}")
    print(f"passed_candidates={passed_txt}")
    print(f"failed_candidates={failed_txt}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
