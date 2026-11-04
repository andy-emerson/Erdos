# Truncation lemma for step S (recursive.md, section 3), tested by perturbation.
# Inputs (A, Y) and (A + pi^Q X, Y + pi^Q Z).  Lemma: every step whose pending valuation
# v satisfies 2v + delta <= Q has the same quotient and gap, and every step with
# 2v + max(delta, 7) <= Q also has the same unit and symbol increment.
# We record, at the first step where something differs, D = Q - 2v; the lemma says
# D < delta for a quotient/gap difference and D < max(delta,7) for any difference.
import random
from rec import *

def seq(A, Y, delta, cap=10**6):
    out = []; j = 0
    while Y != (0, 0) and len(out) < cap:
        g = val(Y)
        A, Y, g, de, q, s, S = step(A, Y, delta)
        out.append((j + g, q, g, s, de)); j += g
    return out

def trial(bitsz, delta, Q, big):
    while True:
        a = (random.getrandbits(bitsz) - (1 << (bitsz - 1)), random.getrandbits(bitsz) - (1 << (bitsz - 1)))
        b = (random.getrandbits(bitsz) - (1 << (bitsz - 1)), random.getrandbits(bitsz) - (1 << (bitsz - 1)))
        if (a[0] + a[1]) % 2 and (b[0] + b[1]) % 2: break
    A = primary(a)[1]; Y = mul_pi(primary(b)[1], 1)
    r = (lambda: random.getrandbits(bitsz) - (1 << (bitsz - 1))) if big else (lambda: random.randrange(-99, 100))
    X = (r(), r()); Z = (r(), r())
    A2 = gadd(A, mul_pi(X, Q)); Y2 = gadd(Y, mul_pi(Z, Q))
    s1 = seq(A, Y, delta); s2 = seq(A2, Y2, delta)
    Dq = Dall = None
    for k in range(min(len(s1), len(s2))):
        v = s1[k][0]
        if Dall is None and s1[k] != s2[k]: Dall = Q - 2 * v
        if s1[k][1:3] != s2[k][1:3]:
            Dq = Q - 2 * v; break
    return Dq, Dall

if __name__ == "__main__":
    random.seed(9)
    for delta in (1, 2, 3, 4, 6):
        mq = mall = -10**9; n = 0; ex_q = ex_all = 0
        for big in (False, True):
            for _ in range(1500):
                Q = random.randint(4, 90)
                Dq, Dall = trial(48, delta, Q, big)
                n += 1
                if Dq is not None: mq = max(mq, Dq)
                if Dall is not None: mall = max(mall, Dall)
                if Dq is not None and Dq >= delta: ex_q += 1
                if Dall is not None and Dall >= max(delta, 7): ex_all += 1
        print("delta %d: %d trials; max Q-2v at first quotient/gap difference %d (lemma: < %d), "
              "at first difference of any kind %d (lemma: < %d); violations %d, %d"
              % (delta, n, mq, delta, mall, max(delta, 7), ex_q, ex_all))
