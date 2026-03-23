#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import random
import re
import secrets
import shutil
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

PASSWORD_EXPANDED_SIZE = 7680
ALIGNMENT = 16
ROUND_KEY_BYTES = 32
ROUND_KEY_STACK_DEPTH = 16
MASK_BYTES = 8
MASK_STACK_DEPTH = 192
MASK_STACK_TOTAL_BYTES = MASK_BYTES * MASK_STACK_DEPTH
SALT_BYTES = 32
REPO_ROOT = Path(__file__).resolve().parents[1]
KNOBS_PATH = REPO_ROOT / "src" / "Knobs.hpp"

KEY_SEED_MIX_BOX = (
    0xFE, 0x72, 0x9A, 0x69, 0x1B, 0x51, 0x58, 0x30, 0x02, 0xE9, 0xC2, 0xBF, 0x9B, 0xD5, 0xA8, 0x15,
    0x79, 0x4C, 0x6E, 0xC7, 0xE4, 0x90, 0xE5, 0x8F, 0xA7, 0xB0, 0xDD, 0x85, 0x41, 0x19, 0xAD, 0xC5,
    0x3B, 0x06, 0xD6, 0xDE, 0x50, 0x66, 0xAC, 0xDB, 0x1C, 0xBD, 0x1E, 0xBA, 0xE6, 0xAE, 0x1F, 0xED,
    0x95, 0x49, 0xCC, 0x12, 0x63, 0x55, 0x28, 0x96, 0x93, 0x27, 0xEE, 0x77, 0x73, 0x5D, 0x43, 0x88,
    0x0D, 0x5B, 0x52, 0xF8, 0x09, 0xEA, 0x48, 0xC1, 0x7D, 0xE0, 0x98, 0x8C, 0x82, 0x86, 0xE1, 0x00,
    0xD7, 0x3F, 0xF7, 0x2A, 0xBB, 0x67, 0x1D, 0x35, 0xA0, 0x4A, 0x60, 0x33, 0x1A, 0xB7, 0x99, 0xB9,
    0x8B, 0x74, 0x01, 0xBC, 0xF0, 0x6C, 0x4F, 0x2F, 0xDA, 0x4D, 0xD4, 0x89, 0x8D, 0x75, 0xAF, 0x5E,
    0x7A, 0x2C, 0xC3, 0x14, 0x8A, 0x7C, 0xD1, 0x9C, 0x16, 0x47, 0x18, 0x3D, 0x56, 0x57, 0x37, 0x61,
    0x81, 0x2E, 0xD2, 0xE8, 0xC0, 0xF3, 0x32, 0x44, 0x13, 0x38, 0xD3, 0x24, 0x7F, 0xB3, 0xFD, 0x3E,
    0xB1, 0xCF, 0x29, 0x78, 0x05, 0x0F, 0x9F, 0x26, 0xEF, 0xE7, 0xC9, 0xA2, 0x7B, 0x42, 0xA4, 0x3C,
    0x92, 0x17, 0xA9, 0x34, 0xD8, 0xFB, 0xE2, 0x9E, 0xEB, 0x94, 0x6A, 0x0A, 0x0C, 0x6D, 0xDF, 0x22,
    0xF4, 0xF5, 0x0E, 0xC4, 0x83, 0x84, 0x59, 0x25, 0x4E, 0xD0, 0x08, 0x36, 0xAB, 0xA1, 0xA6, 0x31,
    0x10, 0x2D, 0xF6, 0xFF, 0x03, 0x4B, 0x07, 0x54, 0x71, 0x68, 0x20, 0x5C, 0x91, 0xF9, 0x7E, 0x39,
    0x53, 0x23, 0x70, 0x5A, 0x04, 0x45, 0x3A, 0xCB, 0x97, 0x8E, 0xE3, 0xCD, 0xD9, 0x87, 0xF2, 0xB4,
    0x62, 0x46, 0x11, 0xFA, 0xEC, 0xAA, 0xDC, 0xA3, 0x40, 0xB2, 0x6F, 0xCA, 0xB6, 0xCE, 0xBE, 0xC6,
    0x6B, 0x5F, 0xB8, 0x9D, 0x0B, 0xF1, 0xA5, 0x64, 0x2B, 0x80, 0x76, 0xB5, 0x21, 0x65, 0xFC, 0xC8,
)

MASK_SEED_MIX_BOX = (
    0x2A, 0x9D, 0xAD, 0x16, 0xD4, 0xD1, 0x93, 0x9C, 0xAC, 0xA4, 0xEC, 0xB2, 0x9E, 0x80, 0xB8, 0xAF,
    0xBC, 0xDB, 0xC3, 0x3E, 0xB6, 0xB0, 0xE5, 0x11, 0x33, 0x02, 0x05, 0x25, 0xFA, 0xFB, 0x91, 0xB3,
    0x6B, 0xF4, 0x1E, 0x66, 0x82, 0x22, 0xEE, 0x4F, 0xF9, 0xB9, 0x76, 0x1D, 0xA6, 0x01, 0xAE, 0x12,
    0x45, 0xF7, 0xA3, 0x2E, 0xE3, 0x7B, 0xC0, 0xBE, 0x67, 0x43, 0x98, 0x99, 0x19, 0x6D, 0xC1, 0x8C,
    0xF1, 0x13, 0x88, 0xB5, 0x30, 0x81, 0x28, 0x77, 0xA0, 0x08, 0x06, 0xCD, 0x87, 0x38, 0x46, 0x40,
    0x5E, 0x5A, 0xFC, 0xB7, 0x5C, 0x59, 0xBD, 0x9F, 0xF5, 0x92, 0xA7, 0x0D, 0x9A, 0x3C, 0x6E, 0xE8,
    0x27, 0x94, 0xA2, 0x14, 0x0F, 0x0A, 0x1C, 0x6C, 0xBB, 0x55, 0x18, 0xF2, 0x3A, 0xC9, 0xEA, 0x39,
    0x54, 0xDE, 0x41, 0x07, 0x17, 0xCB, 0x4D, 0x6F, 0x5D, 0x85, 0x8E, 0x47, 0x24, 0xD6, 0xDD, 0xC2,
    0xDA, 0xAB, 0x7A, 0x97, 0x72, 0xC4, 0x04, 0x00, 0x37, 0xE6, 0x4B, 0x1B, 0x2D, 0x95, 0x4E, 0xCE,
    0x0C, 0x74, 0x0B, 0x7E, 0x7C, 0xEB, 0x0E, 0x69, 0x7D, 0x4A, 0xB4, 0xD3, 0x70, 0x53, 0xD8, 0x03,
    0x75, 0xA1, 0x21, 0x3D, 0xD0, 0x4C, 0x8A, 0x49, 0x36, 0x34, 0x48, 0xA8, 0x79, 0x8D, 0xCC, 0x86,
    0x6A, 0x89, 0x62, 0x96, 0x65, 0x1F, 0x35, 0x5F, 0xD7, 0x83, 0x64, 0x5B, 0x2F, 0x58, 0xF3, 0xF0,
    0xD9, 0xDF, 0x8B, 0x44, 0xA5, 0x26, 0xE2, 0x1A, 0x90, 0x56, 0xCF, 0x57, 0xD5, 0x52, 0x09, 0xA9,
    0xD2, 0x7F, 0xAA, 0x23, 0xDC, 0x3F, 0xE4, 0xE7, 0xFD, 0x51, 0x68, 0x15, 0xED, 0x2B, 0x60, 0x3B,
    0x50, 0xC6, 0xE1, 0xC5, 0x71, 0x32, 0xFE, 0xE9, 0xE0, 0x2C, 0x29, 0xC7, 0x10, 0xF8, 0x84, 0x78,
    0x20, 0x73, 0xC8, 0xBA, 0xB1, 0xBF, 0x42, 0x61, 0x8F, 0xCA, 0xFF, 0x9B, 0x31, 0xF6, 0xEF, 0x63,
)


def render_named_mix_box_lines(values: tuple[int, ...], name: str = "aMixBox") -> list[str]:
    lines = [f"  static constexpr unsigned char {name}[{len(values)}] = {{"]
    for index in range(0, len(values), 16):
        row = ", ".join(f"0x{value:02X}U" for value in values[index:index + 16])
        suffix = "," if index + 16 < len(values) else ""
        lines.append(f"      {row}{suffix}")
    lines.append("  };")
    return lines


def render_mix_box_lines(values: tuple[int, ...]) -> list[str]:
    return render_named_mix_box_lines(values, "aMixBox")


def build_mix_box_values(rng: random.Random, count: int = 128) -> tuple[int, ...]:
    return tuple(rng.sample(range(256), count))


def mix_box_signature(values: tuple[int, ...]) -> str:
    acc_a = 0
    acc_b = 0
    for index, value in enumerate(values, start=1):
        acc_a = (acc_a + index * value) & 0xFFFFFFFF
        acc_b = ((acc_b << 5) ^ (value + index * 17)) & 0xFFFFFFFF
    return f"{len(values)}:{acc_a:08x}:{acc_b:08x}"


def load_cpp_knobs(path: Path) -> dict[str, int | bool]:
    pattern = re.compile(
        r"inline constexpr (?:std::size_t|std::uint64_t|int|bool)\s+(k[A-Za-z0-9_]+)\s*=\s*([^;]+);"
    )
    knobs: dict[str, int | bool] = {}
    text = path.read_text(encoding="utf-8")
    for match in pattern.finditer(text):
        name = match.group(1)
        raw_value = match.group(2).strip()
        if raw_value == "true":
            knobs[name] = True
        elif raw_value == "false":
            knobs[name] = False
        else:
            knobs[name] = int(raw_value)
    return knobs


CPP_KNOBS = load_cpp_knobs(KNOBS_PATH)

