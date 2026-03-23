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
_INDEX_CACHE: dict[Path, dict[int, Path]] = {}


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


def configured_shard_roots() -> list[Path]:
    roots: list[Path] = []
    env_value = os.environ.get("SOURCE_SHARD_ROOT", "").strip()
    if env_value:
        roots.append(Path(env_value))
    roots.extend(
        [
            ROOT / "generated" / "shards_cpp",
            ROOT / "shards_cpp",
            ROOT / "consolidated_winners" / "shards_cpp",
            ROOT / "runner" / "shards_cpp",
        ]
    )
    unique_roots: list[Path] = []
    seen: set[Path] = set()
    for root in roots:
        resolved = root.resolve(strict=False)
        if resolved in seen:
            continue
        seen.add(resolved)
        unique_roots.append(root)
    return unique_roots


def configured_index_paths() -> list[Path]:
    paths: list[Path] = []
    env_value = os.environ.get("SOURCE_SHARD_INDEX", "").strip()
    if env_value:
        paths.append(Path(env_value))
    paths.extend(
        [
            ROOT / "generated" / "consolidated_winners" / "shards_index.json",
            ROOT / "consolidated_winners" / "shards_index.json",
            ROOT / "shards_index.json",
            ROOT / "runner" / "shards_index.json",
            ROOT / "generated" / "shards_index.json",
        ]
    )
    unique_paths: list[Path] = []
    seen: set[Path] = set()
    for path in paths:
        resolved = path.resolve(strict=False)
        if resolved in seen:
            continue
        seen.add(resolved)
        unique_paths.append(path)
    return unique_paths


def index_candidate_map(index_path: Path) -> dict[int, Path]:
    resolved = index_path.resolve(strict=False)
    cached = _INDEX_CACHE.get(resolved)
    if cached is not None:
        return cached

    if not index_path.is_file():
        _INDEX_CACHE[resolved] = {}
        return _INDEX_CACHE[resolved]

    data = json.loads(index_path.read_text(encoding="utf-8"))
    index_dir = index_path.parent
    candidate_map: dict[int, Path] = {}
    for shard_info in data.get("shards", []):
        shard_path_text = str(shard_info.get("path", "")).strip()
        if not shard_path_text:
            continue
        shard_path = Path(shard_path_text)
        if not shard_path.is_absolute():
            shard_path = index_dir / shard_path
        for candidate_id in shard_info.get("source_candidate_ids", []):
            candidate_map[int(candidate_id)] = shard_path
    _INDEX_CACHE[resolved] = candidate_map
    return candidate_map


def resolve_source_shard(candidate_dir: Path, metadata: dict[str, object]) -> Path | None:
    candidate_id = int(metadata.get("candidate_id", 0) or 0)
    if candidate_id > 0:
        for index_path in configured_index_paths():
            shard_path = index_candidate_map(index_path).get(candidate_id)
            if shard_path is not None and shard_path.is_file():
                return shard_path

    shard_path = str(metadata.get("source_shard", "")).strip()
    if shard_path:
        shard_path_obj = Path(shard_path)
        if shard_path_obj.is_file():
            return shard_path_obj

    shard_name = str(metadata.get("source_shard_name", "")).strip()
    if shard_name:
        shard_file_name = shard_name if shard_name.endswith(".cpp") else f"{shard_name}.cpp"
        for root in configured_shard_roots():
            candidate = root / shard_file_name
            if candidate.is_file():
                return candidate

    local_code = candidate_dir / "code.cpp"
    if local_code.is_file():
        return local_code

    return None


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
    if total_bytes % block_bytes == 0 and unit in {"blk", "block", "blocks"}:
        stage_tag = f"{total_bytes // block_bytes:03d}BLK"
    elif total_bytes % (1024 * 1024) == 0:
        stage_tag = f"{total_bytes // (1024 * 1024):03d}MB"
    elif total_bytes % 1024 == 0:
        stage_tag = f"{total_bytes // 1024:03d}KB"
    else:
        stage_tag = f"{total_bytes:05d}B"
    return total_bytes, stage_tag


