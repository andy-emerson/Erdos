# Correctness of the recursive algorithm (recursive.md, section 5).
#  (a) quotient sequence of the recursion == quotient sequence of the naive loop (all steps
#      until Y = 0), and == the sequence of alg.sym_S up to the point where sym_S stops (B = 1);
#  (b) gcd == Euclidean gcd (nearest rounding) up to units, with planted common factors;
#  (c) symbol == Euler's criterion on moduli with known factorisation;
#  (d) symbol == alg.sym_S and the naive loop on random inputs up to 4096 bits.
import random, sys
import rec
from rec import *
import alg, g as G

def rnd(b):
    return (random.getrandbits(b) - (1 << (b - 1)), random.getrandbits(b) - (1 << (b - 1)))
def rnd_odd(b):
    while True:
        z = rnd(b)
        if (z[0] + z[1]) % 2: return z

def symS_seq(a, b, delta):
    """quotient sequence (q, gap, unit) of alg.sym_S's loop, in the same normalisation"""
    st = alg.setup(a, b)
    if st is None: return []
    A, B, _ = st; g_ = 1; out = []
    while B != (1, 0):
        q = alg.pimod_q(A, B, g_ + delta); r = alg.sub(A, alg.mul(q, B))
        out.append((q, g_))
        if r == (0, 0): break
        t, w = alg.strip(r); s, C = alg.primary(w)
        A, B = B, C; g_ = t - g_
    return out

def euclid_gcd(a, b):
    while b != (0, 0): a, b = b, G.gmod(a, b)
    return a
def same_ideal(x, y): return any(unit(s, x) == y for s in range(4))

def main():
    random.seed(10)
    stats = {}
    for leaf in (4, 24):
        rec.LEAF = leaf
        for delta in (2, 3, 4):
            # (a) quotient sequences, random sizes 8..2048 bits
            na = 0
            for _ in range(120 if leaf == 4 else 200):
                bsz = random.choice([8, 16, 40, 100, 300, 700, 1500, 2048])
                a = rnd(bsz); b = rnd_odd(bsz)
                r1 = []; r2 = []
                s1, g1 = quartic(a, b, delta, True, r1, early=False)
                s2, g2 = quartic(a, b, delta, False, r2, early=False)
                assert r1 == r2 and s1 == s2 and g1 == g2, ("seq", a, b, delta)
                if bsz <= 700:
                    r3 = symS_seq(a, b, delta)
                    assert [x[:2] for x in r1[:len(r3)]] == r3, ("symS seq", a, b)
                    assert alg.sym_S(a, b, delta=delta) == s1
                na += 1
            # (b) gcd with planted factors
            nb = 0
            for _ in range(60):
                bsz = random.choice([20, 100, 400, 1000])
                f = rnd_odd(random.choice([5, 30, bsz // 2]))
                a = G.mul(rnd(bsz), f); b = G.mul(rnd_odd(bsz), f)
                s, gg = quartic(a, b, delta, True, early=False)
                assert same_ideal(gg, primary(euclid_gcd(a, b))[1]) , ("gcd", a, b)
                if G.N(gg) > 1: assert s is None
                nb += 1
            stats[(leaf, delta)] = (na, nb)
            print("leaf %d delta %d: %d quotient-sequence checks, %d gcd checks, all agree" % (leaf, delta, na, nb))
            sys.stdout.flush()
    # (c) Euler's criterion
    rec.LEAF = 8
    for delta in (2, 4):
        nc = nz = 0
        for _ in range(250):
            fb = [G.rand_prime(random.choice([12, 30, 64, 160, 400])) for _ in range(random.randint(1, 4))]
            b = unit(random.randint(0, 3), G.prod(fb))
            a = rnd(random.choice([40, 300, 1200]))
            if random.random() < 0.2: a = mul_pi(a, random.randint(1, 5))
            if random.random() < 0.1: a = G.mul(a, fb[0])
            ref = G.direct(a, fb)
            s, _ = quartic(a, b, delta, True)
            assert s == ref, ("euler", a, fb)
            nc += 1; nz += ref is not None
        print("Euler, delta %d: %d cases (%d nonzero), all agree" % (delta, nc, nz))
    # large moduli for Euler: products of two ~1000-bit-norm primes
    for delta in (2, 3):
        nc = 0
        for _ in range(12):
            fb = [G.rand_prime(1000) for _ in range(2)]
            b = G.prod(fb); a = rnd(2048)
            assert quartic(a, b, delta, True)[0] == G.direct(a, fb)
            nc += 1
        print("Euler, delta %d, modulus norm about 2^2000: %d cases agree" % (delta, nc))
    # (d) large random inputs, mutual agreement
    rec.LEAF = 24
    for delta in (2, 4):
        nd = 0
        for bsz in (3000, 4096):
            for _ in range(6):
                a = rnd(bsz); b = rnd_odd(bsz)
                s1 = quartic(a, b, delta, True)[0]
                s2 = quartic(a, b, delta, False)[0]
                s3 = alg.sym_S(a, b, delta=delta)
                assert s1 == s2 == s3
                nd += 1
        print("4096-bit mutual agreement, delta %d: %d cases" % (delta, nd))

if __name__ == "__main__":
    main()
