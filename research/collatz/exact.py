"""Independent exact (Python big-int) iteration of digit-rule maps, for verification."""
import rules as R

def make_S(alpha, D, k):
    """Accelerated map on odd z: (alpha z + d)/(1+i)^v, d in D the digit with (1+i)^k | alpha z + d."""
    tab = {}
    for a in range(4):
        for b in range(4):
            if (a + b) % 2:
                az = R.mul(alpha, (a, b))
                c = [d for d in D if R.val(*R.add(az, d)) >= k]
                assert len(c) == 1
                tab[(a, b)] = c[0]
    def S(z):
        d = tab[(z[0] % 4, z[1] % 4)]
        x, y = R.add(R.mul(alpha, z), d)
        v = 0
        while (x + y) % 2 == 0:
            x, y = (x + y) // 2, (y - x) // 2
            v += 1
        return (x, y), v
    return S

def T_steps(alpha, D, k, z):
    """raw T: z/(1+i) if even, else alpha z + d."""
    pass

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
