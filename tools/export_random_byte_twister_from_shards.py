#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import random
import re
from pathlib import Path


FUNCTION_START_RE = re.compile(r"^// Candidate (\d+): ")


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


def _find_signature_end(lines: list[str], start_index: int) -> int:
    for index in range(start_index, len(lines)):
        if lines[index].rstrip().endswith(") {"):
            return index
    raise ValueError("function signature end not found")


def _function_inner_lines(lines: list[str], start_index: int) -> tuple[list[str], int]:
    signature_end = _find_signature_end(lines, start_index)
    end_index = len(lines) - 1
    while end_index >= 0 and lines[end_index].strip() == "":
        end_index -= 1
    if end_index < 0 or lines[end_index].strip() != "}":
        raise ValueError("function body does not end with a closing brace")
    return lines[signature_end + 1 : end_index], signature_end


def transform_legacy_candidate_source(source: str, candidate_id: int) -> str:
    function_name = f"TwistCandidate_{candidate_id:04d}"
    if f"static void {function_name}_KeySeed(" in source:
        return source.rstrip()

    lines = source.splitlines()
    body_start = next(
        index for index, line in enumerate(lines)
        if line.startswith(f"static void {function_name}_Body(")
    )
    wrapper_start = next(
        index for index, line in enumerate(lines)
        if line.startswith(f"void {function_name}(")
    )

    comment_lines = lines[:body_start]
    body_lines = lines[body_start:wrapper_start]
    wrapper_lines = lines[wrapper_start:]

    body_inner, _ = _function_inner_lines(body_lines, 0)
    wrapper_inner, _ = _function_inner_lines(wrapper_lines, 0)

    push_start = next(
        index for index, line in enumerate(body_inner)
        if "std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);" in line
    )
    twist_block_lines = body_inner[:push_start]
    push_key_lines = body_inner[push_start:]

    salt_decl_index = next(
        index for index, line in enumerate(wrapper_inner)
        if "unsigned char aSalt[kSaltBytes]{};" in line
    )
    salt_loop_index = next(
        index for index, line in enumerate(wrapper_inner)
        if line.strip().startswith("for (unsigned int salt_index = 0;")
    )
    round_loop_index = next(
        index for index, line in enumerate(wrapper_inner)
        if line.strip().startswith("unsigned int aRound = 0U;")
    )

    key_seed_lines = wrapper_inner[salt_decl_index + 1 : salt_loop_index]
    salt_stride_a = (candidate_id % 31) + 1
    salt_stride_b = ((candidate_id * 7) % 31) + 1
    salt_bias = ((candidate_id * 53) % 255) + 1
    salt_rotate = (candidate_id % 7) + 1
    salt_offset0 = ((candidate_id * 137) % (2 * 7680)) - 7680
    salt_offset1 = ((candidate_id * 281) % (2 * 7680)) - 7680
    push_stride_a = ((candidate_id * 5) % 31) + 1
    push_stride_b = ((candidate_id * 11) % 31) + 1
    push_stride_c = ((candidate_id * 17) % 31) + 1
    push_offset0 = ((candidate_id * 389) % (2 * 7680)) - 7680
    push_offset1 = ((candidate_id * 521) % (2 * 7680)) - 7680
    push_offset2 = ((candidate_id * 733) % (2 * 7680)) - 7680
    push_rotate = ((candidate_id * 13) % 31) + 1
    push_spread = ((candidate_id * 19) % 15) * 2 + 3
    push_row = candidate_id % 16
    push_bias = ((candidate_id * 97) % 255) + 1
    salt_seed_lines = [
        "  std::memset(pSalt, 0, kSaltBytes);",
        "  unsigned int aSourceIndex = 0U;",
        "  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {",
        f"    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * {salt_stride_a}u + ({salt_offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * {salt_stride_b}u + ({salt_offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const unsigned int aSaltIndex = (aSourceIndex * {salt_stride_a}u + static_cast<unsigned int>({salt_bias}u)) & 31U;",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pSource[idx0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pSource[idx1]);",
        f"    const std::uint32_t mix_value = static_cast<std::uint32_t>((((a + {push_bias}u) ^ ((b << {salt_rotate}u) & 0xFFu)) & 0xFFu));",
    ]
    if (candidate_id & 1) == 0:
        salt_seed_lines.append("    pSalt[aSaltIndex] ^= static_cast<unsigned char>(mix_value);")
    else:
        salt_seed_lines.append(
            "    pSalt[aSaltIndex] = static_cast<unsigned char>(pSalt[aSaltIndex] + static_cast<unsigned char>(mix_value));"
        )
    salt_seed_lines.extend(
        [
            "    ++aSourceIndex;",
            "  }",
        ]
    )
    push_key_lines = [
        "  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);",
        "  unsigned int aSourceIndex = 0U;",
        "  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {",
        f"    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * {push_stride_a}u + ({push_offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * {push_stride_b}u + ({push_offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * {push_stride_c}u + ({push_offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const unsigned int aKeyIndex = (aSourceIndex * {push_stride_a}u + static_cast<unsigned int>({push_rotate}u)) & 31U;",
        f"    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>({push_spread}u)) & 31U;",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);",
        "    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);",
        "    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);",
        "    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(",
        "        pKeyStack,",
        f"        static_cast<std::size_t>((aSourceIndex + {push_row}u) & 15U),",
        f"        static_cast<std::size_t>(aSourceIndex + {push_bias}u)));",
    ]
    if (candidate_id & 1) == 0:
        push_key_lines.extend(
            [
                "    const std::uint32_t mix_value = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);",
                "    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);",
                "    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);",
            ]
        )
    else:
        push_key_lines.extend(
            [
                "    const std::uint32_t mix_value = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);",
                "    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));",
                "    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));",
            ]
        )
    push_key_lines.extend(
        [
            "    ++aSourceIndex;",
            "  }",
            "  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);",
        ]
    )
    transformed: list[str] = []
    transformed.extend(comment_lines)
    transformed.extend(
        [
            f"static void {function_name}_KeySeed(",
            "    unsigned char* pSource,",
            "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
            "    unsigned int pLength) {",
            "  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
            "    return;",
            "  }",
            *key_seed_lines,
            "}",
            "",
            f"static void {function_name}_SaltSeed(",
            "    unsigned char* pSource,",
            "    unsigned char (&pSalt)[kSaltBytes],",
            "    unsigned int pLength) {",
            "  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
            "    return;",
            "  }",
            *salt_seed_lines,
            "}",
            "",
            f"static void {function_name}_TwistBlock(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorker,",
            "    unsigned char* pDest,",
            "    unsigned int pRound,",
            "    const unsigned char (&pSalt)[kSaltBytes],",
            "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
            "    unsigned int pLength) {",
            "  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
            "    return;",
            "  }",
            *twist_block_lines,
            "}",
            "",
            f"static void {function_name}_PushKeyRound(",
            "    unsigned char* pDest,",
            "    const unsigned char (&pSalt)[kSaltBytes],",
            "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
            "    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],",
            "    unsigned int pLength) {",
            "  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
            "    return;",
            "  }",
            *push_key_lines,
            "}",
            "",
            f"void {function_name}(",
            "    unsigned char* pSource,",
            "    unsigned char* pWorker,",
            "    unsigned char* pDest,",
            "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
            "    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],",
            "    unsigned int pLength) {",
            "  if (pLength == 0U) {",
            "    return;",
            "  }",
            "  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {",
            "    return;",
            "  }",
            "  unsigned char aSalt[kSaltBytes]{};",
            f"  {function_name}_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
            f"  {function_name}_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
            "  unsigned int aRound = 0U;",
            "  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {",
            "    unsigned char* aRoundSource = (aRound == 0U)",
            "        ? pSource",
            "        : (pDest + offset - PASSWORD_EXPANDED_SIZE);",
            "    unsigned char* aRoundDest = pDest + offset;",
            f"    {function_name}_TwistBlock(aRoundSource, pWorker, aRoundDest, aRound, aSalt, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
            f"    {function_name}_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
            "  }",
            "}",
        ]
    )
    return "\n".join(transformed).rstrip()


