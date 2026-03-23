#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from collections import Counter
from pathlib import Path


def bucket_name(fail_rows: int) -> str:
    if fail_rows == 0:
        return "0 fails"
    if fail_rows == 1:
        return "1 fail"
    if 2 <= fail_rows <= 3:
        return "2-3 fails"
    if 4 <= fail_rows <= 7:
        return "4-7 fails"
    return "8+ fails"


def ordered_bucket_names() -> list[str]:
    return [
        "0 fails",
        "1 fail",
        "2-3 fails",
        "4-7 fails",
        "8+ fails",
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize NIST fail_rows into coarse histogram buckets.")
    parser.add_argument("summary_csv")
    parser.add_argument("--output-txt", required=True)
    parser.add_argument("--output-csv", required=True)
    args = parser.parse_args()

    summary_path = Path(args.summary_csv)
    output_txt = Path(args.output_txt)
    output_csv = Path(args.output_csv)

    bucket_counts: Counter[str] = Counter()
    raw_fail_counts: Counter[int] = Counter()
    total_candidates = 0

    with summary_path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            fail_rows = int(row["fail_rows"])
            total_candidates += 1
            bucket_counts[bucket_name(fail_rows)] += 1
            raw_fail_counts[fail_rows] += 1

    output_txt.parent.mkdir(parents=True, exist_ok=True)
    output_csv.parent.mkdir(parents=True, exist_ok=True)

    with output_txt.open("w", encoding="utf-8") as handle:
        handle.write("NIST failure histogram\n")
        handle.write("======================\n\n")
        handle.write(f"summary_csv={summary_path}\n")
        handle.write(f"total_candidates={total_candidates}\n\n")
        handle.write("Bucketed counts\n")
        handle.write("-------------\n")
        for bucket in ordered_bucket_names():
            handle.write(f"{bucket}: {bucket_counts[bucket]}\n")
        handle.write("\nRaw fail_rows counts\n")
        handle.write("--------------------\n")
        for fail_rows in sorted(raw_fail_counts):
            handle.write(f"{fail_rows}: {raw_fail_counts[fail_rows]}\n")

    with output_csv.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["bucket", "candidate_count"])
        for bucket in ordered_bucket_names():
            writer.writerow([bucket, bucket_counts[bucket]])

    print(f"summary_csv={summary_path}")
    print(f"output_txt={output_txt}")
    print(f"output_csv={output_csv}")
    for bucket in ordered_bucket_names():
        print(f"{bucket}: {bucket_counts[bucket]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