BASELINE_CANDIDATES: tuple[dict[str, Any], ...] = (
    {
        "candidate_id": -101,
        "function_name": "Baseline_AES256CTR",
        "stable_file_name": "baseline_aes256ctr.cpp",
        "op_budget": 0,
        "phase1_op_count": 0,
        "phase2_op_count": 0,
        "multiply_count": 0,
        "subop_count": 0,
        "phase1": {
            "offsets": [0, 0, 0],
            "e_input": "source",
            "f_input": "source",
            "op1": "ctr",
            "op2": "encrypt",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "phase2": {
            "offsets": [0, 0, 0],
            "e_input": "worker",
            "f_input": "worker",
            "op1": "ctr",
            "op2": "encrypt",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "recipe_summary": "baseline[aes256-ctr seeded from source; worker/dest filled from consecutive keystream]",
        "function_source": "",
        "is_baseline": True,
        "exportable": False,
    },
    {
        "candidate_id": -102,
        "function_name": "Baseline_ARIA256CTR",
        "stable_file_name": "baseline_aria256ctr.cpp",
        "op_budget": 0,
        "phase1_op_count": 0,
        "phase2_op_count": 0,
        "multiply_count": 0,
        "subop_count": 0,
        "phase1": {
            "offsets": [0, 0, 0],
            "e_input": "source",
            "f_input": "source",
            "op1": "ctr",
            "op2": "hash",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "phase2": {
            "offsets": [0, 0, 0],
            "e_input": "worker",
            "f_input": "worker",
            "op1": "ctr",
            "op2": "hash",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "recipe_summary": "baseline[aria256-ctr-style seeded from source; worker/dest filled from consecutive keystream]",
        "function_source": "",
        "is_baseline": True,
        "exportable": False,
    },
    {
        "candidate_id": -103,
        "function_name": "Baseline_ChaCha20CTR",
        "stable_file_name": "baseline_chacha20ctr.cpp",
        "op_budget": 0,
        "phase1_op_count": 0,
        "phase2_op_count": 0,
        "multiply_count": 0,
        "subop_count": 0,
        "phase1": {
            "offsets": [0, 0, 0],
            "e_input": "source",
            "f_input": "source",
            "op1": "ctr",
            "op2": "quarterround",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "phase2": {
            "offsets": [0, 0, 0],
            "e_input": "worker",
            "f_input": "worker",
            "op1": "ctr",
            "op2": "quarterround",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "recipe_summary": "baseline[chacha20-ctr seeded from source; worker/dest filled from consecutive keystream]",
        "function_source": "",
        "is_baseline": True,
        "exportable": False,
    },
    {
        "candidate_id": -104,
        "function_name": "Baseline_MersenneTwister",
        "stable_file_name": "baseline_mersenne_twister.cpp",
        "op_budget": 0,
        "phase1_op_count": 0,
        "phase2_op_count": 0,
        "multiply_count": 0,
        "subop_count": 0,
        "phase1": {
            "offsets": [0, 0, 0],
            "e_input": "source",
            "f_input": "source",
            "op1": "seed",
            "op2": "twist",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "phase2": {
            "offsets": [0, 0, 0],
            "e_input": "worker",
            "f_input": "worker",
            "op1": "seed",
            "op2": "temper",
            "op3": "stream",
            "e_transform": {"kind": "none", "arg": 0},
            "f_transform": {"kind": "none", "arg": 0},
        },
        "recipe_summary": "baseline[mersenne-twister seeded from source; worker/dest filled from consecutive output]",
        "function_source": "",
        "is_baseline": True,
        "exportable": False,
    },
)


def knob_int(name: str, fallback: int) -> int:
    value = CPP_KNOBS.get(name, fallback)
    return int(value) if isinstance(value, int) else fallback


def knob_bool(name: str, fallback: bool) -> bool:
    value = CPP_KNOBS.get(name, fallback)
    return bool(value) if isinstance(value, bool) else fallback


DEFAULT_COUNT = knob_int("kCandidateCount", 1000)
DEFAULT_TOP_N = knob_int("kTopCandidateCount", 16)
DEFAULT_RANDOMIZE_SEED = knob_bool("kRandomizeSeedByDefault", True)
DEFAULT_RANDOM_SEED = knob_int("kRandomSeed", 0)
DEFAULT_PHASE1_MIN_OPS = knob_int("kPhase1MinOps", 3)
DEFAULT_PHASE1_MAX_OPS = knob_int("kPhase1MaxOps", 4)
DEFAULT_PHASE2_MIN_OPS = knob_int("kPhase2MinOps", 3)
DEFAULT_PHASE2_MAX_OPS = knob_int("kPhase2MaxOps", 4)
DEFAULT_MAX_TRANSFORMS_TOTAL = knob_int("kMaxTransformsTotal", 2)
DEFAULT_ENABLE_THREE_MATRIX = knob_bool("kEnableThreeMatrix", True)
DEFAULT_THREE_MATRIX_RATIO_PERCENT = max(0, min(100, knob_int("kThreeMatrixRatioPercent", 40)))

PRIMARY_LIMIT_KEYS = {
    "add": "kMaxAddOps",
    "sub": "kMaxSubOps",
    "mul": "kMaxMulOps",
    "xor": "kMaxXorOps",
    "and": "kMaxAndOps",
    "or": "kMaxOrOps",
}

TRANSFORM_LIMIT_KEYS = {
    "add_const": "kMaxAddConstTransforms",
    "shl": "kMaxShiftLeftTransforms",
    "shr": "kMaxShiftRightTransforms",
    "not": "kMaxNotTransforms",
    "swap_nibbles": "kMaxSwapNibblesTransforms",
    "byte_lr8_left": "kMaxByteLR8LeftTransforms",
    "byte_lr8_right": "kMaxByteLR8RightTransforms",
}

PRIMARY_OPS = ("add", "sub", "mul", "xor", "and", "or")
TRANSFORM_LIBRARY = (
    {"kind": "none", "arg": 0},
    {"kind": "add_const", "arg": 3},
    {"kind": "add_const", "arg": 17},
    {"kind": "add_const", "arg": 29},
    {"kind": "shl", "arg": 1},
    {"kind": "shl", "arg": 2},
    {"kind": "shr", "arg": 1},
    {"kind": "shr", "arg": 2},
    {"kind": "not", "arg": 0},
    {"kind": "swap_nibbles", "arg": 0},
    {"kind": "byte_lr8_left", "arg": 1},
    {"kind": "byte_lr8_left", "arg": 2},
    {"kind": "byte_lr8_left", "arg": 3},
    {"kind": "byte_lr8_left", "arg": 4},
    {"kind": "byte_lr8_right", "arg": 1},
    {"kind": "byte_lr8_right", "arg": 2},
    {"kind": "byte_lr8_right", "arg": 3},
)

MATRIX_FAST_OPS = (
    "kRotateRowLeft",
    "kRotateRowRight",
    "kRotateColumnUp",
    "kRotateColumnDown",
    "kSwapRows",
    "kSwapColumns",
    "kXorRowIntoRow",
    "kAddRowIntoRow",
    "kXorColumnIntoColumn",
    "kAddColumnIntoColumn",
    "kWeaveRows",
    "kWeaveColumns",
)

MATRIX_SLOW_OPS = (
    "kRotateRight",
    "kRotateLeft",
    "kFlipHorizontal",
    "kFlipVertical",
    "kTranspose",
    "kFlipDiagB",
    "kRotateRing",
    "kTwistCross",
)

LEAN_MATRIX_FAST_OPS = (
    "kRotateRowLeft",
    "kRotateColumnDown",
    "kSwapRows",
    "kXorRowIntoRow",
    "kAddColumnIntoColumn",
    "kWeaveRows",
)

LEAN_MATRIX_SLOW_OPS = (
    "kTranspose",
    "kRotateRing",
    "kTwistCross",
)

DIFFUSION_MATRIX_FAST_OPS = (
    "kRotateRowLeft",
    "kRotateRowRight",
    "kRotateColumnUp",
    "kRotateColumnDown",
    "kXorRowIntoRow",
    "kAddRowIntoRow",
    "kXorColumnIntoColumn",
    "kAddColumnIntoColumn",
    "kWeaveRows",
    "kWeaveColumns",
)

DIFFUSION_MATRIX_SLOW_OPS = (
    "kRotateRight",
    "kRotateLeft",
    "kTranspose",
    "kFlipDiagB",
    "kRotateRing",
    "kTwistCross",
)

HURRICANE_FAST_OPS = (
    "kRotateRowLeft",
    "kRotateRowRight",
    "kRotateColumnUp",
    "kRotateColumnDown",
    "kSwapRows",
    "kSwapColumns",
    "kXorRowIntoRow",
    "kAddRowIntoRow",
    "kXorColumnIntoColumn",
    "kAddColumnIntoColumn",
    "kWeaveRows",
    "kWeaveColumns",
)

HURRICANE_SLOW_OPS = (
    "kRotateRight",
    "kRotateLeft",
    "kFlipHorizontal",
    "kFlipVertical",
    "kTranspose",
    "kFlipDiagB",
    "kRotateRing",
    "kTwistCross",
)

TYPHOON_FAST_OPS = (
    "kRotateRowLeft",
    "kRotateRowRight",
    "kRotateColumnUp",
    "kRotateColumnDown",
    "kSwapRows",
    "kSwapColumns",
    "kXorRowIntoRow",
    "kAddRowIntoRow",
    "kXorColumnIntoColumn",
    "kAddColumnIntoColumn",
    "kWeaveRows",
    "kWeaveColumns",
)

TYPHOON_SLOW_OPS = (
    "kFlipHorizontal",
    "kFlipVertical",
    "kRotateRing",
    "kTwistCross",
)


@dataclass(frozen=True)
class TransformSpec:
    kind: str
    arg: int = 0


@dataclass(frozen=True)
class PhaseSpec:
    offsets: tuple[int, int, int]
    e_input: str
    f_input: str
    op1: str
    op2: str
    op3: str
    e_transform: TransformSpec
    f_transform: TransformSpec


@dataclass(frozen=True)
class CandidateSpec:
    candidate_id: int
    function_name: str
    stable_file_name: str
    op_budget: int
    phase1_op_count: int
    phase2_op_count: int
    multiply_count: int
    subop_count: int
    phase1: PhaseSpec
    phase2: PhaseSpec
    recipe_summary: str
    function_source: str
    family: str = "classic"
    verbose_text: str = ""
    shape_key: str = ""
    matrix_enabled: bool = False
    matrix_breaker_summary: str = ""


@dataclass(frozen=True)
class MatrixPhasePlan:
    source_offsets: tuple[int, ...]
    control_offset: int
    feedback_offset: int
    fast_ops: tuple[tuple[str, ...], ...]
    slow_ops: tuple[tuple[str, ...], ...]
    carry_modes: tuple[str, ...]
    bridge_mode: str
    salt_bias: int
    salt_stride: int


@dataclass(frozen=True)
class MatrixRecipe:
    matrix_count: int
    phase1: MatrixPhasePlan
    phase2: MatrixPhasePlan
    source_mix_offsets: tuple[int, ...]
    emit_rotations: tuple[int, ...]
    final_mix: str
    phase1_seed_blocks: tuple[tuple[int, ...], ...]
    phase2_seed_blocks: tuple[tuple[int, ...], ...]


def aligned_offsets() -> list[int]:
    return list(range(0, PASSWORD_EXPANDED_SIZE, ALIGNMENT))


def choose_aligned_offset(rng: random.Random, exclude: set[int] | None = None) -> int:
    exclude = exclude or set()
    choices = [offset for offset in aligned_offsets() if offset not in exclude]
    if not choices:
        raise ValueError("no aligned offsets remain")
    return rng.choice(choices)


def choose_matrix_source_offsets(rng: random.Random, matrix_count: int) -> tuple[int, ...]:
    offsets = [0]
    used = {0}
    while len(offsets) < matrix_count:
        offset = choose_aligned_offset(rng, used)
        offsets.append(offset)
        used.add(offset)
    return tuple(offsets)


def choose_matrix_count(rng: random.Random) -> int:
    if DEFAULT_ENABLE_THREE_MATRIX and rng.randrange(100) < DEFAULT_THREE_MATRIX_RATIO_PERCENT:
        return 3
    return 2


def choose_matrix_phase_plan(
    rng: random.Random,
    matrix_count: int,
    phase_name: str,
    recipe_style: str,
) -> MatrixPhasePlan:
    used = set()
    control_offset = choose_aligned_offset(rng, used)
    used.add(control_offset)
    feedback_offset = choose_aligned_offset(rng, used)

    if recipe_style == "shape_shift":
        fast_op_pool = MATRIX_FAST_OPS if matrix_count == 3 else DIFFUSION_MATRIX_FAST_OPS
        slow_op_pool = DIFFUSION_MATRIX_SLOW_OPS
        fast_count_range = (3, 5)
        slow_count_range = (1, 2)
    elif recipe_style == "diffusion":
        fast_op_pool = DIFFUSION_MATRIX_FAST_OPS
        slow_op_pool = DIFFUSION_MATRIX_SLOW_OPS
        fast_count_range = (3, 5)
        slow_count_range = (1, 2)
    else:
        fast_op_pool = LEAN_MATRIX_FAST_OPS if matrix_count == 2 else MATRIX_FAST_OPS
        slow_op_pool = LEAN_MATRIX_SLOW_OPS if matrix_count == 2 else DIFFUSION_MATRIX_SLOW_OPS
        fast_count_range = (2, 4 if matrix_count == 2 else 5)
        slow_count_range = (1 if phase_name == "phase2" else 0, 1 if matrix_count == 2 else 2)

    return MatrixPhasePlan(
        source_offsets=choose_matrix_source_offsets(rng, matrix_count),
        control_offset=control_offset,
        feedback_offset=feedback_offset,
        fast_ops=tuple(
            tuple(rng.choice(fast_op_pool) for _ in range(rng.randint(*fast_count_range)))
            for _ in range(matrix_count)
        ),
        slow_ops=tuple(
            tuple(rng.choice(slow_op_pool) for _ in range(rng.randint(*slow_count_range)))
            for _ in range(matrix_count)
        ),
        carry_modes=tuple(rng.choice(("xor", "add")) for _ in range(matrix_count)),
        bridge_mode=rng.choice(
            ("xor_chain", "add_chain", "fan") if matrix_count == 2 else ("xor_chain", "add_chain", "braid")
        ),
        salt_bias=rng.randrange(1, 256, 2),
        salt_stride=rng.choice((3, 5, 7, 9, 11, 13, 17, 19, 29)),
    )


def random_seed_block(rng: random.Random) -> tuple[int, ...]:
    return tuple(rng.randrange(0, 256) for _ in range(ALIGNMENT))


def build_matrix_recipe(rng: random.Random, candidate_id: int) -> MatrixRecipe:
    matrix_count = choose_matrix_count(rng)
    style_choices = ("balanced", "diffusion") if matrix_count == 2 else ("balanced", "diffusion", "shape_shift")
    recipe_style = rng.choice(style_choices)
    return MatrixRecipe(
        matrix_count=matrix_count,
        phase1=choose_matrix_phase_plan(rng, matrix_count, "phase1", recipe_style),
        phase2=choose_matrix_phase_plan(rng, matrix_count, "phase2", recipe_style),
        source_mix_offsets=choose_matrix_source_offsets(rng, matrix_count),
        emit_rotations=tuple(rng.choice((1, 3, 5, 7, 9, 11, 13, 15)) for _ in range(matrix_count)),
        final_mix="xor" if (candidate_id % 2) == 1 else "add",
        phase1_seed_blocks=tuple(random_seed_block(rng) for _ in range(matrix_count)),
        phase2_seed_blocks=tuple(random_seed_block(rng) for _ in range(matrix_count)),
    )


def matrix_phase_key(phase: MatrixPhasePlan) -> tuple[Any, ...]:
    return (
        phase.source_offsets,
        phase.control_offset,
        phase.feedback_offset,
        phase.fast_ops,
        phase.slow_ops,
        phase.carry_modes,
        phase.bridge_mode,
        phase.salt_bias,
        phase.salt_stride,
    )


def matrix_recipe_key(recipe: MatrixRecipe) -> tuple[Any, ...]:
    return (
        recipe.matrix_count,
        matrix_phase_key(recipe.phase1),
        matrix_phase_key(recipe.phase2),
        recipe.source_mix_offsets,
        recipe.emit_rotations,
        recipe.final_mix,
        recipe.phase1_seed_blocks,
        recipe.phase2_seed_blocks,
    )


def matrix_phase_shape_key(phase: MatrixPhasePlan) -> tuple[Any, ...]:
    return (
        tuple(tuple(group) for group in phase.fast_ops),
        tuple(tuple(group) for group in phase.slow_ops),
        phase.carry_modes,
        phase.bridge_mode,
    )


def matrix_recipe_shape_key(recipe: MatrixRecipe) -> tuple[Any, ...]:
    return (
        recipe.matrix_count,
        matrix_phase_shape_key(recipe.phase1),
        matrix_phase_shape_key(recipe.phase2),
        recipe.final_mix,
    )


def matrix_manifest_phase(
    phase: MatrixPhasePlan,
    phase_label: str,
    output_name: str,
) -> PhaseSpec:
    offsets = list(phase.source_offsets[:2])
    offsets.append(phase.control_offset)
    while len(offsets) < 3:
        offsets.append(phase.feedback_offset)
    return PhaseSpec(
        offsets=tuple(offsets[:3]),
        e_input="matrix",
        f_input="control",
        op1=f"{phase_label}_matrix",
        op2=phase.bridge_mode,
        op3=output_name,
        e_transform=TransformSpec("none", 0),
        f_transform=TransformSpec("none", 0),
    )


def matrix_counts_text(groups: tuple[tuple[str, ...], ...]) -> str:
    return ",".join(str(len(group)) for group in groups)


def matrix_phase_summary(phase_label: str, phase: MatrixPhasePlan) -> str:
    source_text = ",".join(str(offset) for offset in phase.source_offsets)
    carry_text = ",".join(phase.carry_modes)
    return (
        f"{phase_label}[src={source_text}; ctrl={phase.control_offset}/{phase.feedback_offset}; "
        f"fast={matrix_counts_text(phase.fast_ops)}; slow={matrix_counts_text(phase.slow_ops)}; "
        f"carry={carry_text}; bridge={phase.bridge_mode}; mix=ctrl_bytes[{phase.salt_bias & 15},{phase.salt_stride & 15}]]"
    )


def format_seed_block(values: tuple[int, ...]) -> str:
    return ", ".join(f"0x{value:02X}u" for value in values)


def trailing_slow_op(slow_ops: tuple[str, ...]) -> str:
    return slow_ops[-1] if slow_ops else "kRotateRing"


def matrix_pre_mix_lines(
    phase_name: str,
    phase: MatrixPhasePlan,
    matrix_index: int,
) -> list[str]:
    lines = [
        f"load=LoadBlock16Wrapped({'source' if phase_name == 'phase1' else ('worker' if phase_name == 'phase2' else 'source')}, "
        f"{'chunk + ' + str(phase.source_offsets[matrix_index]) + 'u' if phase_name == 'phase1' else ('chunk + ' + str(matrix_index * ALIGNMENT) + 'u')})"
        if phase_name == "phase1"
        else f"load=LoadBlock16Wrapped(worker, chunk + {matrix_index * ALIGNMENT}u)",
        f"InjectAdd(salt, start={((matrix_index + 1) * 3) & 15})",
    ]
    if phase_name == "phase2":
        lines.insert(
            1,
            f"InjectAdd(source_mix_{matrix_index}=LoadBlock16Wrapped(source, chunk + source_mix_offsets[{matrix_index}]), "
            f"start={((matrix_index + 1) * 3) & 15})",
        )

    if phase.carry_modes[matrix_index] == "xor":
        lines.append(f"XorWith(carry_{matrix_index})")
        lines.append(f"InjectXor(control_a, start={((matrix_index + 1) * 5) & 15})")
    else:
        lines.append(f"AddWith(carry_{matrix_index})")
        lines.append(f"InjectAdd(control_a, start={((matrix_index + 1) * 5) & 15})")

    if phase_name == "phase1":
        lines.append(f"InjectXor(control_b, start={((matrix_index + 1) * 7) & 15})")
    else:
        lines.append(f"InjectAdd(salt, start={((matrix_index + 1) * 7) & 15})")
        lines.append(f"InjectXor(control_b, start={((matrix_index + 1) * 9) & 15})")
    return lines


def matrix_phase1_slow_formula(matrix_index: int, slow_index: int, matrix_count: int) -> str:
    return (
        f"arg0=u8(control_a[{(matrix_index * 3 + slow_index * 2 + 1) % 16}] + "
        f"salt[{(matrix_index * 5 + slow_index + 3) % 16}]), "
        f"arg1=u8(control_b[{(matrix_index * 7 + slow_index + 5) % 16}] ^ "
        f"carry_{(matrix_index + 1) % matrix_count}.FoldXor())"
    )


def matrix_phase1_fast_formula(matrix_index: int, fast_index: int, matrix_count: int) -> str:
    return (
        f"arg0=u8(control_a[{(matrix_index * 5 + fast_index + 1) % 16}] ^ "
        f"salt[{(matrix_index * 3 + fast_index + 4) % 16}]), "
        f"arg1=u8(control_b[{(matrix_index * 7 + fast_index + 2) % 16}] + "
        f"carry_{(matrix_index + 1) % matrix_count}.FoldAdd())"
    )


def matrix_phase2_slow_formula(matrix_index: int, slow_index: int, matrix_count: int) -> str:
    return (
        f"arg0=u8(control_a[{(matrix_index * 3 + slow_index + 2) % 16}] + "
        f"salt[{(matrix_index * 5 + slow_index + 1) % 16}]), "
        f"arg1=u8(control_b[{(matrix_index * 7 + slow_index + 4) % 16}] ^ "
        f"carry_{(matrix_index + 1) % matrix_count}.FoldXor())"
    )


def matrix_phase2_fast_formula(matrix_index: int, fast_index: int, matrix_count: int) -> str:
    return (
        f"arg0=u8(control_a[{(matrix_index * 5 + fast_index + 3) % 16}] ^ "
        f"salt[{(matrix_index * 3 + fast_index + 6) % 16}]), "
        f"arg1=u8(control_b[{(matrix_index * 7 + fast_index + 5) % 16}] + "
        f"carry_{(matrix_index + 1) % matrix_count}.FoldAdd())"
    )


def matrix_bridge_summary_lines(matrix_count: int, bridge_mode: str) -> list[str]:
    lines = []
    for line in render_matrix_bridge_lines("phase", matrix_count, bridge_mode):
        lines.append(
            line.strip()
            .replace("phase_matrix_", "matrix_")
            .replace("phase_carry_", "carry_")
        )
    return lines


def phase_to_manifest(phase: PhaseSpec) -> dict[str, Any]:
    return {
        "offsets": list(phase.offsets),
        "e_input": phase.e_input,
        "f_input": phase.f_input,
        "op1": phase.op1,
        "op2": phase.op2,
        "op3": phase.op3,
        "e_transform": asdict(phase.e_transform),
        "f_transform": asdict(phase.f_transform),
    }


def candidate_to_manifest(
    candidate: CandidateSpec,
    *,
    include_function_source: bool = False,
    include_verbose_text: bool = False,
) -> dict[str, Any]:
    item = {
        "candidate_id": candidate.candidate_id,
        "function_name": candidate.function_name,
        "stable_file_name": candidate.stable_file_name,
        "op_budget": candidate.op_budget,
        "phase1_op_count": candidate.phase1_op_count,
        "phase2_op_count": candidate.phase2_op_count,
        "multiply_count": candidate.multiply_count,
        "subop_count": candidate.subop_count,
        "phase1": phase_to_manifest(candidate.phase1),
        "phase2": phase_to_manifest(candidate.phase2),
        "recipe_summary": candidate.recipe_summary,
        "function_source": candidate.function_source,
        "family": candidate.family,
        "verbose_text": candidate.verbose_text,
        "shape_key": candidate.shape_key,
        "matrix_enabled": candidate.matrix_enabled,
        "matrix_breaker_summary": candidate.matrix_breaker_summary,
        "is_baseline": False,
        "exportable": True,
    }
    if include_function_source:
        item["function_source"] = candidate.function_source
    if include_verbose_text:
        item["verbose_text"] = candidate.verbose_text
    return item


def render_phase_initializer_from_manifest(phase: dict[str, Any]) -> str:
    offsets = phase["offsets"]
    e_transform = phase["e_transform"]
    f_transform = phase["f_transform"]
    return (
        "{"
        f"{{{offsets[0]}, {offsets[1]}, {offsets[2]}}}, "
        f"\"{phase['op1']}\", "
        f"\"{phase['op2']}\", "
        f"\"{phase['op3']}\", "
        f"\"{phase['e_input']}\", "
        f"\"{phase['f_input']}\", "
        f"\"{e_transform['kind']}\", "
        f"{e_transform['arg']}, "
        f"\"{f_transform['kind']}\", "
        f"{f_transform['arg']}"
        "}"
    )


def render_registry_entry_from_manifest(candidate: dict[str, Any]) -> str:
    return (
        "  {\n"
        f"    {candidate['candidate_id']},\n"
        f"    \"{candidate['function_name']}\",\n"
        f"    {candidate['op_budget']},\n"
        f"    {candidate['multiply_count']},\n"
        f"    {candidate['subop_count']},\n"
        f"    {render_phase_initializer_from_manifest(candidate['phase1'])},\n"
        f"    {render_phase_initializer_from_manifest(candidate['phase2'])},\n"
        f"    {json.dumps(candidate['recipe_summary'])},\n"
        f"    &{candidate['function_name']},\n"
        f"    &{candidate['function_name']}_KeySeed,\n"
        f"    &{candidate['function_name']}_SaltSeed,\n"
        f"    &{candidate['function_name']}_MaskSeedA,\n"
        f"    &{candidate['function_name']}_MaskSeedB,\n"
        f"    &{candidate['function_name']}_TwistBlock,\n"
        f"    &{candidate['function_name']}_PushKeyRound,\n"
        f"    &{candidate['function_name']}_PushMaskRoundA,\n"
        f"    &{candidate['function_name']}_PushMaskRoundB,\n"
        "  }"
    )


def stream_copy_text(source_path: Path, destination_handle: Any) -> None:
    with source_path.open("r", encoding="utf-8") as source_handle:
        for chunk in iter(lambda: source_handle.read(1024 * 1024), ""):
            if not chunk:
                break
            destination_handle.write(chunk)


def write_generated_cpp_from_parts(
    path: Path,
    functions_path: Path,
    registry_path: Path,
    seed: int,
    count: int,
    *,
    verbose: bool,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write('#include "HurricaneMatrix.hpp"\n')
        handle.write('#include "LightningMatrix.hpp"\n')
        handle.write('#include "TwistBreakers.hpp"\n')
        handle.write('#include "TyphoonMatrix.hpp"\n')
        handle.write('#include "TwistTypes.hpp"\n\n')
        handle.write("namespace twist {\n\n")
        handle.write("// Generated by tools/generate_twist_candidates.py\n")
        handle.write(f"// seed={seed} candidate_count={count}\n")
        if verbose:
            handle.write("// This verbose file is the source of truth used by the harness build.\n")
        handle.write("\n")
        stream_copy_text(functions_path, handle)
        handle.write("\n\nconst RegisteredCandidate kRegisteredCandidates[] = {\n")
        stream_copy_text(registry_path, handle)
        handle.write("\n};\n\n")
        handle.write("const std::size_t kRegisteredCandidateCount =\n")
        handle.write("    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);\n\n")
        handle.write("}  // namespace twist\n")


def write_verbose_text_from_parts(
    path: Path,
    details_path: Path,
    seed: int,
    count: int,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write("# Twist candidate verbose report\n")
        handle.write(f"seed={seed}\n")
        handle.write(f"candidate_count={count}\n\n")
        stream_copy_text(details_path, handle)


def write_manifest_from_parts(
    path: Path,
    candidate_items_path: Path,
    generated_count: int,
    seed: int,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write("{\n")
        manifest_header = [
            ("seed", seed),
            ("candidate_count", generated_count + len(BASELINE_CANDIDATES)),
            ("generated_candidate_count", generated_count),
            ("baseline_candidate_count", len(BASELINE_CANDIDATES)),
            ("password_expanded_size", PASSWORD_EXPANDED_SIZE),
            ("alignment", ALIGNMENT),
            ("knobs_path", str(KNOBS_PATH)),
            ("phase1_min_ops", DEFAULT_PHASE1_MIN_OPS),
            ("phase1_max_ops", DEFAULT_PHASE1_MAX_OPS),
            ("phase2_min_ops", DEFAULT_PHASE2_MIN_OPS),
            ("phase2_max_ops", DEFAULT_PHASE2_MAX_OPS),
            ("max_transforms_total", DEFAULT_MAX_TRANSFORMS_TOTAL),
        ]
        for key, value in manifest_header:
            handle.write(f"  {json.dumps(key)}: {json.dumps(value)},\n")
        handle.write('  "candidates": [\n')
        stream_copy_text(candidate_items_path, handle)
        for index, baseline in enumerate(BASELINE_CANDIDATES):
            if generated_count > 0 or index > 0:
                handle.write(",\n")
            handle.write("    ")
            handle.write(json.dumps(baseline, indent=2).replace("\n", "\n    "))
        handle.write("\n  ]\n")
        handle.write("}\n")


def choose_offsets(rng: random.Random) -> tuple[int, int, int]:
    aligned_offsets = list(range(ALIGNMENT, PASSWORD_EXPANDED_SIZE, ALIGNMENT))
    return tuple(sorted(rng.sample(aligned_offsets, 3)))


def normalize_phase_bounds(min_ops: int, max_ops: int) -> tuple[int, int]:
    clamped_min = max(3, min(5, min_ops))
    clamped_max = max(clamped_min, min(5, max_ops))
    return clamped_min, clamped_max


def limit_available(limit_value: int) -> bool:
    return limit_value != 0


def decrement_limit(limits: dict[str, int], key: str) -> None:
    if limits[key] > 0:
        limits[key] -= 1


def build_primary_limits() -> dict[str, int]:
    return {
        op_name: knob_int(knob_name, -1)
        for op_name, knob_name in PRIMARY_LIMIT_KEYS.items()
    }


def build_transform_limits() -> dict[str, int]:
    return {
        transform_name: knob_int(knob_name, -1)
        for transform_name, knob_name in TRANSFORM_LIMIT_KEYS.items()
    }


def choose_phase(rng: random.Random, primary_limits: dict[str, int]) -> PhaseSpec:
    e_input = rng.choice(("b", "c"))
    f_input = "c" if e_input == "b" else "b"

    ops: list[str] = []
    for _ in range(3):
        choices = [op_name for op_name in PRIMARY_OPS if limit_available(primary_limits[op_name])]
        if not choices:
            raise ValueError("no primary ops remain under current knob limits")
        op = rng.choice(choices)
        decrement_limit(primary_limits, op)
        ops.append(op)

    return PhaseSpec(
        offsets=choose_offsets(rng),
        e_input=e_input,
        f_input=f_input,
        op1=ops[0],
        op2=ops[1],
        op3=ops[2],
        e_transform=TransformSpec("none", 0),
        f_transform=TransformSpec("none", 0),
    )


def count_multiplies(phase: PhaseSpec) -> int:
    return sum(1 for op in (phase.op1, phase.op2, phase.op3) if op == "mul")


def choose_transform_counts(
    rng: random.Random,
    phase1_min_ops: int,
    phase1_max_ops: int,
    phase2_min_ops: int,
    phase2_max_ops: int,
    max_transforms_total: int,
) -> tuple[int, int]:
    phase1_min_ops, phase1_max_ops = normalize_phase_bounds(phase1_min_ops, phase1_max_ops)
    phase2_min_ops, phase2_max_ops = normalize_phase_bounds(phase2_min_ops, phase2_max_ops)

    phase1_min_transforms = max(0, phase1_min_ops - 3)
    phase1_max_transforms = max(0, phase1_max_ops - 3)
    phase2_min_transforms = max(0, phase2_min_ops - 3)
    phase2_max_transforms = max(0, phase2_max_ops - 3)

    if max_transforms_total >= 0 and phase1_min_transforms + phase2_min_transforms > max_transforms_total:
        raise ValueError("Knobs.hpp requires more transforms than the total transform cap allows")

    for _ in range(128):
        phase1_count = rng.randint(phase1_min_transforms, phase1_max_transforms)
        phase2_count = rng.randint(phase2_min_transforms, phase2_max_transforms)
        if max_transforms_total < 0 or phase1_count + phase2_count <= max_transforms_total:
            return phase1_count, phase2_count

    raise RuntimeError("unable to choose per-phase transform counts under current knob limits")


def pick_transform_targets(
    rng: random.Random,
    phase1_transform_count: int,
    phase2_transform_count: int,
) -> list[tuple[str, str]]:
    targets: list[tuple[str, str]] = []
    if phase1_transform_count > 0:
        targets.extend(rng.sample(
            [("phase1", "e_transform"), ("phase1", "f_transform")],
            phase1_transform_count,
        ))
    if phase2_transform_count > 0:
        targets.extend(rng.sample(
            [("phase2", "e_transform"), ("phase2", "f_transform")],
            phase2_transform_count,
        ))
    return targets


def with_transform(phase: PhaseSpec, field_name: str, transform: TransformSpec) -> PhaseSpec:
    if field_name == "e_transform":
        return PhaseSpec(
            offsets=phase.offsets,
            e_input=phase.e_input,
            f_input=phase.f_input,
            op1=phase.op1,
            op2=phase.op2,
            op3=phase.op3,
            e_transform=transform,
            f_transform=phase.f_transform,
        )
    return PhaseSpec(
        offsets=phase.offsets,
        e_input=phase.e_input,
        f_input=phase.f_input,
        op1=phase.op1,
        op2=phase.op2,
        op3=phase.op3,
        e_transform=phase.e_transform,
        f_transform=transform,
    )


def transform_summary(input_name: str, transform: TransformSpec) -> str:
    if transform.kind == "none":
        return input_name
    if transform.kind == "add_const":
        return f"{input_name}+{transform.arg}"
    if transform.kind == "shl":
        return f"{input_name}<<{transform.arg}"
    if transform.kind == "shr":
        return f"{input_name}>>{transform.arg}"
    if transform.kind == "not":
        return f"~{input_name}"
    if transform.kind == "swap_nibbles":
        return f"swap_nibbles({input_name})"
    if transform.kind == "byte_lr8_left":
        return f"byte_lr8_l{transform.arg}({input_name})"
    if transform.kind == "byte_lr8_right":
        return f"byte_lr8_r{transform.arg}({input_name})"
    raise ValueError(f"unsupported transform: {transform.kind}")


def phase_summary(phase: PhaseSpec, phase_label: str) -> str:
    e_rhs = transform_summary(phase.e_input, phase.e_transform)
    f_rhs = transform_summary(phase.f_input, phase.f_transform)
    offsets = ",".join(str(offset) for offset in phase.offsets)
    return (
        f"{phase_label}[offs={offsets};"
        f" e={phase.op1}(a,{e_rhs});"
        f" f={phase.op2}(d,{f_rhs});"
        f" out={phase.op3}(e,f)]"
    )


def transform_cpp_expr(input_expr: str, transform: TransformSpec) -> str:
    if transform.kind == "none":
        return input_expr
    if transform.kind == "add_const":
        return f"BytewiseAddConstant({input_expr}, {transform.arg}u)"
    if transform.kind == "shl":
        return f"BytewiseShiftLeft({input_expr}, {transform.arg}u)"
    if transform.kind == "shr":
        return f"BytewiseShiftRight({input_expr}, {transform.arg}u)"
    if transform.kind == "not":
        return f"BytewiseNot({input_expr})"
    if transform.kind == "swap_nibbles":
        return f"SwapNibbles({input_expr})"
    if transform.kind == "byte_lr8_left":
        return f"ByteLR8Left({input_expr}, {transform.arg}u)"
    if transform.kind == "byte_lr8_right":
        return f"ByteLR8Right({input_expr}, {transform.arg}u)"
    raise ValueError(f"unsupported transform: {transform.kind}")


def op_cpp_expr(op_name: str, left_expr: str, right_expr: str) -> str:
    if op_name == "add":
        return f"BytewiseAdd({left_expr}, {right_expr})"
    if op_name == "sub":
        return f"BytewiseSub({left_expr}, {right_expr})"
    if op_name == "mul":
        return f"BytewiseMul({left_expr}, {right_expr})"
    if op_name == "xor":
        return f"BytewiseXor({left_expr}, {right_expr})"
    if op_name == "and":
        return f"BytewiseAnd({left_expr}, {right_expr})"
    if op_name == "or":
        return f"BytewiseOr({left_expr}, {right_expr})"
    raise ValueError(f"unsupported op: {op_name}")


def render_phase_block(
    phase: PhaseSpec,
    phase_number: int,
    a_array: str,
    lookup_array: str,
    output_array: str,
) -> str:
    offset1, offset2, offset3 = phase.offsets
    e_rhs = transform_cpp_expr(phase.e_input, phase.e_transform)
    f_rhs = transform_cpp_expr(phase.f_input, phase.f_transform)
    e_expr = op_cpp_expr(phase.op1, "a", e_rhs)
    f_expr = op_cpp_expr(phase.op2, "d", f_rhs)
    out_expr = op_cpp_expr(phase.op3, "e", "f")

    return f"""  // Phase {phase_number}
for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; i += kMatrixBlockBytes) {{
    const std::size_t idx = i;
    const std::size_t width = std::min<std::size_t>(kMatrixBlockBytes, PASSWORD_EXPANDED_SIZE - idx);
    const ByteVec a = LoadVecWrapped({a_array}, idx);
    const ByteVec b = LoadVecWrapped({lookup_array}, (idx + {offset1}u) % PASSWORD_EXPANDED_SIZE);
    const ByteVec c = LoadVecWrapped({lookup_array}, (idx + {offset2}u) % PASSWORD_EXPANDED_SIZE);
    const ByteVec d = LoadVecWrapped({lookup_array}, (idx + {offset3}u) % PASSWORD_EXPANDED_SIZE);
    const ByteVec e = {e_expr};
    const ByteVec f = {f_expr};
    const ByteVec out = {out_expr};
    if (width == kMatrixBlockBytes) {{
      StoreVecContiguous({output_array}, idx, out);
    }} else {{
      StoreVecPartial({output_array}, idx, out, width);
    }}
  }}
"""


def render_phase_block_verbose(
    phase: PhaseSpec,
    phase_number: int,
    a_array: str,
    lookup_array: str,
    output_array: str,
) -> str:
    offset1, offset2, offset3 = phase.offsets
    e_input_expr = transform_cpp_expr("e_source", phase.e_transform)
    f_input_expr = transform_cpp_expr("f_source", phase.f_transform)
    e_expr = op_cpp_expr(phase.op1, "a", "e_input")
    f_expr = op_cpp_expr(phase.op2, "d", "f_input")
    out_expr = op_cpp_expr(phase.op3, "e", "f")

    return f"""  // Phase {phase_number}
for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; i += kMatrixBlockBytes) {{
    const std::size_t idx = i;
    const std::size_t width = std::min<std::size_t>(kMatrixBlockBytes, PASSWORD_EXPANDED_SIZE - idx);

    const std::size_t a_index = idx;
    const std::size_t b_index = (idx + {offset1}u) % PASSWORD_EXPANDED_SIZE;
    const std::size_t c_index = (idx + {offset2}u) % PASSWORD_EXPANDED_SIZE;
    const std::size_t d_index = (idx + {offset3}u) % PASSWORD_EXPANDED_SIZE;

    const ByteVec a = LoadVecWrapped({a_array}, a_index);
    const ByteVec b = LoadVecWrapped({lookup_array}, b_index);
    const ByteVec c = LoadVecWrapped({lookup_array}, c_index);
    const ByteVec d = LoadVecWrapped({lookup_array}, d_index);

    const ByteVec e_source = {phase.e_input};
    const ByteVec e_input = {e_input_expr};
    const ByteVec f_source = {phase.f_input};
    const ByteVec f_input = {f_input_expr};

    const ByteVec e = {e_expr};
    const ByteVec f = {f_expr};
    const ByteVec out = {out_expr};

    if (width == kMatrixBlockBytes) {{
      StoreVecContiguous({output_array}, idx, out);
    }} else {{
      StoreVecPartial({output_array}, idx, out, width);
    }}
  }}
"""


def render_function(candidate: CandidateSpec) -> str:
    comment_lines = [
        f"// Candidate {candidate.candidate_id}: {candidate.function_name}",
        f"// op_budget={candidate.op_budget} phase1_ops={candidate.phase1_op_count} "
        f"phase2_ops={candidate.phase2_op_count} multiply_count={candidate.multiply_count} "
        f"subop_count={candidate.subop_count}",
        f"// {candidate.recipe_summary}",
    ]
    phase1 = render_phase_block_verbose(candidate.phase1, 1, "source", "source", "worker")
    phase2 = render_phase_block_verbose(candidate.phase2, 2, "source", "worker", "dest")
    comments = "\n".join(comment_lines)
    return f"""{comments}
void {candidate.function_name}(
    const uint8_t source[PASSWORD_EXPANDED_SIZE],
    uint8_t worker[PASSWORD_EXPANDED_SIZE],
    uint8_t dest[PASSWORD_EXPANDED_SIZE]) {{
{phase1}
{phase2}}}
"""


def render_compact_function(candidate: CandidateSpec) -> str:
    comment_lines = [
        f"// Candidate {candidate.candidate_id}: {candidate.function_name}",
        f"// op_budget={candidate.op_budget} phase1_ops={candidate.phase1_op_count} "
        f"phase2_ops={candidate.phase2_op_count} multiply_count={candidate.multiply_count} "
        f"subop_count={candidate.subop_count}",
        f"// {candidate.recipe_summary}",
    ]
    phase1 = render_phase_block(candidate.phase1, 1, "source", "source", "worker")
    phase2 = render_phase_block(candidate.phase2, 2, "source", "worker", "dest")
    comments = "\n".join(comment_lines)
    return f"""{comments}
void {candidate.function_name}(
    const uint8_t source[PASSWORD_EXPANDED_SIZE],
    uint8_t worker[PASSWORD_EXPANDED_SIZE],
    uint8_t dest[PASSWORD_EXPANDED_SIZE]) {{
{phase1}
{phase2}}}
"""


def recipe_key(phase1: PhaseSpec, phase2: PhaseSpec, op_budget: int) -> tuple[Any, ...]:
    return (
        phase1.offsets,
        phase1.e_input,
        phase1.f_input,
        phase1.op1,
        phase1.op2,
        phase1.op3,
        phase1.e_transform.kind,
        phase1.e_transform.arg,
        phase1.f_transform.kind,
        phase1.f_transform.arg,
        phase2.offsets,
        phase2.e_input,
        phase2.f_input,
        phase2.op1,
        phase2.op2,
        phase2.op3,
        phase2.e_transform.kind,
        phase2.e_transform.arg,
        phase2.f_transform.kind,
        phase2.f_transform.arg,
        op_budget,
    )


def build_candidate(
    rng: random.Random,
    candidate_id: int,
    phase1_min_ops: int,
    phase1_max_ops: int,
    phase2_min_ops: int,
    phase2_max_ops: int,
    max_transforms_total: int,
) -> CandidateSpec:
    primary_limits = build_primary_limits()
    transform_limits = build_transform_limits()

    phase1 = choose_phase(rng, primary_limits)
    phase2 = choose_phase(rng, primary_limits)
    multiply_count = count_multiplies(phase1) + count_multiplies(phase2)
    phase1_transform_count, phase2_transform_count = choose_transform_counts(
        rng,
        phase1_min_ops=phase1_min_ops,
        phase1_max_ops=phase1_max_ops,
        phase2_min_ops=phase2_min_ops,
        phase2_max_ops=phase2_max_ops,
        max_transforms_total=max_transforms_total,
    )
    subop_count = phase1_transform_count + phase2_transform_count
    op_budget = 6 + subop_count

    targets = pick_transform_targets(rng, phase1_transform_count, phase2_transform_count)
    transforms = [TransformSpec(spec["kind"], spec["arg"]) for spec in TRANSFORM_LIBRARY if spec["kind"] != "none"]
    for phase_name, field_name in targets:
        allowed_transforms = [
            transform
            for transform in transforms
            if limit_available(transform_limits[transform.kind])
        ]
        if not allowed_transforms:
            raise ValueError("no transforms remain under current knob limits")
        transform = rng.choice(allowed_transforms)
        decrement_limit(transform_limits, transform.kind)
        if phase_name == "phase1":
            phase1 = with_transform(phase1, field_name, transform)
        else:
            phase2 = with_transform(phase2, field_name, transform)

    function_name = f"TwistCandidate_{candidate_id:04d}"
    stable_file_name = f"twist_candidate_{candidate_id:04d}.cpp"
    recipe_summary = f"{phase_summary(phase1, 'phase1')} {phase_summary(phase2, 'phase2')}"
    candidate_stub = CandidateSpec(
        candidate_id=candidate_id,
        function_name=function_name,
        stable_file_name=stable_file_name,
        op_budget=op_budget,
        phase1_op_count=3 + phase1_transform_count,
        phase2_op_count=3 + phase2_transform_count,
        multiply_count=multiply_count,
        subop_count=subop_count,
        phase1=phase1,
        phase2=phase2,
        recipe_summary=recipe_summary,
        function_source="",
    )
    function_source = render_function(candidate_stub)

    return CandidateSpec(
        candidate_id=candidate_id,
        function_name=function_name,
        stable_file_name=stable_file_name,
        op_budget=op_budget,
        phase1_op_count=3 + phase1_transform_count,
        phase2_op_count=3 + phase2_transform_count,
        multiply_count=multiply_count,
        subop_count=subop_count,
        phase1=phase1,
        phase2=phase2,
        recipe_summary=recipe_summary,
        function_source=function_source,
    )


def render_matrix_bridge_lines(phase_prefix: str, matrix_count: int, bridge_mode: str) -> list[str]:
    lines: list[str] = []
    if matrix_count == 2:
        if bridge_mode == "xor_chain":
            lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_0);")
            lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_carry_1);")
            lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_carry_0);")
        elif bridge_mode == "add_chain":
            lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
            lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_carry_0);")
            lines.append(f"    {phase_prefix}_matrix_1.AddWith({phase_prefix}_carry_1);")
        else:
            lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_carry_1);")
            lines.append(f"    {phase_prefix}_matrix_1.AddWith({phase_prefix}_carry_0);")
            lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_0);")
            lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
        return lines

    if bridge_mode == "xor_chain":
        lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_0);")
        lines.append(f"    {phase_prefix}_matrix_2.AddWith({phase_prefix}_matrix_1);")
        lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_matrix_2);")
        lines.append(f"    {phase_prefix}_matrix_1.AddWith({phase_prefix}_carry_2);")
    elif bridge_mode == "add_chain":
        lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
        lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_2);")
        lines.append(f"    {phase_prefix}_matrix_2.AddWith({phase_prefix}_carry_0);")
        lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_carry_1);")
    else:
        lines.append(f"    {phase_prefix}_matrix_2.XorWith({phase_prefix}_matrix_0);")
        lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
        lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_carry_2);")
        lines.append(f"    {phase_prefix}_matrix_2.AddWith({phase_prefix}_carry_1);")
        lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_carry_0);")
    return lines


def render_matrix_seed_lines(seed_blocks: tuple[tuple[int, ...], ...], prefix: str) -> list[str]:
    lines: list[str] = []
    for index, seed_block in enumerate(seed_blocks):
        lines.append(
            f"  static constexpr std::array<std::uint8_t, kMatrixBlockBytes> {prefix}_seed_{index} = "
            f"{{{{{format_seed_block(seed_block)}}}}};"
        )
    for index in range(len(seed_blocks)):
        lines.append(f"  LightningMatrix {prefix}_carry_{index}({prefix}_seed_{index}.data());")
    return lines


def render_dynamic_fast_dispatch(
    phase_prefix: str,
    matrix_index: int,
    selector_expr: str,
    arg0_expr: str,
    arg1_expr: str,
    pool: tuple[str, ...],
) -> list[str]:
    if not pool:
        return []
    lines = [
        f"    switch (static_cast<unsigned>({selector_expr}) % {len(pool)}u) {{"
    ]
    for pool_index, op_name in enumerate(pool):
        lines.append(
            f"      case {pool_index}u: {phase_prefix}_matrix_{matrix_index}.ApplyFastOp("
            f"LightningFastOp::{op_name}, {arg0_expr}, {arg1_expr}); break;"
        )
    lines.append("    }")
    return lines


def render_dynamic_slow_dispatch(
    phase_prefix: str,
    matrix_index: int,
    selector_expr: str,
    arg0_expr: str,
    arg1_expr: str,
    pool: tuple[str, ...],
) -> list[str]:
    if not pool:
        return []
    lines = [
        f"    switch (static_cast<unsigned>({selector_expr}) % {len(pool)}u) {{"
    ]
    for pool_index, op_name in enumerate(pool):
        lines.append(
            f"      case {pool_index}u: {phase_prefix}_matrix_{matrix_index}.ApplySlowOp("
            f"LightningSlowOp::{op_name}, {arg0_expr}, {arg1_expr}); break;"
        )
    lines.append("    }")
    return lines


def render_matrix_phase1_lines(recipe: MatrixRecipe) -> list[str]:
    chunk_bytes = recipe.matrix_count * ALIGNMENT
    phase = recipe.phase1
    lines = render_matrix_seed_lines(recipe.phase1_seed_blocks, "phase1")
    lines.append(f"  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += {chunk_bytes}u) {{")
    lines.append(
        f"    const auto phase1_control_a = LoadBlock16Wrapped(source, (chunk + {phase.control_offset}u) % PASSWORD_EXPANDED_SIZE);"
    )
    lines.append(
        f"    const auto phase1_control_b = LoadBlock16Wrapped(source, (chunk + {phase.feedback_offset}u) % PASSWORD_EXPANDED_SIZE);"
    )
    fold_terms = [
        f"phase1_carry_{index}.Fold{'Xor' if (index % 2) == 0 else 'Add'}()"
        for index in range(recipe.matrix_count)
    ]
    lines.append("    std::array<std::uint8_t, kMatrixBlockBytes> phase1_salt{};")
    lines.append(
        "    const std::uint8_t phase1_fold = static_cast<std::uint8_t>("
        + " + ".join(fold_terms)
        + ");"
    )
    lines.append("    for (std::size_t lane = 0; lane < kMatrixBlockBytes; ++lane) {")
    lines.append(
        "      phase1_salt[lane] = static_cast<std::uint8_t>("
        f"phase1_fold ^ "
        f"phase1_control_a[(lane + {phase.salt_bias & 15}u) & 15U] ^ "
        f"phase1_control_b[(lane + {phase.salt_stride & 15}u) & 15U]);"
    )
    lines.append("    }")

    for index, source_offset in enumerate(phase.source_offsets):
        lines.append(
            f"    const auto phase1_source_block_{index} = LoadBlock16Wrapped(source, (chunk + {source_offset}u) % PASSWORD_EXPANDED_SIZE);"
        )
        lines.append(f"    LightningMatrix phase1_matrix_{index}(phase1_source_block_{index}.data());")
        lines.append(
            f"    phase1_matrix_{index}.InjectAdd(phase1_salt.data(), phase1_salt.size(), {((index + 1) * 3) & 15}u);"
        )
        if phase.carry_modes[index] == "xor":
            lines.append(f"    phase1_matrix_{index}.XorWith(phase1_carry_{index});")
            lines.append(
                f"    phase1_matrix_{index}.InjectXor(phase1_control_a.data(), phase1_control_a.size(), {((index + 1) * 5) & 15}u);"
            )
        else:
            lines.append(f"    phase1_matrix_{index}.AddWith(phase1_carry_{index});")
            lines.append(
                f"    phase1_matrix_{index}.InjectAdd(phase1_control_a.data(), phase1_control_a.size(), {((index + 1) * 5) & 15}u);"
            )
        lines.append(
            f"    phase1_matrix_{index}.InjectXor(phase1_control_b.data(), phase1_control_b.size(), {((index + 1) * 7) & 15}u);"
        )

        lines.extend(
            render_dynamic_slow_dispatch(
                "phase1",
                index,
                f"phase1_source_block_{index}[{(index * 3 + 1) % 16}u]",
                f"static_cast<std::uint8_t>(phase1_control_a[{(index * 3 + 2) % 16}u] + phase1_source_block_{index}[{(index * 5 + 4) % 16}u])",
                f"static_cast<std::uint8_t>(phase1_control_b[{(index * 7 + 5) % 16}u] ^ phase1_carry_{(index + 1) % recipe.matrix_count}.FoldXor())",
                phase.slow_ops[index],
            )
        )
        lines.extend(
            render_dynamic_fast_dispatch(
                "phase1",
                index,
                f"phase1_control_a[{(index * 5 + 1) % 16}u]",
                f"static_cast<std::uint8_t>(phase1_source_block_{index}[{(index * 3 + 6) % 16}u] ^ phase1_control_b[{(index * 7 + 2) % 16}u])",
                f"static_cast<std::uint8_t>(phase1_source_block_{index}[{(index * 5 + 9) % 16}u] + phase1_carry_{(index + 1) % recipe.matrix_count}.FoldAdd())",
                phase.fast_ops[index],
            )
        )
        lines.extend(
            render_dynamic_fast_dispatch(
                "phase1",
                index,
                f"phase1_control_b[{(index * 7 + 3) % 16}u]",
                f"static_cast<std::uint8_t>(phase1_control_a[{(index * 5 + 8) % 16}u] + phase1_salt[{(index * 3 + 4) % 16}u])",
                f"static_cast<std::uint8_t>(phase1_source_block_{index}[{(index * 7 + 11) % 16}u] ^ phase1_salt[{(index * 5 + 10) % 16}u])",
                phase.fast_ops[index],
            )
        )

    lines.extend(render_matrix_bridge_lines("phase1", recipe.matrix_count, phase.bridge_mode))

    for index in range(recipe.matrix_count):
        lines.append(f"    std::array<std::uint8_t, kMatrixBlockBytes> phase1_store_{index}{{}};")
        lines.append(f"    std::array<std::uint8_t, kMatrixBlockBytes> phase1_emit_{index}{{}};")
        lines.append(f"    phase1_matrix_{index}.Store(phase1_store_{index}.data());")
        lines.append(
            f"    const std::uint8_t phase1_emit_fold_{index} = static_cast<std::uint8_t>("
            f"phase1_matrix_{index}.FoldXor() + "
            f"phase1_matrix_{(index + 1) % recipe.matrix_count}.FoldAdd() + "
            f"phase1_carry_{index}.FoldAdd() + "
            f"phase1_carry_{(index + 1) % recipe.matrix_count}.FoldXor());"
        )
        lines.append("    for (std::size_t lane = 0; lane < kMatrixBlockBytes; ++lane) {")
        if (index % 2) == 0:
            lines.append(
                f"      phase1_emit_{index}[lane] = static_cast<std::uint8_t>("
                f"phase1_store_{index}[(lane + {recipe.emit_rotations[index]}u) & 15U] ^ "
                f"phase1_control_a[(lane + {(index * 3 + 1) % 16}u) & 15U] ^ "
                f"phase1_salt[(lane + {(index * 5 + 3) % 16}u) & 15U] ^ "
                f"phase1_emit_fold_{index});"
            )
        else:
            lines.append(
                f"      phase1_emit_{index}[lane] = static_cast<std::uint8_t>("
                f"phase1_store_{index}[(lane + {recipe.emit_rotations[index]}u) & 15U] + "
                f"phase1_control_b[(lane + {(index * 7 + 2) % 16}u) & 15U] + "
                f"phase1_salt[(lane + {(index * 3 + 5) % 16}u) & 15U] + "
                f"phase1_emit_fold_{index});"
            )
        lines.append("    }")
        lines.append(f"    StoreBlock16Contiguous(worker, chunk + {index * ALIGNMENT}u, phase1_emit_{index});")
        lines.append(f"    phase1_carry_{index} = phase1_matrix_{index};")
        lines.append(
            f"    phase1_carry_{index}.ApplySlowOp("
            f"LightningSlowOp::{trailing_slow_op(phase.slow_ops[index])}, "
            f"phase1_control_b[{(index * 5 + 9) % 16}u], "
            f"phase1_control_a[{(index * 3 + 11) % 16}u]);"
        )
        lines.append(
            f"    phase1_carry_{index}.InjectAdd(phase1_salt.data(), phase1_salt.size(), {((index + 1) * 9) & 15}u);"
        )
        if phase.carry_modes[index] == "xor":
            lines.append(
                f"    phase1_carry_{index}.InjectXor(phase1_control_b.data(), phase1_control_b.size(), {((index + 1) * 11) & 15}u);"
            )
        else:
            lines.append(
                f"    phase1_carry_{index}.InjectAdd(phase1_control_b.data(), phase1_control_b.size(), {((index + 1) * 11) & 15}u);"
            )
    lines.append("  }")
    return lines


