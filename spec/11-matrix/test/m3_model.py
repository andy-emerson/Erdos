"""The exact model of spec v0.3 (11-matrix, round m3), in Python integers, for the generator's predicted figures (spec v0.3
§16.9 item 2; rule-m3, Run plan step 0). It shares no code with the C library or the 08 module.

Three parts:
  - the arithmetic: m1's model, m1_model.Arith, used unchanged (TC's cases, order and value arithmetic; every counter
    of the counting build), one Arith per input's entry and one per step, as m2's model; each operation's case is
    recorded for the factored rows;
  - the stores of Hash-V-pack: m2's model, m2_model.Store with the "pack" sizing, unchanged: the build table (h), the
    packed list (l) and the handle fields of every matrix (the factored rows' are the same, v0.3 §16.4 item 1);
  - the factored rows (new), written again here from spec v0.3 §§16.4-16.8 (independently of mh3.c, in the same
    order of work so that every count is exact):
      Pack-primes: each whole number's factorization by sympy.factorint (§16.9 item 2), and the trial division's
        candidates, divisions with remainder and exact divisions in closed form from it (§16.4 item 3);
      Pack-coprime: pass 1 (09's absorb scan), pass 2, the reconciliation at the start of each step, the splits and
        rebuilds, and the leftovers of multiplication's sums, run here in Python integers;
    and, per step, the factored arithmetic: merges and entries compared, the lists of the temporaries and the
    comparisons made forming them, divisibility tests, per-piece gcds (with their gcd work, the core's in Pack-coprime),
    multiplications forming a gcd's value, the new sub-cases, and the obtained gcds the test build checks.
"""
from math import gcd, isqrt

import sympy

import m2_model
import m3_common as C
from m1_model import LIM, Arith, Overflow  # noqa: F401  (m1's, unchanged)

FMAX = 15
SMAX = 32
SUBS = C.SUBCASES


