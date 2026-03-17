# Twist Candidate Pipeline

This repo generates and grades two-phase byte twisters that are shaped for AArch64 NEON.

Each generated twister has exactly:

- `phase 1`: read from `source`, write to `worker`
- `phase 2`: read from `source` and `worker`, write to `dest`

The hot path is byte-oriented but emitted as NEON vector code over `uint8x16_t`.

The pipeline now emits two generated source views:

- `generated/twist_candidates_generated.cpp`
  - compact machine-oriented output
- `generated/twist_candidates_generated_verbose.cpp`
  - readable source of truth used by the harness build

There is also a plain-text step listing at `generated/twist_candidates_generated_verbose.txt`.

The grading harness also includes always-on baseline counters:

- `Baseline_AES256CTR`
- `Baseline_ARIA256CTR`
- `Baseline_ChaCha20CTR`
- `Baseline_MersenneTwister`

These baselines are compiled from the project-local reference sources and are graded alongside every generated twister.

## Where To Tune

All generation and default harness knobs live in `src/Knobs.hpp`.

That header controls:

- how many candidates are generated
- per-phase min/max counted ops
- max transforms total
- max counts for each binary op
- max counts for each transform
- harness length factor and trial count
- shared run seed
- long-repeat verification settings

Example current limits:

- `kMaxMulOps = 1`
- `kMaxSwapNibblesTransforms = 1`
- `kMaxAndOps = 0`
- `kMaxOrOps = 0`
- `kPhase1MinOps = 3`
- `kPhase1MaxOps = 4`
- `kPhase2MinOps = 3`
- `kPhase2MaxOps = 4`
- `kLengthFactor = 5`
- `kTrialCount = 10`
- `kLongRepeatScanBytes = 20000000`

The harness uses `kRandomSeed` as the shared run seed when it is nonzero. If it is `0`, `make_measures.sh` chooses one fresh seed and passes that same seed to both the generator and the harness.

## Test Method

The harness no longer starts from random source buffers by default. It now tests whether a candidate can turn simple low-entropy garbage into good noise.

Each trial starts from one deterministic source pattern, then repeatedly feeds `dest` back into `source`:

- `all_00`
- `all_ff`
- `all_f0`
- `all_0f`
- `all_aa`
- `all_55`
- `alternating_00_ff`
- `alternating_f0_0f`
- `ramp_up`
- `ramp_down`
- `two_byte_00_01`
- `nibble_ramp`

`kTrialCount` controls how many of those patterns are used per candidate.

The pipeline does two repeat checks:

- fast population scoring with sampled rolling 16-byte windows
- exact long-window verification on the provisional top candidates for 64-byte and 128-byte repeated sections

By default the exact verifier checks the top `kLongRepeatTopCandidateCount` candidates over `kLongRepeatScanBytes`.

With the current knobs, the lightest twister is 6 counted ops total and the heaviest is 8 counted ops total.

## Current Supported Operations

These are the operations the generator can emit today.

### Core Binary Byte Ops

- `add`
  - NEON mapping: `vaddq_u8`
- `sub`
  - NEON mapping: `vsubq_u8`
- `mul`
  - NEON mapping: `vmulq_u8`
- `xor`
  - NEON mapping: `veorq_u8`
- `and`
  - NEON mapping: `vandq_u8`
- `or`
  - NEON mapping: `vorrq_u8`

These are lane-wise byte operations across a `uint8x16_t`.

Current default search policy:

- `add`, `sub`, `mul`, and `xor` are enabled
- `and` and `or` remain implemented, but are disabled by default in `Knobs.hpp`

Reason:

- on the current low-entropy source-pattern suite, `and` and `or` tend to be too destructive and collapse variation instead of building useful diffusion

### Current Unary / Transform Ops

- `add_const`
  - adds the same byte constant to all lanes
- `shl`
  - byte-wise left shift
- `shr`
  - byte-wise right shift
- `not`
  - bitwise invert per byte
- `swap_nibbles`
  - swaps high/low nibble in each byte
- `byte_lr8_left`
  - rotates byte positions inside each 8-byte half of the 16-byte NEON register
- `byte_lr8_right`
  - same, opposite direction

Notes:

- `swap_nibbles` is a very good NEON-friendly transform.
- `byte_lr8_*` is useful because it breaks lane stability without needing scalar logic.
- wrapping loads are handled by `LoadVecWrapped`, so the generator does not need separate front/back array parameters.
- the readable verbose file names indices and intermediate values explicitly so you can inspect exactly what the harness is running.

## Other Potential Operations That Could Work

These are good future candidates because they still map cleanly to NEON byte semantics.

- `min` / `max`
  - NEON-friendly and cheap
- saturating `add` / saturating `sub`
  - useful if you want stronger nonlinearity without branches
- byte-wise `select`
  - choose from two vectors using a mask
