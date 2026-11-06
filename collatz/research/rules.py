"""Rule-table generators for gcz (16 residues mod 4)."""
import itertools, subprocess, os, re

def val(x, y):
    """(1+i)-adic valuation of x+iy (x+iy != 0)."""
    v = 0
    while (x + y) % 2 == 0:
        x, y = (x + y) // 2, (y - x) // 2
        v += 1
        if x == 0 and y == 0: return 99
    return v

def mul(a, b): return (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])
def add(a, b): return (a[0]+b[0], a[1]+b[1])
UNITS = [(1,0),(0,1),(-1,0),(0,-1)]

def table(odd_rule, even=None, M=4):
    """odd_rule(r) -> (alpha, beta, delta) for odd residue r=(a,b) mod M; even residues divide by 1+i."""
    rows = []
    for a in range(M):
        for b in range(M):
            if (a + b) % 2 == 0:
                rows.append((a, b, (1,0), (0,0), (1,1)) if even is None else (a, b) + even((a,b)))
            else:
                rows.append((a, b) + tuple(odd_rule((a, b))))
    return rows

def write(rows, path):
    M = int(round(len(rows) ** 0.5))
    with open(path, 'w') as f:
        f.write(f"MOD {M}\n")
        for (a, b, al, be, de) in rows:
            f.write(f"{a} {b} {al[0]} {al[1]} {be[0]} {be[1]} {de[0]} {de[1]}\n")

def digit_rule(alpha, digits, k):
    """odd z -> alpha z + d, d the unique element of `digits` with alpha z + d = 0 mod (1+i)^k."""
    def f(r):
        az = mul(alpha, r)
        c = [d for d in digits if val(*add(az, d)) >= k]
        # residue r is mod M = (1+i)^4 or (1+i)^6, which determines the class mod (1+i)^k
        assert len(c) == 1, (alpha, r, c)
        return (alpha, c[0], (1, 0))
    return f

def run(rows, B, cap=100000, tag='r', quiet=True):
    path = f"/tmp/claude-0/-home-claude/6c273ef9-4bf4-5ea2-bd9e-566136af1776/scratchpad/{tag}.rule"
    write(rows, path)
    args = ['./gcz', path, str(B), str(cap)] + (['q'] if quiet else [])
    out = subprocess.run(args, capture_output=True, text=True, cwd=os.path.dirname(os.path.abspath(__file__)))
    if out.returncode: raise RuntimeError(out.stderr)
    return out.stdout

def parse(out):
    lines = out.strip().split('\n')
    h = dict(zip(lines[0].split()[::2], map(int, lines[0].split()[1::2])))
    m = re.findall(r'(\w+) (-?[\d.]+)', lines[1])
    cyc = []
    for l in lines[2:]:
        p = l.split()
        cyc.append(dict(min=(int(p[2]), int(p[3])), len=int(p[5]), odd=int(p[7]), count=int(p[9]), maxnorm=float(p[11])))
    return h, lines[1], cyc

def norm(z): return z[0]*z[0]+z[1]*z[1]
def key(z): return (norm(z), z[0], z[1])
def assoc_min(z): return min((mul(u, z) for u in UNITS), key=key)

def group_mod_units(cyc, S=None):
    """For a unit-equivariant map: group raw cycles into unit orbits.
    If S (exact accelerated map, see exact.py) is given, the grouping is done by iterating the
    projective cycle exactly from each raw cycle's min (this also re-verifies every cycle);
    the canonical name is the least associate-normalised element of the projective cycle.
    Returns dicts: min, odd (projective length in odd steps), len (T-steps per projective period),
    count (basin size over all raw copies), ncopies."""
    g = {}
    for c in cyc:
        if c['min'] == (0, 0):
            g.setdefault((0, 0), dict(min=(0, 0), odd=0, len=1, count=0, maxnorm=0, ncopies=0))
            g[(0, 0)]['count'] += c['count']; g[(0, 0)]['ncopies'] += 1
            continue
        if S is None:
            m = assoc_min(c['min']); odd = c['odd']; ln = c['len']
        else:
            z = assoc_min(c['min']); seq = []; ln = 0
            while True:
                seq.append(z); w, v = S(z); ln += 1 + v; z = assoc_min(w)
                if z == seq[0]: break
                assert len(seq) < 10**6
            m = min(seq, key=key); odd = len(seq)
        d = g.setdefault(m, dict(min=m, odd=odd, len=ln, count=0, maxnorm=c['maxnorm'], ncopies=0))
        d['count'] += c['count']; d['ncopies'] += 1
    out = sorted(g.values(), key=lambda d: -d['count'])
    return out

D_A = UNITS + [mul(u, (2, 1)) for u in UNITS]
D_B = UNITS + [mul(u, (1, 2)) for u in UNITS]
