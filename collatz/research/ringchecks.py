"""ringchecks.py: checks of the general theorems in each ring (rings-test.md, Section 5 of each ring).

usage: python3 ringchecks.py forced RING k        units eta = eps pi^v - alpha (v = k..k+4) for every associate alpha
       python3 ringchecks.py assoc RING k Nd Nz     respects associates iff commutes: pair test (quotient.md 5.2)
                                                   over every multiplier in the window, digits of size <= Nd, test points |N| <= Nz
       python3 ringchecks.py mirror RING k B nsmall one-step identities for rotated and mirrored maps on random
                                                   tables, and literal verdicts of (alpha, d), rotations and mirror
Logs: logs/rings_<RING>_<cmd>_k<k>.txt
"""
import sys, os, random, itertools, collections
import qring as Q, ringrun as RR, ringtest as RT

HERE = os.path.dirname(os.path.abspath(__file__))


def cmd_forced(R, k):
    L = [f"# forced fixed points in {R.name} at pi^{k}: units eta = eps pi^v - alpha, v = k..k+4 (K1, K2). "
         f"eps = 1 applies to every map; eps != 1 only to maps commuting with the units"]
    for a0 in RT.multipliers(R, k):
        for u in R.U:
            a = R.mul(u, a0)
            hits = []
            for v in range(k, k + 5):
                for e in R.U:
                    eta = R.sub(R.mul(e, R.pw(R.pi, v)), a)
                    if R.is_unit(eta): hits.append(f"eps={Q.fmt(R, e)} v={v} eta={Q.fmt(R, eta)}")
            L.append(f"alpha={Q.fmt(R, a)} (N {R.absN(a)}): {'; '.join(hits) if hits else 'none'}")
    RT.log(R, f'forced_k{k}', L)


def assoc(R, x, y):
    return any(R.mul(u, x) == y for u in R.U)


def S1(R, alpha, d, z):
    w = R.add(R.mul(alpha, z), d)
    return R.oddpart(w)[0] if w != (0, 0) else (0, 0)


def cmd_assoc(R, k, Nd, Nz):
    pts = collections.defaultdict(list)
    for z in R.elements(Nz):
        if R.odd(z): pts[R.idx(z, k)].append(z)
    digs = collections.defaultdict(list)
    for z in R.elements(Nd):
        if R.odd(z): digs[R.idx(z, k)].append(z)
    alphas = set()
    for kk in range(max(0, k - 1), k + 2):
        for a0 in RT.multipliers(R, kk):
            for u in R.U: alphas.add(R.mul(u, a0))
    npairs = ncomm = ncomm_pass = nbad_S = nbad_T = 0
    for alpha in sorted(alphas):
        for c in R.odd_classes(k):
            cz = R.classrep(c, k)
            j1 = R.idx(R.neg(R.mul(alpha, cz)), k)
            for u in R.U:
                if u == (1, 0): continue
                uc = R.idx(R.mul(u, cz), k)
                j2 = R.idx(R.neg(R.mul(alpha, R.classrep(uc, k))), k)
                for d1 in digs[j1]:
                    for d2 in digs[j2]:
                        if uc == c and d2 != d1: continue      # one class, one digit: not a table otherwise
                        npairs += 1
                        okS = okT = True
                        for z in pts[c]:
                            uz = R.mul(u, z)
                            if not assoc(R, S1(R, alpha, d2, uz), S1(R, alpha, d1, z)): okS = False
                            if not assoc(R, R.add(R.mul(alpha, uz), d2), R.add(R.mul(alpha, z), d1)): okT = False
                            if not okS and not okT: break
                        com = d2 == R.mul(u, d1) and uc != c
                        if com:
                            ncomm += 1; ncomm_pass += okS and okT
                        else:
                            nbad_S += okS; nbad_T += okT
    L = [f"# respects associates iff commutes, {R.name} at pi^{k}: {len(alphas)} multipliers (all associates, windows k-1..k+1), "
         f"digits of size <= {Nd}, test points odd |N| <= {Nz}",
         f"pairs {npairs}; commuting pairs {ncomm} (passing {ncomm_pass}); non-commuting pairs passing: S {nbad_S}, T {nbad_T}"]
    RT.log(R, f'assoc_k{k}', L)


def mirror_map(R, alpha, tab, k):
    """(sigma alpha, sigma d sigma) as a map dividing by sigma(pi); returns the ring of sigma(pi) and the table"""
    sa = R.conj(alpha)
    st = {}
    for i in R.odd_classes(k):
        z = R.classrep(i, k)
        st[i] = R.conj(tab[R.idx(R.conj(z), k)])
    return sa, st


