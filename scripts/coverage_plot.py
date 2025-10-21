#!/usr/bin/env python3
"""
coverage_plot.py

Plot coverage over time from one or more CSV files.
Each CSV is expected to have columns:
    - timestamp_iso (ISO8601 string, optional if 'elapsed_sec' exists)
    - elapsed_sec (float seconds since start; preferred if present)
    - covered_lines (int)
    - total_lines (int)
    - percent (float, optional; if missing it will be computed)

Usage:
    python scripts/coverage_plot.py brainstorm_out_7/coverage/coverage_by_event.csv brainstorm_out_8/coverage/coverage_by_event.csv \
        --out coverage_4.png \
        --time-unit seconds \
        --y percent \
        --labels "fread + two memcmp checks, with symbolic branches" \
        --title "Coverage over Time" \
        --out-dir plots/

Notes:
- If multiple rows share the same elapsed time, the last one is kept.
- Coverage is coerced to be monotonically non-decreasing over time.
"""
import argparse
import os
from typing import List
import pandas as pd
import matplotlib.pyplot as plt

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Plot coverage over time from CSV logs.")
    p.add_argument("csvs", nargs="+", help="One or more CSV files.")
    p.add_argument("--labels", type=str, default=None,
                   help="Comma-separated series labels matching CSV order. Defaults to file basenames.")
    p.add_argument("--out", type=str, default="coverage_plot.png",
                   help="Output PNG path (default: coverage_plot.png).")
    p.add_argument("--time-unit", type=str, default="hours",
                   choices=["s","sec","secs","second","seconds",
                            "m","min","mins","minute","minutes",
                            "h","hr","hrs","hour","hours"],
                   help="Unit for x-axis (seconds/minutes/hours). Default: hours.")
    p.add_argument("--y", type=str, default="percent", choices=["percent", "lines"],
                   help="Y-axis: 'percent' for line coverage %, or 'lines' for covered_lines. Default: percent.")
    p.add_argument("--title", type=str, default="Coverage over Time",
                   help="Plot title.")
    p.add_argument("--out-dir", type=str, default=".",
               help="Directory to place the output file if --out is a relative name (default: .).")
    return p.parse_args()

def unit_to_scale_and_label(unit: str):
    u = unit.lower()
    if u in ["s","sec","secs","second","seconds"]:
        return 1.0, "Time (seconds)"
    if u in ["m","min","mins","minute","minutes"]:
        return 1.0/60.0, "Time (minutes)"
    # default hours
    return 1.0/3600.0, "Time (hours)"

def load_and_prepare(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    # Drop exact duplicates
    df = df.drop_duplicates()
    # Prefer elapsed_sec; otherwise derive from timestamp_iso (relative to first row)
    if "elapsed_sec" in df.columns:
        t = df["elapsed_sec"].astype(float)
    elif "timestamp_iso" in df.columns:
        ts = pd.to_datetime(df["timestamp_iso"], utc=True, errors="coerce")
        # Use the first non-null as t0
        t0 = ts.dropna().iloc[0]
        t = (ts - t0).dt.total_seconds()
    else:
        raise ValueError(f"{path}: needs either 'elapsed_sec' or 'timestamp_iso' column.")
    df = df.assign(elapsed_sec=t)
    # Keep last record per time instant to de-jitter logging
    df = df.sort_values("elapsed_sec").drop_duplicates(subset=["elapsed_sec"], keep="last")
    # Compute percent if missing
    if "percent" not in df.columns or df["percent"].isna().any():
        if "covered_lines" in df.columns and "total_lines" in df.columns:
            # Avoid div-by-zero
            df["percent"] = (100.0 * df["covered_lines"].astype(float)
                             / df["total_lines"].replace({0: pd.NA}).astype(float)).fillna(0.0)
        else:
            raise ValueError(f"{path}: missing 'percent' and cannot compute without covered_lines/total_lines.")
    # Ensure monotone coverage in time
    df["percent_monotone"] = df["percent"].astype(float).cummax()
    if "covered_lines" in df.columns:
        df["covered_lines_monotone"] = df["covered_lines"].astype(float).cummax()
    else:
        df["covered_lines_monotone"] = pd.NA
    return df

def main():
    args = parse_args()
    labels: List[str]
    if args.labels:
        labels = [s.strip() for s in args.labels.split(",")]
        if len(labels) != len(args.csvs):
            raise SystemExit("The number of labels must match the number of CSV files.")
    else:
        labels = [os.path.splitext(os.path.basename(p))[0] for p in args.csvs]

    scale, xlabel = unit_to_scale_and_label(args.time_unit)

    plt.figure(figsize=(7.5, 5.0), dpi=160)
    for path, label in zip(args.csvs, labels):
        df = load_and_prepare(path)
        x = df["elapsed_sec"] * scale
        if args.y == "percent":
            y = df["percent_monotone"]
            ylabel = "Line Coverage (%)"
        else:
            # Prefer monotone covered_lines if present; else derive from percent * total_lines / 100
            if "covered_lines_monotone" in df.columns and df["covered_lines_monotone"].notna().any():
                y = df["covered_lines_monotone"]
            else:
                if "total_lines" in df.columns:
                    y = (df["percent_monotone"] * df["total_lines"].astype(float) / 100.0)
                else:
                    raise ValueError(f"{path}: cannot determine covered lines (missing columns).")
            ylabel = "Covered Lines"
        # Plot each series; don't set explicit colors to keep defaults
        plt.plot(x, y, marker="o", label=label)

    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(args.title)
    plt.grid(True, which="both", linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    out = args.out
    if not os.path.isabs(out):
        os.makedirs(args.out_dir, exist_ok=True)
        out = os.path.join(args.out_dir, out)
    plt.savefig(out)
    print(f"Saved: {out}")

if __name__ == "__main__":
    main()
