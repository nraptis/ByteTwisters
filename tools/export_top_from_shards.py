#!/usr/bin/env python3

from __future__ import annotations

import argparse
import bisect
import csv
import json
import re
from pathlib import Path


FUNCTION_START_RE = re.compile(r"^// Candidate (\d+): ")
CATEGORY_FIELDS = [
    "aes_score",
    "chacha_score",
    "zeros_score",
    "ones_score",
    "predictable_a_score",
    "predictable_b_score",
    "predictable_c_score",
]
PERCENTILE_FIELDS = [
    "avalanche_score",
    "bic_score",
    "completeness_score",
    "input_mean",
    "input_floor",
    "input_tail",
]


def parse_float(value: str) -> float:
    return float(value) if value.strip() else 0.0


def row_metric(row: dict[str, str], key: str) -> float:
    if key == "input_mean":
        return sum(parse_float(row.get(field, "0")) for field in CATEGORY_FIELDS) / float(len(CATEGORY_FIELDS))
    if key == "input_floor":
        return min(parse_float(row.get(field, "0")) for field in CATEGORY_FIELDS)
    if key == "input_tail":
        values = sorted(parse_float(row.get(field, "0")) for field in CATEGORY_FIELDS)
        return sum(values[:2]) / 2.0
    return parse_float(row.get(key, "0"))


def build_percentile_context(rows: list[dict[str, str]]) -> dict[str, list[float]]:
    return {
        key: sorted(row_metric(row, key) for row in rows)
        for key in PERCENTILE_FIELDS
    }


def percentile_rank(sorted_values: list[float], value: float) -> float:
    if not sorted_values:
        return 0.0
    if len(sorted_values) == 1:
        return 100.0
    index = bisect.bisect_right(sorted_values, value) - 1
    index = max(0, min(index, len(sorted_values) - 1))
    return 100.0 * float(index) / float(len(sorted_values) - 1)


def balanced_shortlist_score(row: dict[str, str], context: dict[str, list[float]]) -> float:
    avalanche_pct = percentile_rank(context["avalanche_score"], row_metric(row, "avalanche_score"))
    bic_pct = percentile_rank(context["bic_score"], row_metric(row, "bic_score"))
    mean_pct = percentile_rank(context["input_mean"], row_metric(row, "input_mean"))
    floor_pct = percentile_rank(context["input_floor"], row_metric(row, "input_floor"))
    tail_pct = percentile_rank(context["input_tail"], row_metric(row, "input_tail"))
    completeness_pct = percentile_rank(context["completeness_score"], row_metric(row, "completeness_score"))
    return (
        avalanche_pct * 0.22
        + bic_pct * 0.22
        + mean_pct * 0.20
        + floor_pct * 0.20
        + tail_pct * 0.10
        + completeness_pct * 0.06
    )


def finalist_strict_score(row: dict[str, str], context: dict[str, list[float]]) -> float:
    avalanche_pct = percentile_rank(context["avalanche_score"], row_metric(row, "avalanche_score"))
    bic_pct = percentile_rank(context["bic_score"], row_metric(row, "bic_score"))
    mean_pct = percentile_rank(context["input_mean"], row_metric(row, "input_mean"))
    floor_pct = percentile_rank(context["input_floor"], row_metric(row, "input_floor"))
    tail_pct = percentile_rank(context["input_tail"], row_metric(row, "input_tail"))
    completeness_pct = percentile_rank(context["completeness_score"], row_metric(row, "completeness_score"))
    score = (
        avalanche_pct * 0.18
        + bic_pct * 0.18
        + mean_pct * 0.16
        + floor_pct * 0.24
        + tail_pct * 0.16
        + completeness_pct * 0.08
    )

    input_floor = row_metric(row, "input_floor")
    input_tail = row_metric(row, "input_tail")
    avalanche = row_metric(row, "avalanche_score")
    bic = row_metric(row, "bic_score")
    completeness = row_metric(row, "completeness_score")

    if input_floor < 73.0:
        score -= min(20.0, (73.0 - input_floor) * 8.0)
    if input_tail < 73.2:
        score -= min(14.0, (73.2 - input_tail) * 7.0)
    if bic < 12.0:
        score -= min(16.0, (12.0 - bic) * 4.5)
    if avalanche < 98.5:
        score -= min(10.0, (98.5 - avalanche) * 5.0)
    if completeness < 86.8:
        score -= min(10.0, (86.8 - completeness) * 4.0)

    return max(0.0, score)


