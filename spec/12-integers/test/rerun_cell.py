"""rerun_cell.py: re-time one job and operation for every build (the same method as run.py), e.g. 1 divexact."""
import sys, json, statistics
import run, gen
k, op = int(sys.argv[1]), sys.argv[2]
work = run.HERE / "build" / "results" / "work"
idle = run.wait_idle(False)
rows = {r: run.time_cell(r, k, op, work, gen.N, run.MIN_NS, run.REPS) for r in run.ROWS}
noise = abs(rows["control"]["median"] / rows["control-2"]["median"] - 1) if "median" in rows["control-2"] else None
print(json.dumps({"job": k, "op": op, "idle": idle, "noise": noise,
                  "rows": {r: {kk: v for kk, v in d.items() if kk != "ns"} for r, d in rows.items()}}, indent=1))