def render_matrix_phase2_lines(recipe: MatrixRecipe) -> list[str]:
    chunk_bytes = recipe.matrix_count * ALIGNMENT
    phase = recipe.phase2
    lines = render_matrix_seed_lines(recipe.phase2_seed_blocks, "phase2")
    lines.append(f"  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += {chunk_bytes}u) {{")
    lines.append(
        f"    const auto phase2_control_a = LoadBlock16Wrapped(source, (chunk + {phase.control_offset}u) % PASSWORD_EXPANDED_SIZE);"
    )
    lines.append(
        f"    const auto phase2_control_b = LoadBlock16Wrapped(worker, (chunk + {phase.feedback_offset}u) % PASSWORD_EXPANDED_SIZE);"
    )
    fold_terms = [
        f"phase2_carry_{index}.Fold{'Add' if (index % 2) == 0 else 'Xor'}()"
        for index in range(recipe.matrix_count)
    ]
    lines.append("    std::array<std::uint8_t, kMatrixBlockBytes> phase2_salt{};")
    lines.append(
        "    const std::uint8_t phase2_fold = static_cast<std::uint8_t>("
        + " + ".join(fold_terms)
        + ");"
    )
    lines.append("    for (std::size_t lane = 0; lane < kMatrixBlockBytes; ++lane) {")
    lines.append(
        "      phase2_salt[lane] = static_cast<std::uint8_t>("
        f"phase2_fold ^ "
        f"phase2_control_a[(lane + {phase.salt_bias & 15}u) & 15U] ^ "
        f"phase2_control_b[(lane + {phase.salt_stride & 15}u) & 15U]);"
    )
    lines.append("    }")

    for index, source_offset in enumerate(recipe.source_mix_offsets):
        lines.append(f"    const auto phase2_worker_block_{index} = LoadBlock16Wrapped(worker, chunk + {index * ALIGNMENT}u);")
        lines.append(
            f"    const auto phase2_source_mix_{index} = LoadBlock16Wrapped(source, (chunk + {source_offset}u) % PASSWORD_EXPANDED_SIZE);"
        )
        lines.append(f"    LightningMatrix phase2_matrix_{index}(phase2_worker_block_{index}.data());")
        lines.append(
            f"    phase2_matrix_{index}.InjectAdd(phase2_source_mix_{index}.data(), phase2_source_mix_{index}.size(), {((index + 1) * 3) & 15}u);"
        )
        if phase.carry_modes[index] == "xor":
            lines.append(f"    phase2_matrix_{index}.XorWith(phase2_carry_{index});")
            lines.append(
                f"    phase2_matrix_{index}.InjectXor(phase2_control_a.data(), phase2_control_a.size(), {((index + 1) * 5) & 15}u);"
            )
        else:
            lines.append(f"    phase2_matrix_{index}.AddWith(phase2_carry_{index});")
            lines.append(
                f"    phase2_matrix_{index}.InjectAdd(phase2_control_a.data(), phase2_control_a.size(), {((index + 1) * 5) & 15}u);"
            )
        lines.append(
            f"    phase2_matrix_{index}.InjectAdd(phase2_salt.data(), phase2_salt.size(), {((index + 1) * 7) & 15}u);"
        )
        lines.append(
            f"    phase2_matrix_{index}.InjectXor(phase2_control_b.data(), phase2_control_b.size(), {((index + 1) * 9) & 15}u);"
        )

        lines.extend(
            render_dynamic_slow_dispatch(
                "phase2",
                index,
                f"phase2_source_mix_{index}[{(index * 3 + 2) % 16}u]",
                f"static_cast<std::uint8_t>(phase2_control_a[{(index * 5 + 1) % 16}u] + phase2_worker_block_{index}[{(index * 7 + 4) % 16}u])",
                f"static_cast<std::uint8_t>(phase2_control_b[{(index * 3 + 11) % 16}u] ^ phase2_carry_{(index + 1) % recipe.matrix_count}.FoldXor())",
                phase.slow_ops[index],
            )
        )
        lines.extend(
            render_dynamic_fast_dispatch(
                "phase2",
                index,
                f"phase2_control_a[{(index * 5 + 3) % 16}u]",
                f"static_cast<std::uint8_t>(phase2_worker_block_{index}[{(index * 3 + 6) % 16}u] ^ phase2_source_mix_{index}[{(index * 5 + 8) % 16}u])",
                f"static_cast<std::uint8_t>(phase2_control_b[{(index * 7 + 5) % 16}u] + phase2_carry_{(index + 1) % recipe.matrix_count}.FoldAdd())",
                phase.fast_ops[index],
            )
        )
        lines.extend(
            render_dynamic_fast_dispatch(
                "phase2",
                index,
                f"phase2_control_b[{(index * 7 + 7) % 16}u]",
                f"static_cast<std::uint8_t>(phase2_source_mix_{index}[{(index * 5 + 10) % 16}u] + phase2_salt[{(index * 3 + 9) % 16}u])",
                f"static_cast<std::uint8_t>(phase2_worker_block_{index}[{(index * 7 + 13) % 16}u] ^ phase2_salt[{(index * 5 + 12) % 16}u])",
                phase.fast_ops[index],
            )
        )

    lines.extend(render_matrix_bridge_lines("phase2", recipe.matrix_count, phase.bridge_mode))

    for index, source_offset in enumerate(recipe.source_mix_offsets):
        lines.append(f"    std::array<std::uint8_t, kMatrixBlockBytes> phase2_store_{index}{{}};")
        lines.append(f"    std::array<std::uint8_t, kMatrixBlockBytes> phase2_emit_{index}{{}};")
        lines.append(
            f"    const auto phase2_final_source_{index} = LoadBlock16Wrapped(source, (chunk + {source_offset}u) % PASSWORD_EXPANDED_SIZE);"
        )
        lines.append(f"    phase2_matrix_{index}.Store(phase2_store_{index}.data());")
        lines.append(
            f"    const std::uint8_t phase2_fold_x_{index} = static_cast<std::uint8_t>("
            f"phase2_matrix_{index}.FoldXor() ^ "
            f"phase2_matrix_{(index + 1) % recipe.matrix_count}.FoldAdd() ^ "
            f"phase2_carry_{(index + 1) % recipe.matrix_count}.FoldXor());"
        )
        lines.append(
            f"    const std::uint8_t phase2_fold_a_{index} = static_cast<std::uint8_t>("
            f"phase2_matrix_{index}.FoldAdd() + "
            f"phase2_matrix_{(index - 1) % recipe.matrix_count}.FoldXor() + "
            f"phase2_carry_{index}.FoldAdd());"
        )
        lines.append("    for (std::size_t lane = 0; lane < kMatrixBlockBytes; ++lane) {")
        lines.append(
            f"      const std::uint8_t phase2_lane_mix_{index} = static_cast<std::uint8_t>("
            f"phase2_store_{index}[(lane + {recipe.emit_rotations[index]}u) & 15U] + "
            f"phase2_control_a[(lane + {(index * 3 + 7) % 16}u) & 15U] + "
            f"phase2_control_b[(lane + {(index * 5 + 9) % 16}u) & 15U] + "
            f"phase2_salt[(lane + {(index * 7 + 11) % 16}u) & 15U] + "
            f"phase2_fold_a_{index});"
        )
        if recipe.final_mix == "xor":
            lines.append(
                f"      phase2_emit_{index}[lane] = static_cast<std::uint8_t>("
                f"phase2_final_source_{index}[lane] ^ phase2_lane_mix_{index} ^ phase2_fold_x_{index});"
            )
        else:
            lines.append(
                f"      phase2_emit_{index}[lane] = static_cast<std::uint8_t>("
                f"phase2_final_source_{index}[lane] + phase2_lane_mix_{index} + phase2_fold_x_{index});"
            )
        lines.append("    }")
        lines.append(f"    StoreBlock16Contiguous(dest, chunk + {index * ALIGNMENT}u, phase2_emit_{index});")
        lines.append(f"    phase2_carry_{index} = phase2_matrix_{index};")
        lines.append(
            f"    phase2_carry_{index}.ApplySlowOp("
            f"LightningSlowOp::{trailing_slow_op(phase.slow_ops[index])}, "
            f"phase2_control_a[{(index * 5 + 13) % 16}u], "
            f"phase2_control_b[{(index * 3 + 10) % 16}u]);"
        )
        lines.append(
            f"    phase2_carry_{index}.InjectAdd(phase2_emit_{index}.data(), phase2_emit_{index}.size(), {((index + 1) * 13) & 15}u);"
        )
        if phase.carry_modes[index] == "xor":
            lines.append(
                f"    phase2_carry_{index}.InjectXor(phase2_control_b.data(), phase2_control_b.size(), {((index + 1) * 11) & 15}u);"
            )
        else:
            lines.append(
                f"    phase2_carry_{index}.InjectAdd(phase2_control_b.data(), phase2_control_b.size(), {((index + 1) * 11) & 15}u);"
            )
    lines.append("  }")
    return lines


def render_matrix_verbose_text(candidate: CandidateSpec, recipe: MatrixRecipe) -> str:
    lines = [
        (
            f"walk={recipe.matrix_count * ALIGNMENT} matrix_count={recipe.matrix_count} "
            f"final_mix={recipe.final_mix} phase1_ops={candidate.phase1_op_count} "
            f"phase2_ops={candidate.phase2_op_count}"
        ),
        "selection=byte-driven op pools; control/source bytes choose which matrix ops fire per chunk",
        "Phase 1",
        f"  source_offsets={list(recipe.phase1.source_offsets)}",
        f"  control_offsets=[{recipe.phase1.control_offset}, {recipe.phase1.feedback_offset}] from source/source",
        f"  carry_modes={list(recipe.phase1.carry_modes)} bridge={recipe.phase1.bridge_mode}",
        f"  fast_counts={[len(group) for group in recipe.phase1.fast_ops]}",
        f"  slow_counts={[len(group) for group in recipe.phase1.slow_ops]}",
        f"  seed_blocks={[list(block) for block in recipe.phase1_seed_blocks]}",
        (
            f"  salt_formula=phase1_salt[lane] = u8(phase1_fold + {recipe.phase1.salt_bias} + "
            f"lane*{recipe.phase1.salt_stride} + ((chunk/16)+lane)*{recipe.phase1.salt_stride + recipe.matrix_count})"
        ),
    ]
    for index in range(recipe.matrix_count):
        lines.append(f"  matrix_{index}_pre_mix:")
        for item in matrix_pre_mix_lines("phase1", recipe.phase1, index):
            lines.append(f"    - {item}")
        for slow_index, op_name in enumerate(recipe.phase1.slow_ops[index]):
            lines.append(
                f"    - slow_pool[{slow_index}] {op_name}: "
                f"{matrix_phase1_slow_formula(index, slow_index, recipe.matrix_count)}"
            )
        if not recipe.phase1.slow_ops[index]:
            lines.append("    - slow: none")
        for fast_index, op_name in enumerate(recipe.phase1.fast_ops[index]):
            lines.append(
                f"    - fast_pool[{fast_index}] {op_name}: "
                f"{matrix_phase1_fast_formula(index, fast_index, recipe.matrix_count)}"
            )
        lines.append(
            f"    - emit_fold=u8(matrix_{index}.FoldXor() + matrix_{(index + 1) % recipe.matrix_count}.FoldAdd() + carry_{index}.FoldAdd() + "
            f"carry_{(index + 1) % recipe.matrix_count}.FoldXor())"
        )
        if (index % 2) == 0:
            lines.append(
                f"    - emit[lane]=u8(store[(lane + {recipe.emit_rotations[index]}) & 15] ^ "
                f"control_a[(lane + {(index * 3 + 1) % 16}) & 15] ^ "
                f"salt[(lane + {(index * 5 + 3) % 16}) & 15] ^ emit_fold)"
            )
        else:
            lines.append(
                f"    - emit[lane]=u8(store[(lane + {recipe.emit_rotations[index]}) & 15] + "
                f"control_b[(lane + {(index * 7 + 2) % 16}) & 15] + "
                f"salt[(lane + {(index * 3 + 5) % 16}) & 15] + emit_fold)"
            )
        lines.append(
            f"    - carry_refresh={trailing_slow_op(recipe.phase1.slow_ops[index])}"
            f"(arg0=control_b[{(index * 5 + 9) % 16}], arg1=control_a[{(index * 3 + 11) % 16}])"
        )
        lines.append(
            f"    - carry_post=InjectAdd(salt, start={((index + 1) * 9) & 15}), "
            f"{'InjectXor' if recipe.phase1.carry_modes[index] == 'xor' else 'InjectAdd'}"
            f"(control_b, start={((index + 1) * 11) & 15})"
        )
    lines.append("  bridge:")
    for item in matrix_bridge_summary_lines(recipe.matrix_count, recipe.phase1.bridge_mode):
        lines.append(f"    - {item}")
    lines.extend(
        [
            "Phase 2",
            f"  source_mix_offsets={list(recipe.source_mix_offsets)}",
            f"  control_offsets=[{recipe.phase2.control_offset}, {recipe.phase2.feedback_offset}] from source/worker",
            f"  carry_modes={list(recipe.phase2.carry_modes)} bridge={recipe.phase2.bridge_mode}",
            f"  fast_counts={[len(group) for group in recipe.phase2.fast_ops]}",
            f"  slow_counts={[len(group) for group in recipe.phase2.slow_ops]}",
            f"  seed_blocks={[list(block) for block in recipe.phase2_seed_blocks]}",
            (
                f"  salt_formula=phase2_salt[lane] = u8(phase2_fold + {recipe.phase2.salt_bias} + "
                f"lane*{recipe.phase2.salt_stride} + ((chunk/16)+lane)*{recipe.phase2.salt_stride + recipe.matrix_count + 2})"
            ),
        ]
    )
    for index in range(recipe.matrix_count):
        lines.append(f"  matrix_{index}_pre_mix:")
        for item in matrix_pre_mix_lines("phase2", recipe.phase2, index):
            lines.append(f"    - {item}")
        for slow_index, op_name in enumerate(recipe.phase2.slow_ops[index]):
            lines.append(
                f"    - slow_pool[{slow_index}] {op_name}: "
                f"{matrix_phase2_slow_formula(index, slow_index, recipe.matrix_count)}"
            )
        if not recipe.phase2.slow_ops[index]:
            lines.append("    - slow: none")
        for fast_index, op_name in enumerate(recipe.phase2.fast_ops[index]):
            lines.append(
                f"    - fast_pool[{fast_index}] {op_name}: "
                f"{matrix_phase2_fast_formula(index, fast_index, recipe.matrix_count)}"
            )
        lines.append(
            f"    - fold_x=u8(matrix_{index}.FoldXor() ^ matrix_{(index + 1) % recipe.matrix_count}.FoldAdd() ^ "
            f"carry_{(index + 1) % recipe.matrix_count}.FoldXor())"
        )
        lines.append(
            f"    - fold_a=u8(matrix_{index}.FoldAdd() + matrix_{(index - 1) % recipe.matrix_count}.FoldXor() + "
            f"carry_{index}.FoldAdd())"
        )
        lines.append(
            f"    - lane_mix=u8(store[(lane + {recipe.emit_rotations[index]}) & 15] + "
            f"control_a[(lane + {(index * 3 + 7) % 16}) & 15] + "
            f"control_b[(lane + {(index * 5 + 9) % 16}) & 15] + "
            f"salt[(lane + {(index * 7 + 11) % 16}) & 15] + fold_a)"
        )
        if recipe.final_mix == "xor":
            lines.append(
                f"    - final[lane]=u8(final_source[lane] ^ lane_mix ^ fold_x) "
                f"where final_source=LoadBlock16Wrapped(source, chunk + {recipe.source_mix_offsets[index]})"
            )
        else:
            lines.append(
                f"    - final[lane]=u8(final_source[lane] + lane_mix + fold_x) "
                f"where final_source=LoadBlock16Wrapped(source, chunk + {recipe.source_mix_offsets[index]})"
            )
        lines.append(
            f"    - carry_refresh={trailing_slow_op(recipe.phase2.slow_ops[index])}"
            f"(arg0=control_a[{(index * 5 + 13) % 16}], arg1=control_b[{(index * 3 + 10) % 16}])"
        )
        lines.append(
            f"    - carry_post=InjectAdd(emit, start={((index + 1) * 13) & 15}), "
            f"{'InjectXor' if recipe.phase2.carry_modes[index] == 'xor' else 'InjectAdd'}"
            f"(control_b, start={((index + 1) * 11) & 15})"
        )
    lines.append("  bridge:")
    for item in matrix_bridge_summary_lines(recipe.matrix_count, recipe.phase2.bridge_mode):
        lines.append(f"    - {item}")
    lines.extend(
        [
            "Final",
            f"  emit_rotations={list(recipe.emit_rotations)}",
            f"  final_mix={recipe.final_mix}",
        ]
    )
    return "\n".join(lines)


def choose_signed_offset(rng: random.Random) -> int:
    magnitude = rng.randrange(1, PASSWORD_EXPANDED_SIZE)
    return magnitude if rng.choice((True, False)) else -magnitude


def circular_distance(a: int, b: int) -> int:
    diff = abs(abs(a) - abs(b))
    return min(diff, PASSWORD_EXPANDED_SIZE - diff)


def choose_mechanical_offsets(rng: random.Random) -> tuple[int, int, int]:
    offsets: list[int] = []
    while len(offsets) < 3:
        magnitude = rng.randrange(97, PASSWORD_EXPANDED_SIZE - 1)
        candidate = magnitude if rng.choice((True, False)) else -magnitude
        if any(circular_distance(candidate, existing) < 257 for existing in offsets):
            continue
        offsets.append(candidate)
    return tuple(offsets)


def choose_plain_term(rng: random.Random) -> bool:
    return rng.random() < 0.40


def choose_scalar_loop_spec(rng: random.Random, stage: str, loop_index: int) -> dict[str, Any]:
    if stage == "pre":
        input_pool = ("source", "source", "worker")
    else:
        input_pool = ("source", "worker", "worker", "dest")
    return {
        "offsets": (choose_signed_offset(rng), choose_signed_offset(rng), choose_signed_offset(rng)),
        "inputs": (
            rng.choice(input_pool),
            rng.choice(input_pool),
            rng.choice(input_pool),
        ),
        "template": rng.randrange(4),
        "shift_a": rng.randint(1, 3),
        "shift_b": rng.randint(1, 3),
        "const_a": rng.randint(1, 31),
        "const_b": rng.randint(1, 31),
        "mul_a": rng.choice((1, 3, 5, 7)),
        "mul_b": rng.choice((1, 3, 5, 7)),
        "key_row": rng.randrange(ROUND_KEY_STACK_DEPTH),
        "key_offset": rng.randint(0, ROUND_KEY_BYTES - 1),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "twiddle_mode": rng.choice(("xor", "add", "mix")),
        "twiddle_rotate": rng.randint(1, 11),
        "twiddle_const": rng.randint(1, 255),
        "write_mode": "assign" if loop_index == 0 else rng.choice(("xor", "add")),
    }


def choose_mechanical_loop_spec(rng: random.Random, stage: str) -> dict[str, Any]:
    if stage == "loop1":
        inputs = ("source", "source", "source")
        target = "dest"
        write_mode = "assign"
        data_index_count = rng.choice((1, 2, 2, 3))
        chained_indices = False
    elif stage == "loop2":
        inputs = ("dest", "dest", "dest")
        target = "worker"
        write_mode = "assign"
        data_index_count = rng.choice((2, 2, 3, 3))
        chained_indices = rng.random() < 0.45
    else:
        inputs = (
            "source",
            "worker",
            rng.choice(("source", "worker")),
        )
        target = "dest"
        write_mode = rng.choice(("xor", "xor", "add"))
        data_index_count = 3
        chained_indices = False

    return {
        "stage": stage,
        "target": target,
        "data_index_count": data_index_count,
        "chained_indices": chained_indices,
        "offsets": choose_mechanical_offsets(rng),
        "inputs": inputs,
        "template": rng.randrange(4),
        "shift_a": rng.randint(1, 5),
        "shift_b": rng.randint(1, 5),
        "const_a": rng.randint(1, 31),
        "const_b": rng.randint(1, 31),
        "mul_a": rng.choice((3, 5)),
        "mul_b": rng.choice((3, 5)),
        "key_row": rng.randrange(ROUND_KEY_STACK_DEPTH),
        "key_offset": rng.randint(0, ROUND_KEY_BYTES - 1),
        "key_const": rng.randint(1, 31),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "salt_const": rng.randint(1, 31),
        "twiddle_mode": rng.choice(("xor", "add", "mix")),
        "twiddle_rotate": rng.randint(1, 11),
        "twiddle_const": rng.randint(1, 255),
        "twiddle_shift": rng.randint(1, 3),
        "write_mode": write_mode,
        "feedback_mode": rng.choice(("xor", "add", "mix")),
        "feedback_rotate": rng.randint(1, 13),
        "feedback_const": rng.randint(1, 255),
        "feedback_seed": rng.getrandbits(32),
        "feedback_term_const": rng.randint(1, 31),
        "plain_d_a": choose_plain_term(rng),
        "plain_d_b": choose_plain_term(rng),
        "plain_d_c": choose_plain_term(rng),
        "plain_e_a": choose_plain_term(rng),
        "plain_e_b": choose_plain_term(rng),
        "plain_e_c": choose_plain_term(rng),
        "plain_key": choose_plain_term(rng),
        "plain_salt": choose_plain_term(rng),
        "plain_twiddle": choose_plain_term(rng),
        "plain_feedback": choose_plain_term(rng),
    }


def maybe_plain(base_expr: str, transformed_expr: str, plain: bool) -> str:
    return f"({base_expr})" if plain else transformed_expr


def buffer_cpp_name(name: str) -> str:
    mapping = {
        "source": "pSource",
        "dest": "pDest",
        "workera": "pWorkerA",
        "workerb": "pWorkerB",
        "worker": "pWorkerA",
    }
    if name not in mapping:
        raise ValueError(f"unsupported buffer name: {name}")
    return mapping[name]


def render_scalar_loop_body(spec: dict[str, Any], target_array: str) -> list[str]:
    input_a, input_b, input_c = spec["inputs"]
    input_a_cpp = buffer_cpp_name(input_a)
    input_b_cpp = buffer_cpp_name(input_b)
    input_c_cpp = buffer_cpp_name(input_c)
    target_cpp = buffer_cpp_name(target_array)
    offset_a, offset_b, offset_c = spec["offsets"]
    data_index_count = spec["data_index_count"]
    shift_a = spec["shift_a"]
    shift_b = spec["shift_b"]
    const_a = spec["const_a"]
    const_b = spec["const_b"]
    mul_a = spec["mul_a"]
    mul_b = spec["mul_b"]
    template = spec["template"]
    key_term = maybe_plain(
        "key_byte",
        f"((key_byte + {spec['key_const']}u) & 0xFFu)",
        spec["plain_key"],
    )
    salt_term = maybe_plain(
        "salt_byte",
        f"(salt_byte ^ {spec['salt_const']}u)",
        spec["plain_salt"],
    )
    twiddle_term = maybe_plain(
        "twiddle_byte",
        f"(((twiddle_byte << {spec['twiddle_shift']}u) | (twiddle_byte >> {8 - spec['twiddle_shift']}u)) & 0xFFu)",
        spec["plain_twiddle"],
    )
    feedback_term = maybe_plain(
        "feedback_byte",
        f"((feedback_byte + {spec['feedback_term_const']}u) & 0xFFu)",
        spec["plain_feedback"],
    )
    lines = [
        "  {",
        "    const int start = 0;",
        "    const int end = static_cast<int>(PASSWORD_EXPANDED_SIZE);",
        f"    std::uint32_t lane_state = aTwiddle ^ 0x{spec['feedback_seed']:08X}u ^ static_cast<std::uint32_t>({abs(offset_a + offset_b + offset_c)}u);",
        "    for (int i = start; i < end; ++i) {",
        f"      const int index1 = WrapRange(i + ({offset_a}), start, end);",
        f"      const std::uint32_t a = static_cast<std::uint32_t>({input_a_cpp}[index1]);",
        "      const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(",
        "          pKeyStack,",
        f"          static_cast<std::size_t>((pRound + {spec['key_row']}u + static_cast<unsigned int>(i)) & 15U),",
        f"          static_cast<std::size_t>(i + {spec['key_offset']}u)));",
        f"      const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + {spec['salt_offset']}u) & 31U]);",
        "      const std::uint32_t twiddle_byte = static_cast<std::uint32_t>("
        "          (aTwiddle >> (((static_cast<unsigned int>(i) + pRound) & 3U) * 8U)) & 0xFFu);",
        "      const std::uint32_t feedback_byte = static_cast<std::uint32_t>("
        "          (lane_state >> (((static_cast<unsigned int>(i) ^ pRound) & 3U) * 8U)) & 0xFFu);",
    ]
    if data_index_count >= 2:
        if spec["chained_indices"]:
            lines.extend(
                [
                    f"      const int index2 = WrapRange(i + ({offset_b}) + static_cast<int>(a), start, end);",
                    f"      const std::uint32_t b = static_cast<std::uint32_t>({input_b_cpp}[index2]);",
                ]
            )
        else:
            lines.extend(
                [
                    f"      const int index2 = WrapRange(i + ({offset_b}), start, end);",
                    f"      const std::uint32_t b = static_cast<std::uint32_t>({input_b_cpp}[index2]);",
                ]
            )
    if data_index_count >= 3:
        if spec["chained_indices"]:
            lines.extend(
                [
                    f"      const int index3 = WrapRange(i + ({offset_c}) - static_cast<int>(b), start, end);",
                    f"      const std::uint32_t c = static_cast<std::uint32_t>({input_c_cpp}[index3]);",
                ]
            )
        else:
            lines.extend(
                [
                    f"      const int index3 = WrapRange(i + ({offset_c}), start, end);",
                    f"      const std::uint32_t c = static_cast<std::uint32_t>({input_c_cpp}[index3]);",
                ]
            )

    if data_index_count == 1:
        d_a = maybe_plain("a", f"(a + {const_a}u + {key_term})", spec["plain_d_a"])
        d_b = maybe_plain("a", f"((a << {shift_a}u) & 0xFFu)", spec["plain_d_b"])
        e_a = maybe_plain("a", f"((a >> {shift_b}u) & 0xFFu)", spec["plain_e_a"])
        value_expr = f"((d + {twiddle_term} + {const_b}u) ^ {e_a}) & 0xFFu"
        if spec["stage"] == "loop1":
            value_expr = f"(({value_expr}) ^ {key_term}) & 0xFFu"
        lines.extend(
            [
                f"      const std::uint32_t d = (({d_a} ^ {d_b} ^ {salt_term} ^ {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t value = {value_expr};",
            ]
        )
    elif data_index_count == 2:
        d_a = maybe_plain("a", f"(a + {const_a}u + {key_term})", spec["plain_d_a"])
        d_b = maybe_plain("b", f"((b << {shift_a}u) & 0xFFu)", spec["plain_d_b"])
        e_a = maybe_plain("b", f"((b * {mul_a}u) & 0xFFu)", spec["plain_e_b"])
        e_b = maybe_plain("a", f"(a + {const_b}u + {twiddle_term})", spec["plain_e_a"])
        e_c = maybe_plain("a", f"((a >> {shift_b}u) & 0xFFu)", spec["plain_e_c"])
        value_expr = f"((d ^ e) + a + b + {feedback_term}) & 0xFFu"
        if spec["stage"] == "loop1":
            value_expr = f"(({value_expr}) ^ {key_term}) & 0xFFu"
        lines.extend(
            [
                f"      const std::uint32_t d = (({d_a} ^ {d_b} ^ {salt_term} ^ {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t e = (({e_a} ^ {e_b} ^ {e_c}) & 0xFFu);",
                f"      const std::uint32_t value = {value_expr};",
            ]
        )
    elif template == 0:
        d_a = maybe_plain("a", f"(a + {const_a}u + {salt_term})", spec["plain_d_a"])
        d_b = maybe_plain("b", f"((b << {shift_a}u) & 0xFFu)", spec["plain_d_b"])
        d_c = maybe_plain("c", f"((c * {mul_a}u) & 0xFFu)", spec["plain_d_c"])
        e_a = maybe_plain("c", f"((c >> {shift_b}u) & 0xFFu)", spec["plain_e_c"])
        e_b = maybe_plain("b", f"(b + {const_b}u + {twiddle_term})", spec["plain_e_b"])
        e_c = maybe_plain("a", f"((a * {mul_b}u) & 0xFFu)", spec["plain_e_a"])
        value_expr = f"((d ^ e) + a + c + {twiddle_term} + {feedback_term}) & 0xFFu"
        if spec["stage"] == "loop1":
            value_expr = f"(({value_expr}) ^ {key_term}) & 0xFFu"
        lines.extend(
            [
                f"      const std::uint32_t d = ({d_a} ^ {d_b} ^ {d_c} ^ {key_term} ^ {feedback_term});",
                f"      const std::uint32_t e = ({e_a} ^ {e_b} ^ {e_c} ^ {salt_term} ^ ({feedback_term} >> 1U));",
                f"      const std::uint32_t value = {value_expr};",
            ]
        )
    elif template == 1:
        d_a = maybe_plain("a", f"(a ^ (b + {const_a}u + {key_term} + {feedback_term}))", spec["plain_d_a"])
        d_b = maybe_plain("c", f"((c << {shift_a}u) & 0xFFu)", spec["plain_d_c"])
        e_a = maybe_plain("b", f"((b * {mul_a}u) & 0xFFu)", spec["plain_e_b"])
        e_b = maybe_plain("c", f"(c + {const_b}u + {twiddle_term})", spec["plain_e_c"])
        e_c = maybe_plain("a", f"((a >> {shift_b}u) & 0xFFu)", spec["plain_e_a"])
        value_expr = f"((d + e) ^ b ^ c ^ {salt_term} ^ {feedback_term}) & 0xFFu"
        if spec["stage"] == "loop1":
            value_expr = f"(({value_expr}) ^ {key_term}) & 0xFFu"
        lines.extend(
            [
                f"      const std::uint32_t d = (({d_a} + {d_b} + {salt_term}) & 0xFFu);",
                f"      const std::uint32_t e = (({e_a} ^ {e_b} ^ {e_c} ^ {key_term} ^ {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t value = {value_expr};",
            ]
        )
    elif template == 2:
        d_a = maybe_plain("a", f"((a * {mul_a}u) & 0xFFu)", spec["plain_d_a"])
        d_b = maybe_plain("b", f"(b + {const_a}u + {salt_term})", spec["plain_d_b"])
        d_c = maybe_plain("c", f"((c >> {shift_a}u) & 0xFFu)", spec["plain_d_c"])
        e_a = maybe_plain("c", f"((c * {mul_b}u) & 0xFFu)", spec["plain_e_c"])
        e_b = maybe_plain("a", f"(a ^ {const_b}u ^ {key_term})", spec["plain_e_a"])
        e_c = maybe_plain("b", f"((b << {shift_b}u) & 0xFFu)", spec["plain_e_b"])
        value_expr = f"((d ^ e) + a + b + {key_term} + {feedback_term}) & 0xFFu"
        if spec["stage"] == "loop1":
            value_expr = f"(({value_expr}) ^ {key_term}) & 0xFFu"
        lines.extend(
            [
                f"      const std::uint32_t d = (({d_a} ^ {d_b} ^ {d_c} ^ {twiddle_term} ^ {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t e = (({e_a} + {e_b} ^ {e_c} + {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t value = {value_expr};",
            ]
        )
    else:
        d_a = maybe_plain("a", f"(a + {const_a}u + {key_term})", spec["plain_d_a"])
        d_b = maybe_plain("c", f"(c + {const_b}u + {salt_term})", spec["plain_d_c"])
        d_c = maybe_plain("b", f"((b * {mul_a}u) & 0xFFu)", spec["plain_d_b"])
        e_a = maybe_plain("b", f"((b >> {shift_a}u) & 0xFFu)", spec["plain_e_b"])
        e_b = maybe_plain("a", f"((a << {shift_b}u) & 0xFFu)", spec["plain_e_a"])
        e_c = maybe_plain("c", f"((c * {mul_b}u) & 0xFFu)", spec["plain_e_c"])
        value_expr = f"(((d + e) ^ c ^ a ^ {key_term} ^ {feedback_term}) & 0xFFu)"
        if spec["stage"] == "loop1":
            value_expr = f"(({value_expr}) ^ {key_term}) & 0xFFu"
        lines.extend(
            [
                f"      const std::uint32_t d = (({d_a} ^ {d_b} ^ {d_c} ^ {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t e = (({e_a} ^ {e_b} ^ {e_c} ^ {twiddle_term} ^ {feedback_term}) & 0xFFu);",
                f"      const std::uint32_t value = {value_expr};",
            ]
        )

    if spec["write_mode"] == "assign":
        lines.append(f"      {target_cpp}[i] = static_cast<std::uint8_t>(value);")
    elif spec["write_mode"] == "xor":
        lines.append(f"      {target_cpp}[i] ^= static_cast<std::uint8_t>(value);")
    else:
        lines.append(
            f"      {target_cpp}[i] = static_cast<std::uint8_t>({target_cpp}[i] + static_cast<std::uint8_t>(value));"
        )

    if spec["twiddle_mode"] == "xor":
        lines.append(
            f"      aTwiddle = RotateLeft32(aTwiddle ^ (value + key_byte + salt_byte + {spec['twiddle_const']}u), {spec['twiddle_rotate']}u);"
        )
    elif spec["twiddle_mode"] == "add":
        lines.append(
            f"      aTwiddle = RotateLeft32(aTwiddle + (value ^ twiddle_byte ^ {spec['twiddle_const']}u), {spec['twiddle_rotate']}u);"
        )
    else:
        lines.append(
            f"      aTwiddle = RotateLeft32((aTwiddle ^ (value + key_byte)) + (salt_byte << 8U) + {spec['twiddle_const']}u, {spec['twiddle_rotate']}u);"
        )
    if spec["feedback_mode"] == "xor":
        lines.append(
            f"      lane_state = RotateLeft32(lane_state ^ (value + twiddle_byte + {spec['feedback_const']}u), {spec['feedback_rotate']}u);"
        )
    elif spec["feedback_mode"] == "add":
        lines.append(
            f"      lane_state = RotateLeft32(lane_state + (value ^ key_byte ^ {spec['feedback_const']}u), {spec['feedback_rotate']}u);"
        )
    else:
        lines.append(
            f"      lane_state = RotateLeft32((lane_state ^ (value + feedback_byte)) + (salt_byte << 8U) + {spec['feedback_const']}u, {spec['feedback_rotate']}u);"
        )
    lines.extend(["    }", "  }"])
    return lines


