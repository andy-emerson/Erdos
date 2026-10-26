"""An upper bound on the key length of any Q72 value, by dynamic programming over a relaxation.

Write p/q = [a0; a1, ..., an] with continuants p_k, q_k. For k >= 2,
    q_k / q_{k-1} = a_k + q_{k-2}/q_{k-1} >= a_k + 1/(a_{k-1} + 1),
and the same holds for p_k / p_{k-1} when a0 >= 1 (then p_0 = a0 >= 1). With p_0 = a0, p_1 >= a0 a1 and q_1 = a1:
    a0 >= 1:  log2 p + log2 q >= log2 a0 + 2 log2 a1 + sum_{k>=2} 2 log2(a_k + 1/(a_{k-1} + 1)).
When a0 = 0, p_0 = 0 and p_1 = 1, so p_2 = a2 exactly and the k = 2 term is only log2 a1 + ... : this case is the
a0 >= 1 analysis applied to 1/x = [a1; a2, ..., an], giving
    a0 = 0:   log2 p + log2 q >= log2 a1 + 2 log2 a2 + sum_{k>=3} 2 log2(a_k + 1/(a_{k-1} + 1)).
A Q72 magnitude with q >= 2 has log2 p + log2 q < bits(p) + bits(q - 1) + 1 <= 65; an integer has log2 p < 64.

Quotients are grouped by bit length L: the code length depends only on L (for a0 the code is of a0 + 1, which can
have one more bit than a0), and the weight is bounded below using the smallest quotient 2^(L-1) after the largest
previous quotient 2^Lp - 1. Weights are rounded down to multiples of STEP, the last-quotient constraint (>= 2) is
dropped, and every budget row is iterated to a fixpoint (a weight can round down to zero), so the maximum total
code length under the budget is an upper bound on every key length."""
import math

import numpy as np

STEP = 1 / 256


def bound(qcode, budget_bits=65, max_len=64):
    cost = {L: len(qcode(1 << (L - 1))) for L in range(1, max_len + 2)}
    B = int(budget_bits / STEP)
    NEG = -10**9
    best = max(cost.values())                      # integers: a single quotient a0, coded as a0 + 1 (<= 65 bits)
    wt = np.zeros((max_len + 1, max_len + 1), dtype=np.int64)
    for Lp in range(1, max_len + 1):
        for L in range(1, max_len + 1):
            wt[Lp, L] = int(math.floor(2 * math.log2(2 ** (L - 1) + 1 / 2 ** Lp) / STEP))
    for a0_zero in (True, False):
        # dp[b, L, f]: best code length with weight b used, last quotient of bit length L;
        # f = 1 marks the state right after a1 when a0 = 0, whose next weight is 2 log2 a2 exactly
        dp = np.full((B + 1, max_len + 1, 2), NEG, dtype=np.int64)
        if a0_zero:
            starts = [(0.0, cost[1])]              # a0 = 0 is coded as a0 + 1 = 1
        else:
            starts = []
            for L0 in range(1, max_len + 1):       # a0 of bit length L0: a0 + 1 has L0 bits, or L0 + 1 at 2^L0 - 1
                starts.append((L0 - 1, cost[L0]))
                starts.append((math.log2(2 ** L0 - 1), cost[L0 + 1]))
        for w0, c0 in starts:
            for L1 in range(1, max_len + 1):
                w = w0 + (1 if a0_zero else 2) * (L1 - 1)
                b = int(math.floor(w / STEP))
                if b <= B:
                    f = 1 if a0_zero else 0
                    dp[b, L1, f] = max(dp[b, L1, f], c0 + cost[L1])
        for b in range(B + 1):
            done = set()
            while True:                            # iterate the row to a fixpoint
                todo = [(f, Lp) for f in (1, 0) for Lp in np.nonzero(dp[b, :, f] > NEG)[0]
                        if (f, Lp, dp[b, Lp, f]) not in done]
                if not todo:
                    break
                for f, Lp in todo:
                    c = dp[b, Lp, f]
                    done.add((f, Lp, c))
                    best = max(best, int(c))
                    for L in range(1, max_len + 1):
                        w = 2 * (L - 1) * int(1 / STEP) if f == 1 else wt[Lp, L]
                        nb = b + w
                        if nb <= B and c + cost[L] > dp[nb, L, 0]:
                            dp[nb, L, 0] = c + cost[L]
    return best


if __name__ == "__main__":
    import lcf
    for name, qc in (("lexibinary", lcf.lexibinary), ("delta", lcf.delta)):
        print(name, "upper bound", bound(qc), "bits")
