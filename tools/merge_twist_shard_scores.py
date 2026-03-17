#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import html
from collections import Counter
from pathlib import Path


def parse_bool(value: str) -> bool:
    return value.strip().lower() == "true"


def parse_int(value: str) -> int:
    return int(value) if value.strip() else 0


def parse_float(value: str) -> float:
    return float(value) if value.strip() else 0.0


def load_rows(input_dir: Path) -> tuple[list[str], list[dict[str, str]]]:
    header: list[str] | None = None
    rows_by_id: dict[int, dict[str, str]] = {}
    for csv_path in sorted(input_dir.rglob("twist_candidate_scores.csv")):
      with csv_path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        if header is None:
          header = list(reader.fieldnames or [])
        for row in reader:
          candidate_id = parse_int(row["candidate_id"])
          if candidate_id not in rows_by_id:
            rows_by_id[candidate_id] = row
    if header is None:
      raise FileNotFoundError(f"no shard score CSV files found under {input_dir}")
    rows = list(rows_by_id.values())
    rows.sort(
        key=lambda row: (
            -parse_int(row.get("grade_rank", "0")),
            -parse_float(row.get("composite_score", "0")),
            parse_int(row["candidate_id"]),
        )
    )
    return header, rows


def write_csv(path: Path, header: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
      writer = csv.DictWriter(handle, fieldnames=header)
      writer.writeheader()
      writer.writerows(rows)


def write_summary(path: Path, rows: list[dict[str, str]], top_count: int) -> None:
    grade_counts = Counter(row.get("grade", "") for row in rows)
    rejected_count = sum(1 for row in rows if parse_bool(row.get("rejected", "false")))

    lines = [
        "Twist Candidate Sharded Summary",
        "===============================",
        f"evaluated_candidates={len(rows)}",
        f"rejected_candidates={rejected_count}",
        "",
        "grade_counts:",
    ]
    for grade, count in sorted(grade_counts.items(), key=lambda item: item[0]):
      lines.append(f"  {grade}={count}")
    lines.append("")
    lines.append(f"top_{top_count}:")
    for rank, row in enumerate(rows[:top_count], start=1):
      lines.append(
          f"{rank}. candidate_id={row['candidate_id']} function={row['function_name']} "
          f"grade={row.get('grade', '')} composite={row.get('composite_score', '')} "
          f"rejected={row.get('rejected', '')} failure_reason={row.get('failure_reason', '')}"
      )
      lines.append(f"   recipe={row.get('recipe_summary', '')}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_html(path: Path, rows: list[dict[str, str]], max_rows: int) -> None:
    html_rows: list[str] = []
    for row in rows[:max_rows]:
      html_rows.append(
          "<tr>"
          f"<td>{html.escape(row['candidate_id'])}</td>"
          f"<td>{html.escape(row['function_name'])}</td>"
          f"<td>{html.escape(row.get('grade', ''))}</td>"
          f"<td>{html.escape(row.get('composite_score', ''))}</td>"
          f"<td>{html.escape(row.get('rejected', ''))}</td>"
          f"<td>{html.escape(row.get('failure_reason', ''))}</td>"
          f"<td>{html.escape(row.get('recipe_summary', ''))}</td>"
          "</tr>"
      )

    document = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>Twist Candidate Sharded Report</title>
  <style>
    body {{ font-family: Helvetica, Arial, sans-serif; margin: 24px; background: #f7f7f3; color: #1e1e1e; }}
    table {{ border-collapse: collapse; width: 100%; font-size: 13px; background: white; }}
    th, td {{ border: 1px solid #d7d1c7; padding: 8px; vertical-align: top; text-align: left; }}
    th {{ background: #efe8db; }}
    h1 {{ margin-bottom: 8px; }}
    p {{ color: #5f5648; }}
  </style>
</head>
<body>
  <h1>Twist Candidate Sharded Report</h1>
  <p>Showing top {min(max_rows, len(rows))} of {len(rows)} ranked candidates.</p>
  <table>
    <thead>
      <tr>
        <th>ID</th>
        <th>Function</th>
        <th>Grade</th>
        <th>Composite</th>
        <th>Rejected</th>
        <th>Failure</th>
        <th>Recipe</th>
      </tr>
    </thead>
    <tbody>
      {''.join(html_rows)}
    </tbody>
  </table>
</body>
</html>
"""
    path.write_text(document, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Merge shard score CSVs into one ranked report.")
    parser.add_argument("--input-dir", default="generated/shard_measure")
    parser.add_argument("--output-csv", default="generated/twist_candidate_scores.csv")
    parser.add_argument("--output-summary", default="generated/twist_candidate_summary.txt")
    parser.add_argument("--output-html", default="generated/twist_candidate_report.html")
    parser.add_argument("--top-count", type=int, default=200)
    parser.add_argument("--html-max-rows", type=int, default=2000)
    args = parser.parse_args()

    header, rows = load_rows(Path(args.input_dir))
    write_csv(Path(args.output_csv), header, rows)
    write_summary(Path(args.output_summary), rows, args.top_count)
    write_html(Path(args.output_html), rows, args.html_max_rows)

    print(f"merged {len(rows)} unique candidates")
    print(f"scores: {args.output_csv}")
    print(f"summary: {args.output_summary}")
    print(f"html: {args.output_html}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