def render_worker_loop_lines(spec: dict[str, Any], twiddle_spec: dict[str, Any], twiddle_var_name: str) -> list[str]:
    input_a_cpp = buffer_cpp_name(spec["inputs"][0])
    input_b_cpp = buffer_cpp_name(spec["inputs"][1])
    input_c_cpp = buffer_cpp_name(spec["inputs"][2])
    target_cpp = buffer_cpp_name(spec["target"])
    offset_a, offset_b, offset_c = spec["offsets"]
    data_index_count = spec["data_index_count"]
    template = spec["template"]
    control_offset = spec["control_offset"]
    control_shift = spec["control_shift"]
    control_mode = spec["control_mode"]
    reverse_a, reverse_b, reverse_c, reverse_control = spec["reverse_modes"]
    lane_a_expr = lane_expr_for(reverse_a, "i")
    lane_b_expr = lane_expr_for(reverse_b, "i")
    lane_c_expr = lane_expr_for(reverse_c, "i")
    control_lane_expr = lane_expr_for(reverse_control, "i")
    primary_mask_stack = "pMaskStackA" if spec["target"] == "workera" else "pMaskStackB"
    secondary_mask_stack = "pMaskStackB" if spec["target"] == "workera" else "pMaskStackA"
    lines = [
        "  {",
        "    const int aStart = 0;",
        "    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);",
        "    for (int i = aStart; i < aEnd; ++i) {",
        "      const int aReverseIndex = aEnd - 1 - i;",
        f"      const int aIndex1 = WrapRange({lane_a_expr} + ({offset_a}), aStart, aEnd);",
        f"      const std::uint32_t a = static_cast<std::uint32_t>({input_a_cpp}[aIndex1]);",
        f"      const int aControlIndex = WrapRange({control_lane_expr} + ({control_offset}), aStart, aEnd);",
        "      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);",
        f"      const std::uint32_t aWorkerCtrl = static_cast<std::uint32_t>({target_cpp}[aControlIndex]);",
        "      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(",
        "          pKeyStack,",
        f"          static_cast<std::size_t>((pRound + {spec['key_row']}u + static_cast<unsigned int>(i)) & 15U),",
        f"          static_cast<std::size_t>(i + {spec['key_offset']}u)));",
        f"      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * {spec['feedback_const']}u + {spec['salt_offset']}u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;",
        f"      const int aMaskFlatB = WrapRange(static_cast<int>(aMaskFlat) + ({spec['control_offset']} + {spec['salt_offset']}), 0, static_cast<int>(kMaskStackTotalBytes));",
        f"      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>({primary_mask_stack}[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);",
        f"      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>({secondary_mask_stack}[(static_cast<std::size_t>(aMaskFlatB) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatB) % kMaskBytes]);",
        "      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 3U) ^ (aMaskByteB << 1U)) & 0xFFu);",
        f"      const std::uint32_t aTwiddleKey = static_cast<std::uint32_t>(KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + {spec['key_row']}u + 3U) & 15U), static_cast<std::size_t>(i + {twiddle_spec['key_offset']}u)));",
        f"      const std::uint32_t aTwiddleSalt = static_cast<std::uint32_t>(pSalt[(static_cast<unsigned int>(i) + {twiddle_spec['salt_offset']}u) & 31U]);",
    ]
    lines.extend(render_twiddle_position_lines(twiddle_spec, "aPositionBias"))
    if data_index_count >= 2:
        lines.extend(
            [
                f"      const int aIndex2 = WrapRange({lane_b_expr} + ({offset_b}), aStart, aEnd);",
                f"      const std::uint32_t b = static_cast<std::uint32_t>({input_b_cpp}[aIndex2]);",
            ]
        )
    if data_index_count >= 3:
        lines.extend(
            [
                f"      const int aIndex3 = WrapRange({lane_c_expr} + ({offset_c}), aStart, aEnd);",
                f"      const std::uint32_t c = static_cast<std::uint32_t>({input_c_cpp}[aIndex3]);",
            ]
        )
    route_base = "aKeyByte" if control_mode == "key" else "aMaskByte"
    if data_index_count == 1:
        lines.append(
            f"      const std::uint32_t aRoute = ((aSourceCtrl >> {control_shift}u) ^ aWorkerCtrl ^ {route_base}) & 3U;"
        )
        lines.extend(
            [
                "      std::uint32_t aValue = a;",
                "      switch (aRoute) {",
                "        case 0U: aValue = a; break;",
                "        case 1U: aValue = static_cast<std::uint32_t>(a ^ " + route_base + "); break;",
                "        case 2U: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (" + route_base + " & (~aMaskByte & 0xFFu))); break;",
                "        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (" + route_base + " & 0x0Fu)); break;",
                "      }",
            ]
        )
    elif data_index_count == 2:
        lines.append(
            f"      const std::uint32_t aRoute = ((aSourceCtrl >> {control_shift}u) ^ aWorkerCtrl ^ aKeyByte ^ aMaskByte) & 3U;"
        )
        lines.extend(
            [
                "      std::uint32_t aValue = a;",
                "      switch (aRoute) {",
                "        case 0U: aValue = a; break;",
                "        case 1U: aValue = b; break;",
                "        case 2U: aValue = static_cast<std::uint32_t>(a ^ b); break;",
                "        default: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu))); break;",
                "      }",
            ]
        )
    else:
        lines.append(
            f"      const std::uint32_t aRoute = ((aSourceCtrl >> {control_shift}u) ^ aWorkerCtrl ^ aKeyByte ^ aMaskByte ^ static_cast<std::uint32_t>({template}u)) % 5U;"
        )
        lines.extend(
            [
                "      std::uint32_t aValue = a;",
                "      switch (aRoute) {",
                "        case 0U: aValue = a; break;",
                "        case 1U: aValue = b; break;",
                "        case 2U: aValue = c; break;",
                "        case 3U: aValue = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu))); break;",
                "        default: aValue = static_cast<std::uint32_t>((b & 0xF0u) | (c & 0x0Fu)); break;",
                "      }",
            ]
        )
    lines.append(f"      {target_cpp}[i] = static_cast<std::uint8_t>(aValue);")
    lines.append(
        render_twiddle_mix_line(
            twiddle_spec,
            "aValue",
            "aSourceCtrl ^ aTwiddleKey ^ aTwiddleSalt ^ aPositionBias",
            twiddle_var_name,
        )
    )
    lines.extend(["    }", "  }"])
    return lines


def render_twiddle_position_lines(spec: dict[str, Any], var_name: str = "aPositionBias") -> list[str]:
    lines = [
        "      const std::uint32_t aLaneIndex = static_cast<std::uint32_t>(static_cast<unsigned int>(i) & 0xFFu);"
    ]
    if spec["position_mode"] == "i":
        lines.append(
            f"      const std::uint32_t {var_name} = static_cast<std::uint32_t>((aLaneIndex ^ static_cast<std::uint32_t>({spec['position_xor']}u)) & 0xFFu);"
        )
    elif spec["position_mode"] == "i_mul":
        lines.append(
            f"      const std::uint32_t {var_name} = static_cast<std::uint32_t>(((aLaneIndex * static_cast<std::uint32_t>({spec['position_mul']}u)) ^ static_cast<std::uint32_t>({spec['position_xor']}u)) & 0xFFu);"
        )
    else:
        lines.append(f"      const std::uint32_t {var_name} = 0U;")
    return lines


def render_twiddle_mix_line(spec: dict[str, Any], value_expr: str, extra_expr: str, twiddle_var_name: str) -> str:
    if spec["mix_mode"] == "xor":
        return (
            f"      {twiddle_var_name} = AdvanceTwiddle32({twiddle_var_name}, "
            f"static_cast<std::uint32_t>({value_expr}), "
            f"static_cast<std::uint32_t>(({extra_expr}) ^ static_cast<std::uint32_t>({spec['const']}u)), "
            f"0x85EBCA6Bu, {spec['rotate']}u);"
        )
    if spec["mix_mode"] == "add":
        return (
            f"      {twiddle_var_name} = AdvanceTwiddle32({twiddle_var_name} + static_cast<std::uint32_t>({spec['const']}u), "
            f"static_cast<std::uint32_t>(({value_expr}) + ({extra_expr})), "
            f"static_cast<std::uint32_t>(pSalt[{spec['salt_offset']}u]), "
            f"0x27D4EB2Du, {spec['rotate']}u);"
        )
    return (
        f"      {twiddle_var_name} = AdvanceTwiddle32({twiddle_var_name} ^ static_cast<std::uint32_t>(pSalt[{spec['salt_offset']}u]), "
        f"static_cast<std::uint32_t>(({value_expr}) ^ RotateLeft32(static_cast<std::uint32_t>({extra_expr}), 7U)), "
        f"static_cast<std::uint32_t>({spec['const']}u) + static_cast<std::uint32_t>({extra_expr}), "
        f"0x165667B1u, {spec['rotate']}u);"
    )


def render_scalar_breaker_switch(
    matrix_name: str,
    op_kind: str,
    selector_expr: str,
    arg0_expr: str,
    arg1_expr: str,
    pool: tuple[str, ...],
) -> list[str]:
    enum_name = "LightningFastOp" if op_kind == "fast" else "LightningSlowOp"
    method_name = "ApplyFastOp" if op_kind == "fast" else "ApplySlowOp"
    lines = [f"    switch (static_cast<unsigned>({selector_expr}) % {len(pool)}u) {{"]
    for index, op_name in enumerate(pool):
        lines.append(
            f"      case {index}u: {matrix_name}.{method_name}({enum_name}::{op_name}, {arg0_expr}, {arg1_expr}); break;"
        )
    lines.append("    }")
    return lines


def choose_key_schedule_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "seed_offsets": tuple(choose_signed_offset(rng) for _ in range(4)),
        "seed_bias": rng.randint(1, 255),
        "seed_stride": rng.randint(1, 31),
        "seed_rotate": rng.randint(1, 7),
        "seed_scan_stride_a": rng.randint(1, 31),
        "seed_scan_stride_b": rng.randint(1, 31),
        "seed_mix_mode": rng.choice(("xor_add", "add_xor")),
        "round_key_offsets": tuple(choose_signed_offset(rng) for _ in range(3)),
        "round_key_row": rng.randrange(ROUND_KEY_STACK_DEPTH),
        "round_key_bias": rng.randint(1, 255),
        "round_key_rotate": rng.randint(1, ROUND_KEY_BYTES - 1),
        "round_key_spread": rng.choice((3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29)),
        "round_scan_stride_a": rng.randint(1, 31),
        "round_scan_stride_b": rng.randint(1, 31),
        "round_scan_stride_c": rng.randint(1, 31),
        "round_mix_mode": rng.choice(("xor_add", "add_xor")),
        "twiddle_seed": rng.getrandbits(32),
        "twiddle_stride": rng.randint(1, 63),
        "salt_stride": rng.randint(1, 31),
        "salt_second_pass": rng.random() < 0.75,
        "salt_second_stride_a": rng.randint(17, 63),
        "salt_second_stride_b": rng.randint(17, 63),
        "salt_second_rotate": rng.randint(1, 7),
        "salt_second_bias": rng.randint(1, 255),
        "salt_second_offset": choose_signed_offset(rng),
    }


def render_key_stack_seed_lines(spec: dict[str, Any], key_stack_name: str) -> list[str]:
    offset0, offset1, _, _ = spec["seed_offsets"]
    lines = [
        f"  std::memset({key_stack_name}, 0, kRoundKeyStackDepth * kRoundKeyBytes);",
        "  unsigned int aSourceIndex = 0U;",
        "  unsigned int aKeyIndex = 0U;",
        "  unsigned int aKeyPlaneIndex = 0U;",
        "  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {",
        f"    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * {spec['seed_scan_stride_a']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * {spec['seed_scan_stride_b']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);",
        f"    const std::uint32_t aCarry = static_cast<std::uint32_t>({key_stack_name}[aKeyPlaneIndex][aKeyIndex]);",
        "    const std::uint32_t aLane = static_cast<std::uint32_t>(aSourceIndex & 0xFFu);",
        "    const std::uint32_t aPosMix = static_cast<std::uint32_t>(((aLane * 37u) ^ aLane ^ static_cast<std::uint32_t>(aKeyIndex * 13u) ^ static_cast<std::uint32_t>(aKeyPlaneIndex * 29u)) & 0xFFu);",
        f"    const std::uint32_t aPreMix = static_cast<std::uint32_t>((((a + {spec['seed_bias']}u) ^ ((b << {spec['seed_rotate']}u) & 0xFFu) ^ aPosMix ^ aCarry) & 0xFFu));",
        "    const std::uint32_t aMixValue = static_cast<std::uint32_t>(aMixBox[aPreMix]);",
    ]
    if spec["seed_mix_mode"] == "xor_add":
        lines.append(
            f"    {key_stack_name}[aKeyPlaneIndex][aKeyIndex] ^= static_cast<unsigned char>(aMixValue);"
        )
    else:
        lines.append(
            f"    {key_stack_name}[aKeyPlaneIndex][aKeyIndex] = static_cast<unsigned char>({key_stack_name}[aKeyPlaneIndex][aKeyIndex] + static_cast<unsigned char>(aMixValue));"
        )
    lines.extend(
        [
            "    ++aSourceIndex;",
            "    ++aKeyIndex;",
            "    if (aKeyIndex >= kRoundKeyBytes) {",
            "      aKeyIndex = 0U;",
            "      ++aKeyPlaneIndex;",
            "      if (aKeyPlaneIndex >= kRoundKeyStackDepth) {",
            "        aKeyPlaneIndex = 0U;",
            "      }",
            "    }",
            "  }",
        ]
    )
    return lines


def render_salt_seed_lines(
    spec: dict[str, Any],
    salt_name: str,
    salt_mix_box_name: str = "aSaltMixBox",
    var_prefix: str = "a",
) -> list[str]:
    _, _, offset0, offset1 = spec["seed_offsets"]
    source_index = f"{var_prefix}SourceIndex"
    index0_name = f"{var_prefix}Index0"
    index1_name = f"{var_prefix}Index1"
    salt_index = f"{var_prefix}SaltIndex"
    mix_value = f"{var_prefix}MixValue"
    value_a = f"{var_prefix}A"
    value_b = f"{var_prefix}B"
    salt_acc = f"{var_prefix}SaltAcc"
    salt_wave = f"{var_prefix}SaltWave"
    xor_mode = "true" if spec["seed_mix_mode"] == "xor_add" else "false"
    lines = [
        f"  std::memset({salt_name}, 0, kSaltBytes);",
        f"  std::uint32_t {salt_acc} = static_cast<std::uint32_t>(0xA5A5A5A5u ^ {spec['seed_bias']}u ^ {spec['round_key_bias']}u);",
        f"  unsigned int {source_index} = 0U;",
        f"  while ({source_index} < PASSWORD_EXPANDED_SIZE) {{",
        f"    const int {index0_name} = WrapRange(static_cast<int>({source_index} * {spec['salt_stride']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int {index1_name} = WrapRange(static_cast<int>({source_index} * {spec['seed_stride']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const unsigned int {salt_index} = ({source_index} * {spec['salt_stride']}u + static_cast<unsigned int>({spec['seed_bias']}u)) & 31U;",
        f"    const std::uint32_t {value_a} = static_cast<std::uint32_t>(pSource[{index0_name}]);",
        f"    const std::uint32_t {value_b} = static_cast<std::uint32_t>(pSource[{index1_name}]);",
        f"    const std::uint32_t {salt_wave} = static_cast<std::uint32_t>({salt_mix_box_name}[(",
        f"        {value_a} ^",
        f"        static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>({value_b} & 0xFFu), {spec['seed_rotate']}u)) ^",
        f"        static_cast<std::uint32_t>({source_index} + {spec['seed_bias']}u)) & 127U]);",
        f"    {salt_acc} = AdvanceSaltSeedAccumulator("
        f"{salt_name}, {salt_acc} ^ {salt_wave}, {salt_index}, {value_a} ^ {salt_wave}, {value_b}, "
        f"static_cast<std::uint32_t>({source_index}), static_cast<std::uint32_t>({spec['round_key_bias']}u), "
        f"{spec['seed_rotate']}u, {xor_mode});",
        f"    {salt_name}[({salt_index} + 17U) & 31U] ^= static_cast<unsigned char>({salt_mix_box_name}[(",
        f"        {salt_wave} ^ {salt_name}[({salt_index} + 5U) & 31U] ^ static_cast<std::uint32_t>({source_index})) & 127U]);",
    ]
    lines.extend(
        [
            f"    ++{source_index};",
            "  }",
        ]
    )
    lines.append(
        f"  ApplySaltSBoxLayer({salt_name}, {salt_acc}, static_cast<std::uint32_t>({spec['round_key_bias']}u), {spec['seed_rotate']}u);"
    )
    lines.extend(
        [
            "  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {",
            f"    const std::uint32_t {salt_wave} = static_cast<std::uint32_t>({salt_mix_box_name}[(",
            f"        {salt_name}[aSaltLane] ^ {salt_name}[(aSaltLane + 7U) & 31U] ^ static_cast<unsigned char>({spec['round_key_bias']}u) ^ static_cast<unsigned char>(aSaltLane)) & 127U]);",
            f"    {salt_name}[aSaltLane] = static_cast<unsigned char>(",
            f"        FixedSBoxByte(static_cast<unsigned char>({salt_name}[aSaltLane] ^ {salt_wave} ^ {salt_name}[(aSaltLane + 13U) & 31U])) +",
            f"        static_cast<unsigned char>({salt_wave}));",
            "  }",
        ]
    )
    if spec["salt_second_pass"]:
        rotate = spec["salt_second_rotate"]
        offset2 = offset0 + spec["salt_second_offset"]
        offset3 = offset1 - spec["salt_second_offset"]
        lines.extend(
            [
                f"  for (unsigned int {source_index} = 0U; {source_index} < PASSWORD_EXPANDED_SIZE; ++{source_index}) {{",
                f"    const int {index0_name} = WrapRange(static_cast<int>({source_index} * {spec['salt_second_stride_a']}u + ({offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
                f"    const int {index1_name} = WrapRange(static_cast<int>({source_index} * {spec['salt_second_stride_b']}u + ({offset3})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
                f"    const unsigned int {salt_index} = static_cast<unsigned int>(({source_index} * {spec['salt_second_stride_a']}u + {salt_name}[({source_index} + {spec['salt_second_bias']}u) & 31U]) & 31U);",
                f"    const std::uint32_t {value_a} = static_cast<std::uint32_t>(pSource[{index0_name}]);",
                f"    const std::uint32_t {value_b} = static_cast<std::uint32_t>(pSource[{index1_name}]);",
                f"    const std::uint32_t {var_prefix}Carry = static_cast<std::uint32_t>({salt_name}[({salt_index} + 11U) & 31U]);",
                f"    const std::uint32_t {salt_wave} = static_cast<std::uint32_t>({salt_mix_box_name}[(",
                f"        {value_a} ^ {value_b} ^ {var_prefix}Carry ^ static_cast<std::uint32_t>({spec['salt_second_bias']}u)) & 127U]);",
                f"    {salt_acc} = AdvanceSaltSeedAccumulator("
                f"{salt_name}, {salt_acc} ^ {var_prefix}Carry ^ {salt_wave}, {salt_index}, "
                f"{value_a} ^ {var_prefix}Carry, {value_b}, "
                f"static_cast<std::uint32_t>({source_index} + {var_prefix}Carry), "
                f"static_cast<std::uint32_t>({spec['salt_second_bias']}u), {rotate}u, {xor_mode});",
                f"    {salt_name}[({salt_index} + 23U) & 31U] = static_cast<unsigned char>("
                f"{salt_name}[({salt_index} + 23U) & 31U] + static_cast<unsigned char>({salt_wave}));",
            ]
        )
        lines.append("  }")
        lines.append(
            f"  ApplySaltSBoxLayer({salt_name}, {salt_acc} ^ static_cast<std::uint32_t>({spec['salt_second_bias']}u), static_cast<std::uint32_t>({spec['salt_second_bias']}u), {rotate}u);"
        )
        lines.extend(
            [
                "  for (unsigned int aSaltLane = 0U; aSaltLane < kSaltBytes; ++aSaltLane) {",
                f"    const std::uint32_t {salt_wave} = static_cast<std::uint32_t>({salt_mix_box_name}[(",
                f"        {salt_name}[aSaltLane] + {salt_name}[(aSaltLane + 19U) & 31U] + static_cast<unsigned char>({spec['salt_second_bias']}u + 13u)) & 127U]);",
                f"    {salt_name}[aSaltLane] ^= static_cast<unsigned char>({salt_wave});",
                "  }",
            ]
        )
    return lines


def render_twiddle_init_lines(
    spec: dict[str, Any],
    var_name: str,
    seed_offset_index: int,
    seed_xor: int,
    salt_offset: int,
) -> list[str]:
    seed_offset = spec["seed_offsets"][seed_offset_index]
    return [
        f"  std::uint32_t {var_name} = static_cast<std::uint32_t>(0x{seed_xor:08X}u ^ 0x{spec['twiddle_seed']:08X}u ^ (pRound * {spec['twiddle_stride']}u));",
        f"  {var_name} ^= static_cast<std::uint32_t>(pSource[WrapRange(static_cast<int>(pRound) + ({seed_offset}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))]) << 8U;",
        f"  {var_name} ^= static_cast<std::uint32_t>(pSalt[(pRound + {salt_offset}u) & 31U]) << 16U;",
    ]


def choose_custom_tsunami_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "source_offset_a": choose_signed_offset(rng),
        "source_offset_b": choose_signed_offset(rng),
        "worker_offset_a": choose_signed_offset(rng),
        "worker_offset_b": choose_signed_offset(rng),
        "salt_offset_a": rng.randint(0, SALT_BYTES - 1),
        "salt_offset_b": rng.randint(0, SALT_BYTES - 1),
        "seed_const_a": rng.getrandbits(32),
        "seed_const_b": rng.getrandbits(32),
        "mix_const_a": rng.getrandbits(32),
        "mix_const_b": rng.getrandbits(32),
        "rotate_a": rng.randint(3, 11),
        "rotate_b": rng.randint(3, 11),
        "feedback_interval": rng.choice((8, 12, 16, 24, 32)),
        "mode": rng.randrange(4),
        "mix_box": build_mix_box_values(rng, 128),
    }


def choose_custom_final_whitening_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "source_offset": choose_signed_offset(rng),
        "neighbor_stride": rng.choice((1, 3, 5, 7, 11, 13, 17)),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "seed_const": rng.getrandbits(32),
        "mix_const": rng.getrandbits(32),
        "rotate": rng.randint(1, 7),
        "mode": rng.randrange(3),
        "mix_box": build_mix_box_values(rng, 128),
    }


def render_custom_tsunami_breaker_lines(function_name: str, spec: dict[str, Any]) -> list[str]:
    lines = [
        f"static void {function_name}_TsunamiBreaker(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorkerA,",
        "    unsigned char* pWorkerB,",
        "    const unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned int pRound,",
        "    std::uint32_t pTwiddleA,",
        "    std::uint32_t pTwiddleB) {",
        "  if (pSource == nullptr || pWorkerA == nullptr || pWorkerB == nullptr) {",
        "    return;",
        "  }",
    ]
    lines.extend(render_named_mix_box_lines(spec["mix_box"], "aMixBox"))
    lines.extend(
        [
            f"  std::uint32_t aAccA = AdvanceTwiddle32(0x{spec['seed_const_a']:08X}u ^ static_cast<std::uint32_t>(pRound), pTwiddleA, pTwiddleB, 0x{spec['mix_const_a']:08X}u, {spec['rotate_a']}u);",
            f"  std::uint32_t aAccB = AdvanceTwiddle32(0x{spec['seed_const_b']:08X}u ^ static_cast<std::uint32_t>(pRound), pTwiddleB, pTwiddleA, 0x{spec['mix_const_b']:08X}u, {spec['rotate_b']}u);",
            "  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {",
            f"    const std::size_t aSourceIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + ({spec['source_offset_a']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
            f"    const std::size_t aSourceIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + ({spec['source_offset_b']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
            f"    const std::size_t aWorkerIndexA = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + ({spec['worker_offset_a']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
            f"    const std::size_t aWorkerIndexB = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + ({spec['worker_offset_b']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
            "    const std::uint32_t aSourceA = static_cast<std::uint32_t>(pSource[aSourceIndexA]);",
            "    const std::uint32_t aSourceB = static_cast<std::uint32_t>(pSource[aSourceIndexB]);",
            "    const std::uint32_t aTapA = static_cast<std::uint32_t>(pWorkerA[aWorkerIndexA]);",
            "    const std::uint32_t aTapB = static_cast<std::uint32_t>(pWorkerB[aWorkerIndexB]);",
            f"    const std::uint32_t aSaltA = static_cast<std::uint32_t>(pSalt[(i + {spec['salt_offset_a']}u) & 31U]);",
            f"    const std::uint32_t aSaltB = static_cast<std::uint32_t>(pSalt[(i + {spec['salt_offset_b']}u) & 31U]);",
            "    const std::uint32_t aBoxA = static_cast<std::uint32_t>(aMixBox[(aSourceA ^ aSaltA ^ aTapB ^ FoldWordToByte(aAccA)) & 127U]);",
            "    const std::uint32_t aBoxB = static_cast<std::uint32_t>(aMixBox[(aSourceB + aSaltB + aTapA + FoldWordToByte(aAccB)) & 127U]);",
            f"    aAccA = AdvanceTwiddle32(aAccA ^ pTwiddleA, aTapA ^ aBoxA, aSourceB ^ aSaltB, 0x{spec['mix_const_a']:08X}u, {spec['rotate_a']}u);",
            f"    aAccB = AdvanceTwiddle32(aAccB ^ pTwiddleB, aTapB ^ aBoxB, aSourceA ^ aSaltA, 0x{spec['mix_const_b']:08X}u, {spec['rotate_b']}u);",
            "    const unsigned char aWaveA = aMixBox[(FoldWordToByte(aAccA) ^ aSourceA ^ aSaltB ^ static_cast<unsigned char>(i)) & 127U];",
            "    const unsigned char aWaveB = aMixBox[(FoldWordToByte(aAccB) ^ aSourceB ^ aSaltA ^ static_cast<unsigned char>(i >> 1U)) & 127U];",
            f"    switch ((static_cast<unsigned>(aBoxA ^ aBoxB ^ static_cast<std::uint32_t>(pRound)) + {spec['mode']}u) & 3U) {{",
            "      case 0U:",
            "        pWorkerA[i] ^= aWaveB;",
            "        pWorkerB[i] = static_cast<unsigned char>(pWorkerB[i] + aWaveA);",
            "        break;",
            "      case 1U:",
            "        pWorkerA[i] = RotateLeft8(static_cast<std::uint8_t>(pWorkerA[i] + aWaveA), 1U + (aWaveB & 3U));",
            "        pWorkerB[i] ^= RotateLeft8(aWaveB, 1U + (aWaveA & 3U));",
            "        break;",
            "      case 2U: {",
            "        const std::size_t aPartner = (i + 1U + (aBoxA & 15U)) % PASSWORD_EXPANDED_SIZE;",
            "        const unsigned char aMix = static_cast<unsigned char>(aWaveA ^ aWaveB);",
            "        pWorkerA[i] ^= pWorkerB[aPartner] ^ aMix;",
            "        pWorkerB[aPartner] = static_cast<unsigned char>(pWorkerB[aPartner] + aMix);",
            "        break;",
            "      }",
            "      default:",
            "        pWorkerA[i] = static_cast<unsigned char>((pWorkerA[i] & 0xF0u) | (aWaveB & 0x0Fu));",
            "        pWorkerB[i] = static_cast<unsigned char>((pWorkerB[i] & 0x0Fu) | (aWaveA & 0xF0u));",
            "        break;",
            "    }",
            f"    if (((i + 1U) % {spec['feedback_interval']}u) == 0U) {{",
            "      const std::uint32_t aCarry = aAccA;",
            "      aAccA = aAccB ^ RotateLeft32(aCarry, 3U);",
            "      aAccB = aCarry + RotateLeft32(aAccB, 5U);",
            "    }",
            "  }",
            "}",
        ]
    )
    return lines


