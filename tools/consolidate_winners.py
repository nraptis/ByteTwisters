#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import re
import shutil
from dataclasses import dataclass
from pathlib import Path


FUNCTION_START_RE = re.compile(r"^// Candidate (\d+): ")
FUNCTION_NAME_RE = re.compile(r"static void (TwistCandidate_\d+)_KeySeed\(")
REGISTRY_START = "const RegisteredCandidate kRegisteredCandidates[] = {"


@dataclass(frozen=True)
class CandidateBundle:
    candidate_id: int
    function_name: str
    function_text: str
    source_path: Path
    source_dir: Path
    source_size_bytes: int


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def shard_name(index: int) -> str:
    return f"shard_{index:04d}"


def parse_bundle(code_path: Path) -> CandidateBundle:
    lines = code_path.read_text(encoding="utf-8").splitlines(keepends=True)
    function_lines: list[str] = []
    in_registry = False
    entry_lines: list[str] = []
    brace_depth = 0

    for line in lines:
        if not in_registry:
            if line.startswith(REGISTRY_START):
                in_registry = True
                continue
            if line.startswith('#include "') or line.startswith("namespace twist {") or line.startswith("}  // namespace twist"):
                continue
            if line.strip() == "":
                function_lines.append(line)
                continue
            function_lines.append(line)
            continue

        if not entry_lines:
            if line.strip() == "};":
                break
            if line.lstrip().startswith("{"):
                entry_lines = [line]
                brace_depth = line.count("{") - line.count("}")
            continue

        entry_lines.append(line)
        brace_depth += line.count("{") - line.count("}")
        if brace_depth == 0:
            break

    function_text = "".join(function_lines).strip()
    if not function_text:
        raise ValueError(f"no function body found in {code_path}")

    candidate_match = FUNCTION_START_RE.search(function_text)
    if not candidate_match:
        raise ValueError(f"candidate id marker missing in {code_path}")
    function_match = FUNCTION_NAME_RE.search(function_text)
    if not function_match:
        raise ValueError(f"function name marker missing in {code_path}")

    return CandidateBundle(
        candidate_id=int(candidate_match.group(1)),
        function_name=function_match.group(1),
        function_text=function_text,
        source_path=code_path,
        source_dir=code_path.parent,
        source_size_bytes=code_path.stat().st_size,
    )


def render_registry_entry(bundle: CandidateBundle) -> str:
    has_dual_mask = f"{bundle.function_name}_MaskSeedA(" in bundle.function_text
    function_symbol = bundle.function_name if has_dual_mask else f"{bundle.function_name}_Compat"
    mask_seed_a_symbol = f"{bundle.function_name}_MaskSeedA" if has_dual_mask else f"{bundle.function_name}_MaskSeedCompatA"
    mask_seed_b_symbol = f"{bundle.function_name}_MaskSeedB" if has_dual_mask else f"{bundle.function_name}_MaskSeedCompatB"
    twist_block_symbol = f"{bundle.function_name}_TwistBlock" if has_dual_mask else f"{bundle.function_name}_TwistBlockCompat"
    push_mask_a_symbol = (
        f"{bundle.function_name}_PushMaskRoundA"
        if has_dual_mask else f"{bundle.function_name}_PushMaskRoundCompatA"
    )
    push_mask_b_symbol = (
        f"{bundle.function_name}_PushMaskRoundB"
        if has_dual_mask else f"{bundle.function_name}_PushMaskRoundCompatB"
    )
    return "\n".join(
        [
            "  {",
            f"    {bundle.candidate_id},",
            f"    \"{bundle.function_name}\",",
            "    0,",
            "    0,",
            "    0,",
            '    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},',
            '    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},',
            f"    \"{bundle.function_name}\",",
            f"    &{function_symbol},",
            f"    &{bundle.function_name}_KeySeed,",
            f"    &{bundle.function_name}_SaltSeed,",
            f"    &{mask_seed_a_symbol},",
            f"    &{mask_seed_b_symbol},",
            f"    &{twist_block_symbol},",
            f"    &{bundle.function_name}_PushKeyRound,",
            f"    &{push_mask_a_symbol},",
            f"    &{push_mask_b_symbol},",
            "  },",
        ]
    )


