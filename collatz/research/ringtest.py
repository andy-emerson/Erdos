"""ringtest.py: ring-level findings tested across rings (rings-test.md).

usage: python3 ringtest.py inv RING [Kmax]          invariants, smallest digits and ties by modulus
       python3 ringtest.py window RING [kmax]       window, odd norms, multipliers up to units
       python3 ringtest.py barina RING k nsmall B   Barina-type maps (commuting tables, on R/U),
                                                    each orbit digit among the nsmall smallest of its class
       python3 ringtest.py fd RING F k nsmall B     Collatz-type maps on the fundamental domain F,
                                                    each digit among the nsmall smallest of its class
       python3 ringtest.py literal RING k B [nsmall] [maxtables]
                                                    Barina-type maps read literally on R (all associates of
                                                    each multiplier, tables with digits among nsmall smallest)
       python3 ringtest.py confirm RING F k B2 tag alpha digits...   drop below start to B2 for one map
Rings: Z Zi Zw Zs2 Z7 Z7b Z11 Zr2 (qring.py). Engine: ringfd.c via ringrun.py.
Logs: logs/rings_<RING>_<what>.txt
"""
import sys, os, math, itertools, collections
import qring as Q
import ringrun as RR

HERE = os.path.dirname(os.path.abspath(__file__))
CANON = {'Z': 'O', 'Zi': 'Q', 'Zw': 'S', 'Zr2': 'E'}


def canonF(R): return CANON.get(R.name, 'R')


def log(R, what, lines):
    p = os.path.join(HERE, 'logs', f'rings_{R.name}_{what}.txt')
    open(p, 'w').write("\n".join(lines) + "\n")
    print("\n".join(lines))
    print(f"(written {p})")


# ---------------------------------------------------------------- invariants
def window(R, k):
    q = R.q
    return q ** k, q ** (k + 1 / (q - 1))


def odd_elements_absN(R, hi):
    """odd elements with |N| < hi, one per class of associates (canonical F)"""
    F = canonF(R)
    out = set()
    if R.dim == 1:
        for a in range(1, int(hi) + 1, 2): out.add((a, 0))
        return out
    if R.real:
        s = math.sqrt(hi)
        for a in range(-int(2.5 * s) - 3, int(2.5 * s) + 4):
            for b in range(-int(1.8 * s) - 3, int(1.8 * s) + 4):
                z = (a, b)
                if z != (0, 0) and abs(R.N(z)) < hi and R.odd(z) and Q.zr2_inE(z): out.add(z)
        return out
    for z in R.elements(int(hi) + 1):
        if R.N(z) < hi and R.odd(z): out.add(Q.rep(R, F, z))
    return out


def multipliers(R, k):
    lo, hi = window(R, k)
    return sorted([z for z in odd_elements_absN(R, hi) if R.absN(z) > lo],
                  key=lambda z: (R.absN(z), Q.fmt(R, z)))


def least_by_class(R, K):
    bound = 2 * R.q ** K + 8
    while True:
        bc = R.by_class(K, bound)
        if all(i in bc for i in R.odd_classes(K)): break
        bound *= 2
    out = {}
    for i in R.odd_classes(K):
        L = bc[i]; m = R.size(L[0])
        out[i] = [z for z in L if R.size(z) == m]
    return out


def two_factorization(R):
    if R.dim == 1: return "2 is prime (pi = 2), e = 1, f = 1"
    p2 = R.mul(R.pi, R.pi)
    u = R.div_exact((2, 0), p2)
    if u is not None and R.is_unit(u):
        return f"2 = ({Q.fmt(R, u)}) pi^2, ramified, e = 2, f = 1"
    if R.pi == (2, 0): return "2 inert, pi = 2, e = 1, f = 2"
    o = R.div_exact((2, 0), R.pi)
    assoc = o is not None and R.div_exact(o, R.pi) is not None and R.is_unit(R.div_exact(o, R.pi))
    return f"2 = pi * ({Q.fmt(R, o)}), the cofactor is prime and not an associate of pi: split, e = 1, f = 1"


