#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import html
import json
from pathlib import Path
from typing import Any

SCENARIOS = ("stale", "pseudorandom", "structured")


def compute_grade(score: float) -> tuple[str, int]:
    if score >= 97.0:
        return "A+", 12
    if score >= 93.0:
        return "A", 11
    if score >= 90.0:
        return "A-", 10
    if score >= 87.0:
        return "B+", 9
    if score >= 83.0:
        return "B", 8
    if score >= 80.0:
        return "B-", 7
    if score >= 77.0:
        return "C+", 6
    if score >= 73.0:
        return "C", 5
    if score >= 70.0:
        return "C-", 4
    if score >= 65.0:
        return "D+", 3
    if score >= 60.0:
        return "D", 2
    if score >= 50.0:
        return "D-", 1
    return "F", 0


def parse_bool(value: str) -> bool:
    return value.strip().lower() == "true"


def parse_int(value: str) -> int:
    return int(value) if value.strip() else 0


def parse_float(value: str) -> float:
    return float(value) if value.strip() else 0.0


def load_manifest(path: Path) -> dict[int, dict[str, Any]]:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    by_id: dict[int, dict[str, Any]] = {}
    for item in manifest["candidates"]:
        by_id[int(item["candidate_id"])] = item
    return by_id


def load_suite_csv(path: Path) -> dict[int, dict[str, Any]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        rows: dict[int, dict[str, Any]] = {}
        for row in reader:
            candidate_id = parse_int(row["candidate_id"])
            rows[candidate_id] = {
                "candidate_id": candidate_id,
                "function_name": row["function_name"],
                "grade": row["grade"],
                "grade_rank": parse_int(row["grade_rank"]),
                "composite_score": parse_float(row["composite_score"]),
                "repeat_score": parse_float(row["repeat_score"]),
                "cycle_score": parse_float(row["cycle_score"]),
                "uniformity_score": parse_float(row["uniformity_score"]),
                "predictability_score": parse_float(row["predictability_score"]),
                "avalanche_score": parse_float(row["avalanche_score"]),
                "distinctness_score": parse_float(row["distinctness_score"]),
                "entropy": parse_float(row["entropy"]),
                "reduced_chi_squared": parse_float(row["reduced_chi_squared"]),
                "max_deviation": parse_float(row["max_deviation"]),
                "byte_count_spread_ratio": parse_float(row.get("byte_count_spread_ratio", "")),
                "most_common_byte_count": parse_int(row.get("most_common_byte_count", "")),
                "least_common_byte_count": parse_int(row.get("least_common_byte_count", "")),
                "avalanche_byte_ratio": parse_float(row["avalanche_byte_ratio"]),
                "avalanche_bit_ratio": parse_float(row["avalanche_bit_ratio"]),
                "rejected": parse_bool(row["rejected"]),
                "failure_reason": row["failure_reason"],
                "recipe_summary": row["recipe_summary"],
                "repeat_found": parse_bool(row["repeat_found"]),
                "cycle_found": parse_bool(row["cycle_found"]),
                "long_repeat_verified": parse_bool(row.get("long_repeat_verified", "false")),
                "exact_repeat_64_found": parse_bool(row.get("exact_repeat_64_found", "false")),
                "exact_repeat_64_position": parse_int(row.get("exact_repeat_64_position", "")),
                "exact_repeat_64_trial": parse_int(row.get("exact_repeat_64_trial", "")),
                "exact_repeat_128_found": parse_bool(row.get("exact_repeat_128_found", "false")),
                "exact_repeat_128_position": parse_int(row.get("exact_repeat_128_position", "")),
                "exact_repeat_128_trial": parse_int(row.get("exact_repeat_128_trial", "")),
                "first_block_hash": parse_int(row.get("first_block_hash", "")),
                "signature_lo": parse_int(row.get("signature_lo", "")),
                "signature_hi": parse_int(row.get("signature_hi", "")),
            }
        return rows


def load_suite_dir(path: Path) -> dict[int, dict[str, Any]]:
    rows: dict[int, dict[str, Any]] = {}
    for csv_path in sorted(path.rglob("twist_candidate_scores.csv")):
        partial = load_suite_csv(csv_path)
        rows.update(partial)
    return rows


def combined_recipe_distance(left: dict[str, Any], right: dict[str, Any]) -> float:
    total = 0.0
    count = 0
    for phase_name in ("phase1", "phase2"):
        left_phase = left[phase_name]
        right_phase = right[phase_name]
        for key in ("op1", "op2", "op3", "e_input", "f_input"):
            total += 0.0 if left_phase[key] == right_phase[key] else 1.0
            count += 1
        for transform_key in ("e_transform", "f_transform"):
            total += 0.0 if left_phase[transform_key]["kind"] == right_phase[transform_key]["kind"] else 1.0
            count += 1
            total += min(1.0, abs(int(left_phase[transform_key]["arg"]) - int(right_phase[transform_key]["arg"])) / 4.0)
            count += 1
        for idx in range(3):
            total += min(
                1.0,
                abs(int(left_phase["offsets"][idx]) - int(right_phase["offsets"][idx])) / 7680.0,
            )
            count += 1
    return total / max(1, count)


def signature_distance(left: dict[str, Any], right: dict[str, Any]) -> float:
    distances: list[float] = []
    for scenario in SCENARIOS:
        left_lo = int(left[f"{scenario}_signature_lo"])
        left_hi = int(left[f"{scenario}_signature_hi"])
        right_lo = int(right[f"{scenario}_signature_lo"])
        right_hi = int(right[f"{scenario}_signature_hi"])
        hamming = (left_lo ^ right_lo).bit_count() + (left_hi ^ right_hi).bit_count()
        distances.append(hamming / 128.0)
    return sum(distances) / max(1, len(distances))


def format_exact(row: dict[str, Any]) -> str:
    parts: list[str] = []
    if row["exact_repeat_64_found"]:
        parts.append(f"64@{row['exact_repeat_64_position']}")
    if row["exact_repeat_128_found"]:
        parts.append(f"128@{row['exact_repeat_128_position']}")
    return "|".join(parts) if parts else "clear"


def combine_rows(
    manifest: dict[int, dict[str, Any]],
    suites: dict[str, dict[int, dict[str, Any]]],
) -> list[dict[str, Any]]:
    shared_ids = set(manifest.keys())
    for scenario in SCENARIOS:
        shared_ids &= set(suites[scenario].keys())

    combined: list[dict[str, Any]] = []
    for candidate_id in sorted(shared_ids):
        manifest_row = manifest[candidate_id]
        scenario_rows = {scenario: suites[scenario][candidate_id] for scenario in SCENARIOS}
        scores = [scenario_rows[scenario]["composite_score"] for scenario in SCENARIOS]
        mean_score = sum(scores) / len(scores)
        min_score = min(scores)
        max_score = max(scores)
        balance_score = max(0.0, 100.0 - ((max_score - min_score) * 1.25))
        rejected_count = sum(1 for scenario in SCENARIOS if scenario_rows[scenario]["rejected"])
        overall_score = (mean_score * 0.50) + (min_score * 0.35) + (balance_score * 0.15)
        if rejected_count:
            overall_score = min(overall_score, 59.9 - (rejected_count - 1) * 7.5)
        grade, grade_rank = compute_grade(overall_score)

        row: dict[str, Any] = {
            "candidate_id": candidate_id,
            "function_name": manifest_row["function_name"],
            "recipe_summary": manifest_row["recipe_summary"],
            "overall_score": overall_score,
            "mean_score": mean_score,
            "min_score": min_score,
            "max_score": max_score,
            "balance_score": balance_score,
            "rejected_count": rejected_count,
            "grade": grade,
            "grade_rank": grade_rank,
        }
        for scenario in SCENARIOS:
            suite_row = scenario_rows[scenario]
            row[f"{scenario}_score"] = suite_row["composite_score"]
            row[f"{scenario}_rejected"] = suite_row["rejected"]
            row[f"{scenario}_failure"] = suite_row["failure_reason"]
            row[f"{scenario}_repeat"] = suite_row["repeat_score"]
            row[f"{scenario}_cycle"] = suite_row["cycle_score"]
            row[f"{scenario}_uniformity"] = suite_row["uniformity_score"]
            row[f"{scenario}_spread"] = suite_row["byte_count_spread_ratio"]
            row[f"{scenario}_most_common"] = suite_row["most_common_byte_count"]
            row[f"{scenario}_least_common"] = suite_row["least_common_byte_count"]
            row[f"{scenario}_histogram_gap"] = (
                suite_row["most_common_byte_count"] - suite_row["least_common_byte_count"]
            )
            row[f"{scenario}_predictability"] = suite_row["predictability_score"]
            row[f"{scenario}_avalanche"] = suite_row["avalanche_score"]
            row[f"{scenario}_exact64"] = suite_row["exact_repeat_64_found"]
            row[f"{scenario}_exact128"] = suite_row["exact_repeat_128_found"]
            row[f"{scenario}_exact_text"] = format_exact(suite_row)
            row[f"{scenario}_signature_lo"] = suite_row["signature_lo"]
            row[f"{scenario}_signature_hi"] = suite_row["signature_hi"]
        combined.append(row)
    combined.sort(key=lambda item: (-item["grade_rank"], -item["overall_score"], item["candidate_id"]))
    return combined


def choose_shortlist(rows: list[dict[str, Any]], shortlist_count: int) -> list[int]:
    shortlist: list[int] = []
    seen: set[int] = set()

    def add_row(row: dict[str, Any]) -> None:
        candidate_id = int(row["candidate_id"])
        if candidate_id not in seen and len(shortlist) < shortlist_count:
            shortlist.append(candidate_id)
            seen.add(candidate_id)

    for row in rows[: max(1, shortlist_count // 2)]:
        add_row(row)
    for scenario in SCENARIOS:
        ranked = sorted(rows, key=lambda item: (-item[f"{scenario}_score"], item["candidate_id"]))
        for row in ranked[: max(4, shortlist_count // 6)]:
            add_row(row)
    balanced = sorted(rows, key=lambda item: (-item["min_score"], -item["balance_score"], item["candidate_id"]))
    for row in balanced:
        add_row(row)
        if len(shortlist) >= shortlist_count:
            break
    return shortlist


def choose_finalists(
    rows: list[dict[str, Any]],
    manifest: dict[int, dict[str, Any]],
    count: int,
) -> list[dict[str, Any]]:
    if not rows:
        return []

    selected: list[dict[str, Any]] = [rows[0]]
    selected_ids = {int(rows[0]["candidate_id"])}

    while len(selected) < min(count, len(rows)):
        best_row: dict[str, Any] | None = None
        best_value = -1.0
        for row in rows:
            candidate_id = int(row["candidate_id"])
            if candidate_id in selected_ids:
                continue
            score_component = row["overall_score"] / 100.0
            diversity_values: list[float] = []
            for chosen in selected:
                signature_component = signature_distance(row, chosen)
                recipe_component = combined_recipe_distance(
                    manifest[candidate_id], manifest[int(chosen["candidate_id"])]
                )
                diversity_values.append((signature_component * 0.7) + (recipe_component * 0.3))
            diversity_component = min(diversity_values) if diversity_values else 0.0
            candidate_value = (score_component * 0.68) + (diversity_component * 0.32)
            if candidate_value > best_value:
                best_value = candidate_value
                best_row = row
        if best_row is None:
            break
        selected.append(best_row)
        selected_ids.add(int(best_row["candidate_id"]))
    return selected


def write_combined_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    if not rows:
        return
    fieldnames = list(rows[0].keys())
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_ranked_text(
    path: Path,
    title: str,
    rows: list[dict[str, Any]],
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"{title}\n")
        handle.write(f"{'=' * len(title)}\n\n")
        for index, row in enumerate(rows, start=1):
            handle.write(
                f"{index}. candidate_id={row['candidate_id']} function={row['function_name']} "
                f"overall={row['overall_score']:.6f} stale={row['stale_score']:.6f} "
                f"pseudorandom={row['pseudorandom_score']:.6f} structured={row['structured_score']:.6f} "
                f"min={row['min_score']:.6f} balance={row['balance_score']:.6f} "
                f"rejected_count={row['rejected_count']}\n"
            )
            handle.write(
                f"   stale: failure={row['stale_failure']} exact={row['stale_exact_text']} "
                f"uniformity={row['stale_uniformity']:.6f} spread={row['stale_spread']:.6f} "
                f"most={row['stale_most_common']} least={row['stale_least_common']} "
                f"gap={row['stale_histogram_gap']}\n"
            )
            handle.write(
                f"   pseudorandom: failure={row['pseudorandom_failure']} exact={row['pseudorandom_exact_text']} "
                f"uniformity={row['pseudorandom_uniformity']:.6f} spread={row['pseudorandom_spread']:.6f} "
                f"most={row['pseudorandom_most_common']} least={row['pseudorandom_least_common']} "
                f"gap={row['pseudorandom_histogram_gap']}\n"
            )
            handle.write(
                f"   structured: failure={row['structured_failure']} exact={row['structured_exact_text']} "
                f"uniformity={row['structured_uniformity']:.6f} spread={row['structured_spread']:.6f} "
                f"most={row['structured_most_common']} least={row['structured_least_common']} "
                f"gap={row['structured_histogram_gap']}\n"
            )
            handle.write(f"   recipe={row['recipe_summary']}\n\n")


def write_html_report(
    path: Path,
    title: str,
    top_rows: list[dict[str, Any]],
    bottom_rows: list[dict[str, Any]],
    finalists: list[dict[str, Any]],
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)

    def render_rows(rows: list[dict[str, Any]]) -> str:
        pieces: list[str] = []
        for rank, row in enumerate(rows, start=1):
            pieces.append(
                "<tr>"
                f"<td>{rank}</td>"
                f"<td>{row['candidate_id']} / {html.escape(row['function_name'])}</td>"
                f"<td>{row['overall_score']:.4f}</td>"
                f"<td>{row['stale_score']:.4f}</td>"
                f"<td>{row['pseudorandom_score']:.4f}</td>"
                f"<td>{row['structured_score']:.4f}</td>"
                f"<td>{row['min_score']:.4f}</td>"
                f"<td>{row['balance_score']:.4f}</td>"
                f"<td>{row['stale_uniformity']:.4f} / {row['stale_histogram_gap']}</td>"
                f"<td>{row['pseudorandom_uniformity']:.4f} / {row['pseudorandom_histogram_gap']}</td>"
                f"<td>{row['structured_uniformity']:.4f} / {row['structured_histogram_gap']}</td>"
                f"<td>{row['rejected_count']}</td>"
                f"<td>{html.escape(row['recipe_summary'])}</td>"
                "</tr>"
            )
        return "\n".join(pieces)

    html_text = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{html.escape(title)}</title>
  <style>
    body {{ margin: 0; font: 14px/1.5 Menlo, Monaco, monospace; background: #f4efe5; color: #1f1a17; }}
    main {{ max-width: 1280px; margin: 0 auto; padding: 32px 24px 48px; }}
    table {{ width: 100%; border-collapse: collapse; background: #fffaf0; border: 1px solid #d8c9b2; margin-top: 12px; }}
    th, td {{ padding: 10px 8px; border-bottom: 1px solid #eadcc6; text-align: left; vertical-align: top; }}
    th {{ background: #f2e3cc; }}
    .section {{ margin-top: 28px; }}
  </style>
</head>
<body>
<main>
  <h1>{html.escape(title)}</h1>
  <div class="section">
    <h2>Final 16</h2>
    <table>
      <thead><tr><th>Rank</th><th>Candidate</th><th>Overall</th><th>Stale</th><th>Pseudorandom</th><th>Structured</th><th>Min</th><th>Balance</th><th>Stale U/G</th><th>PRNG U/G</th><th>Struct U/G</th><th>Rejected</th><th>Recipe</th></tr></thead>
      <tbody>{render_rows(finalists)}</tbody>
    </table>
  </div>
  <div class="section">
    <h2>Top 25</h2>
    <table>
      <thead><tr><th>Rank</th><th>Candidate</th><th>Overall</th><th>Stale</th><th>Pseudorandom</th><th>Structured</th><th>Min</th><th>Balance</th><th>Stale U/G</th><th>PRNG U/G</th><th>Struct U/G</th><th>Rejected</th><th>Recipe</th></tr></thead>
      <tbody>{render_rows(top_rows)}</tbody>
    </table>
  </div>
  <div class="section">
    <h2>Bottom 25</h2>
    <table>
      <thead><tr><th>Rank</th><th>Candidate</th><th>Overall</th><th>Stale</th><th>Pseudorandom</th><th>Structured</th><th>Min</th><th>Balance</th><th>Stale U/G</th><th>PRNG U/G</th><th>Struct U/G</th><th>Rejected</th><th>Recipe</th></tr></thead>
      <tbody>{render_rows(bottom_rows)}</tbody>
    </table>
  </div>
</main>
</body>
</html>
"""
    path.write_text(html_text, encoding="utf-8")


def command_screen(args: argparse.Namespace) -> int:
    manifest = load_manifest(Path(args.manifest))
    suites = {
        "stale": load_suite_csv(Path(args.stale)),
        "pseudorandom": load_suite_csv(Path(args.pseudorandom)),
        "structured": load_suite_csv(Path(args.structured)),
    }
    combined = combine_rows(manifest, suites)
    output_dir = Path(args.output_dir)
    write_combined_csv(output_dir / "twist_search_screen_combined.csv", combined)

    shortlist = choose_shortlist(combined, args.shortlist)
    (output_dir / "twist_search_shortlist_ids.txt").write_text(
        "\n".join(str(candidate_id) for candidate_id in shortlist) + "\n",
        encoding="utf-8",
    )

    top_rows = combined[: args.top_report]
    bottom_rows = list(reversed(combined[-args.bottom_report :])) if combined else []
    write_ranked_text(output_dir / "twist_search_top25.txt", "Top Performers", top_rows)
    write_ranked_text(output_dir / "twist_search_bottom25.txt", "Bottom Performers", bottom_rows)
    write_html_report(
        output_dir / "twist_search_screen_report.html",
        "Twist Search Screen Report",
        top_rows,
        bottom_rows,
        top_rows[: min(args.final_count, len(top_rows))],
    )
    print(f"screen_rows={len(combined)}")
    print(f"shortlist={len(shortlist)}")
    print(f"combined_csv={output_dir / 'twist_search_screen_combined.csv'}")
    return 0


def command_finalize(args: argparse.Namespace) -> int:
    manifest = load_manifest(Path(args.manifest))
    suites = {
        "stale": load_suite_dir(Path(args.stale_dir)),
        "pseudorandom": load_suite_dir(Path(args.pseudorandom_dir)),
        "structured": load_suite_dir(Path(args.structured_dir)),
    }
    combined = combine_rows(manifest, suites)
    output_dir = Path(args.output_dir)
    write_combined_csv(output_dir / "twist_search_verified_combined.csv", combined)

    top_rows = combined[: args.top_report]
    bottom_rows = list(reversed(combined[-args.bottom_report :])) if combined else []
    finalists = choose_finalists(combined, manifest, args.final_count)
    final_grade_rows: list[dict[str, Any]] = []
    for row in finalists:
        grade, grade_rank = compute_grade(float(row["overall_score"]))
        final_grade_rows.append(
            {
                "candidate_id": row["candidate_id"],
                "function_name": row["function_name"],
                "grade": grade,
                "grade_rank": grade_rank,
                "composite_score": f"{row['overall_score']:.6f}",
                "stale_score": f"{row['stale_score']:.6f}",
                "pseudorandom_score": f"{row['pseudorandom_score']:.6f}",
                "structured_score": f"{row['structured_score']:.6f}",
                "min_score": f"{row['min_score']:.6f}",
                "balance_score": f"{row['balance_score']:.6f}",
                "stale_uniformity": f"{row['stale_uniformity']:.6f}",
                "stale_spread": f"{row['stale_spread']:.6f}",
                "stale_most_common": row["stale_most_common"],
                "stale_least_common": row["stale_least_common"],
                "stale_histogram_gap": row["stale_histogram_gap"],
                "pseudorandom_uniformity": f"{row['pseudorandom_uniformity']:.6f}",
                "pseudorandom_spread": f"{row['pseudorandom_spread']:.6f}",
                "pseudorandom_most_common": row["pseudorandom_most_common"],
                "pseudorandom_least_common": row["pseudorandom_least_common"],
                "pseudorandom_histogram_gap": row["pseudorandom_histogram_gap"],
                "structured_uniformity": f"{row['structured_uniformity']:.6f}",
                "structured_spread": f"{row['structured_spread']:.6f}",
                "structured_most_common": row["structured_most_common"],
                "structured_least_common": row["structured_least_common"],
                "structured_histogram_gap": row["structured_histogram_gap"],
                "recipe_summary": row["recipe_summary"],
            }
        )

    final_scores_path = output_dir / "twist_search_final16_scores.csv"
    with final_scores_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(final_grade_rows[0].keys()) if final_grade_rows else [
            "candidate_id", "function_name", "grade", "grade_rank", "composite_score",
            "stale_score", "pseudorandom_score", "structured_score", "min_score", "balance_score",
            "stale_uniformity", "stale_spread", "stale_most_common", "stale_least_common",
            "stale_histogram_gap", "pseudorandom_uniformity", "pseudorandom_spread",
            "pseudorandom_most_common", "pseudorandom_least_common",
            "pseudorandom_histogram_gap", "structured_uniformity", "structured_spread",
            "structured_most_common", "structured_least_common", "structured_histogram_gap",
            "recipe_summary"
        ])
        writer.writeheader()
        writer.writerows(final_grade_rows)

    write_ranked_text(output_dir / "twist_search_final16.txt", "Final 16 Diverse Picks", finalists)
    write_ranked_text(output_dir / "twist_search_verified_top25.txt", "Verified Top Performers", top_rows)
    write_ranked_text(output_dir / "twist_search_verified_bottom25.txt", "Verified Bottom Performers", bottom_rows)
    write_html_report(
        output_dir / "twist_search_final_report.html",
        "Twist Search Final Report",
        top_rows,
        bottom_rows,
        finalists,
    )

    print(f"verified_rows={len(combined)}")
    print(f"finalists={len(finalists)}")
    print(f"final_scores={final_scores_path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Aggregate multi-suite twist search results.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    screen_parser = subparsers.add_parser("screen", help="combine broad screening suite runs")
    screen_parser.add_argument("--manifest", default="generated/twist_candidates_manifest.json")
    screen_parser.add_argument("--stale", required=True)
    screen_parser.add_argument("--pseudorandom", required=True)
    screen_parser.add_argument("--structured", required=True)
    screen_parser.add_argument("--output-dir", default="generated/search")
    screen_parser.add_argument("--shortlist", type=int, default=48)
    screen_parser.add_argument("--final-count", type=int, default=16)
    screen_parser.add_argument("--top-report", type=int, default=25)
    screen_parser.add_argument("--bottom-report", type=int, default=25)
    screen_parser.set_defaults(func=command_screen)

    finalize_parser = subparsers.add_parser("finalize", help="combine verified shortlist suite runs")
    finalize_parser.add_argument("--manifest", default="generated/twist_candidates_manifest.json")
    finalize_parser.add_argument("--stale-dir", required=True)
    finalize_parser.add_argument("--pseudorandom-dir", required=True)
    finalize_parser.add_argument("--structured-dir", required=True)
    finalize_parser.add_argument("--output-dir", default="generated/search")
    finalize_parser.add_argument("--final-count", type=int, default=16)
    finalize_parser.add_argument("--top-report", type=int, default=25)
    finalize_parser.add_argument("--bottom-report", type=int, default=25)
    finalize_parser.set_defaults(func=command_finalize)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