def rank_value(row: dict[str, str], rank_mode: str, context: dict[str, list[float]]) -> float:
    if rank_mode == "avalanche-bic-equal":
        avalanche = parse_float(row.get("avalanche_score", "0"))
        bic = parse_float(row.get("bic_score", "0"))
        return (avalanche + bic) * 0.5
    if rank_mode == "balanced-shortlist":
        return balanced_shortlist_score(row, context)
    if rank_mode == "finalist-strict":
        return finalist_strict_score(row, context)
    return parse_float(row.get("composite_score", "0"))


def load_top_rows(scores_path: Path, top_n: int, rank_mode: str) -> list[dict[str, str]]:
    with scores_path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = list(reader)

    context = build_percentile_context(rows)
    for row in rows:
        row["_rank_score"] = f"{rank_value(row, rank_mode, context):.6f}"
        row["_input_mean"] = f"{row_metric(row, 'input_mean'):.6f}"
        row["_input_floor"] = f"{row_metric(row, 'input_floor'):.6f}"
        row["_input_tail"] = f"{row_metric(row, 'input_tail'):.6f}"

    rows.sort(
        key=lambda row: (
            -parse_float(row.get("_rank_score", "0")),
            -parse_float(row.get("_input_floor", "0")),
            -parse_float(row.get("_input_mean", "0")),
            -parse_float(row.get("avalanche_score", "0")),
            -parse_float(row.get("bic_score", "0")),
            -parse_float(row.get("composite_score", "0")),
            int(row["candidate_id"]),
        )
    )
    return rows[:top_n]


def shard_for_candidate(index: dict, candidate_id: int) -> Path:
    for shard in index["shards"]:
        if shard["start_candidate_id"] <= candidate_id <= shard["end_candidate_id"]:
            return Path(shard["path"])
    raise KeyError(f"candidate_id {candidate_id} not found in shard index")


def extract_functions(shard_path: Path, wanted_ids: set[int]) -> dict[int, str]:
    found: dict[int, str] = {}
    current_id: int | None = None
    current_lines: list[str] = []
    in_functions = False

    with shard_path.open("r", encoding="utf-8") as handle:
        for line in handle:
            match = FUNCTION_START_RE.match(line)
            if match:
                in_functions = True
                if current_id is not None and current_id in wanted_ids:
                    found[current_id] = "".join(current_lines).rstrip()
                current_id = int(match.group(1))
                current_lines = [line]
                continue

            if in_functions and line.startswith("const RegisteredCandidate kRegisteredCandidates[] = {"):
                if current_id is not None and current_id in wanted_ids:
                    found[current_id] = "".join(current_lines).rstrip()
                break

            if current_id is not None:
                current_lines.append(line)

    return found


