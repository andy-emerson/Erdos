"""realconfirm.py: the literal Collatz maps on Z[sqrt2] at sqrt2^4 (real-quadratic.md, Section 5), checked by
realfd.c literal mode on every odd start with |N| <= B in the box |a| <= 2.5 sqrt B, |b| <= 1.8 sqrt B.
log: logs/realq_confirm.txt"""
import sys, time
import realq as Q, realrun as RN, realbarina as RB
B = int(sys.argv[1]) if len(sys.argv) > 1 else 200000
R = Q.RQ('Zr2'); k = 4; L = []
for a in ((1, 3), (-1, 3)):
    for t in RB.comm_tables(R, a, k, 4, 0, None):
        if ' PASS ' not in RN.run(R, 'A', k, [('x', a, t)], 2, 30000, cap=20000)[0]: continue
        t0 = time.time(); l = RN.run(R, 'A', k, [('x', a, t)], 2, B, cap=100000)[0]
        L.append(f"alpha={R.fmt(a)} {Q.describe(R, t, k)} B={B}: {l[:400]} ({time.time()-t0:.0f} s)")
        print(L[-1], flush=True)
open('logs/realq_confirm.txt', 'w').write("\n".join(L) + "\n")
