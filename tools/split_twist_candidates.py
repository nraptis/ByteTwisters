#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


FUNCTION_START_RE = re.compile(r"^// Candidate (\d+): ")


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def shard_name(index: int) -> str:
    return f"shard_{index:04d}"


def split_functions(input_path: Path, shard_size: int, temp_dir: Path) -> tuple[int, int]:
    temp_dir.mkdir(parents=True, exist_ok=True)
    state = "preamble"
    candidate_index = 0
    current_lines: list[str] = []
    current_shard = -1

    with input_path.open("r", encoding="utf-8") as handle:
      for line in handle:
        if state == "preamble":
          if FUNCTION_START_RE.match(line):
            state = "functions"
            current_lines = [line]
          continue

        if state == "functions":
          if line.startswith("const RegisteredCandidate kRegisteredCandidates[] = {"):
            if current_lines:
              current_shard = candidate_index // shard_size
              function_path = temp_dir / f"{shard_name(current_shard)}.functions.cpp"
              with function_path.open("a", encoding="utf-8") as shard_file:
                shard_file.write("".join(current_lines).rstrip() + "\n\n")
              candidate_index += 1
              current_lines = []
            state = "registry"
            continue

          if FUNCTION_START_RE.match(line) and current_lines:
            current_shard = candidate_index // shard_size
            function_path = temp_dir / f"{shard_name(current_shard)}.functions.cpp"
            with function_path.open("a", encoding="utf-8") as shard_file:
              shard_file.write("".join(current_lines).rstrip() + "\n\n")
            candidate_index += 1
            current_lines = [line]
            continue

          current_lines.append(line)

    shard_count = (candidate_index + shard_size - 1) // shard_size if candidate_index else 0
    return candidate_index, shard_count


def split_registry(input_path: Path, shard_size: int, temp_dir: Path) -> None:
    state = "seek"
    candidate_index = 0
    entry_lines: list[str] = []
    brace_depth = 0

    with input_path.open("r", encoding="utf-8") as handle:
      for line in handle:
        if state == "seek":
          if line.startswith("const RegisteredCandidate kRegisteredCandidates[] = {"):
            state = "registry"
          continue

        if state == "registry":
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
            shard_index = candidate_index // shard_size
            registry_path = temp_dir / f"{shard_name(shard_index)}.registry.txt"
            with registry_path.open("a", encoding="utf-8") as shard_file:
              shard_file.write("".join(entry_lines))
            candidate_index += 1
            entry_lines = []


def assemble_shards(output_dir: Path, temp_dir: Path, candidate_count: int, shard_size: int) -> list[dict[str, int | str]]:
    output_dir.mkdir(parents=True, exist_ok=True)
    shard_metadata: list[dict[str, int | str]] = []
    shard_count = (candidate_count + shard_size - 1) // shard_size if candidate_count else 0

    for index in range(shard_count):
      start_id = index * shard_size + 1
      end_id = min(candidate_count, (index + 1) * shard_size)
      name = shard_name(index)
      function_path = temp_dir / f"{name}.functions.cpp"
      registry_path = temp_dir / f"{name}.registry.txt"
      shard_cpp_path = output_dir / f"{name}.cpp"

      functions = function_path.read_text(encoding="utf-8") if function_path.exists() else ""
      registry = registry_path.read_text(encoding="utf-8") if registry_path.exists() else ""

      shard_source = (
          '#include "LightningMatrix.hpp"\n'
          '#include "TwistTypes.hpp"\n\n'
          "namespace twist {\n\n"
          "// Generated shard from tools/split_twist_candidates.py\n"
          f"// candidate_id_range={start_id}-{end_id}\n\n"
          f"{functions}"
          "const RegisteredCandidate kRegisteredCandidates[] = {\n"
          f"{registry}"
          "};\n\n"
          "const std::size_t kRegisteredCandidateCount =\n"
          "    sizeof(kRegisteredCandidates) / sizeof(kRegisteredCandidates[0]);\n\n"
          "}  // namespace twist\n"
      )
      write_text(shard_cpp_path, shard_source)
      shard_metadata.append(
          {
              "name": name,
              "path": str(shard_cpp_path),
              "start_candidate_id": start_id,
              "end_candidate_id": end_id,
              "candidate_count": end_id - start_id + 1,
          }
      )

    return shard_metadata


def main() -> int:
    parser = argparse.ArgumentParser(description="Split generated twist candidate C++ into compileable shards.")
    parser.add_argument("--input", default="generated/twist_candidates_generated_verbose.cpp")
    parser.add_argument("--output-dir", default="generated/shards_cpp")
    parser.add_argument("--shard-size", type=int, default=2000)
    parser.add_argument("--index-output", default="generated/shards_index.json")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_dir = Path(args.output_dir)
    temp_dir = output_dir.parent / ".shard_tmp"

    candidate_count, shard_count = split_functions(input_path, args.shard_size, temp_dir)
    split_registry(input_path, args.shard_size, temp_dir)
    shard_metadata = assemble_shards(output_dir, temp_dir, candidate_count, args.shard_size)

    index = {
        "input": str(input_path),
        "output_dir": str(output_dir),
        "shard_size": args.shard_size,
        "candidate_count": candidate_count,
        "shard_count": shard_count,
        "shards": shard_metadata,
    }
    write_text(Path(args.index_output), json.dumps(index, indent=2))

    for temp_file in temp_dir.glob("*"):
      temp_file.unlink()
    temp_dir.rmdir()

    print(f"split {candidate_count} candidates into {shard_count} shards")
    print(f"index: {args.index_output}")
    print(f"shards: {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
