"""ringconfirm.py: drop below start to B2 for the first Collatz maps found by ringtest.py (rings-test.md).
usage: python3 ringconfirm.py RING kind F k nsmall tagprefix B1 B2   (kind barina or fd; tag from the log, e.g. a3+s_0)
"""
import sys
import qring as Q, ringrun as RR, ringtest as RT
R = Q.Ring(sys.argv[1]); kind, F, k, ns = sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
tags = sys.argv[6].split(','); B1, B2 = int(float(sys.argv[7])), int(float(sys.argv[8]))
flags = None; K = k
if F == 'P': K, flags, _ = RT.arith_F(R, k)
jobs = []
for a in RT.multipliers(R, k):
    tabs = RT.commuting_tables(R, a, k, ns) if kind == 'barina' else RT.fd_tables(R, F, a, k, ns, flags, K)
    for j, t in enumerate(tabs):
        tag = f"a{Q.fmt(R, a)}_{j}"
        if tag in tags: jobs.append((tag, a, t[0])); print(tag, RT.describe(R, t[3], t[4], k))
for l in RR.run(R, F, K, jobs, 1, B2, N0=B1, cap=10 ** 6, flags=flags): print(l)