def render_custom_final_whitening_lines(function_name: str, spec: dict[str, Any]) -> list[str]:
    lines = [
        f"static void {function_name}_FinalWhitening(",
        "    unsigned char* pSource,",
        "    unsigned char* pDest,",
        "    const unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned int pRound,",
        "    std::uint32_t pTwiddleA,",
        "    std::uint32_t pTwiddleB) {",
        "  if (pSource == nullptr || pDest == nullptr) {",
        "    return;",
        "  }",
    ]
    lines.extend(render_named_mix_box_lines(spec["mix_box"], "aMixBox"))
    lines.extend(
        [
            f"  std::uint32_t aAcc = AdvanceTwiddle32(pTwiddleA ^ static_cast<std::uint32_t>(pRound), pTwiddleB, 0x{spec['seed_const']:08X}u, 0x{spec['mix_const']:08X}u, {spec['rotate']}u);",
            "  // Final whitening",
            "  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; ++i) {",
            f"    const std::size_t aSourceIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(i) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
            f"    const std::size_t aNeighborIndex = (i + {spec['neighbor_stride']}u) % PASSWORD_EXPANDED_SIZE;",
            "    const std::uint32_t aSourceByte = static_cast<std::uint32_t>(pSource[aSourceIndex]);",
            "    const std::uint32_t aDestByte = static_cast<std::uint32_t>(pDest[i]);",
            "    const std::uint32_t aNeighborByte = static_cast<std::uint32_t>(pDest[aNeighborIndex]);",
            f"    const std::uint32_t aSaltByte = static_cast<std::uint32_t>(pSalt[(i + {spec['salt_offset']}u) & 31U]);",
            "    const std::uint32_t aMix = static_cast<std::uint32_t>(aMixBox[(aDestByte ^ aSourceByte ^ aSaltByte ^ FoldWordToByte(aAcc)) & 127U]);",
            f"    aAcc = AdvanceTwiddle32(aAcc, aDestByte ^ aMix, aSourceByte ^ aNeighborByte ^ aSaltByte, 0x{spec['mix_const']:08X}u, {spec['rotate']}u);",
            "    const unsigned char aWave = aMixBox[(FoldWordToByte(aAcc) + aSourceByte + aNeighborByte + aSaltByte) & 127U];",
            f"    switch ((static_cast<unsigned>(aMix) + {spec['mode']}u) % 3U) {{",
            "      case 0U:",
            "        pDest[i] ^= aWave;",
            "        break;",
            "      case 1U:",
            "        pDest[i] = RotateLeft8(static_cast<std::uint8_t>(pDest[i] + aWave), 1U + (aWave & 3U));",
            "        break;",
            "      default:",
            "        pDest[i] = static_cast<unsigned char>((pDest[i] & 0xF0u) | (aWave & 0x0Fu));",
            "        pDest[aNeighborIndex] ^= static_cast<unsigned char>(aWave & 0xF0u);",
            "        break;",
            "    }",
            "  }",
            "}",
        ]
    )
    return lines


def render_round_key_update_lines(spec: dict[str, Any]) -> list[str]:
    offset0, offset1, offset2 = spec["round_key_offsets"]
    lines = [
        "  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);",
        "  unsigned int aSourceIndex = 0U;",
        "  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {",
        f"    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_scan_stride_a']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_scan_stride_b']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_scan_stride_c']}u + ({offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const unsigned int aKeyIndex = (aSourceIndex * {spec['round_scan_stride_a']}u + static_cast<unsigned int>({spec['round_key_rotate']}u)) & 31U;",
        f"    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>({spec['round_key_spread']}u)) & 31U;",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);",
        "    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);",
        "    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);",
        "    const std::uint32_t key_byte = static_cast<std::uint32_t>(KeyStackByte(",
        "        pKeyStack,",
        f"        static_cast<std::size_t>((aSourceIndex + {spec['round_key_row']}u) & 15U),",
        f"        static_cast<std::size_t>(aSourceIndex + {spec['round_key_bias']}u)));",
    ]
    if spec["round_mix_mode"] == "xor_add":
        lines.extend(
            [
                "    const std::uint32_t aMixValue = static_cast<std::uint32_t>((a + b + salt_byte) & 0xFFu);",
                "    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(aMixValue);",
                "    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((c ^ key_byte) & 0xFFu);",
            ]
        )
    else:
        lines.extend(
            [
                "    const std::uint32_t aMixValue = static_cast<std::uint32_t>((a ^ b ^ salt_byte) & 0xFFu);",
                "    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(aMixValue));",
                "    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((c ^ key_byte) & 0xFFu));",
            ]
        )
    lines.extend(
        [
            "    ++aSourceIndex;",
            "  }",
            "  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);",
        ]
    )
    return lines


def choose_scalar_breaker_spec(rng: random.Random) -> dict[str, Any]:
    fast_count = rng.randint(4, 7)
    slow_count = rng.randint(2, 5)
    fast_pool = tuple(rng.sample(MATRIX_FAST_OPS, fast_count))
    slow_pool = tuple(rng.sample(DIFFUSION_MATRIX_SLOW_OPS, slow_count))
    return {
        "source_mix_offset": choose_signed_offset(rng),
        "control_offset": choose_signed_offset(rng),
        "feedback_offset": choose_signed_offset(rng),
        "key_row_a": rng.randrange(ROUND_KEY_STACK_DEPTH),
        "key_row_b": rng.randrange(ROUND_KEY_STACK_DEPTH),
        "key_offset_a": rng.randint(0, ROUND_KEY_BYTES - 1),
        "key_offset_b": rng.randint(0, ROUND_KEY_BYTES - 1),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "emit_offset": choose_signed_offset(rng),
        "twiddle_rotate": rng.randint(1, 13),
        "pre_mix": rng.choice(("xor", "add")),
        "emit_mix": rng.choice(("xor", "add")),
        "fast_pool": fast_pool,
        "slow_pool": slow_pool,
    }


def render_scalar_breaker_lines(spec: dict[str, Any]) -> list[str]:
    lines = [
        "  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMatrixBlockBytes) {",
        f"    const auto breaker_source_mix = LoadBlock16Wrapped(pSource, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_mix_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto breaker_control_a = LoadBlock16Wrapped(pSource, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['control_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto breaker_control_b = LoadBlock16Wrapped(pWorker, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['feedback_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto breaker_key_a = LoadKeyStackBlock16Wrapped(pKeyStack, static_cast<std::size_t>((pRound + {spec['key_row_a']}u) & 15U), chunk + {spec['key_offset_a']}u);",
        f"    const auto breaker_key_b = LoadKeyStackBlock16Wrapped(pKeyStack, static_cast<std::size_t>((pRound + {spec['key_row_b']}u) & 15U), chunk + {spec['key_offset_b']}u);",
        "    auto breaker_block = LoadBlock16Wrapped(pWorker, chunk);",
        "    LightningMatrix breaker(breaker_block.data());",
        "    LightningMatrix breaker_mix(breaker_source_mix.data());",
    ]
    if spec["pre_mix"] == "xor":
        lines.append("    breaker.XorWith(breaker_mix);")
    else:
        lines.append("    breaker.AddWith(breaker_mix);")
    lines.append("    LightningMatrix breaker_key_mix(breaker_key_a.data());")
    lines.append("    breaker.XorWith(breaker_key_mix);")
    lines.append(
        f"    breaker.InjectXor(breaker_control_a.data(), breaker_control_a.size(), static_cast<unsigned char>((breaker_control_a[0] ^ breaker_key_a[{spec['salt_offset'] % 16}U] ^ pSalt[{spec['salt_offset']}U]) & 15U));"
    )
    lines.append(
        f"    breaker.InjectAdd(breaker_control_b.data(), breaker_control_b.size(), static_cast<unsigned char>((breaker_control_b[1] + breaker_key_b[{(spec['salt_offset'] + 3) % 16}U] + pSalt[{(spec['salt_offset'] + 5) % 32}U]) & 15U));"
    )
    lines.extend(
        render_scalar_breaker_switch(
            "breaker",
            "fast",
            f"static_cast<unsigned>(breaker_control_a[0] ^ breaker_key_a[{spec['salt_offset'] % 16}U] ^ pSalt[{spec['salt_offset']}U])",
            "static_cast<std::uint8_t>(breaker_control_a[3] ^ breaker_control_b[5] ^ breaker_key_a[7])",
            "static_cast<std::uint8_t>(breaker_control_b[7] + breaker_source_mix[9] + breaker_key_b[11])",
            spec["fast_pool"],
        )
    )
    lines.extend(
        render_scalar_breaker_switch(
            "breaker",
            "fast",
            f"static_cast<unsigned>(breaker_control_b[2] ^ breaker_key_b[{(spec['salt_offset'] + 1) % 16}U] ^ pSalt[{(spec['salt_offset'] + 1) % 32}U])",
            "static_cast<std::uint8_t>(breaker_control_a[6] + breaker_source_mix[10] + breaker_key_a[12])",
            "static_cast<std::uint8_t>(breaker_control_b[11] ^ breaker_source_mix[13] ^ breaker_key_b[14])",
            spec["fast_pool"],
        )
    )
    lines.extend(
        render_scalar_breaker_switch(
            "breaker",
            "slow",
            f"static_cast<unsigned>(breaker_source_mix[4] ^ breaker_key_a[{(spec['salt_offset'] + 2) % 16}U] ^ pSalt[{(spec['salt_offset'] + 2) % 32}U])",
            "static_cast<std::uint8_t>(breaker_control_a[8] + breaker_control_b[12] + breaker_key_a[5])",
            "static_cast<std::uint8_t>(breaker_control_b[14] ^ breaker_source_mix[15] ^ breaker_key_b[9])",
            spec["slow_pool"],
        )
    )
    lines.extend(
        [
            "    std::array<std::uint8_t, kMatrixBlockBytes> breaker_store{};",
            "    std::array<std::uint8_t, kMatrixBlockBytes> breaker_emit{};",
            "    breaker.Store(breaker_store.data());",
            "    for (std::size_t lane = 0; lane < kMatrixBlockBytes; ++lane) {",
        ]
    )
    if spec["emit_mix"] == "xor":
        lines.append(
            f"      breaker_emit[lane] = static_cast<std::uint8_t>(breaker_store[lane] ^ breaker_control_a[(lane + 3U) & 15U] ^ breaker_control_b[(lane + 5U) & 15U] ^ breaker_source_mix[(lane + {abs(spec['emit_offset']) % 16}U) & 15U] ^ breaker_key_a[(lane + 7U) & 15U] ^ pSalt[(lane + {spec['salt_offset']}U) & 31U] ^ static_cast<unsigned char>((aTwiddle >> ((lane & 3U) * 8U)) & 0xFFu));"
        )
    else:
        lines.append(
            f"      breaker_emit[lane] = static_cast<std::uint8_t>(breaker_store[lane] + breaker_control_a[(lane + 3U) & 15U] + breaker_control_b[(lane + 5U) & 15U] + breaker_source_mix[(lane + {abs(spec['emit_offset']) % 16}U) & 15U] + breaker_key_b[(lane + 9U) & 15U] + pSalt[(lane + {spec['salt_offset']}U) & 31U] + static_cast<unsigned char>((aTwiddle >> ((lane & 3U) * 8U)) & 0xFFu));"
        )
    lines.extend(
        [
            "    }",
            "    StoreBlock16Contiguous(pWorker, chunk, breaker_emit);",
            f"    aTwiddle = RotateLeft32(aTwiddle ^ breaker_store[0] ^ breaker_key_a[1] ^ breaker_key_b[2] ^ pSalt[{spec['salt_offset']}U], {spec['twiddle_rotate']}u);",
            "  }",
        ]
    )
    return lines


def render_mechanical_candidate_verbose(
    loop_specs: list[dict[str, Any]],
    matrix_enabled: bool,
    matrix_breaker_summary: str,
) -> str:
    lines = ["selection=mechanical byte loops"]
    for index, spec in enumerate(loop_specs, start=1):
        lines.append(
            f"  loop{index}: data_indices={spec['data_index_count']} inputs={spec['inputs']} offs={spec['offsets']} "
            f"template={spec['template']} write={spec['write_mode']} chained={str(spec['chained_indices']).lower()} "
            f"key_row={spec['key_row']} key_off={spec['key_offset']} twiddle={spec['twiddle_mode']}"
        )
    if matrix_enabled:
        lines.append(f"  lightning_matrix_after_loop2: {matrix_breaker_summary}")
    else:
        lines.append("  lightning_matrix_after_loop2: none")
    return "\n".join(lines)


def choose_worker_loop_spec(rng: random.Random, target: str) -> dict[str, Any]:
    spec = choose_mechanical_loop_spec(rng, "loop1")
    spec["target"] = target
    spec["stage"] = target
    spec["inputs"] = ("source", "source", "source")
    spec["write_mode"] = "assign"
    spec["data_index_count"] = rng.choice((1, 1, 2, 2, 3))
    spec["template"] = rng.randrange(5)
    spec["control_offset"] = choose_signed_offset(rng)
    spec["control_shift"] = rng.randint(0, 7)
    spec["control_mode"] = "key" if target == "workera" else "mask"
    spec["reverse_modes"] = choose_reverse_quad(rng)
    return spec


def choose_twiddle_spec(rng: random.Random, stage_name: str) -> dict[str, Any]:
    return {
        "stage": stage_name,
        "mix_mode": rng.choice(("xor", "add", "mix")),
        "rotate": rng.randint(1, 11),
        "const": rng.randint(1, 255),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "key_offset": rng.randint(0, ROUND_KEY_BYTES - 1),
        "position_mode": rng.choice(("none", "i", "i_mul")),
        "position_mul": rng.choice((3, 5, 7, 9, 11, 13, 17, 19, 29, 37, 53)),
        "position_xor": rng.randint(0, 255),
    }


def choose_reverse_triplet(rng: random.Random) -> tuple[bool, bool, bool]:
    return tuple(bool(rng.getrandbits(1)) for _ in range(3))


def choose_reverse_quad(rng: random.Random) -> tuple[bool, bool, bool, bool]:
    return tuple(bool(rng.getrandbits(1)) for _ in range(4))


def choose_mask_schedule_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "seed_offsets": tuple(choose_signed_offset(rng) for _ in range(3)),
        "seed_reverse_modes": choose_reverse_triplet(rng),
        "seed_bias": rng.randint(1, 255),
        "seed_shift": rng.randint(1, 7),
        "seed_stride_a": rng.randint(1, 31),
        "seed_stride_b": rng.randint(1, 31),
        "seed_mix_mode": rng.choice(("xor_add", "add_xor")),
        "worker_mode": rng.choice(("off", "xor_wave", "add_wave", "sbox_wave")),
        "worker_offset": choose_signed_offset(rng),
        "worker_stride": rng.randint(1, 63),
        "worker_reverse": bool(rng.getrandbits(1)),
        "worker_bias": rng.randint(1, 255),
        "worker_rotate": rng.randint(1, 7),
        "second_pass": rng.random() < 0.8,
        "second_reverse_modes": choose_reverse_triplet(rng),
        "second_stride_a": rng.randint(17, 63),
        "second_stride_b": rng.randint(17, 63),
        "second_rotate": rng.randint(1, 7),
        "second_bias": rng.randint(1, 255),
        "round_offsets": tuple(choose_signed_offset(rng) for _ in range(3)),
        "round_bias": rng.randint(1, 255),
        "round_stride_a": rng.randint(1, 31),
        "round_stride_b": rng.randint(1, 31),
        "round_stride_c": rng.randint(1, 31),
        "round_mix_mode": rng.choice(("xor_add", "add_xor")),
    }


def loop_lane_expr(reverse: bool) -> str:
    return lane_expr_for(reverse, "aSourceIndex")


def lane_expr_for(reverse: bool, forward_name: str) -> str:
    return "aReverseIndex" if reverse else forward_name


def render_mask_seed_mix_lines(
    spec: dict[str, Any],
    *,
    source_a: str,
    source_b: str,
    source_c: str,
    worker_lane: str,
    seed_a_name: str,
    seed_b_name: str,
    seed_c_name: str,
    worker_byte_name: str,
    turbulence_name: str,
) -> list[str]:
    mode = spec["worker_mode"]
    if mode == "off":
        return [
            f"    const std::uint32_t {worker_byte_name} = 0U;",
            f"    const std::uint32_t {turbulence_name} = 0U;",
            f"    const std::uint32_t {seed_a_name} = {source_a};",
            f"    const std::uint32_t {seed_b_name} = {source_b};",
            f"    const std::uint32_t {seed_c_name} = {source_c};",
        ]

    lines = [
        f"    const int aWorkerIndex = WrapRange(static_cast<int>({worker_lane} * {spec['worker_stride']}u + ({spec['worker_offset']})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const std::uint32_t {worker_byte_name} = (pWorker != nullptr) ? static_cast<std::uint32_t>(pWorker[aWorkerIndex]) : 0U;",
    ]
    if mode == "xor_wave":
        lines.extend(
            [
                f"    const std::uint32_t {turbulence_name} = static_cast<std::uint32_t>(({worker_byte_name} ^ static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(({source_a} + {source_c}) & 0xFFu), {spec['worker_rotate']}u)) ^ static_cast<std::uint32_t>({spec['worker_bias']}u)) & 0xFFu);",
                f"    const std::uint32_t {seed_a_name} = static_cast<std::uint32_t>(({source_a} ^ {turbulence_name}) & 0xFFu);",
                f"    const std::uint32_t {seed_b_name} = static_cast<std::uint32_t>(({source_b} + {turbulence_name}) & 0xFFu);",
                f"    const std::uint32_t {seed_c_name} = static_cast<std::uint32_t>(({source_c} ^ static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>({worker_byte_name} & 0xFFu), {spec['worker_rotate']}u))) & 0xFFu);",
            ]
        )
    elif mode == "add_wave":
        lines.extend(
            [
                f"    const std::uint32_t {turbulence_name} = static_cast<std::uint32_t>(({worker_byte_name} + {source_b} + static_cast<std::uint32_t>({spec['worker_bias']}u)) & 0xFFu);",
                f"    const std::uint32_t {seed_a_name} = static_cast<std::uint32_t>(({source_a} + {turbulence_name}) & 0xFFu);",
                f"    const std::uint32_t {seed_b_name} = static_cast<std::uint32_t>(({source_b} ^ static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>({turbulence_name} & 0xFFu), {spec['worker_rotate']}u))) & 0xFFu);",
                f"    const std::uint32_t {seed_c_name} = static_cast<std::uint32_t>(({source_c} + static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>({worker_byte_name} & 0xFFu), {spec['worker_rotate']}u))) & 0xFFu);",
            ]
        )
    else:
        lines.extend(
            [
                f"    const std::uint32_t {turbulence_name} = static_cast<std::uint32_t>(aMixBox[({worker_byte_name} ^ {source_a} ^ static_cast<std::uint32_t>({spec['worker_bias']}u)) & 0xFFu]);",
                f"    const std::uint32_t {seed_a_name} = static_cast<std::uint32_t>(({source_a} ^ {turbulence_name}) & 0xFFu);",
                f"    const std::uint32_t {seed_b_name} = static_cast<std::uint32_t>(({source_b} + {turbulence_name} + {worker_byte_name}) & 0xFFu);",
                f"    const std::uint32_t {seed_c_name} = static_cast<std::uint32_t>(({source_c} ^ static_cast<std::uint32_t>(aMixBox[({worker_byte_name} + {source_c} + static_cast<std::uint32_t>({spec['worker_bias']}u)) & 0xFFu])) & 0xFFu);",
            ]
        )
    return lines


def render_mask_stack_seed_lines(
    spec: dict[str, Any],
    mask_stack_name: str,
) -> list[str]:
    offset0, offset1, offset2 = spec["seed_offsets"]
    seed_lane0 = loop_lane_expr(spec["seed_reverse_modes"][0])
    seed_lane1 = loop_lane_expr(spec["seed_reverse_modes"][1])
    seed_lane2 = loop_lane_expr(spec["seed_reverse_modes"][2])
    second_lane0 = loop_lane_expr(spec["second_reverse_modes"][0])
    second_lane1 = loop_lane_expr(spec["second_reverse_modes"][1])
    second_lane2 = loop_lane_expr(spec["second_reverse_modes"][2])
    worker_lane = loop_lane_expr(spec["worker_reverse"])
    xor_mode = "true" if spec["seed_mix_mode"] == "xor_add" else "false"
    lines = [
        f"  std::memset({mask_stack_name}, 0, kMaskStackDepth * kMaskBytes);",
        f"  std::uint32_t aMaskSeedState = static_cast<std::uint32_t>(0x6D2B79F5u ^ {spec['seed_bias']}u ^ {spec['round_bias']}u);",
        "  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {",
        "    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);",
        f"    const int aIndex0 = WrapRange(static_cast<int>({seed_lane0} * {spec['seed_stride_a']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex1 = WrapRange(static_cast<int>({seed_lane1} * {spec['seed_stride_b']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex2 = WrapRange(static_cast<int>({seed_lane2} * ({spec['seed_stride_a']}u + {spec['seed_stride_b']}u) + ({offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex1]);",
        "    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);",
    ]
    lines.extend(
        render_mask_seed_mix_lines(
            spec,
            source_a="a",
            source_b="b",
            source_c="c",
            worker_lane=worker_lane,
            seed_a_name="aSeedA",
            seed_b_name="aSeedB",
            seed_c_name="aSeedC",
            worker_byte_name="aWorkerByte",
            turbulence_name="aTurbulence",
        )
    )
    lines.extend(
        [
            f"    const std::size_t aFlatBit = (static_cast<std::size_t>({seed_lane0}) * {spec['seed_stride_a']}u * 8U + static_cast<std::size_t>({seed_lane1}) * {spec['seed_stride_b']}u + {spec['seed_bias']}u + static_cast<std::size_t>(aTurbulence * (({spec['worker_stride']}u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);",
            f"    aMaskSeedState = AdvanceMaskSeedBitstream({mask_stack_name}, aMaskSeedState ^ aTurbulence, aFlatBit, aSeedA, aSeedB, aSeedC, static_cast<std::uint32_t>(aSourceIndex + aTurbulence), static_cast<std::uint32_t>({spec['seed_bias']}u), {spec['seed_shift']}u, {xor_mode});",
        ]
    )
    if spec["worker_mode"] != "off":
        lines.extend(
            [
                f"    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + {spec['worker_bias']}u) * (({spec['worker_stride']}u & 15U) + 1U))) % (kMaskStackTotalBytes * 8U);",
                f"    UpdateMaskSeedBit({mask_stack_name}, aTurbulenceBit, aMaskSeedState ^ aTurbulence ^ aWorkerByte, {spec['worker_rotate']}u, {xor_mode});",
            ]
        )
    lines.append("  }")
    if spec["second_pass"]:
        rotate = spec["second_rotate"]
        lines.extend(
            [
                "  for (unsigned int aSourceIndex = 0U; aSourceIndex < PASSWORD_EXPANDED_SIZE; ++aSourceIndex) {",
                "    const unsigned int aReverseIndex = static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE - 1U - aSourceIndex);",
                f"    const int aIndex3 = WrapRange(static_cast<int>({second_lane0} * {spec['second_stride_a']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
                f"    const int aIndex4 = WrapRange(static_cast<int>({second_lane1} * {spec['second_stride_b']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
                f"    const int aIndex5 = WrapRange(static_cast<int>({second_lane2} * ({spec['second_stride_a']}u + {spec['second_stride_b']}u) + ({offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
                "    const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex3]);",
                "    const std::uint32_t b = static_cast<std::uint32_t>(pSource[aIndex4]);",
                "    const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex5]);",
            ]
        )
        lines.extend(
            render_mask_seed_mix_lines(
                spec,
                source_a="a",
                source_b="b",
                source_c="c",
                worker_lane=worker_lane,
                seed_a_name="aSeedA",
                seed_b_name="aSeedB",
                seed_c_name="aSeedC",
                worker_byte_name="aWorkerByte",
                turbulence_name="aTurbulence",
            )
        )
        lines.extend(
            [
                f"    const std::size_t aFlatBit = (static_cast<std::size_t>({second_lane0}) * {spec['second_stride_a']}u * 8U + static_cast<std::size_t>({second_lane1}) * {spec['second_stride_b']}u * 3U + {spec['second_bias']}u + static_cast<std::size_t>(aTurbulence * (({spec['worker_stride']}u & 7U) + 1U))) % (kMaskStackTotalBytes * 8U);",
                "    const std::size_t aCarryByte = (aFlatBit >> 3U) % kMaskStackTotalBytes;",
                f"    const std::uint32_t aCarry = static_cast<std::uint32_t>({mask_stack_name}[(aCarryByte / kMaskBytes) % kMaskStackDepth][aCarryByte % kMaskBytes]);",
                f"    aMaskSeedState = AdvanceMaskSeedBitstream({mask_stack_name}, aMaskSeedState ^ aCarry ^ aTurbulence, aFlatBit + static_cast<std::size_t>((aCarry + aWorkerByte) * 13U), aSeedA ^ aCarry, static_cast<std::uint32_t>(RotateLeft8(static_cast<std::uint8_t>(aSeedB & 0xFFu), {rotate}u)), aSeedC, static_cast<std::uint32_t>(aSourceIndex + aCarry + aTurbulence), static_cast<std::uint32_t>({spec['second_bias']}u), {rotate}u, {xor_mode});",
            ]
        )
        if spec["worker_mode"] != "off":
            lines.extend(
                [
                    f"    const std::size_t aTurbulenceBit = (aFlatBit + static_cast<std::size_t>((aWorkerByte + aTurbulence + {spec['worker_bias']}u) * (({spec['worker_stride']}u & 31U) + 1U))) % (kMaskStackTotalBytes * 8U);",
                    f"    UpdateMaskSeedBit({mask_stack_name}, aTurbulenceBit, RotateLeft32(aMaskSeedState ^ aTurbulence ^ aCarry, 7U), {rotate}u, true);",
                ]
            )
        lines.append("  }")
    return lines


def choose_mask_phase_spec(rng: random.Random) -> dict[str, Any]:
    chosen_worker = rng.choice(("workera", "workerb"))
    other_worker = "workerb" if chosen_worker == "workera" else "workera"
    return {
        "chosen_worker": chosen_worker,
        "other_worker": other_worker,
        "offsets": (
            choose_signed_offset(rng),
            choose_signed_offset(rng),
            choose_signed_offset(rng),
        ),
        "template": rng.randrange(5),
        "mask_flat_offset": rng.randint(0, MASK_STACK_TOTAL_BYTES - 1),
        "mask_stride": rng.randint(1, 29),
        "control_bias": rng.randint(1, 7),
    }


def choose_lane_breaker_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "enabled": rng.random() < 0.7,
        "source_offset": choose_signed_offset(rng),
        "mask_flat_offset": rng.randint(0, MASK_STACK_TOTAL_BYTES - 1),
        "mode_count": 4,
        "partner_bias": rng.randint(1, 7),
        "partner_stride": rng.randint(1, 7),
        "lane_rotate": rng.randint(1, 7),
    }


def choose_braid_breaker_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "enabled": rng.random() < 0.6,
        "source_offset": choose_signed_offset(rng),
        "mask_flat_offset": rng.randint(0, MASK_STACK_TOTAL_BYTES - 1),
        "partner_offset": choose_signed_offset(rng),
        "chunk_span": rng.choice((16, 32, 64)),
        "mode_count": 3,
    }


def choose_jump_breaker_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "enabled": rng.random() < 0.6,
        "source_offset": choose_signed_offset(rng),
        "key_offset": rng.randint(0, ROUND_KEY_BYTES - 1),
        "mask_flat_offset": rng.randint(0, MASK_STACK_TOTAL_BYTES - 1),
        "partner_offset": choose_signed_offset(rng),
        "jump_stride": rng.choice((4, 8, 16, 32)),
        "section_bytes": rng.choice((2, 4, 8, 16)),
        "mode_count": 5,
    }


def choose_swap_breaker_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "enabled": rng.random() < 0.55,
        "source_offset": choose_signed_offset(rng),
        "partner_offset": choose_signed_offset(rng),
        "chunk_span": rng.choice((8, 16, 32, 64)),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "lane_stride": rng.randint(1, 7),
        "partner_stride": rng.randint(1, 7),
        "mode_count": 4,
    }


def choose_dual_matrix_pass_spec(rng: random.Random, kind: str) -> dict[str, Any]:
    if kind == "lightning":
        fast_pool = tuple(rng.sample(MATRIX_FAST_OPS, rng.randint(3, 6)))
        slow_pool = tuple(rng.sample(DIFFUSION_MATRIX_SLOW_OPS, rng.randint(2, 4)))
    elif kind == "typhoon":
        fast_pool = tuple(rng.sample(TYPHOON_FAST_OPS, rng.randint(4, 7)))
        slow_pool = tuple(rng.sample(TYPHOON_SLOW_OPS, rng.randint(2, 4)))
    else:
        fast_pool = tuple(rng.sample(HURRICANE_FAST_OPS, rng.randint(4, 7)))
        slow_pool = tuple(rng.sample(HURRICANE_SLOW_OPS, rng.randint(2, 5)))
    source_buffer = rng.choice(("workera", "workerb"))
    control_choices = [name for name in ("source", "workera", "workerb") if name != source_buffer]
    return {
        "enabled": rng.random() < 0.7,
        "kind": kind,
        "source_buffer": source_buffer,
        "control_buffer": rng.choice(control_choices),
        "source_offset": choose_signed_offset(rng),
        "control_offset": choose_signed_offset(rng),
        "mask_flat_offset": rng.randint(0, MASK_STACK_TOTAL_BYTES - 1),
        "salt_offset": rng.randint(0, SALT_BYTES - 1),
        "mix_variant": rng.randrange(4),
        "emit_mix": rng.choice(("xor", "add")),
        "fast_skip_mod": rng.choice((0, 0, 2, 3, 4, 5)),
        "slow_skip_mod": rng.choice((0, 0, 2, 3, 4)),
        "fast_pool": fast_pool,
        "slow_pool": slow_pool,
    }


