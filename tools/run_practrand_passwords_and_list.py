#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

from run_practrand_in_place import (
    ROOT,
    candidate_dirs,
    parse_key_value_output,
    parse_size_spec,
    resolve_source_shard,
)


def read_passwords(password_file: Path) -> list[str]:
    passwords: list[str] = []
    for raw_line in password_file.read_text(encoding="utf-8").splitlines():
        password = raw_line.strip()
        if not password or password.startswith("#"):
            continue
        passwords.append(password)
    return passwords


def sanitize_password_label(password: str) -> str:
    label = re.sub(r"[^A-Za-z0-9]+", "_", password).strip("_").lower()
    return label or "blank"


def case_signature(password: str) -> str:
    signature = "".join("u" if char.isupper() else "l" for char in password if char.isalpha())
    return signature or "plain"


def build_password_entries(passwords: list[str]) -> list[tuple[str, str]]:
    slugs = [sanitize_password_label(password) for password in passwords]
    slug_counts: dict[str, int] = {}
    for slug in slugs:
        slug_counts[slug] = slug_counts.get(slug, 0) + 1

    entries: list[tuple[str, str]] = []
    used_labels: dict[str, int] = {}
    for index, password in enumerate(passwords, start=1):
        slug = slugs[index - 1]
        if slug_counts[slug] == 1:
            label = slug
        else:
            base_label = f"{slug}-{case_signature(password)}"
            used_count = used_labels.get(base_label, 0)
            if used_count > 0:
                label = f"{base_label}-{used_count + 1}"
            else:
                label = base_label
            used_labels[base_label] = used_count + 1
        entries.append((password, label))
    return entries


def copy_candidate_tree(source_dir: Path, dest_dir: Path) -> None:
    if dest_dir.exists():
        shutil.rmtree(dest_dir)
    shutil.copytree(source_dir, dest_dir)


def rooted_path(path_text: str) -> Path:
    path = Path(path_text).expanduser()
    if path.is_absolute():
        return path
    return ROOT / path