def cmd_mirror(R, k, B, nsmall, seed=1, nsample=0):
    rnd = random.Random(seed)
    L = [f"# rotated and mirrored maps in {R.name} at pi^{k}"]
    eta = RT.eta(R)
    L.append(f"sigma(pi) = eta pi with eta = {Q.fmt(R, eta) if eta else None}")
    # one-step identities on random tables
    nchk = nbad_rot = nbad_mir = 0
    zs = [z for z in R.elements(400) if R.odd(z)]
    for a0 in RT.multipliers(R, k)[:6]:
        cl = RT.cand_lists(R, a0, k, 6)
        for _ in range(5):
            tab = {i: rnd.choice(cl[i][0]) for i in R.odd_classes(k)}
            for u in R.U:
                ua = R.mul(u, a0)
                tu = {i: tab[R.idx(R.mul(u, R.classrep(i, k)), k)] for i in R.odd_classes(k)}
                for z in zs:
                    nchk += 1
                    if RR.apply_S(R, ua, tu, k, z)[0] != RR.apply_S(R, a0, tab, k, R.mul(u, z))[0]: nbad_rot += 1
            if eta:
                sa, st = mirror_map(R, a0, tab, k)
                for z in zs:
                    y, v = RR.apply_S(R, a0, tab, k, R.conj(z))
                    lhs = RR.apply_S(R, sa, st, k, z)[0]
                    rhs = R.mul(R.pw(eta, v), R.conj(y)) if v is not None else (0, 0)
                    nchk += 1
                    if lhs != rhs: nbad_mir += 1
    L.append(f"one-step identities S_(u alpha)(z) = S_alpha(uz) and S_(sigma alpha)(z) = eta^v sigma(S_alpha(sigma z)): "
             f"{nchk} checks, rotation mismatches {nbad_rot}, mirror mismatches {nbad_mir}")
    # literal verdicts of each smallest-or-small table, its rotations and its mirror
    jobs = []; meta = []
    for a0 in RT.multipliers(R, k):
        cl = RT.cand_lists(R, a0, k, nsmall)
        oc = R.odd_classes(k)
        if nsample:
            chs = {tuple(cl[i][1][0] for i in oc)}
            while len(chs) < nsample: chs.add(tuple(rnd.choice(cl[i][0]) for i in oc))
            chs = sorted(chs)
        else:
            chs = itertools.product(*[cl[i][0] for i in oc])
        for ch in chs:
            tab = dict(zip(oc, ch))
            com = R.acts_freely(k) and RR.commutes(R, tab, k)
            grp = len(meta)
            for u in R.U:
                ua = R.mul(u, a0)
                tu = {i: tab[R.idx(R.mul(u, R.classrep(i, k)), k)] for i in R.odd_classes(k)}
                jobs.append((f"g{grp}_rot{Q.fmt(R, u)}", ua, tu))
            if eta:
                sa, st = mirror_map(R, a0, tab, k)
                jobs.append((f"g{grp}_mir", sa, st))
            meta.append((a0, ch, com))
    out = []
    for s in range(0, len(jobs), 2000): out += RR.run(R, 'A', k, jobs[s:s + 2000], 2, B)
    res = collections.defaultdict(dict)
    for l in out:
        tag = l.split()[0]; g, kind = tag.split('_', 1)
        res[int(g[1:])][kind] = ' PASS ' in l
    agree = collections.Counter()
    for g, (a0, ch, com) in enumerate(meta):
        r = res[g]
        rots = [r[f"rot{Q.fmt(R, u)}"] for u in R.U]
        mir = r.get('mir')
        key = ('commuting' if com else 'non-commuting', 'rotations agree' if len(set(rots)) == 1 else 'rotations differ',
               'mirror n/a' if mir is None else ('mirror agrees' if mir == rots[0] else 'mirror differs'))
        if not (any(rots) or mir): agree['all fail'] += 1; continue
        agree[key] += 1
        if True:
            L.append(f"alpha={Q.fmt(R, a0)} digits {RT.describe(R, ch, R.odd_classes(k), k)} commuting={com}: "
                     f"rotations {dict(zip([Q.fmt(R, u) for u in R.U], rots))} mirror {mir}")
    L.insert(3, f"literal verdicts to |N| <= {B} over {len(meta)} tables (digits among the {nsmall} smallest{', random sample' if nsample else ''}): {dict(agree)}")
    RT.log(R, f'mirror_k{k}', L)


def cmd_split(k, seed=2):
    """Z7: sigma conjugates the theta family to the theta-bar family: S'_(sigma alpha, sigma d sigma)(z) = sigma(S_(alpha,d)(sigma z)),
    where S divides by theta and S' by theta-bar; residue classes mod theta^k go to classes mod theta-bar^k."""
    R = Q.Ring('Z7'); Rb = Q.Ring('Z7b'); rnd = random.Random(seed)
    zs = [z for z in R.elements(600) if R.odd(z)]
    nchk = nbad = 0
    for a in RT.multipliers(R, k):
        cl = RT.cand_lists(R, a, k, 6)
        for _ in range(6):
            tab = {i: rnd.choice(cl[i][0]) for i in R.odd_classes(k)}
            sa = R.conj(a)
            tb = {}
            for i in Rb.odd_classes(k):
                z = Rb.classrep(i, k)
                tb[i] = R.conj(tab[R.idx(R.conj(z), k)])
            for z in zs:
                zb = R.conj(z)          # odd for theta-bar
                lhs = RR.apply_S(Rb, sa, tb, k, zb)[0]
                rhs = R.conj(RR.apply_S(R, a, tab, k, z)[0])
                nchk += 1; nbad += lhs != rhs
    L = [f"# Z7: sigma S_theta sigma = S_theta-bar on random tables at modulus {k}: {nchk} checks, {nbad} mismatches"]
    RT.log(R, f'split_k{k}', L)


if __name__ == '__main__':
    a = sys.argv[1:]
    if a[0] == 'split': cmd_split(int(a[1])); sys.exit()
    R = Q.Ring(a[1])
    if a[0] == 'forced': cmd_forced(R, int(a[2]))
    elif a[0] == 'assoc': cmd_assoc(R, int(a[2]), int(a[3]), int(a[4]))
    elif a[0] == 'mirror': cmd_mirror(R, int(a[2]), int(float(a[3])), int(a[4]), nsample=int(a[5]) if len(a) > 5 else 0)