def fbytes(W, E, w):
    """(f)'s bytes (v0.3 §16.3): 4 (W + 1) + E (w/8 + 1)"""
    return 4 * (W + 1) + E * (w // 8 + 1)


# ------------------------------------------------------------------------------------------------- trial division
def trial_counts(fac):
    """Closed form of §16.4 item 3 from a factorization [(p, e)] sorted by p: the loop over d = 2, 3, 5, 7, ... stops
    at the first candidate d* with d*^2 > m(d*), m(d) the product of the prime factors >= d; within (p_{j-1}, p_j]
    m(d) is constant. Returns (candidates, divisions with remainder, exact divisions)."""
    k = len(fac)
    M = [1] * (k + 1)
    for j in range(k - 1, -1, -1):
        M[j] = M[j + 1] * fac[j][0] ** fac[j][1]
    dstar, jstar = None, k
    for j in range(k + 1):
        lo = fac[j - 1][0] + 1 if j else 2
        hi = fac[j][0] if j < k else None
        r = isqrt(M[j]) + 1                         # the smallest r with r^2 > M[j]
        d = max(r, lo)
        if d > 2 and d % 2 == 0:
            d += 1                                  # the next candidate
        if hi is None or d <= hi:
            dstar, jstar = d, j
            break
    cand = 0 if dstar == 2 else (dstar + 1) // 2 - 1
    removed = sum(e for _, e in fac[:jstar])
    return cand, cand + removed, removed


def factor_primes(v):
    return sorted((int(p), int(e)) for p, e in sympy.factorint(v).items()) if v >= 2 else []   # plain ints


# ------------------------------------------------------------------------------------------------- coprime bases
def compact(S):
    out = []
    for x in S:
        if x != 1 and x not in out:
            out.append(x)
    return out


def cbase(S):
    """the coprime base of S by gcds alone, as mh3.c's cbase: (S, gcds, divisions)"""
    S = compact(S)
    ng = nd = 0
    while True:
        changed = False
        n = len(S)
        for i in range(n):
            for j in range(i + 1, n):
                ng += 1
                g = gcd(S[i], S[j])
                if g == 1:
                    continue
                a, b = S[i] // g, S[j] // g
                nd += 2
                S[i] = g
                del S[j]
                assert len(S) + 2 <= SMAX, "refine's working set past 32 entries"
                S += [a, b]
                S = compact(S)
                changed = True
                break
            if changed:
                break
        if not changed:
            return S, ng, nd


def val_div(u, s):
    """(nu_s(u), u / s^nu, tests)"""
    e = t = 0
    while True:
        t += 1
        if u % s:
            return e, u, t
        u //= s
        e += 1


def pieces_of(v, S):
    """v's pieces among S, in S's order, with their exponents, and the divisions made"""
    P, u, nd = [], v, 0
    for s in S:
        k, u, t = val_div(u, s)
        nd += t
        if k:
            P.append((s, k))
    assert u == 1 and P, "a number not a product of its split's pieces"
    return P, nd


def new_fcount():
    return dict(td_cand=0, td_divs=0, td_exact=0, rgcd=0, rdiv=0, splits=0, sgcd=0, sdiv=0, p2tests=0)


def new_acount():
    a = {k: 0 for k in C.FACC_FIELDS}
    a["core_gcd"] = {"add": [0, 0], "mul": [0, 0]}   # Pack-coprime's per-piece gcds: the core's gcd and gcd work
    return a


class FMat:
    """One stored matrix of a factored row: its list (l) in list order, its cells' values row-major, (f) by handle, and
    (Pack-coprime) its base (q)."""

    def __init__(self, name, lst, w, cells, coprime):
        self.name, self.lst, self.w, self.cells, self.coprime = name, list(lst), w, cells, coprime
        self.W = len(lst)
        self.pos = {v: k for k, v in enumerate(lst)}
        self.fac = None
        self.base = []
        self.fc = new_fcount()

    def F(self, v):
        assert v != 0, "a read of the factorization of 0"
        return self.fac[self.pos[v]]

    def E(self):
        return sum(len(x) for x in self.fac)

    def ffig(self):
        E = self.E()
        return dict(W=self.W, E=E, nq=len(self.base), w=self.w, factored=1, bytes_f=fbytes(self.W, E, self.w),
                    bytes_q=len(self.base) * self.w // 8)

    def factor(self):
        """§16.4 items 2-4, after the pack, in list order"""
        fc = self.fc
        if not self.coprime:
            self.fac = []
            for v in self.lst:
                f = factor_primes(v)
                assert len(f) <= FMAX
                self.fac.append(f)
                if v >= 2:
                    c, d, e = trial_counts(f)
                    fc["td_cand"] += c
                    fc["td_divs"] += d
                    fc["td_exact"] += e
            return
        Q = self.base
        for v in self.lst:                           # pass 1, the base
            if v < 2:
                continue
            rest, p = v, 0
            while rest != 1 and p < len(Q):
                q = Q[p]
                fc["rgcd"] += 1
                h = gcd(rest, q)
                if h == 1:
                    p += 1
                elif h == q:
                    while True:
                        fc["rdiv"] += 1
                        if rest % q:
                            break
                        rest //= q
                else:
                    S, ng, nd = cbase([h, q // h])
                    fc["sgcd"] += ng
                    fc["sdiv"] += nd + 1
                    fc["splits"] += 1
                    Q[p] = S[0]
                    Q.extend(S[1:])
            if rest != 1:
                Q.append(rest)
        self.fac = []
        for v in self.lst:                           # pass 2, the factorizations
            m, ent = v, []
            for q in Q:
                if not m > 1:
                    break
                e = 0
                while True:
                    fc["p2tests"] += 1
                    if m % q:
                        break
                    m //= q
                    e += 1
                if e:
                    ent.append((q, e))
            assert m <= 1, "a whole number not a product of the base's pieces"
            assert len(ent) <= FMAX
            self.fac.append(sorted(ent))

    def split(self, q, P, ac):
        """split_in: if the base holds q, q's entry becomes P[0], the others are appended, and (f) is rebuilt"""
        if len(P) == 1 and P[0] == (q, 1):
            return
        if q not in self.base:
            return
        i = self.base.index(q)
        self.base[i] = P[0][0]
        self.base.extend(s for s, _ in P[1:])
        new = []
        for ent in self.fac:
            out = []
            for f, nu in ent:
                if f != q:
                    out.append((f, nu))
                else:
                    out += [(s, nu * e) for s, e in P]
            assert len(out) <= FMAX
            new.append(sorted(out))
        self.fac = new
        ac["rebuilds"] += 1
        ac["rentries"] += self.E()


# ------------------------------------------------------------------------------------------------- the step's work
class Step:
    """One step of a factored row: its operands, its counters (the result's), and the factored arithmetic"""

    def __init__(self, coprime, L, R, test):
        self.coprime, self.L, self.R, self.test = coprime, L, R, test
        self.ac = new_acount()
        self.s = self.p = None                       # the live temporaries' lists (multiplication's sums)

    # -------------------------------------------------------------------------------------------- reconciliation
    def dmake(self, X):
        D = []
        for _, d in X.cells:
            for q, _ in X.F(d):
                if q not in D:
                    D.append(q)
        return D

    def refine(self, X, D):
        ac = self.ac
        made = 0
        i = 0
        while i < len(D):
            j = 0
            while j < len(X.base):
                q, r = X.base[j], D[i]
                ac["rc_gcd"] += 1
                h = gcd(q, r)
                if h == 1 or q == r:
                    j += 1
                    continue
                S, ng, nd = cbase([q, r])
                ac["sp_gcd"] += ng
                ac["sp_div"] += nd
                ac["rc_splits"] += 1
                made += 1
                Pq, n1 = pieces_of(q, S)
                Pr, n2 = pieces_of(r, S)
                ac["sp_div"] += n1 + n2
                self.split_all(q, Pq)
                self.split_all(r, Pr)
                D[i] = Pr[0][0]
                D.extend(s for s, _ in Pr[1:])
                j = 0
            i += 1
        return made

    def split_all(self, q, P):
        self.L.split(q, P, self.ac)
        if self.R is not self.L:
            self.R.split(q, P, self.ac)

    def reconcile(self):
        ac = self.ac
        while True:
            ac["rc_passes"] += 1
            DL, DR = self.dmake(self.L), self.dmake(self.R)
            made = self.refine(self.L, DR)
            made += self.refine(self.R, DL)
            if not made:
                break
        ac["rc_DL"], ac["rc_DR"] = len(DL), len(DR)
        for X, D in ((self.L, DR), (self.R, DL)):    # every compared pair equal or coprime (§16.9 item 5)
            for q in X.base:
                for r in D:
                    assert q == r or gcd(q, r) == 1

    # -------------------------------------------------------------------------------------------- merges, tests
    def merge(self, x, y):
        ac = self.ac
        ac["merges"] += 1
        G, i, j = [], 0, 0
        while i < len(x) and j < len(y):
            p, q = x[i][0], y[j][0]
            ac["mentries"] += 1
            if p == q:
                e = min(x[i][1], y[j][1])
                G.append({"q": p, "e": e, "k": e, "eq": x[i][1] == y[j][1], "hq": 1, "known": False})
                i += 1
                j += 1
                continue
            assert gcd(p, q) == 1, "a merge compared two factors neither equal nor coprime"
            if p < q:
                i += 1
            else:
                j += 1
        assert len(G) <= FMAX
        return G

    def gval(self, G):
        v = 1
        for g in G:
            v *= g["q"] ** g["e"]
            self.ac["fmul"] += g["e"]
        return v

    def gcheck(self, got, a, b):
        assert got == gcd(a, b), "an obtained gcd differs from Euclid's"
        if self.test:
            self.ac["gchecked"] += 1

    def ptests(self, u, q, nu):
        k = 0
        while k < nu:
            self.ac["tests"] += 1
            if u % q:
                break
            u //= q
            self.ac["fmul"] += 1
            k += 1
        return k, u

    def pgcd(self, ph, t, q, nu):
        ac = self.ac
        qn = q ** nu
        ac["fmul"] += nu + 1                         # q^nu, then h times hq
        ac["pgcd"] += 1
        ac["core_gcd"][ph][0] += 1
        ac["core_gcd"][ph][1] += max(abs(t), qn).bit_length()
        return gcd(t, qn)

    def eq_gcd(self, ph, t, G):
        h, u = 1, abs(t)
        for g in G:
            if not g["eq"]:
                continue
            if not self.coprime:
                g["k"], u = self.ptests(u, g["q"], g["e"])
                g["known"] = True
                h *= g["q"] ** g["k"]
            else:
                g["hq"] = self.pgcd(ph, t, g["q"], g["e"])
                h *= g["hq"]
        return h

    def den_gcd(self, ph, t, fb):
        k, u, G = 1, abs(t), []
        for q, e in fb:
            g = {"q": q, "e": e, "k": e, "eq": True, "hq": 1, "known": False}
            G.append(g)
            if not self.coprime:
                g["k"], u = self.ptests(u, q, e)
                g["known"] = True
                k *= q ** g["k"]
            else:
                g["hq"] = self.pgcd(ph, t, q, e)
                k *= g["hq"]
        return k, G

    # -------------------------------------------------------------------------------------------- lists
    def l_sum(self, a, b):
        o, i, j = [], 0, 0
        while i < len(a) or j < len(b):
            if i < len(a) and j < len(b):
                p, q = a[i][0], b[j][0]
                self.ac["lwalk"] += 1
                if p == q:
                    o.append((p, a[i][1] + b[j][1]))
                    i += 1
                    j += 1
                elif p < q:
                    o.append(a[i])
                    i += 1
                else:
                    o.append(b[j])
                    j += 1
            elif i < len(a):
                o.append(a[i])
                i += 1
            else:
                o.append(b[j])
                j += 1
        assert len(o) <= FMAX
        return o

    def walk_g(self, G, j, p, e):
        while j < len(G):
            self.ac["lwalk"] += 1
            if G[j]["q"] < p:
                j += 1
                continue
            if G[j]["q"] == p:
                e -= G[j]["k"]
                j += 1
            break
        return j, e

    def l_sub(self, a, G):
        o, j = [], 0
        for p, e in a:
            j, e = self.walk_g(G, j, p, e)
            if e:
                o.append((p, e))
        return o

    def l_addsub(self, o, a, G):
        o = list(o)
        j = 0
        for p, e in a:
            j, e = self.walk_g(G, j, p, e)
            if not e:
                continue
            at = 0
            while at < len(o):
                self.ac["lwalk"] += 1
                if o[at][0] >= p:
                    break
                at += 1
            if at < len(o) and o[at][0] == p:
                o[at] = (p, o[at][1] + e)
                continue
            assert len(o) < FMAX
            o.insert(at, (p, e))
        return o

    def l_lcm(self, a, b, G):
        o, i, j, g = [], 0, 0, 0
        while i < len(a) or j < len(b):
            if i < len(a) and j < len(b):
                p, q = a[i][0], b[j][0]
                self.ac["lwalk"] += 1
                if p == q:
                    ea, eb = a[i][1], b[j][1]
                    i += 1
                    j += 1
                    while g < len(G) and G[g]["q"] < p:
                        g += 1
                    assert g < len(G) and G[g]["q"] == p
                    e = ea - G[g]["k"] if ea == eb else max(ea, eb)
                    if e:
                        o.append((p, e))
                elif p < q:
                    o.append(a[i])
                    i += 1
                else:
                    o.append(b[j])
                    j += 1
            elif i < len(a):
                o.append(a[i])
                i += 1
            else:
                o.append(b[j])
                j += 1
        assert len(o) <= FMAX
        return o

    # -------------------------------------------------------------------------------------------- leftovers
    def leftovers(self, G):
        """Pack-coprime, a temporary's sum: each hq checked to be a power of q; a leftover splits q (§16.8)"""
        ac = self.ac
        i = 0
        while i < len(G):
            g = G[i]
            if not g["eq"] or g["known"]:
                i += 1
                continue
            u, k = g["hq"], 0
            while True:
                ac["tests"] += 1
                if u % g["q"]:
                    break
                u //= g["q"]
                k += 1
            if u == 1:
                g["k"], g["known"] = k, True
                i += 1
                continue
            q, hq, nu = g["q"], g["hq"], g["e"]
            S, ng, nd = cbase([q, hq])
            ac["sp_gcd"] += ng
            ac["sp_div"] += nd
            ac["lsplits"] += 1
            P, n1 = pieces_of(q, S)
            ac["sp_div"] += n1
            self.split_all(q, P)
            for lst in (self.s, self.p):             # the live temporaries' lists
                if any(f == q for f, _ in lst):
                    e0 = next(e for f, e in lst if f == q)
                    rest = [x for x in lst if x[0] != q] + [(s, e0 * e) for s, e in P]
                    lst[:] = sorted(rest)
            del G[i]
            left, n2 = hq, 0
            for s, e in P:
                f, left, t = val_div(left, s)
                n2 += t
                G.append({"q": s, "e": nu * e, "k": f, "eq": True, "hq": 1, "known": True})
            ac["sp_div"] += n2
            assert left == 1
            G.sort(key=lambda x: x["q"])
            assert len(G) <= FMAX
            i = 0

    def list_reduced(self, G, fb_ref, fd_ref, den):
        """the result's list of a reduced sum of temporaries (fb_ref, fd_ref: the live lists)"""
        if self.coprime:
            self.leftovers(G)
        if den:
            return self.l_sub(fb_ref, G)
        return self.l_lcm(fb_ref, fd_ref, G)

    # -------------------------------------------------------------------------------------------- an addition
    def addition(self, kase, x, y, fb, fd, temp):
        """add_body's factored work for the case TC took. x, y: the operands (m, d); fb, fd: F(b), F(d) (lists; for
        temporaries the live lists themselves); temp: the result is a temporary (its list is formed and returned)."""
        ac = self.ac
        a, b, xneg = abs(x[0]), x[1], x[0] < 0
        c, d, yneg = abs(y[0]), y[1], y[0] < 0
        sx = -1 if xneg else 1
        sy = -1 if yneg else 1
        if kase in ("add_zero", "add_int_both"):
            return []
        if kase in ("add_zero_x", "add_int_x"):
            return list(fd) if temp else None
        if kase in ("add_zero_y", "add_int_y"):
            return list(fb) if temp else None
        if kase == "add_cross":
            return self.l_sum(fb, fd) if temp else None
        if kase == "add_den":
            t = sx * a + sy * c
            k, G = self.den_gcd("add", t, fb)
            self.gcheck(k, t, d)
            return self.list_reduced(G, fb, fd, True) if temp else None
        if kase == "add_num":
            G = self.merge(fb, fd)
            g = self.gval(G)
            self.gcheck(g, b, d)
            ac["num_cop" if not G else "num_eq" if any(e["eq"] for e in G) else "num_uneq"] += 1
            if g != 1:
                t = sx * (d // g) + sy * (b // g)
                h = self.eq_gcd("add", t, G)
                self.gcheck(h, t, g)
            return self.list_reduced(G, fb, fd, False) if temp else None
        assert kase == "add_general"
        G = self.merge(fb, fd)
        g = self.gval(G)
        self.gcheck(g, b, d)
        ac["gen_cop" if not G else "gen_eq" if any(e["eq"] for e in G) else "gen_uneq"] += 1
        if g != 1:
            t = sx * a * (d // g) + sy * c * (b // g)
            h = self.eq_gcd("add", t, G)
            self.gcheck(h, t, g)
        return self.list_reduced(G, fb, fd, False) if temp else None

    # -------------------------------------------------------------------------------------------- a product
    def product(self, kase, x, y):
        """mul_cells' factored work for the case TC took: the product's denominator list"""
        L, R = self.L, self.R
        a, b, c, d = abs(x[0]), x[1], abs(y[0]), y[1]
        if kase in ("mul_zero", "mul_int_both", "mul_both"):
            return []
        if kase == "mul_one_x":
            return list(R.F(d))
        if kase == "mul_one_y":
            return list(L.F(b))
        if kase == "mul_den":
            return [(q, 2 * e) for q, e in L.F(b)]
        if kase == "mul_num":
            return self.l_sum(L.F(b), R.F(d))
        if kase in ("mul_cross_ad", "mul_int_y"):    # k = gcd(c, b): merge F(c), F(b)
            fb = L.F(b)
            G = self.merge(R.F(c), fb)
            self.gcheck(self.gval(G), c, b)
            return self.l_sub(fb, G)
        if kase in ("mul_cross_cb", "mul_int_x"):    # k = gcd(a, d): merge F(a), F(d)
            fd = R.F(d)
            G = self.merge(L.F(a), fd)
            self.gcheck(self.gval(G), a, d)
            return self.l_sub(fd, G)
        assert kase == "mul_general"
        fb, fd = L.F(b), R.F(d)
        G = self.merge(L.F(a), fd)
        G2 = self.merge(R.F(c), fb)
        g1, g2 = self.gval(G), self.gval(G2)
        self.gcheck(g1, a, d)
        self.gcheck(g2, c, b)
        self.ac["mgen_cop" if not G and not G2 else "mgen_one" if not G or not G2 else "mgen_two"] += 1
        return self.l_addsub(self.l_sub(fb, G2), fd, G)


def denominator_of(lst):
    v = 1
    for q, e in lst:
        v *= q ** e
    return v


# ------------------------------------------------------------------------------------------------- a job
def predict(op, n, inputs):
    """Run the model over a job of set `op`: the inputs' entered pairs (as entered, row-major), in order. Returns every
    matrix's values, the counters of each matrix per phase (TC's and Hash-V-pack's; and per factored row, with the
    operations' gcds and gcd work as that row obtains them), each step's cross tests and cases, Hash-V-pack's stores
    (after each input's build and at every matrix's end), packings and the sha256 of every list, handle fields and
    build table (h), and per factored row ("primes", "coprime") every factoring (in order), every matrix's (f) and (q)
    figures after its build and at its end, each step's factored-arithmetic counts (gchecked: the test build's), and
    the sha256 of every matrix's final (f) and (q) as the gate dumps them."""
    names_in = C.SET_INPUTS[op]
    arith, vals, cases = {}, {}, {}
    for name, X in zip(names_in, inputs):
        a = Arith()
        vals[name] = [a.enter(m, d) for m, d in X]
        arith[name] = a
    for kind, x, y, z in C.SET_STEPS[op]:
        a = Arith()
        seq = []
        if kind == "add":
            out = []
            for i in range(n * n):
                r, k = a.addition(vals[x][i], vals[y][i], True)
                out.append(r)
                seq.append(k)
        else:
            out = []
            for i in range(n):
                for j in range(n):
                    s = (0, 1)
                    for k in range(n):
                        p, k1 = a.multiply(vals[x][i * n + k], vals[y][k * n + j])
                        s, k2 = a.addition(s, p, False)
                        seq += [k1, k2]
                    out.append(s)
        vals[z] = out
        arith[z] = a
        cases[z] = seq
    ncase = {}
    for z in C.SET_MATRICES[op]:
        for k, v in arith[z].ncase.items():
            ncase[k] = ncase.get(k, 0) + v
    steps = {z: {"ncase": dict(arith[z].ncase), "cross": arith[z].cross, "cross_match": arith[z].cross_match,
                 "cross_reads": arith[z].cross_reads_V} for _, _, _, z in C.SET_STEPS[op]}
    counters = {k: arith[k].per_phase() for k in arith}
    # Hash-V-pack's stores (m2's model, the "pack" sizing)
    S, built, packs = {}, {}, []
    for name in C.SET_MATRICES[op]:
        st = S[name] = m2_model.Store(name, n, n, "pack", name in names_in)
        for m, d in vals[name]:
            st.store_cell(m, d)
        st.finish()
        packs.append(st.pack_rec)
        if name in names_in:
            built[name] = st.figures()
        assert [st.value(i) for i in range(n * n)] == vals[name]
    pack = {"stor_built": built, "stor_end": {k: S[k].figures() for k in S}, "growths": [], "packs": packs,
            "tables": {k: C.sha_ints(S[k].vals()) for k in S}, "cells": {k: C.sha_ints(S[k].fields) for k in S},
            "hb": {k: C.sha_ints(S[k].hb) for k in S}}
    fac = {}
    for row in ("primes", "coprime"):
        fac[row] = factored(op, n, vals, S, cases, row == "coprime", counters)
    return {"values": vals, "counters": counters, "steps": steps, "ncase": ncase, "nops": C.nops(op, n),
            "sizings": {"pack": pack}, "factored": fac}


def factored(op, n, vals, S, cases, coprime, counters):
    """one factored row over the job: its factorings, steps and final (f) and (q)"""
    names_in = C.SET_INPUTS[op]
    M, facs, built, end, acc = {}, [], {}, {}, {}

    def make(name):
        st = S[name]
        x = M[name] = FMat(name, st.list, st.w, vals[name], coprime)
        x.factor()
        facs.append(dict(name=name, **{k: v for k, v in x.ffig().items() if k != "factored"}, **x.fc))
        return x

    for name in names_in:
        make(name)
        built[name] = M[name].ffig()
    for kind, x, y, z in C.SET_STEPS[op]:
        L, R = M[x], M[y]
        stp = Step(coprime, L, R, True)
        if coprime:
            stp.reconcile()
        seq = cases[z]
        if kind == "add":
            for i in range(n * n):
                u, v = vals[x][i], vals[y][i]
                k = seq[i]
                need = k in ("add_den", "add_num", "add_general")
                stp.addition(k, u, v, L.F(u[1]) if need else None, R.F(v[1]) if need else None, False)
        else:
            it = iter(seq)
            for i in range(n):
                for j in range(n):
                    s, ls = (0, 1), []
                    stp.s = ls
                    for kk in range(n):
                        u, v = vals[x][i * n + kk], vals[y][kk * n + j]
                        k1, k2 = next(it), next(it)
                        lp = stp.product(k1, u, v)
                        p = mul_value(u, v)
                        assert denominator_of(lp) == p[1]
                        stp.p = lp
                        res = stp.addition(k2, s, p, ls, lp, True)
                        s = add_value(s, p)
                        ls[:] = res
                        assert denominator_of(ls) == s[1]
                    assert s == vals[z][i * n + j]
        make(z)
        acc[z] = stp.ac
    for name in C.SET_MATRICES[op]:
        end[name] = M[name].ffig()
    # the counters as this row's binary prints them: TC's, with the operations' gcds and gcd work its own
    rc = {}
    for z, c in counters.items():
        rc[z] = {ph: dict(v) for ph, v in c.items()}
        if z in acc:
            for ph in ("add", "mul"):
                g = rc[z].setdefault(ph, {})
                g["gcd"], g["gcd_work"] = acc[z]["core_gcd"][ph]
    facc = {z: {k: v for k, v in a.items() if k != "core_gcd"} for z, a in acc.items()}
    dumps = {}
    for name, x in M.items():
        flat = []
        for ent in x.fac:
            flat.append(len(ent))
            for q, e in ent:
                flat += [q, e]
        dumps[name] = {"ff": C.sha_ints(flat), "fq": C.sha_ints(x.base)}
    return {"factorings": facs, "ffig_built": built, "ffig_end": end, "facc": facc, "counters": rc, "dumps": dumps,
            "E": {k: M[k].E() for k in M}, "nq": {k: len(M[k].base) for k in M}}


def mul_value(x, y):
    from fractions import Fraction
    f = Fraction(*x) * Fraction(*y)
    return (f.numerator, f.denominator)


def add_value(x, y):
    from fractions import Fraction
    f = Fraction(*x) + Fraction(*y)
    return (f.numerator, f.denominator)