def export_function_name(exported_id: int) -> str:
    return f"TwistCandidate_{exported_id:04d}"


def rename_for_export(source: str, candidate_id: int, exported_id: int) -> str:
    original_name = re.escape(f"TwistCandidate_{candidate_id:04d}")
    pattern = re.compile(rf"{original_name}[A-Za-z]?")
    return pattern.sub(export_function_name(exported_id), source)


def render_byte_twister_cpp(
    selected_ids: list[int],
    functions_by_id: dict[int, str],
    start_digit: int,
) -> str:
    key_seed_switch_lines = []
    salt_seed_switch_lines = []
    twist_block_switch_lines = []
    push_key_switch_lines = []
    for type_index, candidate_id in enumerate(selected_ids):
        exported_id = start_digit + type_index
        function_name = export_function_name(exported_id)
        key_seed_switch_lines.append(
            f"    case {type_index}u: twist::{function_name}_KeySeed(pSource, pKeyStack, pLength); break;"
        )
        salt_seed_switch_lines.append(
            f"    case {type_index}u: twist::{function_name}_SaltSeed(pSource, pSaltBuffer, pLength); break;"
        )
        twist_block_switch_lines.append(
            f"    case {type_index}u: twist::{function_name}_TwistBlock(pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength); break;"
        )
        push_key_switch_lines.append(
            f"    case {type_index}u: twist::{function_name}_PushKeyRound(pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength); break;"
        )

    pieces = [
        '#include "ByteTwister.hpp"',
        "",
        "#include <array>",
        "#include <cstdlib>",
        "",
        "namespace twist {",
        "",
        "// Exported from shard sources for ByteTwister compatibility",
        f"// HOOK POINT: START_DIGIT={start_digit} (re-export with a different --start-digit to renumber this local series)",
        f"// selected_candidate_ids={','.join(str(candidate_id) for candidate_id in selected_ids)}",
        "",
    ]

    for type_index, candidate_id in enumerate(selected_ids):
        exported_id = start_digit + type_index
        pieces.append(
            f"// HOOK POINT: local export slot {type_index} maps shard candidate {candidate_id} "
            f"to {export_function_name(exported_id)}."
        )
        pieces.append(
            rename_for_export(
                transform_legacy_candidate_source(functions_by_id[candidate_id], candidate_id),
                candidate_id,
                exported_id,
            )
        )
        pieces.append("")

    pieces.extend(
        [
            "}  // namespace twist",
            "",
            "namespace peanutbutter::expansion::key_expansion {",
            "",
            "void ByteTwister::Get(unsigned char* pSource,",
            "                      unsigned char* pWorker,",
            "                      unsigned char* pDestination,",
            "                      unsigned int pLength) {",
            "  TwistBytes(mType, pSource, pWorker, pDestination, pLength);",
            "}",
            "",
            "void ByteTwister::SeedKey(Type pType,",
            "                          unsigned char* pSource,",
            "                          unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                          unsigned int pLength) {",
            "  SeedKeyByIndex(static_cast<unsigned char>(pType), pSource, pKeyStack, pLength);",
            "}",
            "",
            "void ByteTwister::SeedKeyByIndex(unsigned char pType,",
            "                                 unsigned char* pSource,",
            "                                 unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                                 unsigned int pLength) {",
            "  if (pSource == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {",
            "    return;",
            "  }",
            "  switch (pType % static_cast<unsigned char>(kTypeCount)) {",
            *key_seed_switch_lines,
            "    default: std::abort();",
            "  }",
            "}",
            "",
            "void ByteTwister::SeedSalt(Type pType,",
            "                           unsigned char* pSource,",
            "                           unsigned char (&pSaltBuffer)[twist::kSaltBytes],",
            "                           unsigned int pLength) {",
            "  SeedSaltByIndex(static_cast<unsigned char>(pType), pSource, pSaltBuffer, pLength);",
            "}",
            "",
            "void ByteTwister::SeedSaltByIndex(unsigned char pType,",
            "                                  unsigned char* pSource,",
            "                                  unsigned char (&pSaltBuffer)[twist::kSaltBytes],",
            "                                  unsigned int pLength) {",
            "  if (pSource == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {",
            "    return;",
            "  }",
            "  switch (pType % static_cast<unsigned char>(kTypeCount)) {",
            *salt_seed_switch_lines,
            "    default: std::abort();",
            "  }",
            "}",
            "",
            "void ByteTwister::TwistBlock(Type pType,",
            "                             unsigned char* pSource,",
            "                             unsigned char* pWorker,",
            "                             unsigned char* pDestination,",
            "                             unsigned int pRound,",
            "                             const unsigned char (&pSaltBuffer)[twist::kSaltBytes],",
            "                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                             unsigned int pLength) {",
            "  TwistBlockByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pRound, pSaltBuffer, pKeyStack, pLength);",
            "}",
            "",
            "void ByteTwister::TwistBlockByIndex(unsigned char pType,",
            "                                    unsigned char* pSource,",
            "                                    unsigned char* pWorker,",
            "                                    unsigned char* pDestination,",
            "                                    unsigned int pRound,",
            "                                    const unsigned char (&pSaltBuffer)[twist::kSaltBytes],",
            "                                    unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                                    unsigned int pLength) {",
            "  if (pSource == nullptr || pDestination == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {",
            "    return;",
            "  }",
            "  switch (pType % static_cast<unsigned char>(kTypeCount)) {",
            *twist_block_switch_lines,
            "    default: std::abort();",
            "  }",
            "}",
            "",
            "void ByteTwister::PushKeyRound(Type pType,",
            "                               unsigned char* pDestination,",
            "                               const unsigned char (&pSaltBuffer)[twist::kSaltBytes],",
            "                               unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                               unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],",
            "                               unsigned int pLength) {",
            "  PushKeyRoundByIndex(static_cast<unsigned char>(pType), pDestination, pSaltBuffer, pKeyStack, pNextRoundKeyBuffer, pLength);",
            "}",
            "",
            "void ByteTwister::PushKeyRoundByIndex(unsigned char pType,",
            "                                      unsigned char* pDestination,",
            "                                      const unsigned char (&pSaltBuffer)[twist::kSaltBytes],",
            "                                      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                                      unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],",
            "                                      unsigned int pLength) {",
            "  if (pDestination == nullptr || pLength < static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE)) {",
            "    return;",
            "  }",
            "  switch (pType % static_cast<unsigned char>(kTypeCount)) {",
            *push_key_switch_lines,
            "    default: std::abort();",
            "  }",
            "}",
            "",
            "void ByteTwister::TwistBytes(Type pType,",
            "                             unsigned char* pSource,",
            "                             unsigned char* pWorker,",
            "                             unsigned char* pDestination,",
            "                             unsigned int pLength) {",
            "  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};",
            "  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};",
            "  TwistBytes(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);",
            "}",
            "",
            "void ByteTwister::TwistBytes(Type pType,",
            "                             unsigned char* pSource,",
            "                             unsigned char* pWorker,",
            "                             unsigned char* pDestination,",
            "                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],",
            "                             unsigned int pLength) {",
            "  TwistBytesByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pKeyStack, pNextRoundKeyBuffer, pLength);",
            "}",
            "",
            "void ByteTwister::TwistBytesByIndex(unsigned char pType,",
            "                                    unsigned char* pSource,",
            "                                    unsigned char* pWorker,",
            "                                    unsigned char* pDestination,",
            "                                    unsigned int pLength) {",
            "  unsigned char aKeyBuffer[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]{};",
            "  unsigned char aNextRoundKeyBuffer[twist::kRoundKeyBytes]{};",
            "  TwistBytesByIndex(pType, pSource, pWorker, pDestination, aKeyBuffer, aNextRoundKeyBuffer, pLength);",
            "}",
            "",
            "void ByteTwister::TwistBytesByIndex(unsigned char pType,",
            "                                    unsigned char* pSource,",
            "                                    unsigned char* pWorker,",
            "                                    unsigned char* pDestination,",
            "                                    unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],",
            "                                    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],",
            "                                    unsigned int pLength) {",
            "  if (pSource == nullptr || pDestination == nullptr) {",
            "    return;",
            "  }",
            "  constexpr unsigned int kBlockLength = static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE);",
            "  if (pLength == 0U || (pLength % kBlockLength) != 0U) {",
            "    std::abort();",
            "  }",
            "",
            "  std::array<unsigned char, twist::PASSWORD_EXPANDED_SIZE> aWorkspace{};",
            "  unsigned char* aWorkspaceBuffer = (pWorker != nullptr) ? pWorker : aWorkspace.data();",
            "  unsigned char aSaltBuffer[twist::kSaltBytes]{};",
            "",
            "  SeedKeyByIndex(pType, pSource, pKeyStack, kBlockLength);",
            "  SeedSaltByIndex(pType, pSource, aSaltBuffer, kBlockLength);",
            "  for (unsigned int offset = 0U, round = 0U; offset < pLength; offset += kBlockLength, ++round) {",
            "    unsigned char* aRoundSource = (round == 0U) ? pSource : (pDestination + offset - kBlockLength);",
            "    unsigned char* aRoundDestination = pDestination + offset;",
            "    TwistBlockByIndex(pType, aRoundSource, aWorkspaceBuffer, aRoundDestination, round, aSaltBuffer, pKeyStack, kBlockLength);",
            "    PushKeyRoundByIndex(pType, aRoundDestination, aSaltBuffer, pKeyStack, pNextRoundKeyBuffer, kBlockLength);",
            "  }",
            "}",
            "",
            "}  // namespace peanutbutter::expansion::key_expansion",
            "",
        ]
    )

    return "\n".join(pieces)


