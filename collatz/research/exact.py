"""Independent exact (Python big-int) iteration of digit-rule maps, for verification."""
import rules as R

def make_S(alpha, D, k):
    """Accelerated map on odd z: (alpha z + d)/(1+i)^v, d the unique digit in D with (1+i)^k | alpha z + d.
    The digit is found directly for each z (no residue table), as an independent check of gcz."""
    def S(z):
        az = R.mul(alpha, z)
        c = [d for d in D if R.val(*R.add(az, d)) >= k]
        assert len(c) == 1, (z, c)
        x, y = R.add(az, c[0])
        v = 0
        while (x + y) % 2 == 0:
            x, y = (x + y) // 2, (y - x) // 2
            v += 1
        return (x, y), v
    return S

def proj_cycle(S, z, nmax=10**5):
    """cycle reached from z for the map on associate classes; returns (preperiod, cycle) or None."""
    f = lambda w: (R.assoc_min(S(w)[0]), 0)
    return orbit(f, R.assoc_min(z), nmax)

def orbit(S, z, nmax=10**6):
    """iterate accelerated map from odd z until a repeat; return (preperiod, cycle list, maxnorm)."""
    seen = {}
    seq = []
    mx = 0
    while z not in seen and len(seq) < nmax:
        seen[z] = len(seq); seq.append(z); mx = max(mx, R.norm(z))
        z, _ = S(z)
    if z not in seen: return None
    i = seen[z]
    return i, seq[i:], mx
