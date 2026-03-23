#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from export_top_from_shards import extract_functions, shard_for_candidate


FUNCTION_NAME_RE = re.compile(r"static void (TwistCandidate_\d+)_KeySeed\(")


def parse_candidate_ids(text: str) -> list[int]:
    ids: list[int] = []
    for piece in text.split(","):
        piece = piece.strip()
        if not piece:
            continue
        if "-" in piece:
            start_text, end_text = piece.split("-", 1)
            start = int(start_text)
            end = int(end_text)
            if end < start:
                start, end = end, start
            ids.extend(range(start, end + 1))
        else:
            ids.append(int(piece))
    return sorted(set(ids))


def function_name_from_text(function_text: str, candidate_id: int) -> str:
    match = FUNCTION_NAME_RE.search(function_text)
    if match:
        return match.group(1)
    return f"TwistCandidate_{candidate_id:04d}"


def render_compat_wrappers(function_name: str, function_text: str) -> str:
    if f"{function_name}_MaskSeedA(" in function_text:
        return ""
    return "\n".join(
        [
            f"static void {function_name}_MaskSeedCompatA(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorker,",
            "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {function_name}_MaskSeed(pSource, pMaskStackA, pLength);",
            "}",
            "",
            f"static void {function_name}_MaskSeedCompatB(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorker,",
            "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {function_name}_MaskSeed(pSource, pMaskStackB, pLength);",
            "}",
            "",
            f"static void {function_name}_TwistBlockCompat(",
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
            f"  {function_name}_TwistBlock(pSource, pWorkerA, pWorkerB, pDest, pRound, pSalt, pKeyStack, pMaskStackA, pLength);",
            "}",
            "",
            f"static void {function_name}_PushMaskRoundCompatA(",
            "    unsigned char* pDest,",
            "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {function_name}_PushMaskRound(pDest, pMaskStackA, pNextRoundMaskBufferA, pLength);",
            "}",
            "",
            f"static void {function_name}_PushMaskRoundCompatB(",
            "    unsigned char* pDest,",
            "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&)[kMaskStackDepth][kMaskBytes],",
            "    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],",
            "    unsigned int pLength) {",
            f"  {function_name}_PushMaskRound(pDest, pMaskStackB, pNextRoundMaskBufferB, pLength);",
            "}",
            "",
            f"void {function_name}_Compat(",
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
            f"  {function_name}(pSource, pWorkerA, pWorkerB, pDest, pKeyStack, pMaskStackA, pNextRoundKeyBuffer, pNextRoundMaskBufferA, pLength);",
            "}",
        ]
    )


def render_candidate_cpp(candidate_id: int, function_name: str, function_text: str) -> str:
    has_dual_mask = f"{function_name}_MaskSeedA(" in function_text
    function_symbol = function_name if has_dual_mask else f"{function_name}_Compat"
    mask_seed_a_symbol = f"{function_name}_MaskSeedA" if has_dual_mask else f"{function_name}_MaskSeedCompatA"
    mask_seed_b_symbol = f"{function_name}_MaskSeedB" if has_dual_mask else f"{function_name}_MaskSeedCompatB"
    twist_block_symbol = f"{function_name}_TwistBlock" if has_dual_mask else f"{function_name}_TwistBlockCompat"
    push_mask_a_symbol = f"{function_name}_PushMaskRoundA" if has_dual_mask else f"{function_name}_PushMaskRoundCompatA"
    push_mask_b_symbol = f"{function_name}_PushMaskRoundB" if has_dual_mask else f"{function_name}_PushMaskRoundCompatB"
    compat_wrappers = render_compat_wrappers(function_name, function_text)
    return "\n".join(
        [
            '#include "HurricaneMatrix.hpp"',
            '#include "LightningMatrix.hpp"',
            '#include "TwistBreakers.hpp"',
            '#include "TyphoonMatrix.hpp"',
            '#include "TwistTypes.hpp"',
            "",
            "namespace twist {",
            "",
            function_text.rstrip(),
            *([ "", compat_wrappers.rstrip() ] if compat_wrappers else []),
            "",
            "const RegisteredCandidate kRegisteredCandidates[] = {",
            "  {",
            f"    {candidate_id},",
            f"    \"{function_name}\",",
            "    0,",
            "    0,",
            "    0,",
            '    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},',
            '    {{0, 0, 0}, "", "", "", "", "", "", 0, "", 0},',
            f"    \"{function_name}\",",
            f"    &{function_symbol},",
            f"    &{function_name}_KeySeed,",
            f"    &{function_name}_SaltSeed,",
            f"    &{mask_seed_a_symbol},",
            f"    &{mask_seed_b_symbol},",
            f"    &{twist_block_symbol},",
            f"    &{function_name}_PushKeyRound,",
            f"    &{push_mask_a_symbol},",
            f"    &{push_mask_b_symbol},",
            "  }",
            "};",
            "",
            "const std::size_t kRegisteredCandidateCount =",
            "    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);",
            "",
            "}  // namespace twist",
            "",
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Materialize per-candidate folders with bundled code and metadata.")
    parser.add_argument("--index", default="generated/shards_index.json")
    parser.add_argument("--output-dir", default="generated/candidates")
    parser.add_argument("--candidate-ids", default="")
    parser.add_argument("--limit", type=int, default=0)
    args = parser.parse_args()

    index_path = Path(args.index)
    output_dir = Path(args.output_dir)
    index = json.loads(index_path.read_text(encoding="utf-8"))

    if args.candidate_ids.strip():
      candidate_ids = parse_candidate_ids(args.candidate_ids)
    else:
      candidate_ids = list(range(1, int(index["candidate_count"]) + 1))

    if args.limit > 0:
      candidate_ids = candidate_ids[:args.limit]

    shard_to_ids: dict[Path, list[int]] = {}
    for candidate_id in candidate_ids:
      shard_path = shard_for_candidate(index, candidate_id)
      shard_to_ids.setdefault(Path(shard_path), []).append(candidate_id)

    shard_info_by_name = {
        Path(str(shard["path"])): shard
        for shard in index["shards"]
    }

    width = max(5, len(str(max(0, int(index["candidate_count"]) - 1))))
    output_dir.mkdir(parents=True, exist_ok=True)

    created = 0
    for shard_path, shard_candidate_ids in shard_to_ids.items():
      wanted_ids = set(shard_candidate_ids)
      functions_by_id = extract_functions(Path(shard_path), wanted_ids)
      shard_info = shard_info_by_name[Path(shard_path)]
      for candidate_id in shard_candidate_ids:
        function_text = functions_by_id.get(candidate_id)
        if function_text is None:
          continue
        function_name = function_name_from_text(function_text, candidate_id)
        local_index = candidate_id - int(shard_info["start_candidate_id"])
        ordinal = candidate_id - 1
        bundle_dir = output_dir / f"candidate_{ordinal:0{width}d}"
        bundle_dir.mkdir(parents=True, exist_ok=True)

        code_text = render_candidate_cpp(candidate_id, function_name, function_text)
        (bundle_dir / "code.cpp").write_text(code_text, encoding="utf-8")

        metadata = {
            "candidate_id": candidate_id,
            "candidate_ordinal": ordinal,
            "candidate_folder_name": bundle_dir.name,
            "function_name": function_name,
            "source_shard": str(shard_path),
            "source_shard_name": shard_info["name"],
            "source_local_index": local_index,
            "source_index": str(index_path),
            "bundle_dir": str(bundle_dir),
        }
        (bundle_dir / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        created += 1

    print(f"materialized {created} candidate folders")
    print(f"output_dir={output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