def eta(R):
    """sigma(pi) = eta pi for conjugation sigma, or None if sigma moves the prime"""
    if R.dim == 1: return (1, 0)
    e = R.div_exact(R.conj(R.pi), R.pi)
    if e is not None and R.is_unit(e): return e
    return None


def cmd_inv(R, Kmax=5):
    L = [f"# ring {R.name}: t={R.t} n={R.n} (tau^2 = t tau - n), pi = {Q.fmt(R, R.pi)}, q = {R.q}"]
    L.append(f"factorization of 2: {two_factorization(R)}")
    if R.U:
        L.append(f"units: {[Q.fmt(R, u) for u in R.U]}, |U| = {len(R.U)}, orders {[R.unit_order(u) for u in R.U]}")
        L.append(f"kappa = max v_pi(u - 1) = {R.kappa()} (v by unit: {[(Q.fmt(R, u), R.vpi(R.sub(u, (1, 0)))) for u in R.U if u != (1, 0)]})")
        fr = [K for K in range(0, Kmax + 1) if K > 0 and R.acts_freely(K)]
        L.append(f"U acts freely on odd classes mod pi^K from K = {fr[0] if fr else None}")
    else:
        L.append("units: +-(1+r)^n, infinite; v_pi(-1 - 1) = %d, so maps commuting with +-1 need k > %d" % (R.vpi((-2, 0)), R.vpi((-2, 0))))
        for K in range(1, 8):
            x = Q.EPS; n = 1
            while R.idx(x, K) != R.idx((1, 0), K) and n < 1000: x = Q.zr2_mul(x, Q.EPS); n += 1
            y = (-x[0], -x[1]);
            L.append(f"  K={K}: eps^{n} = 1 mod pi^K (eps^{n} = {Q.fmt(R, x)}); kernel of U -> (R/pi^K)^x is nontrivial")
    e = eta(R)
    L.append(f"conjugation: sigma(pi) = {Q.fmt(R, R.conj(R.pi))}; " +
             (f"sigma(pi) = eta pi with eta = {Q.fmt(R, e)}" if e else "sigma(pi) is not a unit times pi (sigma moves the prime)"))
    if R.dim == 2 and not R.real:
        L.append(f"lattice: tau = {R.cx((0, 1)):.4f}; norm form a^2 + {R.t}ab + {R.n}b^2, discriminant {R.t**2 - 4*R.n}")
    if R.real:
        L.append("lattice (Minkowski, sigma1, sigma2): digits ordered by a^2 + 2b^2 = (sigma1^2 + sigma2^2)/2, the lattice of Z[sqrt-2] up to scale")
    L.append("smallest digits by modulus: K, odd classes, tied classes (size: count), smallest tables, commuting smallest tables, orbits of U")
    for K in range(1, Kmax + 1):
        lb = least_by_class(R, K)
        oc = R.odd_classes(K)
        tied = [i for i in oc if len(lb[i]) > 1]
        tsz = collections.Counter((R.size(lb[i][0]), len(lb[i])) for i in tied)
        ntab = 1
        for i in oc: ntab *= len(lb[i])
        if R.U and R.acts_freely(K):
            ncomm = 1
            for o in R.unit_orbits(K): ncomm *= len(lb[o[0][1]])
            norb = len(R.unit_orbits(K))
        else:
            ncomm = 0; norb = (len(R.unit_orbits(K)) if R.U else '-')
        L.append(f"  K={K}: {len(oc)} odd classes, {len(tied)} tied {dict(tsz)}, {ntab} smallest tables, {ncomm} commuting, {norb} orbits; "
                 f"least elements {sorted(set(R.size(lb[i][0]) for i in oc))}")
    log(R, 'inv', L)


