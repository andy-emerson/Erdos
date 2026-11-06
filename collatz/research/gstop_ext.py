"""Checkpointed driver for gstopf (or gstop): runs the drop-below-start check over (B1, B2] in
chunks, appends one result line per finished chunk to the log, and on restart skips every chunk
whose result line is already in the log.  A chunk interrupted midway is simply rerun.

usage: python3 gstop_ext.py rulefile B1 B2 chunk jobs log [binary=./gstopf]
Progress (chunk done, starts per second, ETA) goes to log + ".progress".
A chunk counts as passed only if its line reports fails 0 and cycles 0; any FAIL or cycle-min
line printed by the binary is copied to the log as well.
"""
import os
import re
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

LINE = re.compile(r"^range \((\d+),(\d+)\] starts (\d+) fails (\d+) cycles (\d+)")


def done_chunks(log):
    out = {}
    if os.path.exists(log):
        for s in open(log):
            m = LINE.match(s)
            if m:
                out[(int(m.group(1)), int(m.group(2)))] = s
    return out


def run_chunk(binary, rule, lo, hi):
    p = subprocess.run(["nice", "-n", "10", binary, rule, str(lo), str(hi)], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("chunk (%d,%d] exit %d: %s" % (lo, hi, p.returncode, p.stderr))
    return lo, hi, p.stdout


def main():
    rule, B1, B2, chunk, jobs, log = sys.argv[1], int(float(sys.argv[2])), int(float(sys.argv[3])), \
        int(float(sys.argv[4])), int(sys.argv[5]), sys.argv[6]
    binary = sys.argv[7] if len(sys.argv) > 7 else "./gstopf"
    have = done_chunks(log)
    todo = []
    x = B1
    while x < B2:
        y = min(x + chunk, B2)
        if (x, y) not in have:
            todo.append((x, y))
        x = y
    prog = open(log + ".progress", "a")
    t0 = time.time()
    prog.write("%s start %s (%d,%d] chunk %d jobs %d: %d chunks to do, %d already logged\n" % (
        time.strftime("%Y-%m-%d %H:%M:%S"), rule, B1, B2, chunk, jobs, len(todo), len(have)))
    prog.flush()
    total_norm = sum(y - x for x, y in todo)
    done_norm = 0
    starts = 0
    bad = 0
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        futs = [ex.submit(run_chunk, binary, rule, x, y) for x, y in todo]
        for f in as_completed(futs):
            lo, hi, out = f.result()
            with open(log, "a") as L:
                L.write(out)
            for s in out.splitlines():
                m = LINE.match(s)
                if m:
                    starts += int(m.group(3))
                    if int(m.group(4)) or int(m.group(5)):
                        bad += 1
            done_norm += hi - lo
            el = time.time() - t0
            eta = el * (total_norm - done_norm) / done_norm if done_norm else 0
            prog.write("%s done (%d,%d]  %.1f%%  %.3g starts/s  elapsed %.0f s  ETA %.0f s  bad chunks %d\n" % (
                time.strftime("%H:%M:%S"), lo, hi, 100.0 * done_norm / total_norm, starts / el, el, eta, bad))
            prog.flush()
    prog.write("%s finished, %d starts in %.0f s, bad chunks %d\n" % (time.strftime("%H:%M:%S"), starts, time.time() - t0, bad))
    prog.close()


if __name__ == "__main__":
    main()