def standalone_prelude() -> list[str]:
    return [
        '#include "LightningMatrix.hpp"',
        "#include <array>",
        "#include <cstddef>",
        "#include <cstdint>",
        "#include <cstring>",
        "",
        "namespace twist {",
        "",
        "inline constexpr std::size_t PASSWORD_EXPANDED_SIZE = 7680;",
        "inline constexpr std::size_t kMatrixBlockBytes = 16;",
        "inline constexpr std::size_t kRoundKeyBytes = 32;",
        "inline constexpr std::size_t kRoundKeyStackDepth = 16;",
        "inline constexpr std::size_t kSaltBytes = 32;",
        "",
        "using TwistFunction = void (*)(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorkerA,",
        "    unsigned char* pWorkerB,",
        "    unsigned char* pDest,",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],",
        "    unsigned char (&pNextRoundMaskBufferA)[kMaskBytes],",
        "    unsigned char (&pNextRoundMaskBufferB)[kMaskBytes],",
        "    unsigned int pLength);",
        "using KeySeedFunction = void (*)(",
        "    unsigned char* pSource,",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned int pLength);",
        "using SaltSeedFunction = void (*)(",
        "    unsigned char* pSource,",
        "    unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned int pLength);",
        "using MaskSeedFunction = void (*)(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorker,",
        "    unsigned char (&pMaskStack)[kMaskStackDepth][kMaskBytes],",
        "    unsigned int pLength);",
        "using TwistBlockFunction = void (*)(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorkerA,",
        "    unsigned char* pWorkerB,",
        "    unsigned char* pDest,",
        "    unsigned int pRound,",
        "    const unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
        "    unsigned int pLength);",
        "using PushKeyRoundFunction = void (*)(",
        "    unsigned char* pDest,",
        "    const unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],",
        "    unsigned int pLength);",
        "using PushMaskRoundFunction = void (*)(",
        "    unsigned char* pDest,",
        "    unsigned char (&pMaskStackSelf)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pMaskStackOther)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],",
        "    unsigned int pLength);",
        "",
        "inline std::uint32_t RotateLeft32(std::uint32_t value, unsigned int amount) {",
        "  const unsigned int shift = amount & 31U;",
        "  if (shift == 0U) {",
        "    return value;",
        "  }",
        "  return static_cast<std::uint32_t>((value << shift) | (value >> (32U - shift)));",
        "}",
        "",
        "inline unsigned char KeyStackByte(",
        "    const unsigned char pKeyStack[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    std::size_t row,",
        "    std::size_t column) {",
        "  if (pKeyStack == nullptr) {",
        "    return 0U;",
        "  }",
        "  return pKeyStack[row % kRoundKeyStackDepth][column % kRoundKeyBytes];",
        "}",
        "",
        "inline void RotateKeyStack(",
        "    unsigned char pKeyStack[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    const unsigned char next_round_key[kRoundKeyBytes]) {",
        "  if (pKeyStack == nullptr || next_round_key == nullptr) {",
        "    return;",
        "  }",
        "  for (std::size_t row = 0; row + 1U < kRoundKeyStackDepth; ++row) {",
        "    std::memcpy(pKeyStack[row], pKeyStack[row + 1U], kRoundKeyBytes);",
        "  }",
        "  std::memcpy(pKeyStack[kRoundKeyStackDepth - 1U], next_round_key, kRoundKeyBytes);",
        "}",
        "",
        "inline int WrapRange(int index, int start, int end) {",
        "  const int length = end - start;",
        "  if (length <= 0) {",
        "    return start;",
        "  }",
        "  int offset = (index - start) % length;",
        "  if (offset < 0) {",
        "    offset += length;",
        "  }",
        "  return start + offset;",
        "}",
        "",
        "struct PhaseRecipe {",
        "  std::array<int, 3> offsets;",
        "  const char* op1;",
        "  const char* op2;",
        "  const char* op3;",
        "  const char* e_input;",
        "  const char* f_input;",
        "  const char* e_transform;",
        "  int e_transform_arg;",
        "  const char* f_transform;",
        "  int f_transform_arg;",
        "};",
        "",
        "struct RegisteredCandidate {",
        "  int candidate_id;",
        "  const char* function_name;",
        "  int op_budget;",
        "  int multiply_count;",
        "  int subop_count;",
        "  PhaseRecipe phase1;",
        "  PhaseRecipe phase2;",
        "  const char* recipe_summary;",
        "  TwistFunction function;",
        "  KeySeedFunction key_seed;",
        "  SaltSeedFunction salt_seed;",
        "  MaskSeedFunction mask_seed_a;",
        "  MaskSeedFunction mask_seed_b;",
        "  TwistBlockFunction twist_block;",
        "  PushKeyRoundFunction push_key_round;",
        "  PushMaskRoundFunction push_mask_round_a;",
        "  PushMaskRoundFunction push_mask_round_b;",
        "};",
        "",
        "struct ExportedCandidate {",
        "  int candidate_id;",
        "  const char* function_name;",
        "  double rank_score;",
        "  double avalanche_score;",
        "  double bic_score;",
        "  double input_mean_score;",
        "  double input_floor_score;",
        "  double input_tail_score;",
        "  double composite_score;",
        "  TwistFunction function;",
        "  KeySeedFunction key_seed;",
        "  SaltSeedFunction salt_seed;",
        "  MaskSeedFunction mask_seed_a;",
        "  MaskSeedFunction mask_seed_b;",
        "  TwistBlockFunction twist_block;",
        "  PushKeyRoundFunction push_key_round;",
        "  PushMaskRoundFunction push_mask_round_a;",
        "  PushMaskRoundFunction push_mask_round_b;",
        "};",
        "",
    ]


