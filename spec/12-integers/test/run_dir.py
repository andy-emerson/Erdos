"""run_dir.py: time the five direction-multiply methods (src/dirbench.nim), after an idle check, pinned to one core;
Control runs twice, and their difference is the noise."""
import subprocess, json
import run
out = {}
idle = run.wait_idle(False)
for m in ("control", "table", "and3", "byte", "top", "control-2"):
    p = subprocess.run(["taskset", "-c", run.CORE, str(run.BIN / "dirbench"), m.replace("-2", "")],
                       capture_output=True, text=True, check=True)
    _, ns, spread, h = p.stdout.split()
    out[m] = {"ns": float(ns), "spread": float(spread), "sum": h}
c = out["control"]["ns"]
noise = abs(c / out["control-2"]["ns"] - 1)
res = {"idle": idle, "noise": noise, "rows": out, "ratio_to_control": {m: v["ns"] / c for m, v in out.items()}}
(run.HERE / "build" / "dir-results.json").write_text(json.dumps(res, indent=1))
print(json.dumps(res["ratio_to_control"], indent=1), "noise", round(noise, 3))
