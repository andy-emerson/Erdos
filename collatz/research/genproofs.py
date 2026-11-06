"""Checks for general-proofs.md.

Re-derives by computation the statements copied into general-proofs.md:
  2  moduli that carry multipliers (Z, Z[i], Z[omega]), small k exhaustively
  3  the mirror when conjugation fixes pi (Z[omega], random tables)
  4  the Markov matrix of T' (doubly stochastic, irreducible, uniform weights)
  5  multiplier 3 at modulus 2 (fixed digit, cycles of 3x+1 and 3x-1)
  6  least positive digits on Z (fixed points by brute force)
  7  the 5+2omega score and drift
  8  the unit-table cycles for -3, -2+3i, -3+2i at (1+i)^3
Output: logs/genproofs.txt (run as: python3 genproofs.py > logs/genproofs.txt).
"""
import math
import random
from fractions import Fraction

random.seed(20261106)

# ---------- Gaussian integers as pairs ----------
def gmul(x, y):
    return (x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0])

def gadd(x, y):
    return (x[0] + y[0], x[1] + y[1])

def gN(x):
    return x[0] ** 2 + x[1] ** 2

def gdiv_pi(x):
    # (a+bi)/(1+i) = ((a+b) + (b-a)i)/2, valid when a+b even
    a, b = x
    assert (a + b) % 2 == 0
    return ((a + b) // 2, (b - a) // 2)

def godd(x):
    return (x[0] + x[1]) % 2 == 1

GU = [(1, 0), (0, 1), (-1, 0), (0, -1)]
PI = (1, 1)

def gpow(x, n):
    r = (1, 0)
    for _ in range(n):
        r = gmul(r, x)
    return r

def gdivisible(x, m):
    # is x divisible by m in Z[i]?
    n = gN(m)
    p = gmul(x, (m[0], -m[1]))
    return p[0] % n == 0 and p[1] % n == 0

def gS_unit_table(alpha, k, z):
    """S for the map with D = U at modulus pi^k (k = 3): digit is the unit e with pi^k | alpha z + e."""
    pk = gpow(PI, k)
    w0 = gmul(alpha, z)
    ds = [e for e in GU if gdivisible(gadd(w0, e), pk)]
    assert len(ds) == 1, (alpha, z, ds)
    w = gadd(w0, ds[0])
    v = 0
    while not godd(w):
        w = gdiv_pi(w)
        v += 1
    return w, v, ds[0]

# ---------- Eisenstein integers a + b w, w^2 = -1 - w ----------
def emul(x, y):
    a, b = x
    c, d = y
    # (a + b w)(c + d w) = ac + (ad + bc) w + bd w^2 = (ac - bd) + (ad + bc - bd) w
    return (a * c - b * d, a * d + b * c - b * d)

def eadd(x, y):
    return (x[0] + y[0], x[1] + y[1])

def eN(x):
    a, b = x
    return a * a - a * b + b * b

def econj(x):
    # conj(w) = w^2 = -1 - w
    a, b = x
    return (a - b, -b)

def eodd(x):
    return not (x[0] % 2 == 0 and x[1] % 2 == 0)

EU = [(1, 0), (0, 1), (-1, -1), (-1, 0), (0, -1), (1, 1)]  # 1, w, w^2, -1, -w, -w^2

def emod(x, m):
    # reduce coordinates mod integer m (pi = 2, moduli 2^k are rational integers)
    return (x[0] % m, x[1] % m)

def eS(alpha, mod, table, z):
    """S for (alpha, k, d) on Z[omega] with modulus mod = 2^k, table: class -> digit."""
    w = eadd(emul(alpha, z), table[emod(z, mod)])
    if w == (0, 0):
        return (0, 0)
    while not eodd(w):
        w = (w[0] // 2, w[1] // 2)
    return w


def section(t):
    print()
    print("== " + t)


# ---------- 2. moduli that carry multipliers ----------
def check_moduli():
    section("2. Moduli that carry multipliers")
    # Z: odd norms in (2^k, 2^(k+1))
    for k in range(0, 12):
        lo, hi = 2 ** k, 2 ** (k + 1)
        has = any(n % 2 == 1 for n in range(lo + 1, hi))
        print("Z k=%d window (%d,%d) holds an odd integer: %s" % (k, lo, hi, has))
    # Z[i]: norms of odd a+bi in (2^k, 2^(k+1))
    for k in range(0, 15):
        lo, hi = 2 ** k, 2 ** (k + 1)
        r = int(math.isqrt(hi)) + 1
        norms = sorted({a * a + b * b for a in range(-r, r + 1) for b in range(-r, r + 1)
                        if (a + b) % 2 == 1 and lo < a * a + b * b < hi})
        if k >= 2:
            j = k // 2
            wit = (2 ** j, 1) if k % 2 == 0 else (2 ** j + 1, 2 ** j)
            ok = godd(wit) and lo < gN(wit) < hi
            assert all(n % 4 == 1 for n in norms)
            print("Z[i] k=%d: %d odd norms in window, all 1 mod 4; witness %s norm %d in window: %s"
                  % (k, len(norms), wit, gN(wit), ok))
        else:
            print("Z[i] k=%d: odd norms in window: %s" % (k, norms))
    # Z[omega]: norms of odd a+b w in (4^k, 4^(k+1/3))
    for k in range(0, 9):
        lo, hi = 4 ** k, 4 ** (k + Fraction(1, 3))
        hif = float(hi)
        r = int(math.isqrt(int(hif))) * 2 + 2
        norms = sorted({eN((a, b)) for a in range(-r, r + 1) for b in range(-r, r + 1)
                        if eodd((a, b)) and lo < eN((a, b)) < hif})
        assert all(n % 2 == 1 for n in norms)
        wit = (2 ** k, -1)
        ok = eodd(wit) and lo < eN(wit) < hif
        print("Z[w] k=%d window (%d, %.3f): %d odd norms (smallest %s); witness 2^k - w norm %d in window: %s"
              % (k, lo, hif, len(norms), norms[:3], eN(wit), ok))
    # 5 is not a norm in Z[omega]
    print("5 a norm in Z[w]:", any(eN((a, b)) == 5 for a in range(-5, 6) for b in range(-5, 6)))
    print("bound 2^k + 1 < (4^(1/3) - 1) 4^k for k = 2..20:",
          all(2 ** k + 1 < (4 ** (1 / 3) - 1) * 4 ** k for k in range(2, 21)),
          "; fails at k = 1:", not (2 + 1 < (4 ** (1 / 3) - 1) * 4))


# ---------- Eisenstein tables at modulus 2^k ----------
def eis_odd_classes(mod):
    return [(a, b) for a in range(mod) for b in range(mod) if eodd((a, b))]

def eis_random_table(alpha, mod, bound=6):
    """random digit table: for each odd class c pick a random odd element of the class of -alpha c."""
    table = {}
    for c in eis_odd_classes(mod):
        target = emod(emul((-alpha[0], -alpha[1]), c), mod)
        cands = [(a, b) for a in range(-bound * mod, bound * mod + 1) for b in range(-bound * mod, bound * mod + 1)
                 if emod((a, b), mod) == target]
        table[c] = random.choice(cands)
    return table


# ---------- 3. the mirror when conjugation fixes pi ----------
def check_mirror_eta1():
    section("3. Mirror when sigma fixes pi (Z[omega], conjugation, pi = 2)")
    tests = 0
    for alpha in [(5, 2), (5, 1), (5, 0), (7, 3), (9, 4)]:
        for mod in [4, 8]:
            for _ in range(5):
                table = eis_random_table(alpha, mod)
                # mirrored table d^s(z) = conj(d(conj z))
                mtable = {c: econj(table[emod(econj(c), mod)]) for c in table}
                ma = econj(alpha)
                # check it is a map: 2^k | conj(alpha) z + d^s(z)
                for c in mtable:
                    w = eadd(emul(ma, c), mtable[c])
                    assert w[0] % mod == 0 and w[1] % mod == 0
                for a in range(-30, 31):
                    for b in range(-30, 31):
                        z = (a, b)
                        if not eodd(z):
                            continue
                        lhs = eS(ma, mod, mtable, z)
                        rhs = econj(eS(alpha, mod, table, econj(z)))
                        assert lhs == rhs, (alpha, mod, z, lhs, rhs)
                        tests += 1
    print("S_conj(alpha) = conj o S_alpha o conj on odd z with |a|,|b| <= 30, random tables:", tests, "points, no mismatch")
    print("conj(5+2w) =", econj((5, 2)), "; -w(5+3w) =", emul((0, -1), (5, 3)))


# ---------- 4. the Markov matrix ----------
def markov_check(name, classes_k, classes_2k, red_k, T_prime, q, k, Nalpha):
    """classes_k: list of classes mod pi^k (representatives); classes_2k: reps mod pi^2k;
    red_k: reduce an element to its class rep mod pi^k; T_prime: one step of T'."""
    idx = {c: i for i, c in enumerate(classes_k)}
    n = len(classes_k)
    P = [[Fraction(0)] * n for _ in range(n)]
    cnt = [0] * n
    for z in classes_2k:
        i = idx[red_k(z)]
        cnt[i] += 1
    for z in classes_2k:
        i = idx[red_k(z)]
        j = idx[red_k(T_prime(z))]
        P[i][j] += Fraction(1, cnt[i])
    rows = all(sum(P[i]) == 1 for i in range(n))
    cols = all(sum(P[i][j] for i in range(n)) == 1 for j in range(n))
    # irreducibility: reachability
    reach_all = True
    for s in range(n):
        seen = {s}
        frontier = [s]
        while frontier:
            nf = []
            for i in frontier:
                for j in range(n):
                    if P[i][j] > 0 and j not in seen:
                        seen.add(j)
                        nf.append(j)
            frontier = nf
        reach_all &= len(seen) == n
    # entries
    vals = sorted({P[i][j] for i in range(n) for j in range(n) if P[i][j] > 0})
    prod = (1.0 / q) ** (1.0 / q) * (Nalpha / q ** k) ** ((q - 1) / q)
    print("%s: %d classes, %d subclasses each, rows sum to 1: %s, columns sum to 1: %s, irreducible: %s, entries %s, product %.4f"
          % (name, n, cnt[0], rows, cols, reach_all, [str(v) for v in vals], prod))
    return rows and cols and reach_all

def check_markov():
    section("4. Markov matrix of T' (Lemma 2.5)")
    # Z[i], pi = 1+i. Represent classes mod pi^k by reduced representatives.
    def gred(z, k):
        # canonical representative mod pi^k. pi^(2j) Z[i] = 2^j Z[i];
        # pi^(2j+1) Z[i] is generated by (2^j, 2^j) and (2^(j+1), 0).
        j, odd = divmod(k, 2)
        a, b = z
        if not odd:
            m = 2 ** j
            return (a % m, b % m)
        m = 2 ** j
        t = b // m
        a, b = a - t * m, b - t * m
        return (a % (2 * m), b)
    def classes_g(k):
        seen = set()
        B = 2 ** (k // 2 + 1)
        for a in range(-B, B + 1):
            for b in range(-B, B + 1):
                seen.add(gred((a, b), k))
        assert len(seen) == 2 ** k, (k, len(seen))
        return sorted(seen)
    allok = True
    for k, alphas in [(2, [(2, 1), (2, -1)]), (3, [(3, 0), (3, 2)]), (4, [(5, 2), (3, 0), (1, 4), (4, 3)])]:
        ck = classes_g(k)
        c2k = classes_g(2 * k)
        for alpha in alphas:
            assert godd(alpha), alpha
            # smallest digits: any odd element of the class of -alpha z works for the matrix
            dig = {}
            for c in ck:
                if godd(c):
                    target = gred(gmul((-alpha[0], -alpha[1]), c), k)
                    dig[c] = target
            pk = gpow(PI, k)
            def Tp(z, alpha=alpha, dig=dig, k=k, pk=pk):
                if godd(z):
                    w = gadd(gmul(alpha, z), dig[gred(z, k)])
                    for _ in range(k):
                        w = gdiv_pi(w)
                    return w
                return gdiv_pi(z)
            allok &= markov_check("Z[i] alpha=%s at (1+i)^%d" % (alpha, k), ck, c2k, lambda z, k=k: gred(z, k), Tp, 2, k, gN(alpha))
    # Z[omega], pi = 2, q = 4: classes mod 2^k are coordinate pairs mod 2^k
    for k, alphas in [(1, [(1, 1), (3, 0)]), (2, [(5, 2), (5, 1), (5, 0)]), (3, [(9, 0), (11, 5)])]:
        mod = 2 ** k
        ck = [(a, b) for a in range(mod) for b in range(mod)]
        c2k = [(a, b) for a in range(mod * mod) for b in range(mod * mod)]
        for alpha in alphas:
            def Tp(z, alpha=alpha, mod=mod):
                if eodd(z):
                    target = emod(emul((-alpha[0], -alpha[1]), z), mod)
                    w = eadd(emul(alpha, z), target)
                    assert w[0] % mod == 0 and w[1] % mod == 0
                    return (w[0] // mod, w[1] // mod)
                return (z[0] // 2, z[1] // 2)
            allok &= markov_check("Z[w] alpha=%s at 2^%d" % (alpha, k), ck, c2k, lambda z, mod=mod: emod(z, mod), Tp, 4, k, eN(alpha))
    # Z, pi = 2
    for k, ms in [(1, [3]), (2, [5, 7]), (3, [11, 13]), (4, [21, 29])]:
        mod = 2 ** k
        ck = list(range(mod))
        c2k = list(range(mod * mod))
        for m in ms:
            def Tp(z, m=m, mod=mod):
                if z % 2:
                    w = m * z + ((-m * z) % mod)
                    return w // mod
                return z // 2
            allok &= markov_check("Z m=%d at 2^%d" % (m, k), ck, c2k, lambda z, mod=mod: z % mod, Tp, 2, k, m)
    print("all doubly stochastic and irreducible:", allok)


# ---------- integer accelerated map ----------
def zS(m, k, digit, x):
    w = m * x + digit(x)
    if w == 0:
        return 0
    while w % 2 == 0:
        w //= 2
    return w

def zcycle(m, k, digit, x, cap=10000):
    seen = []
    y = x
    for _ in range(cap):
        if y in seen:
            return seen[seen.index(y):]
        seen.append(y)
        y = zS(m, k, digit, y)
    return None


# ---------- 5. multiplier 3 at modulus 2 ----------
def check_mult3():
    section("5. Multiplier 3 at modulus 2")
    for delta in [1, -1, 3, -3, 5, -5, 7, 9, -11]:
        s = zS(3, 1, lambda x: delta, delta)
        print("3x%+d: S(%d) = %d (3*d + d = %d)" % (delta, delta, s, 4 * delta))
    for delta in [1, -1]:
        cyc = set()
        for x in range(-201, 202, 2):
            c = zcycle(3, 1, lambda y: delta, x)
            cyc.add(tuple(sorted(c)))
        print("3x%+d, odd |x| <= 201, accelerated cycles:" % delta, sorted(cyc, key=lambda c: (len(c), c)))
    # full T cycle lengths through -5 and -17 under 3x+1
    def T(x):
        return x // 2 if x % 2 == 0 else 3 * x + 1
    for x0 in [-5, -17]:
        orb = [x0]
        y = T(x0)
        while y != x0:
            orb.append(y)
            y = T(y)
        print("3x+1 cycle through %d: %d steps of T: %s" % (x0, len(orb), orb))


# ---------- 6. least positive digits ----------
def check_positive_digits():
    section("6. Least positive digits on Z")
    bad = 0
    total = 0
    for k in range(1, 9):
        mod = 2 ** k
        for m in range(mod + 1, 2 * mod, 2):
            dig = lambda x, m=m, mod=mod: (-m * x) % mod
            fixed_pos = [x for x in range(1, 4 * mod, 2) if zS(m, k, dig, x) == x]
            fixed_neg = [x for x in range(-1, -4 * mod, -2) if zS(m, k, dig, x) == x]
            anti = [x for x in range(-4 * mod + 1, 4 * mod, 2) if zS(m, k, dig, x) == -x]
            pred_pos = [x for x in range(1, 4 * mod, 2) if x * (2 * mod - m) < mod]
            pred_neg = [x for x in range(-1, -4 * mod, -2) if (-x) * (m - mod) < mod]
            ok = fixed_pos == pred_pos and fixed_neg == pred_neg and not anti
            ok &= (3 in fixed_pos) == (3 * m > 5 * mod)
            ok &= (-3 in fixed_neg) == (3 * m < 4 * mod)
            ok &= 1 in fixed_pos and -1 in fixed_neg
            total += 1
            if not ok:
                bad += 1
                print("mismatch", k, m, fixed_pos, pred_pos, fixed_neg, pred_neg, anti)
    print("k = 1..8, every odd m in (2^k, 2^(k+1)):", total, "maps, fixed points as predicted, no S(x) = -x, 3 fixed iff m > (5/3)2^k, -3 fixed iff m < (4/3)2^k; mismatches:", bad)


# ---------- 7. 5+2omega score ----------
def check_score():
    section("7. Score and drift of 5+2w at modulus 4")
    M, Nn = 16, eN((5, 2))
    sc = Fraction(1, M) - Fraction(1, Nn)
    drift = math.log2(Nn / M) - math.log2(4) / 3
    print("N(5+2w) = %d, M = N(4) = %d, score = %s = %.4f, drift = %.4f bits = %.4f natural" % (Nn, M, sc, float(sc), drift, drift * math.log(2)))
    print("upper end of score range 1/M - 4^(-k-1/3) at k = 2: %.4f" % (1 / 16 - 4 ** (-2 - 1 / 3)))


# ---------- 8. unit-table cycles at (1+i)^3 ----------
def check_unit_cycles():
    section("8. Unit-table cycles at (1+i)^3")
    pi3, pi4 = gpow(PI, 3), gpow(PI, 4)
    print("pi^3 =", pi3, " pi^4 =", pi4)
    print("pi^4 - (-3) =", gadd(pi4, (3, 0)), "; pi^3 - (-3+2i) =", gadd(pi3, (3, -2)), "; pi^3 - (-2+3i) =", gadd(pi3, (2, -3)))
    for alpha, start in [((-3, 0), (2, 1)), ((-2, 3), (3, 0)), ((-3, 2), (3, 0))]:
        z = start
        orb, vs = [], []
        while True:
            orb.append(z)
            z, v, d = gS_unit_table(alpha, 3, z)
            vs.append(v)
            if z == start:
                break
            assert len(orb) < 1000
        units_on = [x for x in orb if gN(x) == 1]
        norms = sorted({gN(x) for x in orb})
        # closure under units: is the set U*orb invariant and unit-free?
        P = {gmul(u, x) for u in GU for x in orb}
        inv = all(gS_unit_table(alpha, 3, x)[0] in P for x in P)
        print("alpha=%s: cycle of %d elements from %s: %s" % (alpha, len(orb), start, orb))
        print("   exponents %s, norms %s, units on cycle: %s, U*cycle has %d elements, invariant: %s"
              % (vs, norms, units_on, len(P), inv))
    # the alpha = -3+2i cycle as unit multiples of 3, 4+i, 5+2i, 2-3i
    alpha = (-3, 2)
    z = (3, 0)
    orb = []
    while True:
        orb.append(z)
        z = gS_unit_table(alpha, 3, z)[0]
        if z == (3, 0):
            break
    target = {gmul(u, x) for u in GU for x in [(3, 0), (4, 1), (5, 2), (2, -3)]}
    print("alpha=-3+2i: cycle set equals the unit multiples of 3, 4+i, 5+2i, 2-3i:", set(orb) == target)
    # the commuting multiplier 3 and 3 +- 2i cycles of Theorem G26 for comparison
    for alpha, start in [((3, 0), (-2, -1)), ((3, 2), (-3, 0)), ((3, -2), (-3, 0))]:
        z = start
        seq = [z]
        for _ in range(4):
            z = gS_unit_table(alpha, 3, z)[0]
            seq.append(z)
        print("alpha=%s from %s: %s" % (alpha, start, seq))


if __name__ == "__main__":
    check_moduli()
    check_mirror_eta1()
    check_markov()
    check_mult3()
    check_positive_digits()
    check_score()
    check_unit_cycles()
