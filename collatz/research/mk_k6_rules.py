"""Rule files for the single-cycle commuting maps at (1+i)^6 (catalog G41), one per mirror pair:
of the maps listed in logs/k6_1e6.txt with one cycle (classes=1), those with Im(alpha) < 0 are kept
(the others are their mirrors, with conjugate orbits).  Digit set names as in catalog D5: A (1-4i, 3-4i), B (1-4i, 3+4i),
C (1+4i, 3-4i), D (1+4i, 3+4i).
usage: python3 mk_k6_rules.py   (writes k6_a<alpha>_<set>.rule, prints the list)"""
import ast, re
import rules as R
for line in open("logs/k6_1e6.txt"):
    m = re.match(r"N=(\d+) alpha=\((-?\d+), (-?\d+)\) gens=(\[.*?\]) unres=(\d+).*?classes=(\d+) ", line)
    if not m or m.group(6) != "1":
        continue
    al = (int(m.group(2)), int(m.group(3)))
    if al[1] > 0:
        continue
    gens = ast.literal_eval(m.group(4))
    name = {(True, True): "A", (True, False): "B", (False, True): "C", (False, False): "D"}[((1, -4) in gens, (3, -4) in gens)]
    D = set(R.mul(u, g) for g in gens for u in R.UNITS)
    rows = R.table(R.digit_rule(al, sorted(D), 6), M=8)
    path = "k6_a%d%+di_%s.rule" % (al[0], al[1], name)
    R.write(rows, path)
    print(path, "N =", m.group(1), "unres at 1e6 =", m.group(5))
