"""Collect the cycle lists used by convergents.py (convergents.md, Section 3).

usage: convrun.py OUTDIR [B]
  Reads the census logs logs/k345_1e6.txt, logs/k6_1e6.txt, logs/k7_1e6.txt, keeps every rule at levels
  4 to 7 with at least 3 cycle classes, drops one rule of each mirror pair (conjugate alpha and digit set),
  and runs families.py on each at bound B (default 10^5), writing OUTDIR/<tag>.txt and .json.
  Prints one line per rule: tag k alpha gens."""
import sys, os, re, ast, subprocess
from concurrent.futures import ThreadPoolExecutor
import rules as R

HERE = os.path.dirname(os.path.abspath(__file__))

def census_rules():
    out = []
    for f in ('k345_1e6.txt', 'k6_1e6.txt', 'k7_1e6.txt'):
        k = None
        for line in open(os.path.join(HERE, 'logs', f)):
            m = re.match(r'k=(\d+):', line)
            if m: k = int(m.group(1)); continue
            m = re.match(r'N=(\d+) alpha=(\(.*?\)) gens=(\[.*?\]) .*classes=(\d+) ', line)
            if not m or k is None or k < 4: continue
            if int(m.group(4)) < 3: continue
            out.append((k, ast.literal_eval(m.group(2)), ast.literal_eval(m.group(3))))
    return out

def key(k, al, gens):
    D = frozenset(R.mul(u, g) for g in gens for u in R.UNITS)
    return (k, al, D)

def conj(z): return (z[0], -z[1])

def dedup(rules):
    seen, keep = set(), []
    for k, al, gens in rules:
        kk = key(k, al, gens)
        if kk in seen: continue
        mk = (k, conj(al), frozenset(conj(d) for d in kk[2]))
        seen.add(kk); seen.add(mk)
        keep.append((k, al, gens))
    return keep

def run(args):
    i, (k, al, gens), outdir, B = args
    tag = f"k{k}_a{al[0]}_{al[1]}_{i}"
    g = ';'.join(f"{x},{y}" for x, y in gens)
    subprocess.run([sys.executable, os.path.join(HERE, 'families.py'), str(k), f"{al[0]},{al[1]}", g, str(B),
                    os.path.join(outdir, tag)], check=True, capture_output=True, cwd=HERE)
    return f"{tag} {k} {al} {gens}"

if __name__ == '__main__':
    outdir = os.path.abspath(sys.argv[1]); B = int(sys.argv[2]) if len(sys.argv) > 2 else 100000
    os.makedirs(outdir, exist_ok=True)
    rs = dedup(census_rules())
    with ThreadPoolExecutor(4) as ex:
        for line in ex.map(run, [(i, r, outdir, B) for i, r in enumerate(rs)]):
            print(line, flush=True)
