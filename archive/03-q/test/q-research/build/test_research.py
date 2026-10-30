import math
import random
from fractions import Fraction

import numpy as np

import reuse  # noqa: F401
from q72 import fits
import lcf
import lcf_bound
import lcfkern as LK
import hensel_run as H


def test_lcf_keys_preserve_order_brute_force():
    vals = sorted({Fraction(p, q) for p in range(0, 90) for q in range(1, 90)})
    for qcode in (lcf.lexibinary, lcf.delta):
        keys = [lcf.key(v.numerator, v.denominator, 400, qcode=qcode) for v in vals]
        assert all(a < b for a, b in zip(keys, keys[1:])), qcode.__name__
    signed = sorted({s * v for v in vals for s in (1, -1)})
    keys = [lcf.signed_key(1 if v >= 0 else -1, abs(v.numerator), v.denominator, 401, qcode=lcf.delta) for v in signed]
    assert all(a < b for a, b in zip(keys, keys[1:]))


def test_compiled_keys_match_reference_and_order():
    rng = random.Random(2)
    rows = []
    for _ in range(30000):
        k = rng.random()
        if k < 0.3:
            p, q = rng.randrange(0, 2**64), rng.randrange(1, 2**rng.randint(1, 64))
        elif k < 0.6:
            p, q = rng.randrange(0, 10**rng.randint(1, 19)), 10**rng.randint(0, 19)
        else:
            p = rng.choice([0, 1, 2, 3, 2**63 - 1, 2**63, 2**64 - 1, 3**40])
            q = rng.choice([1, 2, 3, 2**32 + 1, 2**63 - 1, 2**63, 2**64 - 1])
        g = math.gcd(p, q) or 1
        p, q = p // g, q // g
        if p == 0:
            q = 1
        if p == 0 or fits(p, q):
            rows.append((rng.random() < 0.5, p, q))
    neg = np.array([r[0] for r in rows])
    p = np.array([r[1] for r in rows], np.uint64)
    q = np.array([r[2] for r in rows], np.uint64)
    kh, kl = np.empty(len(rows), np.uint64), np.empty(len(rows), np.uint64)
    LK.keys(neg, p, q, kh, kl)
    ref = [lcf.signed_key(-1 if n else 1, a, b, 128, qcode=lcf.delta) for n, a, b in rows]
    got = [(int(h) << 64) | int(l) for h, l in zip(kh, kl)]
    assert all(r is not None for r in ref), "every Q72 value has a 128-bit key"
    assert got == ref
    fr = [Fraction(-a if n else a, b) for n, a, b in rows]
    pairs = sorted(zip(got, fr))
    assert all((k1 < k2) == (f1 < f2) and (k1 == k2) == (f1 == f2) for (k1, f1), (k2, f2) in zip(pairs, pairs[1:]))


def test_sorts_are_exact():
    rng = np.random.default_rng(3)
    n = 5000
    neg = rng.random(n) < 0.5
    p = rng.integers(0, 2**40, size=n).astype(np.uint64)
    q = rng.integers(1, 2**20, size=n).astype(np.uint64)
    p[:50] = p[50:100]                      # duplicates, and values whose floats collide
    q[:50] = q[50:100]
    p[100:120] = np.uint64(2**53 + 1)
    q[100:120] = np.arange(1, 21, dtype=np.uint64) * np.uint64(2**10) + np.uint64(1)
    p[0] = 0
    fr = [Fraction(-int(a) if s else int(a), int(b)) for s, a, b in zip(neg, p, q)]
    want = sorted(fr)
    assert [fr[i] for i in LK.merge_sort_exact(neg, p, q)] == want
    f = np.array([float(x) for x in fr])
    order = np.argsort(f, kind="stable")
    LK.fix_ties(order, f, neg, p, q)
    assert [fr[i] for i in order] == want


def test_lcf_bound_is_at_least_the_search():
    for qcode in (lcf.lexibinary, lcf.delta):
        b = lcf_bound.bound(qcode)
        assert b >= len(lcf.code(2**64 - 1, 1, qcode=qcode)[0])
        assert b >= len(lcf.code(4224250756, 2986996355, qcode=qcode)[0])
    assert lcf_bound.bound(lcf.delta) <= 127, "a sign bit and a delta-coded key fit 128 bits"


def test_hensel_and_pairs_exact_including_edges():
    rng = np.random.default_rng(4)
    n = 600
    nums = [rng.integers(-10**6, 10**6, size=n) for _ in range(3)]
    dens = [rng.integers(1, 1000, size=n) for _ in range(3)]
    # edges: zero, the int64 extremes, powers of two in denominators, big shifts
    nums[0][:6] = [0, -(2**63), 2**63 - 1, 1, -1, 7]
    dens[0][:6] = [1, 2**62, 3, 2**61, 1, 2**40]
    nums[1][:6] = [5, 1, -1, 2**62, 3, 0]
    dens[1][:6] = [2**30, 1, 2**63 - 1, 1, 2**62, 1]
    nums[2][:6] = [-(2**63), 0, 1, -7, 2**62, 1]
    dens[2][:6] = [1, 3, 2**50, 5, 1, 2**62]
    for K in (1, 2, 3):
        want = H.exact(nums, dens, K)
        for m in ("mpq", "pairs", "hensel"):
            med, mn, falls, got = H.run(m, nums, dens, K, reps=1)
            assert got == want, (m, K, sum(a != b for a, b in zip(got, want)))


def test_merge_sort_keys_and_big_denominator_print():
    rng = np.random.default_rng(8)
    kh = rng.integers(0, 4, size=3000).astype(np.uint64)
    kl = rng.integers(0, 2**63, size=3000).astype(np.uint64)
    order = LK.merge_sort_keys(kh, kl)
    keys = [(int(kh[i]) << 64) | int(kl[i]) for i in order]
    assert keys == sorted(keys)
    # a result whose denominator is at least 2^127 must print correctly (it falls back or prints unsigned)
    nums = [np.array([1, 3]), np.array([1, 1]), np.array([0, 0])]
    dens = [np.array([2**62 + 1, 3]), np.array([2**62 + 3, 2**61 + 1]), np.array([1, 1])]
    for m in ("pairs", "hensel"):
        assert H.run(m, nums, dens, 1, reps=1)[3] == H.exact(nums, dens, 1)