def choose_final_weave_spec(rng: random.Random) -> dict[str, Any]:
    return {
        "offsets": (
            choose_signed_offset(rng),
            choose_signed_offset(rng),
            choose_signed_offset(rng),
        ),
        "reverse_modes": choose_reverse_quad(rng),
        "template": rng.randrange(6),
        "key_row": rng.randrange(ROUND_KEY_STACK_DEPTH),
        "key_offset": rng.randint(0, ROUND_KEY_BYTES - 1),
        "control_offset": choose_signed_offset(rng),
        "write_mode": "assign",
    }


def render_mask_phase_lines(spec: dict[str, Any]) -> list[str]:
    chosen_cpp = buffer_cpp_name(spec["chosen_worker"])
    other_cpp = buffer_cpp_name(spec["other_worker"])
    chosen_mask_stack = "pMaskStackA" if spec["chosen_worker"] == "workera" else "pMaskStackB"
    other_mask_stack = "pMaskStackB" if spec["chosen_worker"] == "workera" else "pMaskStackA"
    off_a, off_b, off_c = spec["offsets"]
    lines = [
        "  {",
        "    const int aStart = 0;",
        "    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);",
        "    for (int i = aStart; i < aEnd; ++i) {",
        f"      const int aIndex0 = WrapRange(i + ({off_a}), aStart, aEnd);",
        f"      const int aIndex1 = WrapRange(i + ({off_b}), aStart, aEnd);",
        f"      const int aIndex2 = WrapRange(i + ({off_c}), aStart, aEnd);",
        f"      const std::uint32_t a = static_cast<std::uint32_t>({chosen_cpp}[aIndex0]);",
        f"      const std::uint32_t b = static_cast<std::uint32_t>({other_cpp}[aIndex1]);",
        "      const std::uint32_t c = static_cast<std::uint32_t>(pSource[aIndex2]);",
        f"      const std::size_t aMaskFlat = (static_cast<std::size_t>(i) * {spec['mask_stride']}u + {spec['mask_flat_offset']}u + static_cast<std::size_t>(pRound)) % kMaskStackTotalBytes;",
        f"      const int aMaskFlatOther = WrapRange(static_cast<int>(aMaskFlat) + ({spec['control_bias']} + {spec['template']}), 0, static_cast<int>(kMaskStackTotalBytes));",
        f"      const std::uint32_t aMaskByteA = static_cast<std::uint32_t>({chosen_mask_stack}[(aMaskFlat / kMaskBytes) % kMaskStackDepth][aMaskFlat % kMaskBytes]);",
        f"      const std::uint32_t aMaskByteB = static_cast<std::uint32_t>({other_mask_stack}[(static_cast<std::size_t>(aMaskFlatOther) / kMaskBytes) % kMaskStackDepth][static_cast<std::size_t>(aMaskFlatOther) % kMaskBytes]);",
        "      const std::uint32_t aMaskByte = static_cast<std::uint32_t>((aMaskByteA ^ RotateLeft32(aMaskByteB, 5U) ^ (aMaskByteB << 1U)) & 0xFFu);",
        f"      const std::uint32_t aRoute = (c ^ aMaskByte ^ aMaskByteB ^ static_cast<std::uint32_t>({spec['control_bias']}u)) % 5U;",
        "      std::uint32_t aOutA = a;",
        "      std::uint32_t aOutB = b;",
    ]
    lines.extend(
        [
            "      switch ((aRoute + static_cast<std::uint32_t>(" + str(spec["template"]) + "u)) % 5U) {",
            "        case 0U:",
            "          aOutA = static_cast<std::uint32_t>((a & aMaskByte) | (b & (~aMaskByte & 0xFFu)));",
            "          break;",
            "        case 1U:",
            "          aOutB = static_cast<std::uint32_t>((b & aMaskByte) | (a & (~aMaskByte & 0xFFu)));",
            "          break;",
            "        case 2U:",
            "          if ((c & 1U) != 0U) { const std::uint32_t aTemp = aOutA; aOutA = aOutB; aOutB = aTemp; }",
            "          break;",
            "        case 3U:",
            "          aOutA = static_cast<std::uint32_t>((a & 0xF0u) | (b & 0x0Fu));",
            "          aOutB = static_cast<std::uint32_t>((b & 0xF0u) | (a & 0x0Fu));",
            "          break;",
            "        default:",
            "          aOutA = ((aMaskByte & 1U) != 0U) ? b : a;",
            "          aOutB = ((aMaskByte & 2U) != 0U) ? a : b;",
            "          break;",
            "      }",
            f"      {chosen_cpp}[i] = static_cast<std::uint8_t>(aOutA);",
            f"      {other_cpp}[i] = static_cast<std::uint8_t>(aOutB);",
            "    }",
            "  }",
        ]
    )
    return lines


def render_lane_breaker_lines(spec: dict[str, Any]) -> list[str]:
    return [
        "  // LaneBreaker",
        "  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMaskBytes) {",
        f"    const auto aWeaveSource = LoadBlockWrapped<kMaskBytes>(pSource, PASSWORD_EXPANDED_SIZE, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto aWeaveMask = LoadMaskStackBlockWrapped<kMaskBytes>(pMaskStack, chunk + {spec['mask_flat_offset']}u);",
        "    auto lane_a = LoadBlockWrapped<kMaskBytes>(pWorkerA, PASSWORD_EXPANDED_SIZE, chunk);",
        "    auto lane_b = LoadBlockWrapped<kMaskBytes>(pWorkerB, PASSWORD_EXPANDED_SIZE, chunk);",
        f"    switch (static_cast<unsigned>(aWeaveSource[0] ^ aWeaveSource[3] ^ aWeaveMask[1]) % {spec['mode_count']}u) {{",
        "      case 0u:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        f"          const std::size_t aPartner = (aLane + {spec['partner_bias']}u + (aWeaveSource[aLane] & {spec['partner_stride']}u)) & 7U;",
        "          if (((aWeaveSource[aLane] ^ aWeaveMask[aLane]) & 1U) != 0U) {",
        "            const auto aTemp = lane_a[aLane];",
        "            lane_a[aLane] = lane_b[aPartner];",
        "            lane_b[aPartner] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "      case 1u:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        "          const unsigned aBit = static_cast<unsigned>((aWeaveSource[aLane] >> (aLane & 7U)) & 1U);",
        "          if (aBit != 0U) {",
        "            const std::uint8_t aMask = aWeaveMask[aLane];",
        "            const std::uint8_t a = lane_a[aLane];",
        "            const std::uint8_t b = lane_b[aLane];",
        "            lane_a[aLane] = static_cast<std::uint8_t>((a & ~aMask) | (b & aMask));",
        "            lane_b[aLane] = static_cast<std::uint8_t>((b & ~aMask) | (a & aMask));",
        "          }",
        "        }",
        "        break;",
        "      case 2u:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        f"          const std::size_t aPartner = (aLane + {spec['lane_rotate']}u + (aWeaveMask[aLane] & 3U)) & 7U;",
        "          if (((aWeaveMask[aLane] >> (aLane & 7U)) & 1U) != 0U) {",
        "            const std::uint8_t a = lane_a[aLane];",
        "            const std::uint8_t b = lane_b[aPartner];",
        "            lane_a[aLane] = static_cast<std::uint8_t>((a & 0xF0u) | (b & 0x0Fu));",
        "            lane_b[aPartner] = static_cast<std::uint8_t>((b & 0xF0u) | (a & 0x0Fu));",
        "          }",
        "        }",
        "        break;",
        "      default:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        "          const std::size_t aPartner = (aLane + 1U + ((aWeaveSource[aLane] ^ aWeaveMask[aLane]) & 3U)) & 7U;",
        "          if (((aWeaveSource[aPartner] + aWeaveMask[aLane]) & 1U) != 0U) {",
        "            const auto aTemp = lane_a[aPartner];",
        "            lane_a[aPartner] = lane_b[aLane];",
        "            lane_b[aLane] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "    }",
        "    for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        "      pWorkerA[(chunk + aLane) % PASSWORD_EXPANDED_SIZE] = lane_a[aLane];",
        "      pWorkerB[(chunk + aLane) % PASSWORD_EXPANDED_SIZE] = lane_b[aLane];",
        "    }",
        "  }",
    ]


def render_braid_breaker_lines(spec: dict[str, Any]) -> list[str]:
    return [
        "  // BraidBreaker",
        f"  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += {spec['chunk_span']}U) {{",
        f"    const std::size_t aPartnerBase = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['partner_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
        f"    const auto aSourceBlock = LoadBlockWrapped<kMaskBytes>(pSource, PASSWORD_EXPANDED_SIZE, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto aMaskBlock = LoadMaskStackBlockWrapped<kMaskBytes>(pMaskStack, chunk + {spec['mask_flat_offset']}u);",
        f"    switch (static_cast<unsigned>(aSourceBlock[0] ^ aMaskBlock[0]) % {spec['mode_count']}u) {{",
        "      case 0u:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          if (((aSourceBlock[aLane] ^ aMaskBlock[aLane]) & 1U) != 0U) {",
        "            const unsigned char aTemp = pWorkerA[aLeft];",
        "            pWorkerA[aLeft] = pWorkerB[aRight];",
        "            pWorkerB[aRight] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "      case 1u:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + ((aLane + 3U) & 7U)) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aMask = aMaskBlock[aLane];",
        "          const unsigned char a = pWorkerA[aLeft];",
        "          const unsigned char b = pWorkerB[aRight];",
        "          pWorkerA[aLeft] = static_cast<unsigned char>((a & ~aMask) | (b & aMask));",
        "          pWorkerB[aRight] = static_cast<unsigned char>((b & ~aMask) | (a & aMask));",
        "        }",
        "        break;",
        "      default:",
        "        for (std::size_t aLane = 0; aLane < kMaskBytes; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          if ((aSourceBlock[aLane] & 1U) != 0U) {",
        "            const unsigned char a = pWorkerA[aLeft];",
        "            const unsigned char b = pWorkerB[aRight];",
        "            pWorkerA[aLeft] = static_cast<unsigned char>((a & 0xF0u) | (b & 0x0Fu));",
        "            pWorkerB[aRight] = static_cast<unsigned char>((b & 0xF0u) | (a & 0x0Fu));",
        "          }",
        "        }",
        "        break;",
        "    }",
        "  }",
    ]


def render_jump_breaker_lines(spec: dict[str, Any]) -> list[str]:
    return [
        "  // JumpBreaker",
        f"  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += {spec['jump_stride']}U) {{",
        f"    const std::size_t aControlIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
        f"    const std::size_t aPartnerBase = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['partner_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
        "    const unsigned char aSourceByte = pSource[aControlIndex];",
        "    const unsigned char aKeyByte = KeyStackByte(pKeyStack, static_cast<std::size_t>((pRound + 3U + chunk) & 15U), chunk + " + str(spec["key_offset"]) + "U);",
        f"    const auto aMaskBlock = LoadMaskStackBlockWrapped<kMaskBytes>(pMaskStack, chunk + {spec['mask_flat_offset']}u);",
        "    const unsigned char aMaskByte0 = aMaskBlock[0];",
        f"    const std::size_t aSpan = 1U + (static_cast<std::size_t>(aSourceByte ^ aKeyByte ^ aMaskByte0) % {spec['section_bytes']}U);",
        f"    switch (static_cast<unsigned>(aSourceByte ^ aKeyByte) % {spec['mode_count']}u) {{",
        "      case 0u:",
        "        break;",
        "      case 1u:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aTemp = pWorkerA[aLeft];",
        "          pWorkerA[aLeft] = pWorkerB[aRight];",
        "          pWorkerB[aRight] = aTemp;",
        "        }",
        "        break;",
        "      case 2u:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + ((aLane + (aMaskByte0 & 3U)) % aSpan)) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aMask = aMaskBlock[aLane & 7U];",
        "          const unsigned char a = pWorkerA[aLeft];",
        "          const unsigned char b = pWorkerB[aRight];",
        "          pWorkerA[aLeft] = static_cast<unsigned char>((a & ~aMask) | (b & aMask));",
        "          pWorkerB[aRight] = static_cast<unsigned char>((b & ~aMask) | (a & aMask));",
        "        }",
        "        break;",
        "      case 3u:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + ((aLane * 3U + (aSourceByte & 3U)) % aSpan)) % PASSWORD_EXPANDED_SIZE;",
        "          if (((aMaskBlock[aLane & 7U] >> (aLane & 7U)) & 1U) != 0U) {",
        "            const unsigned char aTemp = pWorkerA[aLeft];",
        "            pWorkerA[aLeft] = pWorkerA[aRight];",
        "            pWorkerA[aRight] = aTemp;",
        "          } else {",
        "            const unsigned char aTemp = pWorkerB[aLeft];",
        "            pWorkerB[aLeft] = pWorkerB[aRight];",
        "            pWorkerB[aRight] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "      default:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + ((aLane + 1U + (aKeyByte & 3U)) % aSpan)) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aMask = aMaskBlock[(aLane + aSourceByte) & 7U];",
        "          const unsigned char a = pWorkerA[aLeft];",
        "          const unsigned char b = pWorkerB[aRight];",
        "          if (((aSourceByte ^ aKeyByte ^ aMask) & 1U) != 0U) {",
        "            pWorkerA[aLeft] = static_cast<unsigned char>((a & 0xF0u) | (b & 0x0Fu));",
        "            pWorkerB[aRight] = static_cast<unsigned char>((b & 0xF0u) | (a & 0x0Fu));",
        "          } else {",
        "            pWorkerA[aLeft] = ((aMask & 1U) != 0U) ? b : a;",
        "            pWorkerB[aRight] = ((aMask & 2U) != 0U) ? a : b;",
        "          }",
        "        }",
        "        break;",
        "    }",
        "  }",
    ]


def render_swap_breaker_lines(spec: dict[str, Any]) -> list[str]:
    return [
        "  // SwapBreaker",
        f"  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += {spec['chunk_span']}U) {{",
        f"    const std::size_t aPartnerBase = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['partner_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
        f"    const std::size_t aControlIndex = static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE)));",
        f"    const unsigned char aRuleByte = static_cast<unsigned char>(pSource[aControlIndex] ^ pSalt[(chunk / {spec['chunk_span']}U + {spec['salt_offset']}u) & 31U] ^ static_cast<unsigned char>(pRound));",
        f"    const std::size_t aSpan = 1U + (static_cast<std::size_t>(aRuleByte ^ pSalt[{spec['salt_offset']}u]) % {spec['chunk_span']}U);",
        f"    switch (static_cast<unsigned>(aRuleByte) % {spec['mode_count']}u) {{",
        "      case 0u:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        f"          const std::size_t aLeft = (chunk + aLane * {spec['lane_stride']}u) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aSourceByte = pSource[(aLeft + static_cast<std::size_t>(aRuleByte)) % PASSWORD_EXPANDED_SIZE];",
        "          const unsigned char aSaltByte = pSalt[(aLane + static_cast<std::size_t>(aRuleByte)) & 31U];",
        "          if (((aSourceByte ^ aSaltByte) & 1U) != 0U) {",
        "            const unsigned char aTemp = pWorkerA[aLeft];",
        "            pWorkerA[aLeft] = pWorkerB[aRight];",
        "            pWorkerB[aRight] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "      case 1u:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        f"          const std::size_t aRight = (aPartnerBase + ((aLane * {spec['partner_stride']}u + static_cast<std::size_t>(aRuleByte)) % aSpan)) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aSourceByte = pSource[(aRight + static_cast<std::size_t>(aRuleByte)) % PASSWORD_EXPANDED_SIZE];",
        f"          const unsigned char aSaltByte = pSalt[(aLane + {spec['salt_offset']}u) & 31U];",
        "          if (((aSourceByte + aSaltByte) & 3U) != 0U) {",
            "            const unsigned char aTemp = pWorkerA[aLeft];",
            "            pWorkerA[aLeft] = pWorkerB[aRight];",
            "            pWorkerB[aRight] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "      case 2u:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + (aSpan - 1U - aLane)) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aSourceByte = pSource[(aLeft + aRight) % PASSWORD_EXPANDED_SIZE];",
        "          const unsigned char aSaltByte = pSalt[(aLane + 11U + static_cast<std::size_t>(aRuleByte)) & 31U];",
        "          if (((aSourceByte ^ aSaltByte ^ aRuleByte) & 1U) != 0U) {",
        "            const unsigned char aTemp = pWorkerA[aLeft];",
        "            pWorkerA[aLeft] = pWorkerB[aRight];",
        "            pWorkerB[aRight] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "      default:",
        "        for (std::size_t aLane = 0; aLane < aSpan; ++aLane) {",
        "          const std::size_t aLeft = (chunk + aLane) % PASSWORD_EXPANDED_SIZE;",
        "          const std::size_t aRight = (aPartnerBase + ((aLane + static_cast<std::size_t>(aRuleByte)) % aSpan)) % PASSWORD_EXPANDED_SIZE;",
        "          const unsigned char aSourceByte = pSource[(aControlIndex + aLane) % PASSWORD_EXPANDED_SIZE];",
        "          const unsigned char aSaltByte = pSalt[(aLane + 17U + static_cast<std::size_t>(aRuleByte)) & 31U];",
        "          if (((aSourceByte + aSaltByte + aRuleByte) & 1U) == 0U) {",
            "            const unsigned char aTemp = pWorkerA[aLeft];",
        "            pWorkerA[aLeft] = pWorkerB[aRight];",
        "            pWorkerB[aRight] = aTemp;",
        "          }",
        "        }",
        "        break;",
        "    }",
        "  }",
    ]


def build_breaker_source_offsets(desired_count: int, *candidate_groups: Any) -> tuple[int, ...]:
    unique_offsets: list[int] = []
    for group in candidate_groups:
        if isinstance(group, (tuple, list)):
            values = group
        else:
            values = (group,)
        for value in values:
            offset = int(value)
            if offset not in unique_offsets:
                unique_offsets.append(offset)

    fill_stride = ALIGNMENT * (desired_count + 3)
    cursor = 0
    while len(unique_offsets) < desired_count:
        base = unique_offsets[cursor % len(unique_offsets)] if unique_offsets else fill_stride
        candidate = int(base + ((cursor + 1) * fill_stride))
        while candidate in unique_offsets:
            candidate += ALIGNMENT
        unique_offsets.append(candidate)
        cursor += 1
    return tuple(unique_offsets[:desired_count])


def render_dual_worker_matrix_breaker_lines(recipe_name: str, recipe: dict[str, Any]) -> list[str]:
    recipe_type = f"DualWorkerBreakerRecipe{recipe['index_count']}"
    source_offsets = ", ".join(str(offset) for offset in recipe["source_offsets"])
    return [
        "  {",
        f"    const {recipe_type} {recipe_name}{{",
        f"        {{{source_offsets}}},",
        f"        {recipe['control_offset']},",
        f"        {recipe['partner_offset']},",
        f"        static_cast<std::size_t>({recipe['mask_flat_offset']}u),",
        f"        static_cast<std::uint8_t>({recipe['key_row']}u),",
        f"        static_cast<std::uint8_t>({recipe['key_offset']}u),",
        f"        static_cast<std::uint8_t>({recipe['salt_offset']}u),",
        f"        static_cast<std::uint8_t>({recipe['mixlane_a']}u),",
        f"        static_cast<std::uint8_t>({recipe['mixlane_b']}u),",
        f"        static_cast<std::uint8_t>({recipe['fast_rule']}u),",
        f"        static_cast<std::uint8_t>({recipe['slow_rule']}u),",
        f"        static_cast<std::uint8_t>({recipe['index_mode']}u),",
        f"        static_cast<std::uint8_t>({recipe['emit_mode']}u),",
        f"        static_cast<std::uint8_t>({recipe['source_mix_variant']}u),",
        f"        static_cast<std::uint8_t>({recipe['lane_mix_variant_a']}u),",
        f"        static_cast<std::uint8_t>({recipe['lane_mix_variant_b']}u),",
        f"        static_cast<std::uint8_t>({recipe['inject_mode']}u),",
        f"        static_cast<std::uint8_t>({recipe['matrix_mode']}u),",
        "    };",
        f"    ApplyDualWorkerMatrixBreaker({recipe_name}, pSource, pWorkerA, pWorkerB, pSalt, pKeyStack, pMaskStackA, pMaskStackB, pRound);",
        "  }",
    ]


def build_dual_worker_breaker_recipes(
    worker_a_spec: dict[str, Any],
    worker_b_spec: dict[str, Any],
    lane_breaker_spec: dict[str, Any],
    braid_breaker_spec: dict[str, Any],
    jump_breaker_spec: dict[str, Any],
    swap_breaker_spec: dict[str, Any],
    final_spec: dict[str, Any],
) -> list[tuple[str, dict[str, Any]]]:
    recipes: list[tuple[str, dict[str, Any]]] = []

    if lane_breaker_spec["enabled"] or braid_breaker_spec["enabled"]:
        index_count_ab = max(2, min(4, 2 + int(lane_breaker_spec["enabled"]) + int(braid_breaker_spec["enabled"])))
        recipes.append(
            (
                "aBreakerAB",
                {
                    "index_count": index_count_ab,
                    "source_offsets": build_breaker_source_offsets(
                        index_count_ab,
                        worker_a_spec["offsets"],
                        lane_breaker_spec["source_offset"],
                        braid_breaker_spec["source_offset"],
                        worker_b_spec["offsets"][0],
                        final_spec["offsets"][0],
                    ),
                    "control_offset": braid_breaker_spec["source_offset"] if braid_breaker_spec["enabled"] else worker_a_spec["control_offset"],
                    "partner_offset": braid_breaker_spec["partner_offset"] if braid_breaker_spec["enabled"] else lane_breaker_spec["partner_bias"] * 4,
                    "mask_flat_offset": (
                        (lane_breaker_spec["mask_flat_offset"] if lane_breaker_spec["enabled"] else 0)
                        + (braid_breaker_spec["mask_flat_offset"] if braid_breaker_spec["enabled"] else 0)
                    ) % MASK_STACK_TOTAL_BYTES,
                    "key_row": worker_a_spec["key_row"] % ROUND_KEY_STACK_DEPTH,
                    "key_offset": worker_a_spec["key_offset"] % ROUND_KEY_BYTES,
                    "salt_offset": (worker_a_spec["salt_offset"] + lane_breaker_spec["partner_bias"]) % SALT_BYTES,
                    "mixlane_a": (lane_breaker_spec["lane_rotate"] + worker_a_spec["control_shift"]) % 16,
                    "mixlane_b": (
                        lane_breaker_spec["partner_bias"]
                        + braid_breaker_spec["mode_count"]
                        + worker_b_spec["control_shift"]
                    ) % 16,
                    "fast_rule": (
                        lane_breaker_spec["mode_count"] * 17
                        + worker_a_spec["template"] * 13
                        + int(braid_breaker_spec["enabled"]) * 29
                    ) & 0xFF,
                    "slow_rule": (
                        braid_breaker_spec["chunk_span"]
                        + braid_breaker_spec["mode_count"] * 19
                        + worker_b_spec["template"] * 7
                    ) & 0xFF,
                    "index_mode": (
                        worker_a_spec["data_index_count"]
                        + lane_breaker_spec["mode_count"]
                        + braid_breaker_spec["mode_count"]
                    ) & 0x0F,
                    "emit_mode": 0 if worker_a_spec["control_mode"] == "key" else 1,
                    "source_mix_variant": (
                        worker_a_spec["template"]
                        + lane_breaker_spec["mode_count"]
                        + int(braid_breaker_spec["enabled"])
                    ) & 0x03,
                    "lane_mix_variant_a": (
                        worker_a_spec["data_index_count"]
                        + worker_b_spec["template"]
                        + final_spec["template"]
                    ) & 0x03,
                    "lane_mix_variant_b": (
                        lane_breaker_spec["partner_bias"]
                        + braid_breaker_spec["chunk_span"]
                        + worker_b_spec["data_index_count"]
                    ) & 0x03,
                    "inject_mode": (
                        worker_a_spec["template"]
                        + worker_b_spec["template"]
                        + lane_breaker_spec["mode_count"]
                    ) & 0x03,
                    "matrix_mode": (
                        worker_a_spec["data_index_count"]
                        + braid_breaker_spec["mode_count"]
                        + final_spec["template"]
                    ) & 0x03,
                },
            )
        )

    if jump_breaker_spec["enabled"] or swap_breaker_spec["enabled"]:
        index_count_cd = max(2, min(4, 2 + int(jump_breaker_spec["enabled"]) + int(swap_breaker_spec["enabled"])))
        recipes.append(
            (
                "aBreakerCD",
                {
                    "index_count": index_count_cd,
                    "source_offsets": build_breaker_source_offsets(
                        index_count_cd,
                        worker_b_spec["offsets"],
                        jump_breaker_spec["source_offset"],
                        swap_breaker_spec["source_offset"],
                        final_spec["offsets"],
                        worker_a_spec["offsets"][0],
                    ),
                    "control_offset": swap_breaker_spec["source_offset"] if swap_breaker_spec["enabled"] else worker_b_spec["control_offset"],
                    "partner_offset": jump_breaker_spec["partner_offset"] if jump_breaker_spec["enabled"] else swap_breaker_spec["partner_offset"],
                    "mask_flat_offset": (
                        jump_breaker_spec["mask_flat_offset"] if jump_breaker_spec["enabled"] else swap_breaker_spec["salt_offset"] * 17
                    ) % MASK_STACK_TOTAL_BYTES,
                    "key_row": (worker_b_spec["key_row"] + 3) % ROUND_KEY_STACK_DEPTH,
                    "key_offset": (jump_breaker_spec["key_offset"] if jump_breaker_spec["enabled"] else worker_b_spec["key_offset"]) % ROUND_KEY_BYTES,
                    "salt_offset": (swap_breaker_spec["salt_offset"] if swap_breaker_spec["enabled"] else jump_breaker_spec["key_offset"]) % SALT_BYTES,
                    "mixlane_a": (
                        (jump_breaker_spec["section_bytes"] if jump_breaker_spec["enabled"] else 4)
                        + worker_b_spec["control_shift"]
                        + final_spec["template"]
                    ) % 16,
                    "mixlane_b": (
                        (swap_breaker_spec["partner_stride"] if swap_breaker_spec["enabled"] else 3)
                        + (jump_breaker_spec["jump_stride"] if jump_breaker_spec["enabled"] else 5)
                    ) % 16,
                    "fast_rule": (
                        jump_breaker_spec["mode_count"] * 23
                        + swap_breaker_spec["mode_count"] * 11
                        + worker_b_spec["template"] * 5
                    ) & 0xFF,
                    "slow_rule": (
                        (jump_breaker_spec["section_bytes"] if jump_breaker_spec["enabled"] else 8)
                        + (swap_breaker_spec["chunk_span"] if swap_breaker_spec["enabled"] else 16)
                        + final_spec["template"] * 31
                    ) & 0xFF,
                    "index_mode": (
                        worker_b_spec["data_index_count"]
                        + jump_breaker_spec["mode_count"]
                        + swap_breaker_spec["mode_count"]
                    ) & 0x0F,
                    "emit_mode": 1 if worker_b_spec["control_mode"] == "mask" else 0,
                    "source_mix_variant": (
                        worker_b_spec["template"]
                        + jump_breaker_spec["mode_count"]
                        + swap_breaker_spec["mode_count"]
                    ) & 0x03,
                    "lane_mix_variant_a": (
                        jump_breaker_spec["jump_stride"]
                        + worker_a_spec["template"]
                        + final_spec["template"]
                    ) & 0x03,
                    "lane_mix_variant_b": (
                        swap_breaker_spec["chunk_span"]
                        + worker_b_spec["data_index_count"]
                        + worker_a_spec["control_shift"]
                    ) & 0x03,
                    "inject_mode": (
                        worker_b_spec["template"]
                        + jump_breaker_spec["section_bytes"]
                        + swap_breaker_spec["mode_count"]
                    ) & 0x03,
                    "matrix_mode": (
                        worker_b_spec["data_index_count"]
                        + jump_breaker_spec["mode_count"]
                        + final_spec["template"]
                    ) & 0x03,
                },
            )
        )

    return recipes


def render_matrix_dispatch_switch(
    matrix_name: str,
    enum_prefix: str,
    op_kind: str,
    selector_expr: str,
    arg0_expr: str,
    arg1_expr: str,
    pool: tuple[str, ...],
) -> list[str]:
    enum_name = f"{enum_prefix}{'FastOp' if op_kind == 'fast' else 'SlowOp'}"
    method_name = "ApplyFastOp" if op_kind == "fast" else "ApplySlowOp"
    lines = [f"    switch (static_cast<unsigned>({selector_expr}) % {len(pool)}u) {{"]
    for index, op_name in enumerate(pool):
        lines.append(
            f"      case {index}u: {matrix_name}.{method_name}({enum_name}::{op_name}, {arg0_expr}, {arg1_expr}); break;"
        )
    lines.append("    }")
    return lines


def render_matrix_dispatch_maybe_skip(
    matrix_name: str,
    enum_prefix: str,
    op_kind: str,
    selector_expr: str,
    arg0_expr: str,
    arg1_expr: str,
    pool: tuple[str, ...],
    skip_mod: int,
) -> list[str]:
    lines: list[str] = []
    if skip_mod > 0:
        lines.append(f"    if ((static_cast<unsigned>({selector_expr}) % {skip_mod}u) != 0u) {{")
    lines.extend(
        render_matrix_dispatch_switch(
            matrix_name,
            enum_prefix,
            op_kind,
            selector_expr,
            arg0_expr,
            arg1_expr,
            pool,
        )
    )
    if skip_mod > 0:
        lines.append("    }")
    return lines


