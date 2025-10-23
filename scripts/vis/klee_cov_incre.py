#!/usr/bin/env python3
import argparse, csv, json, os, subprocess, sys, math
from datetime import datetime, timezone
from pathlib import Path

def run(cmd, check=True, env=None):
    p = subprocess.run(cmd if isinstance(cmd, list) else cmd.split(),
                       check=False, text=True, capture_output=True, env=env)
    if check and p.returncode != 0:
        raise RuntimeError(f"cmd failed ({p.returncode}): {cmd}\nOUT:\n{p.stdout}\nERR:\n{p.stderr}")
    return p

def list_ktests(outdir: Path):
    ks = list(outdir.glob("test*.ktest")); ks.sort(); return ks

def mtime(path: Path) -> float:
    return path.stat().st_mtime

def export_totals(llvm_cov: str, target: Path, profdata: Path):
    p = run([llvm_cov, "export", f"--instr-profile={profdata}", str(target)], check=False)
    if p.returncode == 0:
        data = json.loads(p.stdout)
        lines = data.get("data", [{}])[0].get("totals", {}).get("lines", {})
        return dict(covered=int(lines.get("covered",0)),
                    count=int(lines.get("count",0)),
                    percent=float(lines.get("percent",0.0)))
    p = run([llvm_cov, "report", str(target), f"-instr-profile={profdata}"], check=True)
    total = next((ln for ln in p.stdout.splitlines() if ln.strip().startswith("TOTAL")), None)
    nums  = [x for x in total.replace('%','').split() if x.replace('.','',1).isdigit()]
    covered, total_lines, pct = map(float, nums[-3:])
    return dict(covered=int(covered), count=int(total_lines), percent=float(pct))

def merge_prof(llvm_profdata: str, inputs, out: Path):
    run([llvm_profdata, "merge", "-sparse", *map(str, inputs), "-o", str(out)], check=True)

def main():
    ap = argparse.ArgumentParser(description="Incremental coverage vs time (post-run).")
    ap.add_argument("--target", required=True)
    ap.add_argument("--outdir", required=True)
    ap.add_argument("--group-sec", type=float, default=0.0,
                    help="Group ktests by this bucket size in seconds; 0 = one event per ktest.")
    ap.add_argument("--klee-replay", default="klee-replay")
    ap.add_argument("--llvm-profdata", default="llvm-profdata")
    ap.add_argument("--llvm-cov", default="llvm-cov")
    ap.add_argument("--no-plot", action="store_true")
    args = ap.parse_args()

    target  = Path(args.target).resolve()
    outdir  = Path(args.outdir).resolve()
    covdir  = outdir / "coverage"
    profraw = covdir / "profraw"
    covdir.mkdir(exist_ok=True); profraw.mkdir(exist_ok=True)

    # 1) sort tests by mtime
    tests = list_ktests(outdir)
    if not tests:
        print("[inc] no ktests found"); return
    tests = sorted(tests, key=mtime)
    first_klee_file = outdir / "assembly.ll"
    t0 = first_klee_file.stat().st_mtime

    # 2) build groups: either per-ktest or bucketed by group-sec
    groups = []  # list of (bucket_elapsed_sec, [tests...])
    if args.group_sec <= 0.0:
        for t in tests:
            groups.append((mtime(t) - t0, [t]))
    else:
        from collections import defaultdict
        buckets = defaultdict(list)
        for t in tests:
            dt = mtime(t) - t0
            k  = math.floor(dt / args.group_sec) * args.group_sec
            buckets[k].append(t)
        for k in sorted(buckets.keys()):
            groups.append((k, buckets[k]))

    # incrementally merge: cum = merge(cum, profraw_of_group)
    tmp = covdir / "tmp"; tmp.mkdir(exist_ok=True)
    cum_prof = None
    csv_path = covdir / "coverage_by_event.csv"
    baseline_written = False
    last_cov = None
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["timestamp_iso","elapsed_sec","covered_lines","total_lines","percent"])
        for elapsed, ts in groups:
            # replay ONLY the tests in this group to produce new raw files
            raws_this = []
            for t in ts:
                pat = str(profraw / f"{t.name}-%p.profraw")
                env = os.environ.copy(); env["LLVM_PROFILE_FILE"] = pat
                run([args.klee_replay, str(target), str(t)], check=False, env=env)
                raws_this += sorted(profraw.glob(f"{t.name}-*.profraw"))
            if not raws_this:
                continue

            # merge new raws to a small bucket.profdata
            bucket_prof = tmp / f"bucket_{int(round(elapsed*1e6))}.profdata"
            merge_prof(args.llvm_profdata, raws_this, bucket_prof)

            # accumulate
            if cum_prof is None:
                cum_prof = tmp / f"cum_{int(round(elapsed*1e6))}.profdata"
                merge_prof(args.llvm_profdata, [bucket_prof], cum_prof)
            else:
                new_cum = tmp / f"cum_{int(round(elapsed*1e6))}.profdata"
                merge_prof(args.llvm_profdata, [cum_prof, bucket_prof], new_cum)
                cum_prof = new_cum

            # report totals for THIS cumulative state
            totals = export_totals(args.llvm_cov, target, cum_prof)
            if not baseline_written:
                w.writerow([
                    datetime.fromtimestamp(t0, timezone.utc).isoformat(),
                    f"{0.0:.6f}",
                    0,
                    totals["count"],    # same total line count as your run
                    "0.00",
                ])
                baseline_written = True
            ts_abs = t0 + elapsed
            w.writerow([datetime.fromtimestamp(ts_abs, timezone.utc).isoformat(),
                        f"{elapsed:.6f}", totals["covered"], totals["count"],
                        f"{totals['percent']:.2f}"])

    # copy final profile for convenience
    if cum_prof:
        try: os.replace(cum_prof, covdir / "coverage.profdata")
        except Exception: pass

    # plot (optional)
    if not args.no_plot:
        try:
            import matplotlib; matplotlib.use("Agg")
            import matplotlib.pyplot as plt
            xs, ys = [], []
            with open(csv_path, "r", encoding="utf-8") as f:
                r = csv.DictReader(f)
                for row in r:
                    xs.append(float(row["elapsed_sec"]))
                    ys.append(float(row["percent"]))  # not covered_lines
            if xs:
                plt.figure(); plt.step(xs, ys, where="post"); plt.scatter(xs, ys, s=10)
                plt.xlim(left=0.0)
                plt.xlabel("Elapsed (s)")
                plt.ylim(0, 100)
                plt.ylabel("Line coverage (%)")
                plt.ylabel("Covered lines")
                plt.title(outdir.name)
                plt.grid(True, alpha=0.35); plt.tight_layout()
                plt.savefig(covdir / "coverage_by_event.png"); plt.close()
        except Exception as e:
            sys.stderr.write(f"[inc] WARN: plotting failed: {e}\n")

    print(f"[inc] wrote {csv_path}")

if __name__ == "__main__":
    main()