def include_prelude() -> list[str]:
    return [
        '#include "LightningMatrix.hpp"',
        '#include "TwistBreakers.hpp"',
        '#include "TwistTypes.hpp"',
        "",
        "namespace twist {",
        "",
    ]


def render_output(
    top_rows: list[dict[str, str]],
    functions_by_id: dict[int, str],
    rank_mode: str,
    standalone: bool,
) -> str:
    pieces = standalone_prelude() if standalone else include_prelude()
    pieces.append("// Top candidates exported from sharded measurement")
    pieces.append(f"// rank_mode={rank_mode}")
    if rank_mode == "avalanche-bic-equal":
        pieces.append("// rank_score = 0.5 * avalanche_score + 0.5 * bic_score")
    elif rank_mode == "balanced-shortlist":
        pieces.append("// rank_score = percentile blend of avalanche, BIC, input mean, input floor, input lower-tail, and completeness")
    elif rank_mode == "finalist-strict":
        pieces.append("// rank_score = stricter percentile blend with penalties for weak floor, weak tail, weak BIC, weak avalanche, and weak completeness")
    pieces.append("")

    exported_rows: list[dict[str, str]] = []
    for row in top_rows:
        candidate_id = int(row["candidate_id"])
        function_text = functions_by_id.get(candidate_id)
        if function_text is None:
            continue
        pieces.append(
            f"// rank={parse_float(row.get('_rank_score', '0')):.3f} avalanche={parse_float(row.get('avalanche_score', '0')):.3f} "
            f"bic={parse_float(row.get('bic_score', '0')):.3f} input_mean={parse_float(row.get('_input_mean', '0')):.3f} "
            f"input_floor={parse_float(row.get('_input_floor', '0')):.3f} input_tail={parse_float(row.get('_input_tail', '0')):.3f} "
            f"composite={parse_float(row.get('composite_score', '0')):.3f} candidate_id={candidate_id}"
        )
        pieces.append(function_text)
        pieces.append("")
        exported_rows.append(row)

    pieces.append("inline constexpr ExportedCandidate kExportedTopCandidates[] = {")
    for row in exported_rows:
        candidate_id = int(row["candidate_id"])
        function_name = row["function_name"]
        pieces.append(
            "    {"
            f"{candidate_id}, "
            f"\"{function_name}\", "
            f"{parse_float(row.get('_rank_score', '0')):.3f}, "
            f"{parse_float(row.get('avalanche_score', '0')):.3f}, "
            f"{parse_float(row.get('bic_score', '0')):.3f}, "
            f"{parse_float(row.get('_input_mean', '0')):.3f}, "
            f"{parse_float(row.get('_input_floor', '0')):.3f}, "
            f"{parse_float(row.get('_input_tail', '0')):.3f}, "
            f"{parse_float(row.get('composite_score', '0')):.3f}, "
            f"&{function_name}, "
            f"&{function_name}_KeySeed, "
            f"&{function_name}_SaltSeed, "
            f"&{function_name}_MaskSeedA, "
            f"&{function_name}_MaskSeedB, "
            f"&{function_name}_TwistBlock, "
            f"&{function_name}_PushKeyRound, "
            f"&{function_name}_PushMaskRoundA, "
            f"&{function_name}_PushMaskRoundB"
            "},"
        )
    pieces.append("};")
    pieces.append(
        "inline constexpr std::size_t kExportedTopCandidateCount = sizeof(kExportedTopCandidates) / sizeof(kExportedTopCandidates[0]);"
    )
    pieces.append("")
    pieces.append("namespace {")
    pieces.append(
        "constexpr PhaseRecipe kEmptyPhaseRecipe{{0, 0, 0}, \"\", \"\", \"\", \"\", \"\", \"\", 0, \"\", 0};"
    )
    pieces.append("}  // namespace")
    pieces.append("")
    pieces.append("extern const RegisteredCandidate kRegisteredCandidates[] = {")
    for row in exported_rows:
        function_name = row["function_name"]
        candidate_id = int(row["candidate_id"])
        pieces.append(
            "    {"
            f"{candidate_id}, "
            f"\"{function_name}\", "
            "0, 0, 0, "
            "kEmptyPhaseRecipe, "
            "kEmptyPhaseRecipe, "
            f"\"{function_name}\", "
            f"&{function_name}, "
            f"&{function_name}_KeySeed, "
            f"&{function_name}_SaltSeed, "
            f"&{function_name}_MaskSeedA, "
            f"&{function_name}_MaskSeedB, "
            f"&{function_name}_TwistBlock, "
            f"&{function_name}_PushKeyRound, "
            f"&{function_name}_PushMaskRoundA, "
            f"&{function_name}_PushMaskRoundB"
            "},"
        )
    pieces.append("};")
    pieces.append(
        "extern const std::size_t kRegisteredCandidateCount = sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);"
    )
    pieces.append("")
    pieces.append("}  // namespace twist")
    pieces.append("")
    return "\n".join(pieces)