def cmd_window(R, kmax=5):
    L = [f"# window q^k < |N(alpha)| < q^(k + 1/(q-1)) for {R.name}, q = {R.q}"]
    for k in range(0, kmax + 1):
        lo, hi = window(R, k)
        ms = multipliers(R, k)
        norms = sorted(set(R.absN(z) for z in ms))
        ints = [m for m in range(int(lo) + 1, int(math.ceil(hi))) if m < hi]
        L.append(f"k={k}: ({lo}, {hi:.3f}); integers {ints[:12]}{'...' if len(ints) > 12 else ''}; norms of odd elements {norms}; "
                 f"{len(ms)} multipliers up to units: " + ", ".join(f"{Q.fmt(R, z)} ({R.absN(z)})" for z in ms[:40]))
        if R.real and ms:
            s2 = 2 ** ((k + 1) / 2)
            bal = []
            for z in ms:
                # associates z * eps^j, j in a small range: is some associate contracting in both embeddings?
                good = []
                for j in range(-4, 5):
                    w = z
                    for _ in range(abs(j)): w = Q.zr2_mul(w, Q.EPS if j > 0 else Q.EPSINV)
                    s1, s2_ = R.cx(w)
                    if abs(s1) < s2 and abs(s2_) < s2: good.append(Q.fmt(R, w))
                bal.append(f"{Q.fmt(R, z)}: {good if good else 'none'}")
            L.append(f"   associates with |sigma_1|, |sigma_2| < 2^((k+1)/2) = {s2:.3f}: " + "; ".join(bal))
    log(R, 'window', L)


# ---------------------------------------------------------------- tables
def cand_lists(R, alpha, k, nsmall, classes=None):
    """for each odd z-class c (mod pi^k), the nsmall smallest digits in the class of -alpha c"""
    bound = 2 * R.q ** k + 8
    while True:
        bc = R.by_class(k, bound)
        if all(i in bc and len(bc[i]) >= nsmall for i in R.odd_classes(k)): break
        bound *= 2
    out = {}
    for i in (classes if classes is not None else R.odd_classes(k)):
        c = R.classrep(i, k)
        j = R.idx(R.neg(R.mul(alpha, c)), k)
        L = bc[j]
        m = R.size(L[0])
        least = [z for z in L if R.size(z) == m]
        cut = R.size(L[nsmall - 1])          # all digits of size <= the size of the nsmall-th (ties at the cut kept)
        out[i] = ([z for z in L if R.size(z) <= cut], least)
    return out


def commuting_tables(R, alpha, k, nsmall, nsample=0, seed=1):
    """commuting tables: per orbit a digit among the nsmall smallest of the class; yields (table, total size, is_smallest).
    nsample > 0: every smallest table plus nsample random tables instead of the full product"""
    orbs = R.unit_orbits(k)
    cl = cand_lists(R, alpha, k, nsmall, [o[0][1] for o in orbs])
    lists = [cl[o[0][1]][0] for o in orbs]
    leasts = [cl[o[0][1]][1] for o in orbs]
    res = []
    if nsample:
        import random
        rnd = random.Random(seed)
        chs = set(itertools.product(*leasts))
        while len(chs) < nsample + len(set(itertools.product(*leasts))): chs.add(tuple(rnd.choice(l) for l in lists))
        it = sorted(chs)
    else:
        it = itertools.product(*lists)
    for ch in it:
        tab = {}
        for o, d in zip(orbs, ch):
            for u, j in o: tab[j] = R.mul(u, d)
        smallest = all(d in l for d, l in zip(ch, leasts))
        res.append((tab, sum(R.size(d) for d in ch) * len(R.U), smallest, ch, [o[0][1] for o in orbs]))
    res.sort(key=lambda r: (not r[2], r[1]))
    return res