def zero_failers_dir_from_passers_dir(passers_dir: Path) -> Path:
    name = passers_dir.name
    if name.endswith("_passers"):
        sibling_name = f"{name[:-8]}_zero_failers"
    else:
        sibling_name = f"{name}_zero_failers"
    return passers_dir.parent / sibling_name


def copy_candidate_tree(source_dir: Path, dest_dir: Path) -> None:
    if dest_dir.exists():
        shutil.rmtree(dest_dir)
    shutil.copytree(source_dir, dest_dir)
    # Preserve file contents but refresh the top-level directory mtime so the
    # copied result is visibly new in Finder and directory listings.
    os.utime(dest_dir, None)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run PractRand on every candidate folder and store artifacts in place.")
    parser.add_argument("input_dir")
    parser.add_argument("password_text")
    parser.add_argument("size_spec")
    parser.add_argument("--passers-dir", default="")
    parser.add_argument("--zero-failers-dir", default="")
    parser.add_argument("--max-fail", type=int, default=0)
    parser.add_argument("--max-very-suspicious", type=int, default=0)
    parser.add_argument("--max-suspicious", type=int, default=0)
    parser.add_argument("--max-mildly-suspicious", type=int, default=999999)
    parser.add_argument("--max-unusual", type=int, default=999999)
    parser.add_argument("--no-result-files", action="store_true")
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    if not input_dir.is_dir():
        raise SystemExit(f"missing input_dir: {input_dir}")
    if args.resume and args.no_result_files:
        raise SystemExit("--resume requires saved result files; rerun without --no-result-files")
    if args.no_result_files and (args.passers_dir or args.zero_failers_dir):
        raise SystemExit("--no-result-files cannot be combined with --passers-dir or --zero-failers-dir")

    size_bytes, stage_tag = parse_size_spec(args.size_spec)
    summary_csv = None if args.no_result_files else input_dir / f"summary_practrand_{args.password_text}_{stage_tag}.csv"
    build_dir = input_dir / ".practrand_build"
    stage_root = input_dir / ".practrand_stage"
    passers_dir = None if args.no_result_files else (Path(args.passers_dir) if args.passers_dir else None)
    zero_failers_dir = None
    if not args.no_result_files:
        if args.zero_failers_dir:
            zero_failers_dir = Path(args.zero_failers_dir)
        elif passers_dir is not None:
            zero_failers_dir = zero_failers_dir_from_passers_dir(passers_dir)

    build_dir.mkdir(parents=True, exist_ok=True)
    stage_root.mkdir(parents=True, exist_ok=True)
    if passers_dir is not None:
        passers_dir.mkdir(parents=True, exist_ok=True)
        if not args.resume:
            for path in passers_dir.glob("candidate_*"):
                if path.is_dir():
                    shutil.rmtree(path)
    if zero_failers_dir is not None:
        zero_failers_dir.mkdir(parents=True, exist_ok=True)
        if not args.resume:
            for path in zero_failers_dir.glob("candidate_*"):
                if path.is_dir():
                    shutil.rmtree(path)

    passed_txt = None if args.no_result_files else input_dir / f"passed_candidates_practrand_{args.password_text}_{stage_tag}.txt"
    failed_txt = None if args.no_result_files else input_dir / f"failed_candidates_practrand_{args.password_text}_{stage_tag}.txt"
    zero_failed_txt = None if args.no_result_files else input_dir / f"zero_fail_candidates_practrand_{args.password_text}_{stage_tag}.txt"

    if summary_csv is not None and (not summary_csv.exists() or not args.resume):
        summary_csv.write_text(
            "folder_name,candidate_id,function_name,status,gate_status,fail,very_suspicious,suspicious,mildly_suspicious,unusual,normalish,normal,total_results,score_file,stream_file,report_file\n",
            encoding="utf-8",
        )
    if passed_txt is not None and (not passed_txt.exists() or not args.resume):
        passed_txt.write_text("", encoding="utf-8")
    if failed_txt is not None and (not failed_txt.exists() or not args.resume):
        failed_txt.write_text("", encoding="utf-8")
    if zero_failed_txt is not None and (not zero_failed_txt.exists() or not args.resume):
        zero_failed_txt.write_text("", encoding="utf-8")

    compiled_bins: dict[str, Path] = {}

    for candidate_dir in candidate_dirs(input_dir):
        metadata_path = candidate_dir / "metadata.json"
        if not metadata_path.exists():
            continue

        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        folder_name = candidate_dir.name
        candidate_id = str(metadata["candidate_id"])
        function_name = metadata.get("function_name", "")
        score_name = f"scores_practrand_{args.password_text}_{stage_tag}.txt"
        stream_name = f"{args.password_text}_byte_stream_{stage_tag}.bin"
        report_name = f"practrand_{args.password_text}_{stage_tag}.txt"

        if args.resume and (candidate_dir / score_name).exists():
            print(f"skipping {folder_name} candidate_id={candidate_id} (already has {score_name})")
            continue

        shard_path_obj = resolve_source_shard(candidate_dir, metadata)
        if shard_path_obj is None:
            print(f"skipping {folder_name} (unable to resolve source_shard)")
            continue

        shard_name = shard_path_obj.stem
        if shard_name not in compiled_bins:
            aBinPath = build_dir / f"{shard_name}_stream"
            env = {
                "INPUT_CPP": str(shard_path_obj),
                "OUTPUT_BIN": str(aBinPath),
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
                print(f"ERROR building shard {shard_name} from {shard_path_obj}")
                if stdout_text:
                    print(stdout_text)
                if stderr_text:
                    print(stderr_text)
                raise SystemExit(exc.returncode)
            compiled_bins[shard_name] = aBinPath

        aTmpDir = stage_root / folder_name
        if aTmpDir.exists():
            shutil.rmtree(aTmpDir)
        aTmpDir.mkdir(parents=True, exist_ok=True)

        aReportRoot = aTmpDir / "practrand_reports"
        aRunLabel = f"{folder_name}_{args.password_text}_{stage_tag}"

        env = {
            "INPUT_CPP": str(shard_path_obj),
            "OUTPUT_BIN": str(compiled_bins[shard_name]),
            "CANDIDATE_ID": candidate_id,
            "PASSWORD_TEXT": args.password_text,
            "STREAM_BYTES": str(size_bytes),
            "RUN_LABEL": aRunLabel,
            "OUTPUT_ROOT": str(aReportRoot),
            "KEEP_STREAM": "0" if args.no_result_files else "1",
            "KEEP_REPORT": "0" if args.no_result_files else "1",
        }
        if not args.no_result_files:
            aScoreFile = aTmpDir / score_name
            aStreamFile = aTmpDir / stream_name
            env["OUTPUT_STREAM_PATH"] = str(aStreamFile)
            env["SCORE_FILE"] = str(aScoreFile)

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
            print(f"ERROR {folder_name} candidate_id={candidate_id} practrand runner failed")
            if stdout_text:
                print(stdout_text)
            if stderr_text:
                print(stderr_text)
            raise SystemExit(exc.returncode)

        parsed = parse_key_value_output(output)
        aFail = int(parsed.get("fail", "0"))
        aVerySuspicious = int(parsed.get("very_suspicious", "0"))
        aSuspicious = int(parsed.get("suspicious", "0"))
        aMildlySuspicious = int(parsed.get("mildly_suspicious", "0"))
        aUnusual = int(parsed.get("unusual", "0"))
        aNormalish = int(parsed.get("normalish", "0"))
        aNormal = int(parsed.get("normal", "0"))
        aTotalResults = int(parsed.get("total_results", "0"))

        if args.no_result_files:
            aFinalScoreFile = Path("")
            aFinalStreamFile = Path("")
            aFinalReportFile = Path("")
            aStatus = "NOT_SAVED" if aTotalResults > 0 else "NO_REPORT"
        else:
            aReportSource = aReportRoot / aRunLabel / "practrand_output.txt"
            aFinalScoreFile = candidate_dir / score_name
            aFinalStreamFile = candidate_dir / stream_name
            aFinalReportFile = candidate_dir / report_name

            shutil.copy2(aScoreFile, aFinalScoreFile)
            shutil.copy2(aStreamFile, aFinalStreamFile)
            if aReportSource.exists():
                shutil.copy2(aReportSource, aFinalReportFile)
                aStatus = "REPORT"
            else:
                aStatus = "NO_REPORT"
                aFinalReportFile = Path("")

        if aTotalResults <= 0:
            aGateStatus = "NO_REPORT"
            aPassesGate = False
        elif (
            aFail <= args.max_fail
            and aVerySuspicious <= args.max_very_suspicious
            and aSuspicious <= args.max_suspicious
            and aMildlySuspicious <= args.max_mildly_suspicious
            and aUnusual <= args.max_unusual
        ):
            aGateStatus = "PASS"
            aPassesGate = True
        elif aFail > args.max_fail:
            aGateStatus = "TOO_MANY_FAIL"
            aPassesGate = False
        elif aVerySuspicious > args.max_very_suspicious:
            aGateStatus = "TOO_MANY_VERY_SUSPICIOUS"
            aPassesGate = False
        elif aSuspicious > args.max_suspicious:
            aGateStatus = "TOO_MANY_SUSPICIOUS"
            aPassesGate = False
        elif aMildlySuspicious > args.max_mildly_suspicious:
            aGateStatus = "TOO_MANY_MILDLY_SUSPICIOUS"
            aPassesGate = False
        else:
            aGateStatus = "TOO_MANY_UNUSUAL"
            aPassesGate = False

        if aFail == 0 and aTotalResults > 0:
            if zero_failed_txt is not None:
                with zero_failed_txt.open("a", encoding="utf-8") as handle:
                    handle.write(f"{folder_name}\n")
            if zero_failers_dir is not None:
                aZeroFailerDir = zero_failers_dir / folder_name
                copy_candidate_tree(candidate_dir, aZeroFailerDir)
                print(f"ZERO_FAILER_SAVED {folder_name} dir={aZeroFailerDir}")

        if aPassesGate:
            if passed_txt is not None:
                with passed_txt.open("a", encoding="utf-8") as handle:
                    handle.write(f"{folder_name}\n")
            if passers_dir is not None:
                aPasserDir = passers_dir / folder_name
                copy_candidate_tree(candidate_dir, aPasserDir)
                print(f"PASSER_SAVED {folder_name} dir={aPasserDir}")
        else:
            if failed_txt is not None:
                with failed_txt.open("a", encoding="utf-8") as handle:
                    handle.write(f"{folder_name}\n")

        print(
            f"RESULT {folder_name} candidate_id={candidate_id} status={aStatus} gate_status={aGateStatus} "
            f"fail={aFail} very_suspicious={aVerySuspicious} suspicious={aSuspicious} "
            f"mildly_suspicious={aMildlySuspicious} unusual={aUnusual} total_results={aTotalResults}"
        )

        if summary_csv is not None:
            with summary_csv.open("a", encoding="utf-8", newline="") as handle:
                writer = csv.writer(handle)
                writer.writerow(
                    [
                        folder_name,
                        candidate_id,
                        function_name,
                        aStatus,
                        aGateStatus,
                        aFail,
                        aVerySuspicious,
                        aSuspicious,
                        aMildlySuspicious,
                        aUnusual,
                        aNormalish,
                        aNormal,
                        aTotalResults,
                        str(aFinalScoreFile),
                        str(aFinalStreamFile),
                        str(aFinalReportFile),
                    ]
                )

        shutil.rmtree(aTmpDir, ignore_errors=True)

    if args.no_result_files:
        shutil.rmtree(stage_root, ignore_errors=True)
        print("result_files=disabled")
    else:
        print(f"summary_csv={summary_csv}")
        print(f"passed_candidates={passed_txt}")
        print(f"failed_candidates={failed_txt}")
        print(f"zero_fail_candidates={zero_failed_txt}")
    if passers_dir is not None:
        print(f"passers_dir={passers_dir}")
    if zero_failers_dir is not None:
        print(f"zero_failers_dir={zero_failers_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
