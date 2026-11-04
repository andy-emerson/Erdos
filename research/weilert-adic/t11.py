# Operation counts for the recursive algorithm against the naive loop (recursive.md, section 6).
# Nothing is timed.  Every integer product is recorded with its operand sizes and priced in
# three models: schoolbook a*b, Karatsuba max*min^0.585, and FFT-like (a+b) log2(a+b);
# an inverse mod 2^h is priced as 3 products of size h (Newton).
import random, sys
from math import log2
import rec
from rec import *

def price(cnt):
    s = k = f = 0.0
    for a, b in cnt.mults:
        lo, hi = min(a, b), max(a, b)
        s += a * b; k += hi * lo ** 0.585; f += (a + b) * log2(a + b + 1)
    for h in cnt.inv_bits:
        s += 3 * h * h; k += 3 * h ** 1.585; f += 3 * 2 * h * log2(2 * h + 1)
    return s, k, f

def run(a, b, delta, recursive, alpha=1.0):
    rec.CNT = Counter()
    st, B0 = setup(a, b)
    A, Y, e = st
    rounds = []; Rbits = []; steps = [0]
    record = []
    while Y != (0, 0):
        pass  # no early exit: both versions run to Y = 0
        if recursive:
            K = max(val(Y), int(alpha * max(bits(A), bits(Y))))
            j, R, de = hgcd(A, Y, K, delta, record)
            rb = max(bits(R[r][c]) for r in range(2) for c in range(2))
            Rbits.append((K, j, rb))
            A, Y = apply(R, A, Y, j)
            rounds.append((K, max(bits(A), bits(Y))))
        else:
            _, _, de, A, Y, _ = naive(A, Y, None, delta, record)
        e += de
    c = rec.CNT; rec.CNT = None
    return c, rounds, Rbits, len(record)

if __name__ == "__main__":
    random.seed(11)
    rec.LEAF = 24
    delta = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    print("delta", delta, " LEAF", rec.LEAF)
    print("%6s %5s | %6s %6s %6s %6s %6s | %8s %8s | %7s %7s %7s | %7s %7s"
          % ("n", "steps", "calls", "leaves", "mid", "depth", "rounds", "R/K", "j/K",
             "kar/n^1.585lg", "fft/nlg^2", "sch/n^2", "naive sch/n^2", "speedup kar"))
    for n in (128, 256, 512, 1024, 2048, 4096, 8192):
        trials = 3 if n <= 2048 else 1
        acc = None
        for _ in range(trials):
            a = (random.getrandbits(n) - (1 << (n - 1)), random.getrandbits(n) - (1 << (n - 1)))
            while True:
                b = (random.getrandbits(n) - (1 << (n - 1)), random.getrandbits(n) - (1 << (n - 1)))
                if (b[0] + b[1]) % 2: break
            c, rounds, Rb, ns = run(a, b, delta, True)
            s, k, f = price(c)
            cn, _, _, ns2 = run(a, b, delta, False)
            sn, kn, fn = price(cn)
            assert ns == ns2
            top = Rb[0]
            row = [ns, c.calls, c.leaves, c.midsteps, c.maxdepth, len(rounds),
                   top[2] / top[0], top[1] / top[0],
                   k / (n ** 1.585 * log2(n)), f / (n * log2(n) ** 2), s / n ** 2, sn / n ** 2, kn / k]
            acc = row if acc is None else [x + y for x, y in zip(acc, row)]
        acc = [x / trials for x in acc]
        print("%6d %5d | %6d %6d %6d %6d %6.1f | %8.3f %8.3f | %13.2f %9.2f %7.1f | %13.1f %11.2f"
              % tuple([n] + acc))
        sys.stdout.flush()
    # round-by-round sizes for one 4096-bit input
    a = (random.getrandbits(4096), random.getrandbits(4096)); b = (random.getrandbits(4096) | 1, random.getrandbits(4096) & ~1)
    _, rounds, Rb, _ = run(a, b, delta, True)
    print("4096-bit input, rounds (K, size after):", rounds[:12])
    print("top-level (K, j, bits of R):", Rb[:6])