def ensure_compiled_bin(
    compiled_bins: dict[Path, Path],
    build_dir: Path,
    shard_path: Path,
) -> Path:
    shard_key = shard_path.resolve()
    cached = compiled_bins.get(shard_key)
    if cached is not None:
        return cached

    output_bin = build_dir / f"{shard_path.stem}_stream"
    env = {
        "INPUT_CPP": str(shard_path),
        "OUTPUT_BIN": str(output_bin),
    }
    try:
        subprocess.run(
            ["bash", "./run_build_twist_registered_stream.sh"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
            env={**os.environ, **env},
        )
    except subprocess.CalledProcessError as exc:
        stdout_text = (exc.stdout or "").strip()
        stderr_text = (exc.stderr or "").strip()
        print(f"ERROR building shard {shard_path.stem} from {shard_path}")
        if stdout_text:
            print(stdout_text)
        if stderr_text:
            print(stderr_text)
        raise SystemExit(exc.returncode)

    compiled_bins[shard_key] = output_bin
    return output_bin


def evaluate_gate(
    fail: int,
    very_suspicious: int,
    suspicious: int,
    mildly_suspicious: int,
    unusual: int,
    total_results: int,
    args: argparse.Namespace,
) -> tuple[str, bool]:
    if total_results <= 0:
        return "NO_REPORT", False
    if (
        fail <= args.max_fail
        and very_suspicious <= args.max_very_suspicious
        and suspicious <= args.max_suspicious
        and mildly_suspicious <= args.max_mildly_suspicious
        and unusual <= args.max_unusual
    ):
        return "PASS", True
    if fail > args.max_fail:
        return "TOO_MANY_FAIL", False
    if very_suspicious > args.max_very_suspicious:
        return "TOO_MANY_VERY_SUSPICIOUS", False
    if suspicious > args.max_suspicious:
        return "TOO_MANY_SUSPICIOUS", False
    if mildly_suspicious > args.max_mildly_suspicious:
        return "TOO_MANY_MILDLY_SUSPICIOUS", False
    return "TOO_MANY_UNUSUAL", False


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run a PractRand password list as an AND gauntlet. Survivors keep all per-password artifacts."
    )
    parser.add_argument("input_dir")
    parser.add_argument("password_file")
    parser.add_argument("size_spec")
    parser.add_argument("output_dir")
    parser.add_argument("--work-dir", default="")
    parser.add_argument("--max-fail", type=int, default=0)
    parser.add_argument("--max-very-suspicious", type=int, default=0)
    parser.add_argument("--max-suspicious", type=int, default=0)
    parser.add_argument("--max-mildly-suspicious", type=int, default=999999)
    parser.add_argument("--max-unusual", type=int, default=999999)
    parser.add_argument("--keep-failing-output", action="store_true")
    args = parser.parse_args()

    input_dir = rooted_path(args.input_dir)
    password_file = rooted_path(args.password_file)
    output_dir = rooted_path(args.output_dir)
    work_dir = rooted_path(args.work_dir) if args.work_dir else rooted_path(f"{args.output_dir}_work")

    if not input_dir.is_dir():
        raise SystemExit(f"missing input_dir: {input_dir}")
    if not password_file.is_file():
        raise SystemExit(f"missing password file: {password_file}")

    passwords = read_passwords(password_file)
    if not passwords:
        raise SystemExit(f"no passwords found in {password_file}")
    password_entries = build_password_entries(passwords)

    size_bytes, stage_tag = parse_size_spec(args.size_spec)
    build_dir = work_dir / ".practrand_build"
    logs_dir = work_dir / "logs"
    staging_dir = work_dir / "candidates"
    compiled_bins: dict[Path, Path] = {}

    output_dir.mkdir(parents=True, exist_ok=True)
    work_dir.mkdir(parents=True, exist_ok=True)
    build_dir.mkdir(parents=True, exist_ok=True)
    logs_dir.mkdir(parents=True, exist_ok=True)
    staging_dir.mkdir(parents=True, exist_ok=True)

    for path in output_dir.glob("candidate_*"):
        if path.is_dir():
            shutil.rmtree(path)
    for path in staging_dir.glob("candidate_*"):
        if path.is_dir():
            shutil.rmtree(path)

    summary_csv = output_dir / f"summary_passwords_and_list_{stage_tag}.csv"
    passed_txt = output_dir / f"passed_candidates_passwords_and_list_{stage_tag}.txt"
    failed_txt = output_dir / f"failed_candidates_passwords_and_list_{stage_tag}.txt"
    summary_csv.parent.mkdir(parents=True, exist_ok=True)
    summary_csv.write_text(
        "folder_name,candidate_id,password,label,status,gate_status,fail,very_suspicious,suspicious,mildly_suspicious,unusual,normalish,normal,total_results,stream_file,score_file,report_file\n",
        encoding="utf-8",
    )
    passed_txt.parent.mkdir(parents=True, exist_ok=True)
    passed_txt.write_text("", encoding="utf-8")
    failed_txt.parent.mkdir(parents=True, exist_ok=True)
    failed_txt.write_text("", encoding="utf-8")

    print(f"input_dir={input_dir}")
    print(f"password_file={password_file}")
    print(f"password_count={len(password_entries)}")
    print(f"size_spec={args.size_spec}")
    print(f"stage_tag={stage_tag}")
    print(f"output_dir={output_dir}")
    print(f"work_dir={work_dir}")

    for candidate_dir in candidate_dirs(input_dir):
        metadata_path = candidate_dir / "metadata.json"
        if not metadata_path.exists():
            continue

        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        folder_name = candidate_dir.name
        candidate_id = str(metadata.get("candidate_id", ""))
        staging_candidate_dir = staging_dir / folder_name
        output_candidate_dir = output_dir / folder_name
        candidate_log_file = logs_dir / f"{folder_name}.log"

        shard_path = resolve_source_shard(candidate_dir, metadata)
        if shard_path is None:
            print(f"SKIP {folder_name} unable_to_resolve_source_shard")
            continue

        compiled_bin = ensure_compiled_bin(compiled_bins, build_dir, shard_path)
        copy_candidate_tree(candidate_dir, staging_candidate_dir)

        passed_all = True
        last_gate_status = "PASS"
        failure_label = ""
        log_lines = [
            f"candidate_dir={candidate_dir}",
            f"output_candidate_dir={output_candidate_dir}",
            f"candidate_id={candidate_id}",
            f"source_shard={shard_path}",
            f"compiled_bin={compiled_bin}",
        ]

        for password, label in password_entries:
            stream_file = staging_candidate_dir / f"{label}.bin"
            score_file = staging_candidate_dir / f"{label}_score.txt"
            report_file = staging_candidate_dir / f"{label}_practrand.txt"
            run_label = f"{folder_name}_{label}_{stage_tag}"
            report_root = staging_candidate_dir / ".practrand_reports"

            env = {
                "INPUT_CPP": str(shard_path),
                "OUTPUT_BIN": str(compiled_bin),
                "CANDIDATE_ID": candidate_id,
                "PASSWORD_TEXT": password,
                "STREAM_BYTES": str(size_bytes),
                "RUN_LABEL": run_label,
                "OUTPUT_ROOT": str(report_root),
                "OUTPUT_STREAM_PATH": str(stream_file),
                "SCORE_FILE": str(score_file),
                "KEEP_STREAM": "1",
                "KEEP_REPORT": "1",
            }

            try:
                output = subprocess.run(
                    ["bash", "./run_registered_candidate_practrand_once.sh"],
                    cwd=ROOT,
                    check=True,
                    capture_output=True,
                    text=True,
                    env={**os.environ, **env},
                ).stdout
            except subprocess.CalledProcessError as exc:
                stdout_text = (exc.stdout or "").strip()
                stderr_text = (exc.stderr or "").strip()
                print(f"ERROR {folder_name} candidate_id={candidate_id} password={password} practrand runner failed")
                if stdout_text:
                    print(stdout_text)
                if stderr_text:
                    print(stderr_text)
                raise SystemExit(exc.returncode)

            parsed = parse_key_value_output(output)
            fail = int(parsed.get("fail", "0"))
            very_suspicious = int(parsed.get("very_suspicious", "0"))
            suspicious = int(parsed.get("suspicious", "0"))
            mildly_suspicious = int(parsed.get("mildly_suspicious", "0"))
            unusual = int(parsed.get("unusual", "0"))
            normalish = int(parsed.get("normalish", "0"))
            normal = int(parsed.get("normal", "0"))
            total_results = int(parsed.get("total_results", "0"))
            report_source = report_root / run_label / "practrand_output.txt"
            if report_source.exists():
                shutil.copy2(report_source, report_file)

            status = "REPORT" if report_file.exists() else "NO_REPORT"
            gate_status, passed_gate = evaluate_gate(
                fail,
                very_suspicious,
                suspicious,
                mildly_suspicious,
                unusual,
                total_results,
                args,
            )
            log_lines.append(
                f"{label}: password={password} gate_status={gate_status} fail={fail} very_suspicious={very_suspicious} suspicious={suspicious} mildly_suspicious={mildly_suspicious} unusual={unusual} total_results={total_results}"
            )

            summary_csv.parent.mkdir(parents=True, exist_ok=True)
            with summary_csv.open("a", encoding="utf-8", newline="") as handle:
                writer = csv.writer(handle)
                writer.writerow(
                    [
                        folder_name,
                        candidate_id,
                        password,
                        label,
                        status,
                        gate_status,
                        fail,
                        very_suspicious,
                        suspicious,
                        mildly_suspicious,
                        unusual,
                        normalish,
                        normal,
                        total_results,
                        str(stream_file) if stream_file.exists() else "",
                        str(score_file) if score_file.exists() else "",
                        str(report_file) if report_file.exists() else "",
                    ]
                )

            shutil.rmtree(report_root / run_label, ignore_errors=True)

            print(
                f"GAUNTLET_RESULT {folder_name} candidate_id={candidate_id} password={password} label={label} "
                f"gate_status={gate_status} fail={fail} very_suspicious={very_suspicious} suspicious={suspicious} "
                f"mildly_suspicious={mildly_suspicious} unusual={unusual} total_results={total_results}"
            )

            if not passed_gate:
                passed_all = False
                last_gate_status = gate_status
                failure_label = label
                break

        candidate_log_file.write_text("\n".join(log_lines) + "\n", encoding="utf-8")
        shutil.rmtree(staging_candidate_dir / ".practrand_reports", ignore_errors=True)

        if passed_all:
            if output_candidate_dir.exists():
                shutil.rmtree(output_candidate_dir, ignore_errors=True)
            shutil.move(str(staging_candidate_dir), str(output_candidate_dir))
            passed_txt.parent.mkdir(parents=True, exist_ok=True)
            with passed_txt.open("a", encoding="utf-8") as handle:
                handle.write(f"{folder_name}\n")
            print(f"GAUNTLET_PASSER_SAVED {folder_name} dir={output_candidate_dir}")
        else:
            failed_txt.parent.mkdir(parents=True, exist_ok=True)
            with failed_txt.open("a", encoding="utf-8") as handle:
                handle.write(f"{folder_name},{failure_label},{last_gate_status}\n")
            if not args.keep_failing_output:
                shutil.rmtree(staging_candidate_dir, ignore_errors=True)
                print(f"GAUNTLET_FAIL_PRUNED {folder_name} label={failure_label} gate_status={last_gate_status}")
            else:
                print(f"GAUNTLET_FAIL_KEPT {folder_name} dir={staging_candidate_dir} label={failure_label} gate_status={last_gate_status}")

    print(f"summary_csv={summary_csv}")
    print(f"passed_candidates={passed_txt}")
    print(f"failed_candidates={failed_txt}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