def render_compat_wrappers(bundle: CandidateBundle) -> str:
    if f"{bundle.function_name}_MaskSeedA(" in bundle.function_text:
        return ""
    name = bundle.function_name
    return "\n".join(
        [
            f"static void {name}_MaskSeedCompatA(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorker,",
            "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {name}_MaskSeed(pSource, pMaskStackA, pLength);",
            "}",
            "",
            f"static void {name}_MaskSeedCompatB(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorker,",
            "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {name}_MaskSeed(pSource, pMaskStackB, pLength);",
            "}",
            "",
            f"static void {name}_TwistBlockCompat(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorkerA,",
            "    unsigned char* pWorkerB,",
            "    unsigned char* pDest,",
            "    unsigned int pRound,",
            "    const unsigned char (&pSalt)[kSaltBytes],",
            "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
            "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&)[kMaskStackDepth][kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {name}_TwistBlock(pSource, pWorkerA, pWorkerB, pDest, pRound, pSalt, pKeyStack, pMaskStackA, pLength);",
            "}",
            "",
            f"static void {name}_PushMaskRoundCompatA(",
            "    unsigned char* pDest,",
            "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {name}_PushMaskRound(pDest, pMaskStackA, pNextRoundMaskBufferA, pLength);",
            "}",
            "",
            f"static void {name}_PushMaskRoundCompatB(",
            "    unsigned char* pDest,",
            "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {name}_PushMaskRound(pDest, pMaskStackB, pNextRoundMaskBufferB, pLength);",
            "}",
            "",
            f"void {name}_Compat(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorkerA,",
            "    unsigned char* pWorkerB,",
            "    unsigned char* pDest,",
            "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
            "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],",
            "    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],",
            "    unsigned char (&)[kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {name}(pSource, pWorkerA, pWorkerB, pDest, pKeyStack, pMaskStackA, pNextRoundKeyBuffer, pNextRoundMaskBufferA, pLength);",
            "}",
        ]
    )


def collect_bundles(input_dir: Path) -> list[CandidateBundle]:
    code_paths = sorted(input_dir.glob("candidate_*/code.cpp"))
    if not code_paths:
        raise FileNotFoundError(f"no candidate_*/code.cpp files found under {input_dir}")
    bundles = [parse_bundle(path) for path in code_paths]
    bundles.sort(key=lambda bundle: bundle.candidate_id)
    return bundles


def pack_sequential(
    bundles: list[CandidateBundle],
    target_bytes: int,
    max_candidates_per_shard: int,
) -> list[list[CandidateBundle]]:
    shards: list[list[CandidateBundle]] = []
    current: list[CandidateBundle] = []
    current_bytes = 0

    for bundle in bundles:
        would_exceed_target = current and current_bytes + bundle.source_size_bytes > target_bytes
        would_exceed_count = max_candidates_per_shard > 0 and len(current) >= max_candidates_per_shard
        if would_exceed_target or would_exceed_count:
            shards.append(current)
            current = []
            current_bytes = 0
        current.append(bundle)
        current_bytes += bundle.source_size_bytes

    if current:
        shards.append(current)
    return shards


def pack_balanced(
    bundles: list[CandidateBundle],
    target_bytes: int,
    max_candidates_per_shard: int,
) -> list[list[CandidateBundle]]:
    ordered = sorted(bundles, key=lambda bundle: (-bundle.source_size_bytes, bundle.candidate_id))
    shard_bundles: list[list[CandidateBundle]] = []
    shard_sizes: list[int] = []

    for bundle in ordered:
        best_index = -1
        best_remaining = math.inf
        for index, current_size in enumerate(shard_sizes):
            if max_candidates_per_shard > 0 and len(shard_bundles[index]) >= max_candidates_per_shard:
                continue
            next_size = current_size + bundle.source_size_bytes
            if shard_bundles[index] and next_size > target_bytes:
                continue
            remaining = target_bytes - next_size
            if remaining < best_remaining:
                best_remaining = remaining
                best_index = index

        if best_index == -1:
            shard_bundles.append([bundle])
            shard_sizes.append(bundle.source_size_bytes)
            continue

        shard_bundles[best_index].append(bundle)
        shard_sizes[best_index] += bundle.source_size_bytes

    for shard in shard_bundles:
        shard.sort(key=lambda bundle: bundle.candidate_id)
    shard_bundles.sort(key=lambda shard: (min(bundle.candidate_id for bundle in shard), len(shard)))
    return shard_bundles


def render_shard_source(
    shard_index: int,
    bundles: list[CandidateBundle],
    source_label: str,
    pack_mode: str,
    target_bytes: int,
) -> str:
    candidate_ids = [bundle.candidate_id for bundle in bundles]
    function_blocks: list[str] = []
    for bundle in bundles:
        function_blocks.append(bundle.function_text.rstrip())
        compat_block = render_compat_wrappers(bundle)
        if compat_block:
            function_blocks.append(compat_block.rstrip())
    function_text = "\n\n".join(function_blocks)
    registry_blocks: list[str] = []
    for bundle in bundles:
        registry_blocks.append(render_registry_entry(bundle))
    registry_text = "\n".join(registry_blocks)
    return (
        '#include "HurricaneMatrix.hpp"\n'
        '#include "LightningMatrix.hpp"\n'
        '#include "TwistBreakers.hpp"\n'
        '#include "TyphoonMatrix.hpp"\n'
        '#include "TwistTypes.hpp"\n\n'
        "namespace twist {\n\n"
        "// Consolidated winner shard from tools/consolidate_winners.py\n"
        f"// source_dir={source_label}\n"
        f"// pack_mode={pack_mode}\n"
        f"// target_shard_bytes={target_bytes}\n"
        f"// candidate_id_range={min(candidate_ids)}-{max(candidate_ids)}\n"
        f"// candidate_ids={','.join(str(candidate_id) for candidate_id in candidate_ids)}\n"
        f"// shard_name={shard_name(shard_index)}\n\n"
        f"{function_text}\n\n"
        "const RegisteredCandidate kRegisteredCandidates[] = {\n"
        f"{registry_text}\n"
        "};\n\n"
        "const std::size_t kRegisteredCandidateCount =\n"
        "    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);\n\n"
        "}  // namespace twist\n"
    )