def render_lightning_pass_lines(spec: dict[str, Any]) -> list[str]:
    source_cpp = buffer_cpp_name(spec["source_buffer"])
    control_cpp = buffer_cpp_name(spec["control_buffer"])
    lines = [
        "  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kMatrixBlockBytes) {",
        f"    auto storm_block = LoadBlock16Wrapped({source_cpp}, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto storm_control = LoadBlock16Wrapped({control_cpp}, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['control_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto storm_mask_a = LoadMaskStackBlockWrapped<kMatrixBlockBytes>(pMaskStackA, chunk + {spec['mask_flat_offset']}u);",
        f"    const auto storm_mask_b = LoadMaskStackBlockWrapped<kMatrixBlockBytes>(pMaskStackB, chunk + {spec['mask_flat_offset']}u + 17u);",
        "    LightningMatrix storm(storm_block.data());",
        "    storm.InjectXor(storm_mask_a.data(), storm_mask_a.size(), storm_control[0] & 15U);",
        "    storm.InjectAdd(storm_mask_b.data(), storm_mask_b.size(), storm_control[1] & 15U);",
        f"    storm.InjectAdd(pSalt, kSaltBytes, {spec['salt_offset']}u);",
    ]
    lines.extend(
        render_matrix_dispatch_maybe_skip(
            "storm",
            "Lightning",
            "fast",
            "storm_control[1] ^ storm_mask_a[2] ^ storm_mask_b[3] ^ pSalt[0]",
            "static_cast<std::uint8_t>(storm_control[3] ^ storm_mask_a[4] ^ storm_mask_b[5])",
            "static_cast<std::uint8_t>(storm_control[5] + storm_mask_a[6] + storm_mask_b[7])",
            spec["fast_pool"],
            spec["fast_skip_mod"],
        )
    )
    lines.extend(
        render_matrix_dispatch_maybe_skip(
            "storm",
            "Lightning",
            "slow",
            "storm_control[7] ^ storm_mask_a[8] ^ storm_mask_b[9] ^ pSalt[1]",
            "static_cast<std::uint8_t>(storm_control[9] + storm_mask_a[10] + storm_mask_b[11])",
            "static_cast<std::uint8_t>(storm_block[11] ^ storm_mask_a[12] ^ storm_mask_b[13])",
            spec["slow_pool"],
            spec["slow_skip_mod"],
        )
    )
    lines.append(
        "    storm.ApplySlowOp(static_cast<LightningSlowOp>((storm_control[13] ^ storm_mask_a[14] ^ storm_mask_b[15] ^ pSalt[2]) % 8U), "
        "static_cast<std::uint8_t>(storm_control[1] + storm_mask_a[3] + storm_mask_b[4]), "
        "static_cast<std::uint8_t>(storm_block[5] ^ storm_mask_a[7] ^ storm_mask_b[8]));"
    )
    lines.append(
        f"    ApplyLightningMixColumns(storm, pSalt, static_cast<std::uint8_t>((storm_control[11] + pRound + {spec['salt_offset']}u) & 31U), static_cast<std::uint8_t>(storm_control[15] ^ storm_mask_a[0] ^ storm_mask_b[1] ^ pSalt[3]), static_cast<std::uint8_t>({spec['mix_variant']}u));"
    )
    lines.extend(
        [
            "    std::array<std::uint8_t, kMatrixBlockBytes> storm_store{};",
            "    std::array<std::uint8_t, kMatrixBlockBytes> storm_emit{};",
            "    storm.Store(storm_store.data());",
            "    for (std::size_t aLane = 0; aLane < kMatrixBlockBytes; ++aLane) {",
        ]
    )
    if spec["emit_mix"] == "xor":
        lines.append(
            "      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] ^ storm_control[(aLane + 3U) & 15U] ^ storm_mask_a[(aLane + 5U) & 15U] ^ storm_mask_b[(aLane + 9U) & 15U] ^ pSalt[(aLane + 7U) & 31U]);"
        )
    else:
        lines.append(
            "      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] + storm_control[(aLane + 3U) & 15U] + storm_mask_a[(aLane + 5U) & 15U] + storm_mask_b[(aLane + 9U) & 15U] + pSalt[(aLane + 7U) & 31U]);"
        )
    lines.extend(
        [
            "    }",
            f"    StoreBlock16Contiguous({source_cpp}, chunk, storm_emit);",
            "  }",
        ]
    )
    return lines


def render_hurricane_pass_lines(spec: dict[str, Any]) -> list[str]:
    source_cpp = buffer_cpp_name(spec["source_buffer"])
    control_cpp = buffer_cpp_name(spec["control_buffer"])
    lines = [
        "  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kHurricaneBlockBytes) {",
        f"    auto storm_block = LoadBlock256Wrapped({source_cpp}, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto storm_control = LoadBlock256Wrapped({control_cpp}, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['control_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto storm_mask_a = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStackA, chunk + {spec['mask_flat_offset']}u);",
        f"    const auto storm_mask_b = LoadMaskStackBlockWrapped<kHurricaneBlockBytes>(pMaskStackB, chunk + {spec['mask_flat_offset']}u + 29u);",
        "    HurricaneMatrix storm(storm_block.data());",
        "    storm.InjectXor(storm_mask_a.data(), storm_mask_a.size(), storm_control[0] & 0xFFU);",
        "    storm.InjectAdd(storm_mask_b.data(), storm_mask_b.size(), storm_control[1] & 0xFFU);",
        f"    storm.InjectAdd(pSalt, kSaltBytes, {spec['salt_offset']}u);",
    ]
    lines.extend(
        render_matrix_dispatch_maybe_skip(
            "storm",
            "Hurricane",
            "fast",
            "storm_control[13] ^ storm_mask_a[17] ^ storm_mask_b[19] ^ pSalt[2]",
            "static_cast<std::uint8_t>(storm_control[29] ^ storm_mask_a[31] ^ storm_mask_b[37])",
            "static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])",
            spec["fast_pool"],
            spec["fast_skip_mod"],
        )
    )
    lines.extend(
        render_matrix_dispatch_maybe_skip(
            "storm",
            "Hurricane",
            "slow",
            "storm_control[53] ^ storm_mask_a[59] ^ storm_mask_b[61] ^ pSalt[3]",
            "static_cast<std::uint8_t>(storm_control[61] + storm_mask_a[67] + storm_mask_b[71])",
            "static_cast<std::uint8_t>(storm_block[71] ^ storm_mask_a[73] ^ storm_mask_b[79])",
            spec["slow_pool"],
            spec["slow_skip_mod"],
        )
    )
    lines.append(
        "    storm.ApplySlowOp(static_cast<HurricaneSlowOp>((storm_control[79] ^ storm_mask_a[83] ^ storm_mask_b[89] ^ pSalt[4]) % 8U), "
        "static_cast<std::uint8_t>(storm_control[89] + storm_mask_a[97] + storm_mask_b[101]), "
        "static_cast<std::uint8_t>(storm_block[101] ^ storm_mask_a[103] ^ storm_mask_b[107]));"
    )
    lines.extend(
        [
            "    std::array<std::uint8_t, kHurricaneBlockBytes> storm_store{};",
            "    std::array<std::uint8_t, kHurricaneBlockBytes> storm_emit{};",
            "    storm.Store(storm_store.data());",
            "    for (std::size_t aLane = 0; aLane < kHurricaneBlockBytes; ++aLane) {",
        ]
    )
    if spec["emit_mix"] == "xor":
        lines.append(
            "      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] ^ storm_control[(aLane + 11U) & 255U] ^ storm_mask_a[(aLane + 17U) & 255U] ^ storm_mask_b[(aLane + 29U) & 255U] ^ pSalt[aLane & 31U]);"
        )
    else:
        lines.append(
            "      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] + storm_control[(aLane + 11U) & 255U] + storm_mask_a[(aLane + 17U) & 255U] + storm_mask_b[(aLane + 29U) & 255U] + pSalt[aLane & 31U]);"
        )
    lines.extend(
        [
            "    }",
            f"    StoreBlock256Contiguous({source_cpp}, chunk, storm_emit);",
            "  }",
        ]
    )
    return lines


def render_typhoon_pass_lines(spec: dict[str, Any]) -> list[str]:
    source_cpp = buffer_cpp_name(spec["source_buffer"])
    control_cpp = buffer_cpp_name(spec["control_buffer"])
    lines = [
        "  for (std::size_t chunk = 0; chunk < PASSWORD_EXPANDED_SIZE; chunk += kTyphoonBlockBytes) {",
        f"    auto storm_block = LoadBlock128Wrapped({source_cpp}, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['source_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto storm_control = LoadBlock128Wrapped({control_cpp}, static_cast<std::size_t>(WrapRange(static_cast<int>(chunk) + ({spec['control_offset']}), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE))));",
        f"    const auto storm_mask_a = LoadMaskStackBlockWrapped<kTyphoonBlockBytes>(pMaskStackA, chunk + {spec['mask_flat_offset']}u);",
        f"    const auto storm_mask_b = LoadMaskStackBlockWrapped<kTyphoonBlockBytes>(pMaskStackB, chunk + {spec['mask_flat_offset']}u + 23u);",
        "    TyphoonMatrix storm(storm_block.data());",
        "    storm.InjectXor(storm_mask_a.data(), storm_mask_a.size(), storm_control[0] & 127U);",
        "    storm.InjectAdd(storm_mask_b.data(), storm_mask_b.size(), storm_control[1] & 127U);",
        f"    storm.InjectAdd(pSalt, kSaltBytes, {spec['salt_offset']}u);",
    ]
    lines.extend(
        render_matrix_dispatch_maybe_skip(
            "storm",
            "Typhoon",
            "fast",
            "storm_control[3] ^ storm_mask_a[11] ^ storm_mask_b[13] ^ pSalt[0]",
            "static_cast<std::uint8_t>(storm_control[17] ^ storm_mask_a[23] ^ storm_mask_b[29])",
            "static_cast<std::uint8_t>(storm_control[29] + storm_mask_a[31] + storm_mask_b[37])",
            spec["fast_pool"],
            spec["fast_skip_mod"],
        )
    )
    lines.extend(
        render_matrix_dispatch_maybe_skip(
            "storm",
            "Typhoon",
            "slow",
            "storm_control[37] ^ storm_mask_a[41] ^ storm_mask_b[43] ^ pSalt[1]",
            "static_cast<std::uint8_t>(storm_control[43] + storm_mask_a[47] + storm_mask_b[53])",
            "static_cast<std::uint8_t>(storm_block[53] ^ storm_mask_a[59] ^ storm_mask_b[61])",
            spec["slow_pool"],
            spec["slow_skip_mod"],
        )
    )
    lines.append(
        "    storm.ApplySlowOp(static_cast<TyphoonSlowOp>((storm_control[61] ^ storm_mask_a[67] ^ storm_mask_b[71] ^ pSalt[2]) % 4U), "
        "static_cast<std::uint8_t>(storm_control[71] + storm_mask_a[73] + storm_mask_b[79]), "
        "static_cast<std::uint8_t>(storm_block[79] ^ storm_mask_a[83] ^ storm_mask_b[89]));"
    )
    lines.extend(
        [
            "    std::array<std::uint8_t, kTyphoonBlockBytes> storm_store{};",
            "    std::array<std::uint8_t, kTyphoonBlockBytes> storm_emit{};",
            "    storm.Store(storm_store.data());",
            "    for (std::size_t aLane = 0; aLane < kTyphoonBlockBytes; ++aLane) {",
        ]
    )
    if spec["emit_mix"] == "xor":
        lines.append(
            "      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] ^ storm_control[(aLane + 7U) & 127U] ^ storm_mask_a[(aLane + 13U) & 127U] ^ storm_mask_b[(aLane + 27U) & 127U] ^ pSalt[(aLane + 19U) & 31U]);"
        )
    else:
        lines.append(
            "      storm_emit[aLane] = static_cast<std::uint8_t>(storm_store[aLane] + storm_control[(aLane + 7U) & 127U] + storm_mask_a[(aLane + 13U) & 127U] + storm_mask_b[(aLane + 27U) & 127U] + pSalt[(aLane + 19U) & 31U]);"
        )
    lines.extend(
        [
            "    }",
            f"    StoreBlock128Contiguous({source_cpp}, chunk, storm_emit);",
            "  }",
        ]
    )
    return lines


def render_final_weave_lines(spec: dict[str, Any]) -> list[str]:
    off_a, off_b, off_c = spec["offsets"]
    reverse_a, reverse_b, reverse_c, reverse_control = spec["reverse_modes"]
    lane_a_expr = lane_expr_for(reverse_a, "i")
    lane_b_expr = lane_expr_for(reverse_b, "i")
    lane_c_expr = lane_expr_for(reverse_c, "i")
    control_lane_expr = lane_expr_for(reverse_control, "i")
    lines = [
        "  {",
        "    const int aStart = 0;",
        "    const int aEnd = static_cast<int>(PASSWORD_EXPANDED_SIZE);",
        "    for (int i = aStart; i < aEnd; ++i) {",
        "      const int aReverseIndex = aEnd - 1 - i;",
        f"      const int aIndex0 = WrapRange({lane_a_expr} + ({off_a}), aStart, aEnd);",
        f"      const int aIndex1 = WrapRange({lane_b_expr} + ({off_b}), aStart, aEnd);",
        f"      const int aIndex2 = WrapRange({lane_c_expr} + ({off_c}), aStart, aEnd);",
        f"      const int aControlIndex = WrapRange({control_lane_expr} + ({spec['control_offset']}), aStart, aEnd);",
        "      const std::uint32_t a = static_cast<std::uint32_t>(pSource[aIndex0]);",
        "      const std::uint32_t b = static_cast<std::uint32_t>(pWorkerA[aIndex1]);",
        "      const std::uint32_t c = static_cast<std::uint32_t>(pWorkerB[aIndex2]);",
        "      const std::uint32_t aSourceCtrl = static_cast<std::uint32_t>(pSource[aControlIndex]);",
        "      const std::uint32_t aKeyByte = static_cast<std::uint32_t>(KeyStackByte(",
        "          pKeyStack,",
        f"          static_cast<std::size_t>((pRound + {spec['key_row']}u + static_cast<unsigned int>(i)) & 15U),",
        f"          static_cast<std::size_t>(i + {spec['key_offset']}u)));",
    ]
    lines.extend(
        [
        f"      const std::uint32_t aRoute = (aSourceCtrl ^ aKeyByte ^ static_cast<std::uint32_t>({spec['template']}u)) % 6U;",
        "      std::uint32_t aValue = a;",
    ]
    )
    lines.extend(
        [
            "      switch (aRoute) {",
            "        case 0U: aValue = a; break;",
            "        case 1U: aValue = b; break;",
            "        case 2U: aValue = c; break;",
            "        case 3U: aValue = static_cast<std::uint32_t>(a ^ b); break;",
            "        case 4U: aValue = static_cast<std::uint32_t>((b & aKeyByte) | (c & (~aKeyByte & 0xFFu))); break;",
            "        default: aValue = static_cast<std::uint32_t>((a & 0xF0u) | (b & 0x0Fu)); break;",
            "      }",
            "      pDest[i] = static_cast<std::uint8_t>(aValue);",
        ]
    )
    lines.extend(["    }", "  }"])
    return lines


def render_round_key_update_lines_ferocious(spec: dict[str, Any]) -> list[str]:
    offset0, offset1, offset2 = spec["round_key_offsets"]
    lines = [
        "  std::memset(pNextRoundKeyBuffer, 0, kRoundKeyBytes);",
        "  unsigned int aSourceIndex = 0U;",
        "  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {",
        f"    const int idx0 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_scan_stride_a']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int idx1 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_scan_stride_b']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int idx2 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_scan_stride_c']}u + ({offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const unsigned int aKeyIndex = (aSourceIndex * {spec['round_scan_stride_a']}u + static_cast<unsigned int>({spec['round_key_rotate']}u)) & 31U;",
        f"    const unsigned int aKeyIndex2 = (aKeyIndex + static_cast<unsigned int>({spec['round_key_spread']}u)) & 31U;",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pDest[idx0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pDest[idx1]);",
        "    const std::uint32_t c = static_cast<std::uint32_t>(pDest[idx2]);",
        "    const std::uint32_t salt_byte = static_cast<std::uint32_t>(pSalt[aSourceIndex & 31U]);",
    ]
    if spec["round_mix_mode"] == "xor_add":
        lines.extend(
            [
                "    const std::uint32_t mix_value = static_cast<std::uint32_t>(((a + b) ^ c ^ salt_byte) & 0xFFu);",
                "    pNextRoundKeyBuffer[aKeyIndex] ^= static_cast<unsigned char>(mix_value);",
                "    pNextRoundKeyBuffer[aKeyIndex2] ^= static_cast<unsigned char>((b + c + salt_byte) & 0xFFu);",
            ]
        )
    else:
        lines.extend(
            [
                "    const std::uint32_t mix_value = static_cast<std::uint32_t>(((a ^ b) + c + salt_byte) & 0xFFu);",
                "    pNextRoundKeyBuffer[aKeyIndex] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex] + static_cast<unsigned char>(mix_value));",
                "    pNextRoundKeyBuffer[aKeyIndex2] = static_cast<unsigned char>(pNextRoundKeyBuffer[aKeyIndex2] + static_cast<unsigned char>((b ^ c ^ salt_byte) & 0xFFu));",
            ]
        )
    lines.extend(["    ++aSourceIndex;", "  }", "  RotateKeyStack(pKeyStack, pNextRoundKeyBuffer);"])
    return lines


def render_round_mask_update_lines(
    spec: dict[str, Any],
    mask_stack_name: str,
    other_mask_stack_name: str,
    next_round_mask_name: str,
) -> list[str]:
    offset0, offset1, offset2 = spec["round_offsets"]
    lines = [
        f"  std::memset({next_round_mask_name}, 0, kMaskBytes);",
        "  unsigned int aSourceIndex = 0U;",
        "  while (aSourceIndex < PASSWORD_EXPANDED_SIZE) {",
        f"    const int aIndex0 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_stride_a']}u + ({offset0})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex1 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_stride_b']}u + ({offset1})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const int aIndex2 = WrapRange(static_cast<int>(aSourceIndex * {spec['round_stride_c']}u + ({offset2})), 0, static_cast<int>(PASSWORD_EXPANDED_SIZE));",
        f"    const unsigned int aMaskIndex = (aSourceIndex + static_cast<unsigned int>({spec['round_bias']}u)) & 7U;",
        f"    const unsigned int aOtherMaskIndex = (aSourceIndex + static_cast<unsigned int>({spec['round_bias']}u) + 3U) & 7U;",
        "    const std::uint32_t a = static_cast<std::uint32_t>(pDest[aIndex0]);",
        "    const std::uint32_t b = static_cast<std::uint32_t>(pDest[aIndex1]);",
        "    const std::uint32_t c = static_cast<std::uint32_t>(pDest[aIndex2]);",
        f"    const std::uint32_t aOtherMask = static_cast<std::uint32_t>({other_mask_stack_name}[aSourceIndex % kMaskStackDepth][aOtherMaskIndex]);",
    ]
    if spec["round_mix_mode"] == "xor_add":
        lines.append(
            f"    {next_round_mask_name}[aMaskIndex] ^= static_cast<unsigned char>((((a + c) ^ b) + aOtherMask) & 0xFFu);"
        )
    else:
        lines.append(
            f"    {next_round_mask_name}[aMaskIndex] = static_cast<unsigned char>({next_round_mask_name}[aMaskIndex] + static_cast<unsigned char>((((a ^ c) + b) ^ aOtherMask) & 0xFFu));"
        )
    lines.extend(["    ++aSourceIndex;", "  }", f"  RotateMaskStack({mask_stack_name}, {next_round_mask_name});"])
    return lines


def render_horrid_candidate_verbose(
    loop_a_spec: dict[str, Any],
    loop_b_spec: dict[str, Any],
    loop1_twiddle_spec: dict[str, Any],
    loop2_twiddle_spec: dict[str, Any],
    loop2_uses_twiddle_a: bool,
    mask_schedule_spec_a: dict[str, Any],
    mask_schedule_spec_b: dict[str, Any],
    lane_breaker_spec: dict[str, Any],
    braid_breaker_spec: dict[str, Any],
    jump_breaker_spec: dict[str, Any],
    swap_breaker_spec: dict[str, Any],
    mask_phase_spec: dict[str, Any],
    lightning_spec: dict[str, Any],
    typhoon_spec: dict[str, Any],
    hurricane_spec: dict[str, Any],
    final_spec: dict[str, Any],
) -> str:
    return "\n".join(
        [
            "selection=ferocious dual-worker twister",
            "  tsunami: custom_per_candidate",
            f"  workerA_loop: idx={loop_a_spec['data_index_count']} template={loop_a_spec['template']} offs={loop_a_spec['offsets']} reverse={loop_a_spec['reverse_modes']}",
            f"  workerB_loop: idx={loop_b_spec['data_index_count']} template={loop_b_spec['template']} offs={loop_b_spec['offsets']} reverse={loop_b_spec['reverse_modes']}",
            f"  twiddle_loop1: mode={loop1_twiddle_spec['mix_mode']} pos={loop1_twiddle_spec['position_mode']} rot={loop1_twiddle_spec['rotate']}",
            f"  twiddle_loop2: mode={loop2_twiddle_spec['mix_mode']} pos={loop2_twiddle_spec['position_mode']} rot={loop2_twiddle_spec['rotate']} shared={'aTwiddleA' if loop2_uses_twiddle_a else 'aTwiddleB'}",
            f"  lane_breaker: enabled={str(lane_breaker_spec['enabled']).lower()} source_off={lane_breaker_spec['source_offset']} mask_off={lane_breaker_spec['mask_flat_offset']} partner_bias={lane_breaker_spec['partner_bias']}",
            f"  braid_breaker: enabled={str(braid_breaker_spec['enabled']).lower()} source_off={braid_breaker_spec['source_offset']} partner_off={braid_breaker_spec['partner_offset']} chunk_span={braid_breaker_spec['chunk_span']}",
            f"  jump_breaker: enabled={str(jump_breaker_spec['enabled']).lower()} source_off={jump_breaker_spec['source_offset']} partner_off={jump_breaker_spec['partner_offset']} stride={jump_breaker_spec['jump_stride']} span={jump_breaker_spec['section_bytes']} mask_off={jump_breaker_spec['mask_flat_offset']}",
            f"  swap_breaker: enabled={str(swap_breaker_spec['enabled']).lower()} source_off={swap_breaker_spec['source_offset']} partner_off={swap_breaker_spec['partner_offset']} chunk_span={swap_breaker_spec['chunk_span']}",
            f"  mask_phase: chosen={mask_phase_spec['chosen_worker']} other={mask_phase_spec['other_worker']} offs={mask_phase_spec['offsets']} template={mask_phase_spec['template']}",
            f"  mask_seed_a: worker={mask_schedule_spec_a['worker_mode']} reverse={mask_schedule_spec_a['seed_reverse_modes']}/{mask_schedule_spec_a['second_reverse_modes']}",
            f"  mask_seed_b: worker={mask_schedule_spec_b['worker_mode']} reverse={mask_schedule_spec_b['seed_reverse_modes']}/{mask_schedule_spec_b['second_reverse_modes']}",
            f"  lightning: enabled={str(lightning_spec['enabled']).lower()} source={lightning_spec['source_buffer']} control={lightning_spec['control_buffer']} fast={list(lightning_spec['fast_pool'])} slow={list(lightning_spec['slow_pool'])} skip={lightning_spec['fast_skip_mod']}/{lightning_spec['slow_skip_mod']} mix_variant={lightning_spec['mix_variant']}",
            f"  typhoon: enabled={str(typhoon_spec['enabled']).lower()} source={typhoon_spec['source_buffer']} control={typhoon_spec['control_buffer']} fast={list(typhoon_spec['fast_pool'])} slow={list(typhoon_spec['slow_pool'])} skip={typhoon_spec['fast_skip_mod']}/{typhoon_spec['slow_skip_mod']}",
            f"  hurricane: enabled={str(hurricane_spec['enabled']).lower()} source={hurricane_spec['source_buffer']} control={hurricane_spec['control_buffer']} fast={list(hurricane_spec['fast_pool'])} slow={list(hurricane_spec['slow_pool'])} skip={hurricane_spec['fast_skip_mod']}/{hurricane_spec['slow_skip_mod']}",
            f"  final_weave: template={final_spec['template']} offs={final_spec['offsets']} reverse={final_spec['reverse_modes']} write={final_spec['write_mode']} key_row={final_spec['key_row']}",
        ]
    )


