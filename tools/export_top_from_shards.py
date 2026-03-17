#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import re
from pathlib import Path


FUNCTION_START_RE = re.compile(r"^// Candidate (\d+): ")


def load_top_rows(scores_path: Path, top_n: int) -> list[dict[str, str]]:
    with scores_path.open("r", encoding="utf-8", newline="") as handle:
      reader = csv.DictReader(handle)
      rows = list(reader)
    rows.sort(
        key=lambda row: (
            -int(row.get("grade_rank", "0") or 0),
            -float(row.get("composite_score", "0") or 0.0),
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


def main() -> int:
    parser = argparse.ArgumentParser(description="Export top-ranked candidate functions from shard sources.")
    parser.add_argument("--scores", default="generated/twist_candidate_scores.csv")
    parser.add_argument("--index", default="generated/shards_index.json")
    parser.add_argument("--output", default="generated/top_twist_candidates.cpp")
    parser.add_argument("--top", type=int, default=25)
    args = parser.parse_args()

    top_rows = load_top_rows(Path(args.scores), args.top)
    index = json.loads(Path(args.index).read_text(encoding="utf-8"))

    functions_by_id: dict[int, str] = {}
    rows_by_id = {int(row["candidate_id"]): row for row in top_rows}
    shard_to_ids: dict[Path, set[int]] = {}
    for row in top_rows:
      candidate_id = int(row["candidate_id"])
      if candidate_id < 0:
        continue
      shard_path = shard_for_candidate(index, candidate_id)
      shard_to_ids.setdefault(shard_path, set()).add(candidate_id)

    for shard_path, wanted_ids in shard_to_ids.items():
      functions_by_id.update(extract_functions(shard_path, wanted_ids))

    pieces = [
        '#include "LightningMatrix.hpp"',
        '#include "TwistTypes.hpp"',
        "",
        "namespace twist {",
        "",
        "// Top candidates exported from sharded measurement",
    ]
    for row in top_rows:
      candidate_id = int(row["candidate_id"])
      if candidate_id < 0:
        continue
      function_text = functions_by_id.get(candidate_id)
      if function_text is None:
        continue
      pieces.append(
          f"// grade={row.get('grade', 'NA')} composite={float(row.get('composite_score', '0') or 0.0):.4f} "
          f"candidate_id={candidate_id}"
      )
      pieces.append(function_text)
      pieces.append("")
    pieces.append("}  // namespace twist")
    pieces.append("")

    Path(args.output).write_text("\n".join(pieces), encoding="utf-8")
    print(f"exported top {args.top} candidates to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