def main() -> int:
    parser = argparse.ArgumentParser(description="Export top-ranked candidate functions from shard sources.")
    parser.add_argument("--scores", default="generated/twist_candidate_scores.csv")
    parser.add_argument("--index", default="generated/shards_index.json")
    parser.add_argument("--output", default="generated/top_twist_candidates.cpp")
    parser.add_argument("--top", type=int, default=25)
    parser.add_argument(
        "--rank-mode",
        choices=["composite", "avalanche-bic-equal", "balanced-shortlist", "finalist-strict"],
        default="composite",
    )
    parser.add_argument("--standalone", action="store_true")
    args = parser.parse_args()

    top_rows = load_top_rows(Path(args.scores), args.top, args.rank_mode)
    index = json.loads(Path(args.index).read_text(encoding="utf-8"))

    functions_by_id: dict[int, str] = {}
    shard_to_ids: dict[Path, set[int]] = {}
    for row in top_rows:
        candidate_id = int(row["candidate_id"])
        shard_path = shard_for_candidate(index, candidate_id)
        shard_to_ids.setdefault(shard_path, set()).add(candidate_id)

    for shard_path, wanted_ids in shard_to_ids.items():
        functions_by_id.update(extract_functions(shard_path, wanted_ids))

    output_text = render_output(top_rows, functions_by_id, args.rank_mode, args.standalone)
    Path(args.output).write_text(output_text, encoding="utf-8")
    print(f"exported top {args.top} candidates to {args.output}")
    print(f"rank_mode={args.rank_mode}")
    print(f"standalone={'true' if args.standalone else 'false'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