- per-byte rotate-by-bits
  - can be built from shifts plus `or`
- byte reverse inside each 8-byte half
  - cheap shuffle-style diffusion op
- zip/unzip style interleave transforms
  - good for mixing neighboring lanes
- `absdiff`
  - potentially interesting for nonlinear mixing
- table-driven fixed shuffles
  - acceptable if the table is static and small

Good rule: prefer operations that stay in vector registers and avoid scalar fallback in the hot loop.

## Operations To Avoid

These are poor fits because they are slow, awkward, or work against the NEON-first design.

- integer division
  - very slow and not a natural byte-vector primitive
- integer modulo in the hot path
  - expensive; wrapping should stay in load helpers, not inside the math
- data-dependent branches
  - harms throughput and makes SIMD execution uneven
- scalar per-byte loops inside the twister body
  - defeats the purpose of the NEON layout
- arbitrary gather/scatter
  - NEON does not provide general byte gather loads
- large data-dependent table lookups
  - bad for cache behavior and awkward for SIMD
- cross-register variable shuffles with complex control
  - often much more expensive than fixed-pattern shuffles
- popcount-heavy per-byte logic in the hot path
  - possible, but usually too expensive for a baseline search space
- 64-bit carry-based arithmetic pretending to be byte arithmetic
  - wrong semantics for this pipeline

## Operations To Use Carefully

- `mul`
  - supported and sometimes useful, but usually heavier than `xor/and/or/add/sub`
- shift transforms
  - cheap, but too many shifts can make outputs overly structured
- `byte_lr8_*`
  - good for diffusion, but too much reliance on permutation without arithmetic can reduce mixing quality
- `and` / `or`
  - implemented, but disabled by default because they often zero-lock or saturate low-entropy sources too early

## Lightest And Heaviest Twisters

These examples are taken from the current generator limits in `src/Knobs.hpp`.

### Lightest Twister We Can Generate

This is a 6-op twister: 3 core ops in phase 1 and 3 core ops in phase 2, with no transforms.

Example recipe:

```text
phase1[offs=880,2840,6640; e=sub(a,b); f=and(d,c); out=xor(e,f)]
phase2[offs=48,3992,6136; e=add(a,b); f=mul(d,c); out=add(e,f)]
```

Why it is light:

- no transforms
- exactly 3 binary ops in each phase
- only one multiply total

### Heaviest Twister We Can Generate

This is an 8-op twister: 4 counted ops in phase 1 and 4 counted ops in phase 2.

Example recipe:

```text
phase1[offs=1960,3880,7152; e=and(a,b>>1); f=add(d,c); out=or(e,f)]
phase2[offs=616,1328,5280; e=sub(a,b<<1); f=and(d,c); out=and(e,f)]
```

Why it is heavy:

- one transform in phase 1
- one transform in phase 2
- still only two loops total
- still respects the current multiply and nibble-swap caps

Another valid heavy example using `byte_lr8`:

```text
phase1[offs=3000,4616,5592; e=or(a,b+3); f=add(d,c); out=add(e,f)]
phase2[offs=3672,5576,6736; e=sub(a,b); f=add(d,byte_lr8_r2(c)); out=add(e,f)]
```

## Report Outputs

The pipeline writes:

- `generated/twist_candidate_scores.csv`
- `generated/twist_candidate_summary.txt`
- `generated/twist_candidate_report.html`
- `generated/twist_candidates_generated_verbose.cpp`
- `generated/twist_candidates_generated_verbose.txt`
- `generated/top_twist_candidates.cpp`

The manifest now records:

- `generated_candidate_count`
- `baseline_candidate_count`
- `candidate_count` for the combined total used by reporting

The HTML report is the easiest place to inspect the ranked candidates.
The verbose `.cpp` and `.txt` files are the easiest way to inspect the exact generated loop logic.
The CSV and summary now also include `exact_repeat_64_*` and `exact_repeat_128_*` fields.

## Run

Default run:

```bash
./make_measures.sh
```

Split run:

```bash
./generate_twists.sh
./run_measure_twists.sh
```

Fixed seed run:

```bash
./make_measures.sh 1000 1234
```

By default the script generates a fresh random seed each run and prints it so the run can be reproduced later.

`run_measure_twists.sh` cleans `build`, compiles the current generated verbose input, and prints:

- the input file path
- the input file timestamp
- the manifest test count
- the function count found in the generated input
- `kCandidateCount`
- `kTopCandidateCount`
- `kLengthFactor`
- `kTrialCount`

If the generated inputs do not match `Knobs.hpp`, it prints a warning before running the harness.

Example with smaller verification settings for a quick pass:

```bash
LENGTH_FACTOR=2 TRIAL_COUNT=4 LONG_REPEAT_BYTES=8192 LONG_REPEAT_TOP=1 ./make_measures.sh 64 4242
```