def arith_F(R, k):
    """an arithmetic fundamental domain: odd z with z mod pi^m in a transversal H of the image of U in (R/pi^m)^x,
    m = kappa + 1, H a subgroup complement when one exists. Returns (K, flags, description)."""
    m = R.kappa() + 1
    K = max(k, m)
    oc = R.odd_classes(m)
    Ubar = {R.idx(u, m) for u in R.U}
    def mulc(i, j): return R.idx(R.mul(R.classrep(i, m), R.classrep(j, m)), m)
    G = oc
    target = len(G) // len(Ubar)
    one = R.idx((1, 0), m)
    best = None
    # subgroups generated by at most two elements
    def gen(gs):
        S = {one}
        while True:
            new = {mulc(a, g) for a in S for g in gs} | S
            if new == S: return S
            S = new
    for a in G:
        for b in G:
            H = gen([a, b])
            if len(H) == target and not (H & Ubar) - {one}:
                cand = sorted(H)
                if best is None or cand < best: best = cand
    if best is None:
        # transversal: smallest class index in each coset
        seen = set(); best = []
        for i in sorted(G):
            if i in seen: continue
            best.append(i); seen |= {mulc(i, u) for u in Ubar}
        desc = 'transversal (no subgroup complement)'
    else:
        desc = 'subgroup complement'
    H = set(best)
    flags = [0] * R.nclasses(K)
    for i in R.odd_classes(K):
        if R.idx(R.classrep(i, K), m) in H: flags[i] = 1
    return K, flags, f"z mod pi^{m} in {{{', '.join(Q.fmt(R, R.classrep(h, m)) for h in sorted(H))}}} ({desc})"


def fd_tables(R, F, alpha, k, nsmall, flags=None, K=None, nsample=0, seed=1):
    """tables at modulus pi^k for the classes meeting F, each digit among the nsmall smallest; lifted to K.
    nsample > 0: every smallest table plus nsample random tables instead of the full product"""
    K = K or k
    if F == 'P':
        meet = sorted({R.idx(R.classrep(i, K), k) for i in R.odd_classes(K) if flags[i]})
    else:
        meet = R.odd_classes(k)
    cl = cand_lists(R, alpha, k, nsmall)
    lists = [cl[i][0] for i in meet]
    leasts = [cl[i][1] for i in meet]
    res = []
    if nsample:
        import random
        rnd = random.Random(seed)
        chs = set(itertools.product(*leasts)); n0 = len(chs)
        while len(chs) < nsample + n0: chs.add(tuple(rnd.choice(l) for l in lists))
        it = sorted(chs)
    else:
        it = itertools.product(*lists)
    for ch in it:
        tab = {i: cl[i][0][0] for i in R.odd_classes(k)}
        for i, d in zip(meet, ch): tab[i] = d
        smallest = all(d in l for d, l in zip(ch, leasts))
        res.append((RR.lift_table(R, tab, k, K) if K != k else tab, sum(R.size(d) for d in ch), smallest, ch, meet))
    res.sort(key=lambda r: (not r[2], r[1]))
    return res


def fd_multipliers(R, k):
    """one multiplier per class of associates (Prop 2.3(1) of fundamental-domain.md), conjugates included"""
    return multipliers(R, k)


def describe(R, ch, meet, k):
    return ", ".join(f"[{Q.fmt(R, R.classrep(i, k))}]->{Q.fmt(R, d)}" for i, d in zip(meet, ch))


def run_search(R, F, k, alphas_tables, B, flags=None, K=None, label=''):
    K = K or k
    jobs = []; info = {}
    for alpha, tabs in alphas_tables:
        for j, (tab, tot, sm, ch, meet) in enumerate(tabs):
            tag = f"a{Q.fmt(R, alpha)}_{j}"
            jobs.append((tag, alpha, tab)); info[tag] = (alpha, tot, sm, ch, meet)
    out = []
    for s in range(0, len(jobs), 2000):
        out += RR.run(R, F, K, jobs[s:s + 2000], 0, B, flags=flags)
    return out, info