def main() -> int:
    parser = argparse.ArgumentParser(description="Export a 16-entry ByteTwister compatibility file from shard candidates.")
    parser.add_argument("--index", default="generated/shards_index.json")
    parser.add_argument("--output", default="src/ByteTwister.cpp")
    parser.add_argument("--count", type=int, default=16)
    parser.add_argument("--seed", type=int, default=1337)
    parser.add_argument("--start-digit", type=int, default=0)
    args = parser.parse_args()

    index = json.loads(Path(args.index).read_text(encoding="utf-8"))
    candidate_count = int(index["candidate_count"])
    if candidate_count < args.count:
        raise ValueError(f"not enough candidates in shard index ({candidate_count} < {args.count})")

    rng = random.Random(args.seed)
    selected_ids = sorted(rng.sample(range(1, candidate_count + 1), args.count))

    shard_to_ids: dict[Path, set[int]] = {}
    for candidate_id in selected_ids:
        shard_path = shard_for_candidate(index, candidate_id)
        shard_to_ids.setdefault(shard_path, set()).add(candidate_id)

    functions_by_id: dict[int, str] = {}
    for shard_path, wanted_ids in shard_to_ids.items():
        functions_by_id.update(extract_functions(shard_path, wanted_ids))

    missing = [candidate_id for candidate_id in selected_ids if candidate_id not in functions_by_id]
    if missing:
        raise KeyError(f"failed to extract candidates: {missing}")

    output_text = render_byte_twister_cpp(selected_ids, functions_by_id, args.start_digit)
    Path(args.output).write_text(output_text, encoding="utf-8")
    print(f"exported {args.count} byte twisters to {args.output}")
    print(f"selected_candidate_ids={','.join(str(candidate_id) for candidate_id in selected_ids)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
