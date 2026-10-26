"""Longest key any Q72 value needs: a beam search over continued fractions within the fit rule."""
import sys
from q72 import fits
import lcf


def cost(a, index, a0_fixed):
    if index == 0:
        return 65 if a0_fixed else len(lcf.lexibinary(a + 1))
    return len(lcf.lexibinary(a))


def longest(word=64, a0_fixed=False, beam=40, choices=None):
    """Beam search: states are continuants (p, q, p_prev, q_prev) with the code length so far, bucketed by the
    bit lengths of p and q; the best few per bucket survive. Returns (length, p, q)."""
    choices = choices or ([0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 31, 32, 63, 64, 127, 255, 1023, 2**16 - 1, 2**32 - 1]
                          + [2**k - 1 for k in range(40, 65, 4)])
    best = (0, 0, 1)
    frontier = {}
    for a0 in choices + [2**64 - 1, 2**63]:
        if a0 >= 2**word:
            continue
        st = (a0, 1, 1, 0)   # p_0, q_0, p_{-1}, q_{-1}
        c = cost(a0, 0, a0_fixed)
        if fits(a0, 1, word) or a0 == 0:
            best = max(best, (c, a0, 1))
        frontier.setdefault((a0.bit_length(), 1), []).append((c, st))
    index = 1
    while frontier:
        nxt = {}
        for bucket, states in frontier.items():
            for c, (p, q, pp, qp) in sorted(states, reverse=True)[:beam]:
                for a in choices[1:] + [2**k + 1 for k in range(1, 40, 3)]:
                    np_, nq = a * p + pp, a * q + qp
                    if np_.bit_length() + (nq - 1).bit_length() > word + 1:
                        continue
                    nc = c + cost(a, index, a0_fixed)
                    if a >= 2 and fits(np_, nq, word):
                        best = max(best, (nc, np_, nq))
                    nxt.setdefault((np_.bit_length(), nq.bit_length()), []).append((nc, (np_, nq, p, q)))
        frontier = nxt
        index += 1
    return best


if __name__ == "__main__":
    for fixed in (False, True):
        c, p, q = longest(a0_fixed=fixed)
        print("a0 fixed" if fixed else "lexibinary a0", "longest code", c, "bits; at", p, "/", q,
              "check", len(lcf.code(p, q, fixed)[0]), "fits", fits(p, q))