def summarize(R, F, k, out, info, B, title):
    L = [title]
    per = collections.OrderedDict(); per_s = collections.Counter(); tot = collections.Counter(); tot_s = collections.Counter()
    reasons = collections.Counter()
    rows = []
    for l in out:
        tag = l.split()[0]; a, t, sm, ch, meet = info[tag]
        tot[a] += 1; tot_s[a] += sm
        if ' PASS ' in l:
            per[a] = per.get(a, 0) + 1; per_s[a] += sm
            rows.append((R.absN(a), not sm, t, tag, a, ch, meet, l.split(' PASS ')[1]))
        else:
            r = l.split(' FAIL ')[1]
            reasons['cycle' if r.startswith('cycle') else '1 not periodic' if r.startswith('1 not') else 'orbit of 1 escapes' if r.startswith('orbit') else 'escape/cap/zero'] += 1
    L.append(f"maps tested {len(out)}; Collatz at {B}: {len(rows)}; failures {dict(reasons)}")
    for a in tot:
        L.append(f"  alpha {Q.fmt(R, a)} (|N| {R.absN(a)}): tables {tot[a]} (smallest {tot_s[a]}), Collatz {per.get(a, 0)} (smallest {per_s[a]})")
    rows.sort()
    for r in rows:
        L.append(f"  PASS {r[3]} alpha={Q.fmt(R, r[4])} smallest={not r[1]} total digit size {r[2]} table {describe(R, r[5], r[6], k)} | {r[7]}")
    return L, rows


def cmd_barina(R, k, nsmall, B, nsample=0):
    if not R.acts_freely(k):
        log(R, f'barina_k{k}', [f"# k={k}: units do not act freely on odd classes mod pi^{k} (k <= kappa = {R.kappa()}), no commuting table"]); return
    F = canonF(R)
    at = []
    for a in multipliers(R, k):
        at.append((a, commuting_tables(R, a, k, nsmall, nsample)))
    out, info = run_search(R, F, k, at, B)
    smp = f" (all smallest tables plus {nsample} random tables per multiplier)" if nsample else ''
    L, rows = summarize(R, F, k, out, info, B, f"# Barina-type maps on {R.name} at pi^{k}: commuting tables, each orbit digit among the {nsmall} smallest{smp}; checked on R/U through F={F}, full classification to |N| <= {B}")
    log(R, f'barina_k{k}_n{nsmall}' + (f'_s{nsample}' if nsample else ''), L)


def cmd_fd(R, F, k, nsmall, B, nsample=0):
    flags = None; K = k; extra = ''
    if F == 'P':
        K, flags, extra = arith_F(R, k)
    at = [(a, fd_tables(R, F, a, k, nsmall, flags, K, nsample)) for a in fd_multipliers(R, k)]
    if nsample: extra += f' (all smallest tables plus {nsample} random tables per multiplier)'
    out, info = run_search(R, F, k, at, B, flags=flags, K=K)
    L, rows = summarize(R, F, k, out, info, B, f"# Collatz-type maps on {R.name}, F={F} {extra}, modulus pi^{k} (table read mod pi^{K}), each digit among the {nsmall} smallest; full classification to |N| <= {B}")
    if F == 'P':
        # how many of the P tables are commuting tables (their commuting extension is a residue-class table mod pi^k)
        L.append(f"  (on P the commuting extension D(uf) = u d(f) is a residue-class table mod pi^{K})")
    log(R, f'fd_{F}_k{k}_n{nsmall}' + (f'_s{nsample}' if nsample else ''), L)