def build_matrix_candidate(rng: random.Random, candidate_id: int) -> CandidateSpec:
    worker_a_spec = choose_worker_loop_spec(rng, "workera")
    worker_b_spec = choose_worker_loop_spec(rng, "workerb")
    loop1_twiddle_spec = choose_twiddle_spec(rng, "loop1")
    loop2_twiddle_spec = choose_twiddle_spec(rng, "loop2")
    loop2_uses_twiddle_a = rng.random() < 0.35
    mask_schedule_spec_a = choose_mask_schedule_spec(rng)
    mask_schedule_spec_b = choose_mask_schedule_spec(rng)
    lane_breaker_spec = choose_lane_breaker_spec(rng)
    braid_breaker_spec = choose_braid_breaker_spec(rng)
    jump_breaker_spec = choose_jump_breaker_spec(rng)
    swap_breaker_spec = choose_swap_breaker_spec(rng)
    mask_phase_spec = choose_mask_phase_spec(rng)
    lightning_spec = choose_dual_matrix_pass_spec(rng, "lightning")
    lightning_spec["enabled"] = rng.random() < 0.5
    typhoon_spec = choose_dual_matrix_pass_spec(rng, "typhoon")
    typhoon_spec["enabled"] = rng.random() < 0.45
    hurricane_spec = choose_dual_matrix_pass_spec(rng, "hurricane")
    hurricane_spec["enabled"] = rng.random() < 0.5
    tsunami_spec = choose_custom_tsunami_spec(rng)
    final_whitening_spec = choose_custom_final_whitening_spec(rng)
    salt_mix_box_values = build_mix_box_values(rng, 128)
    final_spec = choose_final_weave_spec(rng)
    key_schedule_spec = choose_key_schedule_spec(rng)
    breaker_recipes = build_dual_worker_breaker_recipes(
        worker_a_spec,
        worker_b_spec,
        lane_breaker_spec,
        braid_breaker_spec,
        jump_breaker_spec,
        swap_breaker_spec,
        final_spec,
    )
    breaker_shape_summary = ",".join(
        f"{recipe_name}:{recipe['index_count']}m{recipe['matrix_mode']}i{recipe['inject_mode']}v{recipe['source_mix_variant']}{recipe['lane_mix_variant_a']}{recipe['lane_mix_variant_b']}"
        for recipe_name, recipe in breaker_recipes
    ) or "none"

    shape_key = json.dumps(
        {
            "worker_a": worker_a_spec,
            "worker_b": worker_b_spec,
            "twiddle_loop1": loop1_twiddle_spec,
            "twiddle_loop2": loop2_twiddle_spec,
            "loop2_uses_twiddle_a": loop2_uses_twiddle_a,
            "mask_schedule_a": mask_schedule_spec_a,
            "mask_schedule_b": mask_schedule_spec_b,
            "lane_breaker": lane_breaker_spec,
            "braid_breaker": braid_breaker_spec,
            "jump_breaker": jump_breaker_spec,
            "swap_breaker": swap_breaker_spec,
            "mask_phase": mask_phase_spec,
            "lightning": lightning_spec,
            "typhoon": typhoon_spec,
            "hurricane": hurricane_spec,
            "breaker_recipes": [{"name": recipe_name, **recipe} for recipe_name, recipe in breaker_recipes],
            "tsunami": {
                **tsunami_spec,
                "mix_box_signature": mix_box_signature(tsunami_spec["mix_box"]),
            },
            "final_whitening": {
                **final_whitening_spec,
                "mix_box_signature": mix_box_signature(final_whitening_spec["mix_box"]),
            },
            "salt_mix_box_signature": mix_box_signature(salt_mix_box_values),
            "final_weave": final_spec,
            "key_schedule": key_schedule_spec,
        },
        sort_keys=True,
        separators=(",", ":"),
    )
    phase1_op_count = (
        3
        + int(lane_breaker_spec["enabled"])
        + int(braid_breaker_spec["enabled"])
        + int(jump_breaker_spec["enabled"])
        + int(swap_breaker_spec["enabled"])
        + int(lightning_spec["enabled"])
    )
    phase2_op_count = 2 + int(typhoon_spec["enabled"]) + int(hurricane_spec["enabled"])
    subop_count = 0
    op_budget = (
        5
        + int(lane_breaker_spec["enabled"])
        + int(braid_breaker_spec["enabled"])
        + int(jump_breaker_spec["enabled"])
        + int(swap_breaker_spec["enabled"])
        + int(lightning_spec["enabled"])
        + int(typhoon_spec["enabled"])
        + int(hurricane_spec["enabled"])
    )
    multiply_count = 6 + int(lightning_spec["enabled"]) + int(typhoon_spec["enabled"]) + int(hurricane_spec["enabled"])

    function_name = f"TwistCandidate_{candidate_id:04d}"
    stable_file_name = f"twist_candidate_{candidate_id:04d}.cpp"
    recipe_summary = (
        f"ferocious[wa={worker_a_spec['data_index_count']}x{worker_a_spec['template']}; "
        f"wb={worker_b_spec['data_index_count']}x{worker_b_spec['template']}; "
        f"tw1={loop1_twiddle_spec['mix_mode']}/{loop1_twiddle_spec['position_mode']}; "
        f"tw2={loop2_twiddle_spec['mix_mode']}/{loop2_twiddle_spec['position_mode']}; "
        f"tw2src={'A' if loop2_uses_twiddle_a else 'B'}; "
        f"saltbox={mix_box_signature(salt_mix_box_values)}; "
        f"tsunami=custom; "
        f"final_whitening=custom; "
        f"breaker_shapes={breaker_shape_summary}; "
        f"wa_rev={worker_a_spec['reverse_modes']}; "
        f"wb_rev={worker_b_spec['reverse_modes']}; "
        f"lane={'on' if lane_breaker_spec['enabled'] else 'off'}; "
        f"braid={'on' if braid_breaker_spec['enabled'] else 'off'}; "
        f"jump={'on' if jump_breaker_spec['enabled'] else 'off'}; "
        f"swap={'on' if swap_breaker_spec['enabled'] else 'off'}; "
        f"mask={mask_phase_spec['chosen_worker']}x{mask_phase_spec['template']}; "
        f"lightning={'on' if lightning_spec['enabled'] else 'off'}:{lightning_spec['mix_variant']}; "
        f"typhoon={'on' if typhoon_spec['enabled'] else 'off'}; "
        f"hurricane={'on' if hurricane_spec['enabled'] else 'off'}; "
        f"final={final_spec['template']}/{final_spec['reverse_modes']}; "
        f"key_rot={key_schedule_spec['round_key_rotate']}; "
        f"maskA_seed={mask_schedule_spec_a['worker_mode']}; "
        f"maskB_seed={mask_schedule_spec_b['worker_mode']}; "
        f"maskA_bias={mask_schedule_spec_a['round_bias']}; "
        f"maskB_bias={mask_schedule_spec_b['round_bias']}]"
    )
    phase1 = PhaseSpec(
        offsets=worker_a_spec["offsets"],
        e_input="source",
        f_input="workerA",
        op1="workerA",
        op2="workerB",
        op3="mask",
        e_transform=TransformSpec("none", 0),
        f_transform=TransformSpec("none", 0),
    )
    phase2 = PhaseSpec(
        offsets=final_spec["offsets"][:3],
        e_input="dest",
        f_input="dest",
        op1="matrix",
        op2="weave",
        op3="dest",
        e_transform=TransformSpec("none", 0),
        f_transform=TransformSpec("none", 0),
    )

    function_lines = [
        f"// Candidate {candidate_id}: {function_name}",
        (
            f"// family=ferocious_dual_worker op_budget={op_budget} "
            f"worker_shapes={worker_a_spec['data_index_count']}x{worker_a_spec['template']}/"
            f"{worker_b_spec['data_index_count']}x{worker_b_spec['template']} "
            f"worker_reverse={worker_a_spec['reverse_modes']}/{worker_b_spec['reverse_modes']} "
            f"twiddle1={loop1_twiddle_spec['mix_mode']}/{loop1_twiddle_spec['position_mode']} "
            f"twiddle2={loop2_twiddle_spec['mix_mode']}/{loop2_twiddle_spec['position_mode']} "
            f"twiddle2_source={'A' if loop2_uses_twiddle_a else 'B'} "
            f"tsunami=custom "
            f"breaker_shapes={breaker_shape_summary} "
            f"lane_breaker={str(lane_breaker_spec['enabled']).lower()} "
            f"braid_breaker={str(braid_breaker_spec['enabled']).lower()} "
            f"jump_breaker={str(jump_breaker_spec['enabled']).lower()} "
            f"swap_breaker={str(swap_breaker_spec['enabled']).lower()} "
            f"mask_template={mask_phase_spec['template']} "
            f"mask_seed_a={mask_schedule_spec_a['worker_mode']} "
            f"mask_seed_b={mask_schedule_spec_b['worker_mode']} "
            f"lightning={str(lightning_spec['enabled']).lower()}/{lightning_spec['mix_variant']} "
            f"typhoon={str(typhoon_spec['enabled']).lower()} "
            f"hurricane={str(hurricane_spec['enabled']).lower()} "
            f"final_reverse={final_spec['reverse_modes']}"
        ),
        f"// {recipe_summary}",
        f"static void {function_name}_SaltSeed(",
        "    unsigned char* pSource,",
        "    unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned int pLength) {",
        "  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
    ]
    function_lines.extend(render_named_mix_box_lines(salt_mix_box_values, "aSaltMixBox"))
    function_lines.extend(render_salt_seed_lines(key_schedule_spec, "pSalt", "aSaltMixBox"))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_KeySeed(",
        "    unsigned char* pSource,",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned int pLength) {",
        "  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_mix_box_lines(KEY_SEED_MIX_BOX))
    function_lines.extend(render_key_stack_seed_lines(key_schedule_spec, "pKeyStack"))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_MaskSeedA(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorker,",
        "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
        "    unsigned int pLength) {",
        "  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_mix_box_lines(MASK_SEED_MIX_BOX))
    function_lines.extend(render_mask_stack_seed_lines(mask_schedule_spec_a, "pMaskStackA"))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_MaskSeedB(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorker,",
        "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
        "    unsigned int pLength) {",
        "  if (pSource == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_mix_box_lines(MASK_SEED_MIX_BOX))
    function_lines.extend(render_mask_stack_seed_lines(mask_schedule_spec_b, "pMaskStackB"))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(render_custom_tsunami_breaker_lines(function_name, tsunami_spec))
    function_lines.append("")
    function_lines.extend(render_custom_final_whitening_lines(function_name, final_whitening_spec))
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_TwistBlock(",
        "    unsigned char* pSource,",
        "    unsigned char* pWorkerA,",
        "    unsigned char* pWorkerB,",
        "    unsigned char* pDest,",
        "    unsigned int pRound,",
        "    const unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned char (&pMaskStackA)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pMaskStackB)[kMaskStackDepth][kMaskBytes],",
        "    unsigned int pLength) {",
        "  if (pSource == nullptr || pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_twiddle_init_lines(key_schedule_spec, "aTwiddleA", 0, 0xA511E9B3, 0))
    function_lines.extend(render_twiddle_init_lines(key_schedule_spec, "aTwiddleB", 1, 0xB44B1D93, 11))
    function_lines.extend(
        [
        ]
    )
    function_lines.extend(render_worker_loop_lines(worker_a_spec, loop1_twiddle_spec, "aTwiddleA"))
    function_lines.append("")
    function_lines.extend(
        render_worker_loop_lines(
            worker_b_spec,
            loop2_twiddle_spec,
            "aTwiddleA" if loop2_uses_twiddle_a else "aTwiddleB",
        )
    )
    function_lines.append("")
    function_lines.append(f"  {function_name}_TsunamiBreaker(pSource, pWorkerA, pWorkerB, pSalt, pRound, aTwiddleA, aTwiddleB);")
    function_lines.append("")
    for recipe_name, recipe in breaker_recipes:
        function_lines.extend(render_dual_worker_matrix_breaker_lines(recipe_name, recipe))
        function_lines.append("")
    function_lines.extend(render_mask_phase_lines(mask_phase_spec))
    function_lines.append("")
    if lightning_spec["enabled"]:
        function_lines.extend(render_lightning_pass_lines(lightning_spec))
        function_lines.append("")
    if typhoon_spec["enabled"]:
        function_lines.extend(render_typhoon_pass_lines(typhoon_spec))
        function_lines.append("")
    if hurricane_spec["enabled"]:
        function_lines.extend(render_hurricane_pass_lines(hurricane_spec))
        function_lines.append("")
    function_lines.extend(render_final_weave_lines(final_spec))
    function_lines.append(f"  {function_name}_FinalWhitening(pSource, pDest, pSalt, pRound, aTwiddleA, aTwiddleB);")
    function_lines.append("")
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_PushKeyRound(",
        "    unsigned char* pDest,",
        "    const unsigned char (&pSalt)[kSaltBytes],",
        "    unsigned char (&pKeyStack)[kRoundKeyStackDepth][kRoundKeyBytes],",
        "    unsigned char (&pNextRoundKeyBuffer)[kRoundKeyBytes],",
        "    unsigned int pLength) {",
        "  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_round_key_update_lines(key_schedule_spec))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_PushMaskRoundA(",
        "    unsigned char* pDest,",
        "    unsigned char (&pMaskStackSelf)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pMaskStackOther)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],",
        "    unsigned int pLength) {",
        "  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_round_mask_update_lines(mask_schedule_spec_a, "pMaskStackSelf", "pMaskStackOther", "pNextRoundMaskBuffer"))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"static void {function_name}_PushMaskRoundB(",
        "    unsigned char* pDest,",
        "    unsigned char (&pMaskStackSelf)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pMaskStackOther)[kMaskStackDepth][kMaskBytes],",
        "    unsigned char (&pNextRoundMaskBuffer)[kMaskBytes],",
        "    unsigned int pLength) {",
        "  if (pDest == nullptr || pLength < PASSWORD_EXPANDED_SIZE) {",
        "    return;",
        "  }",
        ]
    )
    function_lines.extend(render_round_mask_update_lines(mask_schedule_spec_b, "pMaskStackSelf", "pMaskStackOther", "pNextRoundMaskBuffer"))
    function_lines.append("}")
    function_lines.append("")
    function_lines.extend(
        [
        f"void {function_name}(",
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
        "    unsigned int pLength) {",
        "  if (pLength == 0U) {",
        "    return;",
        "  }",
        "  if ((pLength % PASSWORD_EXPANDED_SIZE) != 0U) {",
        "    return;",
        "  }",
        "  unsigned char aSalt[kSaltBytes]{};",
        f"  {function_name}_SaltSeed(pSource, aSalt, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        f"  {function_name}_KeySeed(pSource, pKeyStack, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        f"  {function_name}_MaskSeedA(pSource, pWorkerA, pMaskStackA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        f"  {function_name}_MaskSeedB(pSource, pWorkerB, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        "  unsigned int aRound = 0U;",
        "  for (unsigned int offset = 0U; offset < pLength; offset += PASSWORD_EXPANDED_SIZE, ++aRound) {",
        "    unsigned char* aRoundSource = (aRound == 0U)",
        "        ? pSource",
        "        : (pDest + offset - PASSWORD_EXPANDED_SIZE);",
        "    unsigned char* aRoundDest = pDest + offset;",
        f"    {function_name}_TwistBlock(aRoundSource, pWorkerA, pWorkerB, aRoundDest, aRound, aSalt, pKeyStack, pMaskStackA, pMaskStackB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        f"    {function_name}_PushKeyRound(aRoundDest, aSalt, pKeyStack, pNextRoundKeyBuffer, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        f"    {function_name}_PushMaskRoundA(aRoundDest, pMaskStackA, pMaskStackB, pNextRoundMaskBufferA, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        f"    {function_name}_PushMaskRoundB(aRoundDest, pMaskStackB, pMaskStackA, pNextRoundMaskBufferB, static_cast<unsigned int>(PASSWORD_EXPANDED_SIZE));",
        "  }",
        "}",
        ]
    )
    function_source = "\n".join(function_lines) + "\n"

    return CandidateSpec(
        candidate_id=candidate_id,
        function_name=function_name,
        stable_file_name=stable_file_name,
        op_budget=op_budget,
        phase1_op_count=phase1_op_count,
        phase2_op_count=phase2_op_count,
        multiply_count=multiply_count,
        subop_count=subop_count,
        phase1=phase1,
        phase2=phase2,
        recipe_summary=recipe_summary,
        function_source=function_source,
        family="ferocious_dual_worker",
        verbose_text=(
            render_horrid_candidate_verbose(
                worker_a_spec,
                worker_b_spec,
                loop1_twiddle_spec,
                loop2_twiddle_spec,
                loop2_uses_twiddle_a,
                mask_schedule_spec_a,
                mask_schedule_spec_b,
                lane_breaker_spec,
                braid_breaker_spec,
                jump_breaker_spec,
                swap_breaker_spec,
                mask_phase_spec,
                lightning_spec,
                typhoon_spec,
                hurricane_spec,
                final_spec,
            )
            + f"\n  breaker_shapes: {breaker_shape_summary}"
            + f"\n  salt_mix_box: {mix_box_signature(salt_mix_box_values)}"
            + f"\n  tsunami_mix_box: {mix_box_signature(tsunami_spec['mix_box'])}"
            + f"\n  final_mix_box: {mix_box_signature(final_whitening_spec['mix_box'])}"
            + "\n  salt_sbox: enabled=true"
            + "\n  tsunami: custom_per_candidate"
            + "\n  final_whitening: custom_per_candidate"
            + "\n  lightning_mix_columns: enabled=true"
            + "\n  mask_seed_mode: roaming_bitstream"
        ),
        shape_key=shape_key,
        matrix_enabled=lightning_spec["enabled"] or typhoon_spec["enabled"] or hurricane_spec["enabled"],
        matrix_breaker_summary=(
            f"lightning={'on' if lightning_spec['enabled'] else 'off'};"
            f"typhoon={'on' if typhoon_spec['enabled'] else 'off'};"
            f"hurricane={'on' if hurricane_spec['enabled'] else 'off'}"
        ),
    )


def render_phase_initializer(phase: PhaseSpec) -> str:
    return (
        "{"
        f"{{{phase.offsets[0]}, {phase.offsets[1]}, {phase.offsets[2]}}}, "
        f"\"{phase.op1}\", "
        f"\"{phase.op2}\", "
        f"\"{phase.op3}\", "
        f"\"{phase.e_input}\", "
        f"\"{phase.f_input}\", "
        f"\"{phase.e_transform.kind}\", "
        f"{phase.e_transform.arg}, "
        f"\"{phase.f_transform.kind}\", "
        f"{phase.f_transform.arg}"
        "}"
    )


def render_generated_cpp(candidates: list[CandidateSpec], seed: int, count: int) -> str:
    functions = "\n\n".join(candidate.function_source.rstrip() for candidate in candidates)
    registry_entries = []
    for candidate in candidates:
        registry_entries.append(
            "  {\n"
            f"    {candidate.candidate_id},\n"
            f"    \"{candidate.function_name}\",\n"
            f"    {candidate.op_budget},\n"
            f"    {candidate.multiply_count},\n"
            f"    {candidate.subop_count},\n"
            f"    {render_phase_initializer(candidate.phase1)},\n"
            f"    {render_phase_initializer(candidate.phase2)},\n"
            f"    {json.dumps(candidate.recipe_summary)},\n"
            f"    &{candidate.function_name},\n"
            f"    &{candidate.function_name}_KeySeed,\n"
            f"    &{candidate.function_name}_SaltSeed,\n"
            f"    &{candidate.function_name}_MaskSeedA,\n"
            f"    &{candidate.function_name}_MaskSeedB,\n"
            f"    &{candidate.function_name}_TwistBlock,\n"
            f"    &{candidate.function_name}_PushKeyRound,\n"
            f"    &{candidate.function_name}_PushMaskRoundA,\n"
            f"    &{candidate.function_name}_PushMaskRoundB,\n"
            "  }"
        )
    registry_text = ",\n".join(registry_entries)
    return f"""#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TwistBreakers.hpp"
#include "TyphoonMatrix.hpp"
#include "TwistTypes.hpp"

namespace twist {{

// Generated by tools/generate_twist_candidates.py
// seed={seed} candidate_count={count}

{functions}

const RegisteredCandidate kRegisteredCandidates[] = {{
{registry_text}
}};

const std::size_t kRegisteredCandidateCount =
    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}}  // namespace twist
"""


def render_generated_verbose_cpp(candidates: list[CandidateSpec], seed: int, count: int) -> str:
    functions = "\n\n".join(candidate.function_source.rstrip() for candidate in candidates)
    registry_entries = []
    for candidate in candidates:
        registry_entries.append(
            "  {\n"
            f"    {candidate.candidate_id},\n"
            f"    \"{candidate.function_name}\",\n"
            f"    {candidate.op_budget},\n"
            f"    {candidate.multiply_count},\n"
            f"    {candidate.subop_count},\n"
            f"    {render_phase_initializer(candidate.phase1)},\n"
            f"    {render_phase_initializer(candidate.phase2)},\n"
            f"    {json.dumps(candidate.recipe_summary)},\n"
            f"    &{candidate.function_name},\n"
            f"    &{candidate.function_name}_KeySeed,\n"
            f"    &{candidate.function_name}_SaltSeed,\n"
            f"    &{candidate.function_name}_MaskSeedA,\n"
            f"    &{candidate.function_name}_MaskSeedB,\n"
            f"    &{candidate.function_name}_TwistBlock,\n"
            f"    &{candidate.function_name}_PushKeyRound,\n"
            f"    &{candidate.function_name}_PushMaskRoundA,\n"
            f"    &{candidate.function_name}_PushMaskRoundB,\n"
            "  }"
        )
    registry_text = ",\n".join(registry_entries)
    return f"""#include "HurricaneMatrix.hpp"
#include "LightningMatrix.hpp"
#include "TwistBreakers.hpp"
#include "TyphoonMatrix.hpp"
#include "TwistTypes.hpp"

namespace twist {{

// Generated by tools/generate_twist_candidates.py
// seed={seed} candidate_count={count}
// This verbose file is the source of truth used by the harness build.

{functions}

const RegisteredCandidate kRegisteredCandidates[] = {{
{registry_text}
}};

const std::size_t kRegisteredCandidateCount =
    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);

}}  // namespace twist
"""


def render_phase_text(
    phase: PhaseSpec,
    phase_number: int,
    a_array: str,
    lookup_array: str,
    output_array: str,
) -> str:
    offset1, offset2, offset3 = phase.offsets
    e_rhs = transform_summary(phase.e_input, phase.e_transform)
    f_rhs = transform_summary(phase.f_input, phase.f_transform)
    return f"""Phase {phase_number}
for (i = 0; i < PASSWORD_EXPANDED_SIZE; i += kMatrixBlockBytes) {{
  idx = i;
  a_index = idx;
  b_index = (idx + {offset1}) % PASSWORD_EXPANDED_SIZE;
  c_index = (idx + {offset2}) % PASSWORD_EXPANDED_SIZE;
  d_index = (idx + {offset3}) % PASSWORD_EXPANDED_SIZE;
  a = load16({a_array}, a_index);
  b = load16({lookup_array}, b_index);
  c = load16({lookup_array}, c_index);
  d = load16({lookup_array}, d_index);
  e_input = {e_rhs};
  f_input = {f_rhs};
  e = {phase.op1}(a, e_input);
  f = {phase.op2}(d, f_input);
  out = {phase.op3}(e, f);
  store16({output_array}, idx, out);
}}"""


def render_verbose_text(candidates: list[CandidateSpec], seed: int, count: int) -> str:
    pieces = [
        "Twist Candidate Verbose Listing",
        f"seed={seed}",
        f"candidate_count={count}",
        "Each candidate uses three fixed mechanical loops across PASSWORD_EXPANDED_SIZE bytes.",
        "",
    ]
    for candidate in candidates:
        candidate_details = candidate.verbose_text or "\n".join(
            [
                render_phase_text(candidate.phase1, 1, "source", "source", "worker"),
                "",
                render_phase_text(candidate.phase2, 2, "source", "worker", "dest"),
            ]
        )
        pieces.extend(
            [
                f"Candidate {candidate.candidate_id}: {candidate.function_name}",
                (
                    f"family={candidate.family} op_budget={candidate.op_budget} "
                    f"phase1_ops={candidate.phase1_op_count} phase2_ops={candidate.phase2_op_count} "
                    f"multiply_count={candidate.multiply_count} subop_count={candidate.subop_count}"
                ),
                candidate.recipe_summary,
                candidate_details,
                "",
                "-" * 72,
                "",
            ]
        )
    return "\n".join(pieces).rstrip() + "\n"


def generate_candidates(
    count: int,
    seed: int,
    phase1_min_ops: int,
    phase1_max_ops: int,
    phase2_min_ops: int,
    phase2_max_ops: int,
    max_transforms_total: int,
) -> tuple[list[CandidateSpec], bool]:
    rng = random.Random(seed)
    candidates: list[CandidateSpec] = []
    seen_shapes: set[str] = set()
    attempts = 0
    stagnant_attempts = 0
    max_attempts = max(5000, count * 100)
    max_stagnant_attempts = max(5000, count * 20)
    stopped_early = False
    while len(candidates) < count:
        attempts += 1
        stagnant_attempts += 1
        if attempts > max_attempts or stagnant_attempts > max_stagnant_attempts:
            stopped_early = True
            break
        candidate = build_matrix_candidate(
            rng=rng,
            candidate_id=len(candidates) + 1,
        )
        if candidate.shape_key in seen_shapes:
            continue
        seen_shapes.add(candidate.shape_key)
        candidates.append(candidate)
        stagnant_attempts = 0
    return candidates, stopped_early


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def write_generated_cpp_file(
    path: Path,
    candidates: list[CandidateSpec],
    seed: int,
    count: int,
    *,
    verbose: bool,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write('#include "HurricaneMatrix.hpp"\n')
        handle.write('#include "LightningMatrix.hpp"\n')
        handle.write('#include "TwistBreakers.hpp"\n')
        handle.write('#include "TyphoonMatrix.hpp"\n')
        handle.write('#include "TwistTypes.hpp"\n\n')
        handle.write("namespace twist {\n\n")
        handle.write("// Generated by tools/generate_twist_candidates.py\n")
        handle.write(f"// seed={seed} candidate_count={count}\n")
        if verbose:
            handle.write("// This verbose file is the source of truth used by the harness build.\n")
        handle.write("\n")

        for index, candidate in enumerate(candidates):
            if index > 0:
                handle.write("\n\n")
            handle.write(candidate.function_source.rstrip())
        handle.write("\n\n")

        handle.write("const RegisteredCandidate kRegisteredCandidates[] = {\n")
        for index, candidate in enumerate(candidates):
            if index > 0:
                handle.write(",\n")
            handle.write("  {\n")
            handle.write(f"    {candidate.candidate_id},\n")
            handle.write(f"    \"{candidate.function_name}\",\n")
            handle.write(f"    {candidate.op_budget},\n")
            handle.write(f"    {candidate.multiply_count},\n")
            handle.write(f"    {candidate.subop_count},\n")
            handle.write(f"    {render_phase_initializer(candidate.phase1)},\n")
            handle.write(f"    {render_phase_initializer(candidate.phase2)},\n")
            handle.write(f"    {json.dumps(candidate.recipe_summary)},\n")
            handle.write(f"    &{candidate.function_name},\n")
            handle.write(f"    &{candidate.function_name}_KeySeed,\n")
            handle.write(f"    &{candidate.function_name}_SaltSeed,\n")
            handle.write(f"    &{candidate.function_name}_MaskSeedA,\n")
            handle.write(f"    &{candidate.function_name}_MaskSeedB,\n")
            handle.write(f"    &{candidate.function_name}_TwistBlock,\n")
            handle.write(f"    &{candidate.function_name}_PushKeyRound,\n")
            handle.write(f"    &{candidate.function_name}_PushMaskRoundA,\n")
            handle.write(f"    &{candidate.function_name}_PushMaskRoundB,\n")
            handle.write("  }")
        handle.write("\n};\n\n")
        handle.write("const std::size_t kRegisteredCandidateCount =\n")
        handle.write("    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);\n\n")
        handle.write("}  // namespace twist\n")


def write_verbose_text_file(path: Path, candidates: list[CandidateSpec], seed: int, count: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write("# Twist candidate verbose report\n")
        handle.write(f"seed={seed}\n")
        handle.write(f"candidate_count={count}\n\n")
        for index, candidate in enumerate(candidates):
            candidate_details = candidate.verbose_text or "\n".join(
                [
                    f"phase1_ops={candidate.phase1_op_count}",
                    f"phase2_ops={candidate.phase2_op_count}",
                    f"matrix={candidate.matrix_breaker_summary or 'n/a'}",
                ]
            )
            handle.write(f"Candidate {candidate.candidate_id}: {candidate.function_name}\n")
            handle.write(f"{candidate.recipe_summary}\n")
            handle.write(f"{candidate_details}\n")
            if index + 1 < len(candidates):
                handle.write("\n")
                handle.write("-" * 72)
                handle.write("\n\n")
            else:
                handle.write("\n")


def write_manifest_file(
    path: Path,
    candidates: list[CandidateSpec],
    seed: int,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write("{\n")
        manifest_header = [
            ("seed", seed),
            ("candidate_count", len(candidates) + len(BASELINE_CANDIDATES)),
            ("generated_candidate_count", len(candidates)),
            ("baseline_candidate_count", len(BASELINE_CANDIDATES)),
            ("password_expanded_size", PASSWORD_EXPANDED_SIZE),
            ("alignment", ALIGNMENT),
            ("knobs_path", str(KNOBS_PATH)),
            ("phase1_min_ops", DEFAULT_PHASE1_MIN_OPS),
            ("phase1_max_ops", DEFAULT_PHASE1_MAX_OPS),
            ("phase2_min_ops", DEFAULT_PHASE2_MIN_OPS),
            ("phase2_max_ops", DEFAULT_PHASE2_MAX_OPS),
            ("max_transforms_total", DEFAULT_MAX_TRANSFORMS_TOTAL),
        ]
        for key, value in manifest_header:
            handle.write(f"  {json.dumps(key)}: {json.dumps(value)},\n")
        handle.write('  "candidates": [\n')
        manifest_candidates = [candidate_to_manifest(candidate) for candidate in candidates] + list(BASELINE_CANDIDATES)
        for index, item in enumerate(manifest_candidates):
            prefix = "    " + json.dumps(item, indent=2).replace("\n", "\n    ")
            handle.write(prefix)
            if index + 1 < len(manifest_candidates):
                handle.write(",")
            handle.write("\n")
        handle.write("  ]\n")
        handle.write("}\n")


def load_scores(path: Path) -> list[dict[str, Any]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = []
        for row in reader:
            row["candidate_id"] = int(row["candidate_id"])
            row["composite_score"] = float(row["composite_score"])
            row["grade_rank"] = int(row.get("grade_rank", 0))
            rows.append(row)
    return rows


def render_top_cpp(
    candidates: list[CandidateSpec],
    score_rows: list[dict[str, Any]],
    top_n: int,
    seed: int,
) -> str:
    score_by_id = {row["candidate_id"]: row for row in score_rows}
    candidate_by_id = {candidate.candidate_id: candidate for candidate in candidates}
    selected: list[CandidateSpec] = []
    for row in score_rows:
        candidate = candidate_by_id.get(row["candidate_id"])
        if candidate is not None:
            selected.append(candidate)
        if len(selected) >= top_n:
            break

    pieces = [
        "#include \"HurricaneMatrix.hpp\"",
        "#include \"LightningMatrix.hpp\"",
        "#include \"TyphoonMatrix.hpp\"",
        "#include \"TwistTypes.hpp\"",
        "",
        "namespace twist {",
        "",
        f"// Top candidates exported from manifest seed={seed}",
    ]
    for candidate in selected:
        row = score_by_id[candidate.candidate_id]
        pieces.append(
            f"// grade={row.get('grade', 'NA')} composite={float(row['composite_score']):.3f} "
            f"candidate_id={candidate.candidate_id}"
        )
        pieces.append(candidate.function_source.rstrip())
        pieces.append("")
    pieces.append("}  // namespace twist")
    pieces.append("")
    return "\n".join(pieces)


def command_generate(args: argparse.Namespace) -> int:
    output_dir = Path(args.output_dir)
    manifest_path = output_dir / "twist_candidates_manifest.json"
    cpp_path = output_dir / "twist_candidates_generated.cpp"
    verbose_cpp_path = output_dir / "twist_candidates_generated_verbose.cpp"
    verbose_txt_path = output_dir / "twist_candidates_generated_verbose.txt"
    seed = args.seed if args.seed is not None else (
        DEFAULT_RANDOM_SEED if DEFAULT_RANDOM_SEED != 0
        else (secrets.randbits(64) if DEFAULT_RANDOMIZE_SEED else 1337)
    )

    temp_dir = output_dir / ".generate_tmp"
    if temp_dir.exists():
        shutil.rmtree(temp_dir)
    temp_dir.mkdir(parents=True, exist_ok=True)

    functions_temp_path = temp_dir / "functions.cpp.txt"
    registry_temp_path = temp_dir / "registry.txt"
    verbose_temp_path = temp_dir / "verbose.txt"
    manifest_items_temp_path = temp_dir / "manifest_items.jsonl"

    rng = random.Random(seed)
    seen_shapes: set[str] = set()
    attempts = 0
    stagnant_attempts = 0
    max_attempts = max(5000, args.count * 100)
    max_stagnant_attempts = max(5000, args.count * 20)
    stopped_early = False
    generated_count = 0

    with (
        functions_temp_path.open("w", encoding="utf-8") as functions_handle,
        registry_temp_path.open("w", encoding="utf-8") as registry_handle,
        verbose_temp_path.open("w", encoding="utf-8") as verbose_handle,
        manifest_items_temp_path.open("w", encoding="utf-8") as manifest_handle,
    ):
        while generated_count < args.count:
            attempts += 1
            stagnant_attempts += 1
            if attempts > max_attempts or stagnant_attempts > max_stagnant_attempts:
                stopped_early = True
                break

            candidate = build_matrix_candidate(
                rng=rng,
                candidate_id=generated_count + 1,
            )
            if candidate.shape_key in seen_shapes:
                continue

            seen_shapes.add(candidate.shape_key)
            stagnant_attempts = 0

            if generated_count > 0:
                functions_handle.write("\n\n")
                registry_handle.write(",\n")
                verbose_handle.write("\n")
                verbose_handle.write("-" * 72)
                verbose_handle.write("\n\n")
                manifest_handle.write(",\n")

            functions_handle.write(candidate.function_source.rstrip())
            registry_handle.write(render_registry_entry_from_manifest(candidate_to_manifest(candidate)))

            candidate_details = candidate.verbose_text or "\n".join(
                [
                    f"phase1_ops={candidate.phase1_op_count}",
                    f"phase2_ops={candidate.phase2_op_count}",
                    f"matrix={candidate.matrix_breaker_summary or 'n/a'}",
                ]
            )
            verbose_handle.write(f"Candidate {candidate.candidate_id}: {candidate.function_name}\n")
            verbose_handle.write(f"{candidate.recipe_summary}\n")
            verbose_handle.write(f"{candidate_details}\n")

            manifest_handle.write("    ")
            manifest_handle.write(
                json.dumps(candidate_to_manifest(candidate), indent=2).replace("\n", "\n    ")
            )

            generated_count += 1

    write_generated_cpp_from_parts(
        cpp_path,
        functions_temp_path,
        registry_temp_path,
        seed,
        generated_count,
        verbose=False,
    )
    write_generated_cpp_from_parts(
        verbose_cpp_path,
        functions_temp_path,
        registry_temp_path,
        seed,
        generated_count,
        verbose=True,
    )
    write_verbose_text_from_parts(verbose_txt_path, verbose_temp_path, seed, generated_count)
    write_manifest_from_parts(manifest_path, manifest_items_temp_path, generated_count, seed)
    shutil.rmtree(temp_dir)

    print(f"generated {generated_count} candidates with seed={seed}")
    if stopped_early and generated_count < args.count:
        print("stopped early after exhausting new unique shapes under current generator rules")
    print(f"baselines: {len(BASELINE_CANDIDATES)}")
    print(f"total_manifest_candidates: {generated_count + len(BASELINE_CANDIDATES)}")
    print(f"manifest: {manifest_path}")
    print(f"cpp: {cpp_path}")
    print(f"verbose_cpp: {verbose_cpp_path}")
    print(f"verbose_txt: {verbose_txt_path}")
    return 0


def command_export_top(args: argparse.Namespace) -> int:
    from export_top_from_shards import extract_functions, shard_for_candidate

    manifest_path = Path(args.manifest)
    scores_path = Path(args.scores)
    output_path = Path(args.output)

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest_candidates = {
        int(item["candidate_id"]): item
        for item in manifest["candidates"]
        if bool(item.get("exportable", True))
    }
    score_rows = sorted(
        load_scores(scores_path),
        key=lambda row: (-row["grade_rank"], -row["composite_score"], row["candidate_id"]),
    )
    selected_ids: list[int] = []
    for row in score_rows:
        candidate_id = int(row["candidate_id"])
        if candidate_id in manifest_candidates:
            selected_ids.append(candidate_id)
        if len(selected_ids) >= args.top:
            break

    function_sources: dict[int, str] = {}
    ids_needing_source = [
        candidate_id
        for candidate_id in selected_ids
        if "function_source" not in manifest_candidates[candidate_id]
    ]
    if ids_needing_source:
        index_path = Path(args.index) if args.index else manifest_path.with_name("shards_index.json")
        index = json.loads(index_path.read_text(encoding="utf-8"))
        shard_to_ids: dict[Path, set[int]] = {}
        for candidate_id in ids_needing_source:
            shard_path = shard_for_candidate(index, candidate_id)
            shard_to_ids.setdefault(Path(shard_path), set()).add(candidate_id)
        for shard_path, wanted_ids in shard_to_ids.items():
            function_sources.update(extract_functions(Path(shard_path), wanted_ids))

    candidates = []
    for candidate_id in selected_ids:
        item = manifest_candidates[candidate_id]
        function_source = item.get("function_source", function_sources.get(candidate_id, ""))
        candidates.append(
            CandidateSpec(
                candidate_id=item["candidate_id"],
                function_name=item["function_name"],
                stable_file_name=item["stable_file_name"],
                op_budget=item["op_budget"],
                phase1_op_count=item.get("phase1_op_count", 3),
                phase2_op_count=item.get("phase2_op_count", 3),
                multiply_count=item["multiply_count"],
                subop_count=item["subop_count"],
                phase1=PhaseSpec(
                    offsets=tuple(item["phase1"]["offsets"]),
                    e_input=item["phase1"]["e_input"],
                    f_input=item["phase1"]["f_input"],
                    op1=item["phase1"]["op1"],
                    op2=item["phase1"]["op2"],
                    op3=item["phase1"]["op3"],
                    e_transform=TransformSpec(**item["phase1"]["e_transform"]),
                    f_transform=TransformSpec(**item["phase1"]["f_transform"]),
                ),
                phase2=PhaseSpec(
                    offsets=tuple(item["phase2"]["offsets"]),
                    e_input=item["phase2"]["e_input"],
                    f_input=item["phase2"]["f_input"],
                    op1=item["phase2"]["op1"],
                    op2=item["phase2"]["op2"],
                    op3=item["phase2"]["op3"],
                    e_transform=TransformSpec(**item["phase2"]["e_transform"]),
                    f_transform=TransformSpec(**item["phase2"]["f_transform"]),
                ),
                recipe_summary=item["recipe_summary"],
                function_source=function_source,
                family=item.get("family", "classic"),
                verbose_text=item.get("verbose_text", ""),
                shape_key=item.get("shape_key", ""),
            )
        )
    top_cpp = render_top_cpp(candidates, score_rows, args.top, manifest["seed"])
    write_text(output_path, top_cpp)
    print(f"exported top {args.top} candidates to {output_path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate byte-twister candidates and export top-ranked functions."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_parser = subparsers.add_parser("generate", help="generate candidate manifest and C++")
    generate_parser.add_argument("--count", type=int, default=DEFAULT_COUNT)
    generate_parser.add_argument("--seed", type=int)
    generate_parser.add_argument("--output-dir", default="generated")
    generate_parser.set_defaults(func=command_generate)

    export_parser = subparsers.add_parser("export-top", help="emit top-ranked candidate functions")
    export_parser.add_argument("--manifest", default="generated/twist_candidates_manifest.json")
    export_parser.add_argument("--scores", default="generated/twist_candidate_scores.csv")
    export_parser.add_argument("--output", default="generated/top_twist_candidates.cpp")
    export_parser.add_argument("--index", default="")
    export_parser.add_argument("--top", type=int, default=DEFAULT_TOP_N)
    export_parser.set_defaults(func=command_export_top)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
