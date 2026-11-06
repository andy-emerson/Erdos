"""Exact cycle-length bounds at the extended verification bounds of cycle-bounds.md, Section 7
(same certified routine as cyclebound_gauss.py; every value re-checked with inner enclosures).

Usage: python3 cyclebound_ext.py > logs/cyclebound_ext.txt
"""
import time
import certarith as C
import cyclebound_gauss as G

# (name, alpha, old B, new B, dmax2, mode)
EXT = [
    ("5+2i, (1+i)^4, commuting", (5, 2), 3 * 10**9, 7600 * 10**6, 5, "units"),
    ("5+2i, (1+i)^4, commuting", (5, 2), 3 * 10**9, 130 * 10**9, 5, "units"),
    ("7+4i, (1+i)^6, commuting", (7, 4), 10**6, 33 * 10**9, 25, "units"),
    ("8+3i, (1+i)^6, commuting", (8, 3), 10**6, 12 * 10**9, 25, "units"),
    ("9+2i, (1+i)^6, commuting", (9, 2), 10**6, 4200 * 10**6, 25, "units"),
    ("10+i, (1+i)^6, commuting", (10, 1), 10**6, 160 * 10**6, 25, "units"),
    ("10+3i, (1+i)^6, commuting", (10, 3), 10**6, 8200 * 10**6, 25, "units"),
    ("11+2i, (1+i)^6, commuting", (11, 2), 10**6, 30 * 10**6, 25, "units"),
    ("-11+4i, (1+i)^7, commuting", (-11, 4), 10**9, 11 * 10**9, 49, "units"),
    ("-12+3i, (1+i)^7, commuting", (-12, 3), 10**9, 4600 * 10**6, 49, "units"),
    ("-11+8i, (1+i)^7, commuting", (-11, 8), 10**9, 10**9, 49, "units"),
    ("-12+7i, (1+i)^7, commuting", (-12, 7), 10**9, 16 * 10**9, 49, "units"),
    ("2+i, (1+i)^2, cone C (on Z[i]/U)", (2, 1), 10**7, 68 * 10**9, 1, "units"),
    ("2-i, (1+i)^2, D={1,i}, eps=1", (2, -1), 10**7, 10**10, 1, "one"),
    ("-1-2i, (1+i)^2, D={1,i}, eps=1", (-1, -2), 10**7, 10**10, 1, "one"),
    ("-2-i, (1+i)^2, D={1,i}, eps=1", (-2, -1), 10**7, 10**10, 1, "one"),
    ("1+2i, (1+i)^2, D={1,i}, eps=1", (1, 2), 10**7, 10**10, 1, "one"),
    ("-2+i, (1+i)^2, D={1,i}, eps=1", (-2, 1), 10**7, 10**10, 1, "one"),
]


def bound(name, alpha, B, dmax2, mode):
    M = G.Map(name, alpha, B, dmax2, mode)
    nj = M.n_joint(3 * 10**7)
    nd = M.n_joint(3 * 10**7, disc=True)
    vmin = C.ceil_frac(nj[0] * M.s_lo_out)
    return M.n_size(), M.n_angle(), nj, M.joint_attained(*nj), nd, vmin


def main():
    print("# cyclebound_ext.py: exact bounds at the old and new verification bounds (box, then disc)")
    for name, alpha, B0, B1, dmax2, mode in EXT:
        t0 = time.time()
        old = bound(name, alpha, B0, dmax2, mode)
        new = bound(name, alpha, B1, dmax2, mode)
        print("%s: B %.3g -> %.3g | old: size %d angle %d box %d disc %d | new: size %d angle %d box %d "
              "(V = %d, W = %d, attained %s) disc %d; V >= %d, steps of T >= %d  [%.1f s]" % (
                  name, B0, B1, old[0][0], old[1][0], old[2][0], old[4][0], new[0][0], new[1][0],
                  new[2][0], new[2][1], new[2][2], new[3], new[4][0], new[5], new[2][0] + new[5], time.time() - t0),
              flush=True)


if __name__ == "__main__":
    main()