def cmd_literal(R, k, B, nsmall=1, maxtables=0):
    """Barina-type maps read literally on R: every associate of every multiplier, tables with digits among the
    nsmall smallest per class (smallest = all tie-breaks when nsmall = 1)."""
    lines = [f"# literal Barina-type maps on {R.name} at pi^{k}: all associates, digits among the {nsmall} smallest per class, every odd z with |N| <= {B}"]
    jobs = []; info = {}
    for a0 in multipliers(R, k):
        for u in R.U:
            a = R.mul(u, a0)
            cl = cand_lists(R, a, k, nsmall)
            oc = R.odd_classes(k)
            lists = [cl[i][0] if nsmall > 1 else cl[i][1] for i in oc]
            n = 0
            for ch in itertools.product(*lists):
                tab = dict(zip(oc, ch))
                tag = f"a{Q.fmt(R, a)}_{n}"; n += 1
                jobs.append((tag, a, tab)); info[tag] = (a, tab, RR.commutes(R, tab, k) if R.acts_freely(k) else False, ch, oc)
                if maxtables and n >= maxtables: break
    out = []
    for s in range(0, len(jobs), 2000): out += RR.run(R, 'A', k, jobs[s:s + 2000], 2, B)
    npass = 0; cnt = collections.Counter()
    for l in out:
        tag = l.split()[0]; a, tab, com, ch, oc = info[tag]
        ok = ' PASS ' in l
        npass += ok
        if ok:
            ntc = int(l.split('trivial cycles ')[1].split(':')[0]); cnt[(com, ntc)] += 1
        s1 = RR.apply_S(R, a, tab, k, (1, 0))[0]
        pred = ''
        if com and ok:
            # period p of [1] and eps = S^p(1); predicted number of trivial cycles |U|/ord(eps) (A6, T2, T3)
            x = s1; p = 1
            while not R.is_unit(x) and p < 10000: x = RR.apply_S(R, a, tab, k, x)[0]; p += 1
            pr = len(R.U) // R.unit_order(x)
            cnt[('pred_ok', pr == ntc)] += 1
            pred = f" [p={p} eps={Q.fmt(R, x)} predicted {pr}{' OK' if pr == ntc else ' MISMATCH'}]"
        lines.append(f"{tag} alpha={Q.fmt(R, a)} commuting={com} S(1)={Q.fmt(R, s1)} digits {describe(R, ch, oc, k)} | {l.split(tag, 1)[1].strip()}{pred}")
    lines.insert(1, f"maps {len(out)}, Collatz on R (literal cycles) {npass}; (commuting, trivial-cycle count) among them: {dict(cnt)}")
    log(R, f'literal_k{k}_n{nsmall}', lines)


def cmd_confirm(R, F, k, B2, B1, tag, alpha, digits):
    """drop below start to B2 for one map given by its digits on the odd classes mod pi^k (in R.odd_classes order)"""
    flags = None; K = k
    if F == 'P': K, flags, _ = arith_F(R, k)
    oc = R.odd_classes(k)
    tab = dict(zip(oc, digits))
    if K != k: tab = RR.lift_table(R, tab, k, K)
    out = RR.run(R, F, K, [(tag, alpha, tab)], 1, B2, N0=B1, cap=10 ** 6, flags=flags)
    return out


def parse(R, s):
    if R.dim == 1: return (int(s), 0)
    a, b = s.split(',')
    return (int(a), int(b))


if __name__ == '__main__':
    a = sys.argv[1:]
    R = Q.Ring(a[1])
    if a[0] == 'inv': cmd_inv(R, int(a[2]) if len(a) > 2 else 5)
    elif a[0] == 'window': cmd_window(R, int(a[2]) if len(a) > 2 else 5)
    elif a[0] == 'barina': cmd_barina(R, int(a[2]), int(a[3]), int(float(a[4])), int(a[5]) if len(a) > 5 else 0)
    elif a[0] == 'fd': cmd_fd(R, a[2], int(a[3]), int(a[4]), int(float(a[5])), int(a[6]) if len(a) > 6 else 0)
    elif a[0] == 'literal': cmd_literal(R, int(a[2]), int(float(a[3])), int(a[4]) if len(a) > 4 else 1, int(a[5]) if len(a) > 5 else 0)
    elif a[0] == 'confirm':
        F, k, B2, B1, tag, al = a[2], int(a[3]), int(float(a[4])), int(float(a[5])), a[6], parse(R, a[7])
        print(cmd_confirm(R, F, k, B2, B1, tag, al, [parse(R, s) for s in a[8:]]))
