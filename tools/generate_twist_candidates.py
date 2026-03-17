#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import random
import re
import secrets
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

PASSWORD_EXPANDED_SIZE = 7680
ALIGNMENT = 16
REPO_ROOT = Path(__file__).resolve().parents[1]
KNOBS_PATH = REPO_ROOT / "src" / "Knobs.hpp"


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


def choose_matrix_phase_plan(rng: random.Random, matrix_count: int) -> MatrixPhasePlan:
    used = set()
    control_offset = choose_aligned_offset(rng, used)
    used.add(control_offset)
    feedback_offset = choose_aligned_offset(rng, used)
    fast_op_pool = LEAN_MATRIX_FAST_OPS if matrix_count == 2 else MATRIX_FAST_OPS
    slow_op_pool = LEAN_MATRIX_SLOW_OPS if matrix_count == 2 else LEAN_MATRIX_SLOW_OPS
    return MatrixPhasePlan(
        source_offsets=choose_matrix_source_offsets(rng, matrix_count),
        control_offset=control_offset,
        feedback_offset=feedback_offset,
        fast_ops=tuple(
            tuple(rng.choice(fast_op_pool) for _ in range(rng.randint(2, 4 if matrix_count == 2 else 5)))
            for _ in range(matrix_count)
        ),
        slow_ops=tuple(
            tuple(rng.choice(slow_op_pool) for _ in range(rng.randint(0, 1 if matrix_count == 2 else 1)))
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
    matrix_count = 2
    return MatrixRecipe(
        matrix_count=matrix_count,
        phase1=choose_matrix_phase_plan(rng, matrix_count),
        phase2=choose_matrix_phase_plan(rng, matrix_count),
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
        f"carry={carry_text}; bridge={phase.bridge_mode}; salt={phase.salt_bias}+n*{phase.salt_stride}]"
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


def candidate_to_manifest(candidate: CandidateSpec) -> dict[str, Any]:
    return {
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
        "is_baseline": False,
        "exportable": True,
    }


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
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; i += kNeonWidth) {{
    const std::size_t idx = i;
    const std::size_t width = std::min<std::size_t>(kNeonWidth, PASSWORD_EXPANDED_SIZE - idx);
    const ByteVec a = LoadVecWrapped({a_array}, idx);
    const ByteVec b = LoadVecWrapped({lookup_array}, (idx + {offset1}u) % PASSWORD_EXPANDED_SIZE);
    const ByteVec c = LoadVecWrapped({lookup_array}, (idx + {offset2}u) % PASSWORD_EXPANDED_SIZE);
    const ByteVec d = LoadVecWrapped({lookup_array}, (idx + {offset3}u) % PASSWORD_EXPANDED_SIZE);
    const ByteVec e = {e_expr};
    const ByteVec f = {f_expr};
    const ByteVec out = {out_expr};
    if (width == kNeonWidth) {{
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
  for (std::size_t i = 0; i < PASSWORD_EXPANDED_SIZE; i += kNeonWidth) {{
    const std::size_t idx = i;
    const std::size_t width = std::min<std::size_t>(kNeonWidth, PASSWORD_EXPANDED_SIZE - idx);

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

    if (width == kNeonWidth) {{
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
        elif bridge_mode == "add_chain":
            lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
            lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_carry_0);")
        else:
            lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_carry_1);")
            lines.append(f"    {phase_prefix}_matrix_1.AddWith({phase_prefix}_carry_0);")
            lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_0);")
        return lines

    if bridge_mode == "xor_chain":
        lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_0);")
        lines.append(f"    {phase_prefix}_matrix_2.AddWith({phase_prefix}_matrix_1);")
        lines.append(f"    {phase_prefix}_matrix_0.XorWith({phase_prefix}_matrix_2);")
    elif bridge_mode == "add_chain":
        lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
        lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_matrix_2);")
        lines.append(f"    {phase_prefix}_matrix_2.AddWith({phase_prefix}_carry_0);")
    else:
        lines.append(f"    {phase_prefix}_matrix_2.XorWith({phase_prefix}_matrix_0);")
        lines.append(f"    {phase_prefix}_matrix_0.AddWith({phase_prefix}_matrix_1);")
        lines.append(f"    {phase_prefix}_matrix_1.XorWith({phase_prefix}_carry_2);")
        lines.append(f"    {phase_prefix}_matrix_2.AddWith({phase_prefix}_carry_1);")
    return lines


def render_matrix_seed_lines(seed_blocks: tuple[tuple[int, ...], ...], prefix: str) -> list[str]:
    lines: list[str] = []
    for index, seed_block in enumerate(seed_blocks):
        lines.append(
            f"  static constexpr std::array<std::uint8_t, kNeonWidth> {prefix}_seed_{index} = "
            f"{{{{{format_seed_block(seed_block)}}}}};"
        )
    for index in range(len(seed_blocks)):
        lines.append(f"  LightningMatrix {prefix}_carry_{index}({prefix}_seed_{index}.data());")
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
    lines.append("    std::array<std::uint8_t, kNeonWidth> phase1_salt{};")
    lines.append(
        "    const std::uint8_t phase1_fold = static_cast<std::uint8_t>("
        + " + ".join(fold_terms)
        + ");"
    )
    lines.append("    for (std::size_t lane = 0; lane < kNeonWidth; ++lane) {")
    lines.append(
        "      phase1_salt[lane] = static_cast<std::uint8_t>("
        f"phase1_fold + {phase.salt_bias}u + "
        f"static_cast<std::uint8_t>(lane * {phase.salt_stride}u) + "
        f"static_cast<std::uint8_t>(((chunk / 16u) + lane) * {phase.salt_stride + recipe.matrix_count}u));"
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

        for slow_index, op_name in enumerate(phase.slow_ops[index]):
            lines.append(
                f"    phase1_matrix_{index}.ApplySlowOp("
                f"LightningSlowOp::{op_name}, "
                f"static_cast<std::uint8_t>(phase1_control_a[{(index * 3 + slow_index * 2 + 1) % 16}u] + phase1_salt[{(index * 5 + slow_index + 3) % 16}u]), "
                f"static_cast<std::uint8_t>(phase1_control_b[{(index * 7 + slow_index + 5) % 16}u] ^ phase1_carry_{(index + 1) % recipe.matrix_count}.FoldXor()));"
            )
        for fast_index, op_name in enumerate(phase.fast_ops[index]):
            lines.append(
                f"    phase1_matrix_{index}.ApplyFastOp("
                f"LightningFastOp::{op_name}, "
                f"static_cast<std::uint8_t>(phase1_control_a[{(index * 5 + fast_index + 1) % 16}u] ^ phase1_salt[{(index * 3 + fast_index + 4) % 16}u]), "
                f"static_cast<std::uint8_t>(phase1_control_b[{(index * 7 + fast_index + 2) % 16}u] + phase1_carry_{(index + 1) % recipe.matrix_count}.FoldAdd()));"
            )

    lines.extend(render_matrix_bridge_lines("phase1", recipe.matrix_count, phase.bridge_mode))

    for index in range(recipe.matrix_count):
        lines.append(f"    std::array<std::uint8_t, kNeonWidth> phase1_store_{index}{{}};")
        lines.append(f"    std::array<std::uint8_t, kNeonWidth> phase1_emit_{index}{{}};")
        lines.append(f"    phase1_matrix_{index}.Store(phase1_store_{index}.data());")
        lines.append(
            f"    const std::uint8_t phase1_emit_fold_{index} = static_cast<std::uint8_t>("
            f"phase1_matrix_{index}.FoldXor() + "
            f"phase1_carry_{index}.FoldAdd() + "
            f"phase1_carry_{(index + 1) % recipe.matrix_count}.FoldXor());"
        )
        lines.append("    for (std::size_t lane = 0; lane < kNeonWidth; ++lane) {")
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
    lines.append("    std::array<std::uint8_t, kNeonWidth> phase2_salt{};")
    lines.append(
        "    const std::uint8_t phase2_fold = static_cast<std::uint8_t>("
        + " + ".join(fold_terms)
        + ");"
    )
    lines.append("    for (std::size_t lane = 0; lane < kNeonWidth; ++lane) {")
    lines.append(
        "      phase2_salt[lane] = static_cast<std::uint8_t>("
        f"phase2_fold + {phase.salt_bias}u + "
        f"static_cast<std::uint8_t>(lane * {phase.salt_stride}u) + "
        f"static_cast<std::uint8_t>(((chunk / 16u) + lane) * {phase.salt_stride + recipe.matrix_count + 2}u));"
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

        for slow_index, op_name in enumerate(phase.slow_ops[index]):
            lines.append(
                f"    phase2_matrix_{index}.ApplySlowOp("
                f"LightningSlowOp::{op_name}, "
                f"static_cast<std::uint8_t>(phase2_control_a[{(index * 3 + slow_index + 2) % 16}u] + phase2_salt[{(index * 5 + slow_index + 1) % 16}u]), "
                f"static_cast<std::uint8_t>(phase2_control_b[{(index * 7 + slow_index + 4) % 16}u] ^ phase2_carry_{(index + 1) % recipe.matrix_count}.FoldXor()));"
            )
        for fast_index, op_name in enumerate(phase.fast_ops[index]):
            lines.append(
                f"    phase2_matrix_{index}.ApplyFastOp("
                f"LightningFastOp::{op_name}, "
                f"static_cast<std::uint8_t>(phase2_control_a[{(index * 5 + fast_index + 3) % 16}u] ^ phase2_salt[{(index * 3 + fast_index + 6) % 16}u]), "
                f"static_cast<std::uint8_t>(phase2_control_b[{(index * 7 + fast_index + 5) % 16}u] + phase2_carry_{(index + 1) % recipe.matrix_count}.FoldAdd()));"
            )

    lines.extend(render_matrix_bridge_lines("phase2", recipe.matrix_count, phase.bridge_mode))

    for index, source_offset in enumerate(recipe.source_mix_offsets):
        lines.append(f"    std::array<std::uint8_t, kNeonWidth> phase2_store_{index}{{}};")
        lines.append(f"    std::array<std::uint8_t, kNeonWidth> phase2_emit_{index}{{}};")
        lines.append(
            f"    const auto phase2_final_source_{index} = LoadBlock16Wrapped(source, (chunk + {source_offset}u) % PASSWORD_EXPANDED_SIZE);"
        )
        lines.append(f"    phase2_matrix_{index}.Store(phase2_store_{index}.data());")
        lines.append(
            f"    const std::uint8_t phase2_fold_x_{index} = static_cast<std::uint8_t>("
            f"phase2_matrix_{index}.FoldXor() ^ phase2_carry_{(index + 1) % recipe.matrix_count}.FoldXor());"
        )
        lines.append(
            f"    const std::uint8_t phase2_fold_a_{index} = static_cast<std::uint8_t>("
            f"phase2_matrix_{index}.FoldAdd() + phase2_carry_{index}.FoldAdd());"
        )
        lines.append("    for (std::size_t lane = 0; lane < kNeonWidth; ++lane) {")
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
                f"    - slow[{slow_index}] {op_name}: "
                f"{matrix_phase1_slow_formula(index, slow_index, recipe.matrix_count)}"
            )
        if not recipe.phase1.slow_ops[index]:
            lines.append("    - slow: none")
        for fast_index, op_name in enumerate(recipe.phase1.fast_ops[index]):
            lines.append(
                f"    - fast[{fast_index}] {op_name}: "
                f"{matrix_phase1_fast_formula(index, fast_index, recipe.matrix_count)}"
            )
        lines.append(
            f"    - emit_fold=u8(matrix_{index}.FoldXor() + carry_{index}.FoldAdd() + "
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
                f"    - slow[{slow_index}] {op_name}: "
                f"{matrix_phase2_slow_formula(index, slow_index, recipe.matrix_count)}"
            )
        if not recipe.phase2.slow_ops[index]:
            lines.append("    - slow: none")
        for fast_index, op_name in enumerate(recipe.phase2.fast_ops[index]):
            lines.append(
                f"    - fast[{fast_index}] {op_name}: "
                f"{matrix_phase2_fast_formula(index, fast_index, recipe.matrix_count)}"
            )
        lines.append(
            f"    - fold_x=u8(matrix_{index}.FoldXor() ^ carry_{(index + 1) % recipe.matrix_count}.FoldXor())"
        )
        lines.append(
            f"    - fold_a=u8(matrix_{index}.FoldAdd() + carry_{index}.FoldAdd())"
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


def build_matrix_candidate(rng: random.Random, candidate_id: int) -> CandidateSpec:
    recipe = build_matrix_recipe(rng, candidate_id)
    shape_key = json.dumps(matrix_recipe_shape_key(recipe), separators=(",", ":"))
    phase1_op_count = sum(len(group) for group in recipe.phase1.fast_ops) + sum(
        len(group) for group in recipe.phase1.slow_ops
    )
    phase2_op_count = sum(len(group) for group in recipe.phase2.fast_ops) + sum(
        len(group) for group in recipe.phase2.slow_ops
    )
    op_budget = phase1_op_count + phase2_op_count
    subop_count = sum(len(group) for group in recipe.phase1.slow_ops) + sum(
        len(group) for group in recipe.phase2.slow_ops
    )

    function_name = f"TwistCandidate_{candidate_id:04d}"
    stable_file_name = f"twist_candidate_{candidate_id:04d}.cpp"
    recipe_summary = (
        f"matrix[walk={recipe.matrix_count * ALIGNMENT}; final={recipe.final_mix}; "
        f"{matrix_phase_summary('phase1', recipe.phase1)} "
        f"{matrix_phase_summary('phase2', recipe.phase2)}]"
    )
    phase1 = matrix_manifest_phase(recipe.phase1, "phase1", "worker")
    phase2 = matrix_manifest_phase(recipe.phase2, "phase2", "dest")

    comment_lines = [
        f"// Candidate {candidate_id}: {function_name}",
        (
            f"// family=matrix walk={recipe.matrix_count * ALIGNMENT} final_mix={recipe.final_mix} "
            f"op_budget={op_budget} phase1_ops={phase1_op_count} phase2_ops={phase2_op_count} "
            f"expensive_ops={subop_count}"
        ),
        f"// {recipe_summary}",
    ]
    function_lines = comment_lines + [
        f"void {function_name}(",
        "    const uint8_t source[PASSWORD_EXPANDED_SIZE],",
        "    uint8_t worker[PASSWORD_EXPANDED_SIZE],",
        "    uint8_t dest[PASSWORD_EXPANDED_SIZE]) {",
        *render_matrix_phase1_lines(recipe),
        "",
        *render_matrix_phase2_lines(recipe),
        "}",
    ]
    function_source = "\n".join(function_lines) + "\n"

    candidate = CandidateSpec(
        candidate_id=candidate_id,
        function_name=function_name,
        stable_file_name=stable_file_name,
        op_budget=op_budget,
        phase1_op_count=phase1_op_count,
        phase2_op_count=phase2_op_count,
        multiply_count=0,
        subop_count=subop_count,
        phase1=phase1,
        phase2=phase2,
        recipe_summary=recipe_summary,
        function_source=function_source,
        family="matrix",
        verbose_text="",
        shape_key=shape_key,
    )
    verbose_text = render_matrix_verbose_text(candidate, recipe)
    return CandidateSpec(
        candidate_id=candidate.candidate_id,
        function_name=candidate.function_name,
        stable_file_name=candidate.stable_file_name,
        op_budget=candidate.op_budget,
        phase1_op_count=candidate.phase1_op_count,
        phase2_op_count=candidate.phase2_op_count,
        multiply_count=candidate.multiply_count,
        subop_count=candidate.subop_count,
        phase1=candidate.phase1,
        phase2=candidate.phase2,
        recipe_summary=candidate.recipe_summary,
        function_source=candidate.function_source,
        family=candidate.family,
        verbose_text=verbose_text,
        shape_key=candidate.shape_key,
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
            "  }"
        )
    registry_text = ",\n".join(registry_entries)
    return f"""#include "LightningMatrix.hpp"
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
            "  }"
        )
    registry_text = ",\n".join(registry_entries)
    return f"""#include "LightningMatrix.hpp"
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
for (i = 0; i < PASSWORD_EXPANDED_SIZE; i += kNeonWidth) {{
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
        "Each loop iteration processes 16 bytes (one NEON register).",
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
        "#include \"LightningMatrix.hpp\"",
        "#include \"TwistTypes.hpp\"",
        "",
        "namespace twist {",
        "",
        f"// Top candidates exported from manifest seed={seed}",
    ]
    for candidate in selected:
        row = score_by_id[candidate.candidate_id]
        pieces.append(
            f"// grade={row.get('grade', 'NA')} composite={float(row['composite_score']):.4f} "
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

    candidates, stopped_early = generate_candidates(
        count=args.count,
        seed=seed,
        phase1_min_ops=DEFAULT_PHASE1_MIN_OPS,
        phase1_max_ops=DEFAULT_PHASE1_MAX_OPS,
        phase2_min_ops=DEFAULT_PHASE2_MIN_OPS,
        phase2_max_ops=DEFAULT_PHASE2_MAX_OPS,
        max_transforms_total=DEFAULT_MAX_TRANSFORMS_TOTAL,
    )
    manifest = {
        "seed": seed,
        "candidate_count": len(candidates) + len(BASELINE_CANDIDATES),
        "generated_candidate_count": len(candidates),
        "baseline_candidate_count": len(BASELINE_CANDIDATES),
        "password_expanded_size": PASSWORD_EXPANDED_SIZE,
        "alignment": ALIGNMENT,
        "knobs_path": str(KNOBS_PATH),
        "phase1_min_ops": DEFAULT_PHASE1_MIN_OPS,
        "phase1_max_ops": DEFAULT_PHASE1_MAX_OPS,
        "phase2_min_ops": DEFAULT_PHASE2_MIN_OPS,
        "phase2_max_ops": DEFAULT_PHASE2_MAX_OPS,
        "max_transforms_total": DEFAULT_MAX_TRANSFORMS_TOTAL,
        "candidates": [candidate_to_manifest(candidate) for candidate in candidates] + list(BASELINE_CANDIDATES),
    }
    generated_cpp = render_generated_cpp(candidates, seed, len(candidates))
    generated_verbose_cpp = render_generated_verbose_cpp(candidates, seed, len(candidates))
    generated_verbose_text = render_verbose_text(candidates, seed, len(candidates))
    write_text(cpp_path, generated_cpp)
    write_text(verbose_cpp_path, generated_verbose_cpp)
    write_text(verbose_txt_path, generated_verbose_text)
    write_text(manifest_path, json.dumps(manifest, indent=2))

    print(f"generated {len(candidates)} candidates with seed={seed}")
    if stopped_early and len(candidates) < args.count:
        print("stopped early after exhausting new unique shapes under current generator rules")
    print(f"baselines: {len(BASELINE_CANDIDATES)}")
    print(f"total_manifest_candidates: {manifest['candidate_count']}")
    print(f"manifest: {manifest_path}")
    print(f"cpp: {cpp_path}")
    print(f"verbose_cpp: {verbose_cpp_path}")
    print(f"verbose_txt: {verbose_txt_path}")
    return 0


def command_export_top(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest)
    scores_path = Path(args.scores)
    output_path = Path(args.output)

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    candidates = [
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
            function_source=item["function_source"],
            family=item.get("family", "classic"),
            verbose_text=item.get("verbose_text", ""),
            shape_key=item.get("shape_key", ""),
        )
        for item in manifest["candidates"]
        if bool(item.get("exportable", True))
    ]
    score_rows = sorted(
        load_scores(scores_path),
        key=lambda row: (-row["grade_rank"], -row["composite_score"], row["candidate_id"]),
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
    export_parser.add_argument("--top", type=int, default=DEFAULT_TOP_N)
    export_parser.set_defaults(func=command_export_top)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