def write_shards(
    output_dir: Path,
    bundles_by_shard: list[list[CandidateBundle]],
    source_label: str,
    pack_mode: str,
    target_bytes: int,
) -> list[dict[str, object]]:
    shard_metadata: list[dict[str, object]] = []
    total_index = 1

    for shard_index, shard_bundles in enumerate(bundles_by_shard):
        name = shard_name(shard_index)
        shard_cpp_path = output_dir / f"{name}.cpp"
        source_text = render_shard_source(shard_index, shard_bundles, source_label, pack_mode, target_bytes)
        write_text(shard_cpp_path, source_text)

        ordered_candidate_ids = [bundle.candidate_id for bundle in shard_bundles]
        shard_metadata.append(
            {
                "name": name,
                "path": str(shard_cpp_path),
                "start_candidate_id": total_index,
                "end_candidate_id": total_index + len(shard_bundles) - 1,
                "candidate_count": len(shard_bundles),
                "source_candidate_ids": ordered_candidate_ids,
                "source_candidate_min": min(ordered_candidate_ids),
                "source_candidate_max": max(ordered_candidate_ids),
                "packed_source_bytes": sum(bundle.source_size_bytes for bundle in shard_bundles),
            }
        )
        total_index += len(shard_bundles)

    return shard_metadata


def main() -> int:
    parser = argparse.ArgumentParser(description="Re-shard winner candidate folders into smaller compileable shard sources.")
    parser.add_argument("--input-dir", default="practrand_a_34_passers")
    parser.add_argument("--output-dir", default="generated/consolidated_winners/shards_cpp")
    parser.add_argument("--index-output", default="generated/consolidated_winners/shards_index.json")
    parser.add_argument("--target-shard-kb", type=int, default=2048)
    parser.add_argument("--max-candidates-per-shard", type=int, default=0)
    parser.add_argument("--pack-mode", choices=["sequential", "balanced"], default="balanced")
    parser.add_argument("--clean", action="store_true")
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    index_output = Path(args.index_output)
    target_bytes = max(1, args.target_shard_kb) * 1024

    bundles = collect_bundles(input_dir)

    if args.clean:
        if output_dir.exists():
            shutil.rmtree(output_dir)
        if index_output.exists():
            index_output.unlink()

    output_dir.mkdir(parents=True, exist_ok=True)
    index_output.parent.mkdir(parents=True, exist_ok=True)

    if args.pack_mode == "balanced":
        bundles_by_shard = pack_balanced(bundles, target_bytes, args.max_candidates_per_shard)
    else:
        bundles_by_shard = pack_sequential(bundles, target_bytes, args.max_candidates_per_shard)

    shard_metadata = write_shards(output_dir, bundles_by_shard, str(input_dir), args.pack_mode, target_bytes)

    index = {
        "input_dir": str(input_dir),
        "output_dir": str(output_dir),
        "index_output": str(index_output),
        "source_candidate_count": len(bundles),
        "candidate_count": len(bundles),
        "shard_count": len(bundles_by_shard),
        "target_shard_bytes": target_bytes,
        "target_shard_kb": args.target_shard_kb,
        "max_candidates_per_shard": args.max_candidates_per_shard,
        "pack_mode": args.pack_mode,
        "source_bundle_sizes": {
            "min": min(bundle.source_size_bytes for bundle in bundles),
            "max": max(bundle.source_size_bytes for bundle in bundles),
            "total": sum(bundle.source_size_bytes for bundle in bundles),
        },
        "shards": shard_metadata,
    }
    write_text(index_output, json.dumps(index, indent=2))

    print(f"input_dir={input_dir}")
    print(f"candidate_count={len(bundles)}")
    print(f"shard_count={len(bundles_by_shard)}")
    print(f"target_shard_kb={args.target_shard_kb}")
    print(f"pack_mode={args.pack_mode}")
    print(f"output_dir={output_dir}")
    print(f"index_output={index_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
