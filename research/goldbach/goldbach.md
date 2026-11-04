# Gaussian Goldbach: literature and a first verification

Working notes. Code in this directory. Status: in progress.

## 1. Literature

Terminology. A Gaussian integer z = a+bi is even if (1+i) divides z, equivalently a+b is even. N(z) = a^2+b^2. A Gaussian prime is x+yi with xy != 0 and x^2+y^2 a rational prime (2 or p = 1 mod 4), or an associate of a rational prime q = 3 mod 4 (x or y zero, other coordinate +-q).

### Holben and Jordan (1968)

C. A. Holben and J. H. Jordan, "The twin prime problem and Goldbach's conjecture in the Gaussian integers", Fibonacci Quarterly 6(5) (Nov. 1968), 81-85 and 92. Scanned at mathstat.dal.ca/FQ/Scanned/6-5/holben-a.pdf (read through an OCR summary; the scan is garbled in places, flagged below).

Conjectures A, B, C are twin, triplet, quadruplet prime statements (differences (1+i)e, e a unit). Theorem 1: only finitely many Gaussian quintuplets. The Goldbach part:

| label | statement (as printed, OCR cleaned) |
|---|---|
| D | If a is an even Gaussian integer, there are Gaussian primes p1, p2 with a = p1 + p2. |
| E | If a is even with abs(a) > sqrt(2) (OCR "\/z"; Knill and later papers read it as N(a) > 2), then a = p1 + p2 with angles p1 O a and a O p2 both < 45 degrees. |
| F | If a is even with abs(a) > sqrt(10) (N(a) > 10), then a = p1 + p2 with both angles < 30 degrees. |

Comments in the paper: D is called "a poor generalization", closer to "every even integer is a sum or difference of two primes", since positivity is meaningless in Z[i]. Two repairs are proposed: require p1, p2 in the same half plane, or bound abs(p1), abs(p2) (this sentence is garbled in the scan; the authors prefer the bound). E implies D and both repairs. E and F were checked for all even a in the square with vertices +-50+-50i (their figure). 1+3i, 3+i, 2 and associates need 45 degrees (so F has the N > 10 threshold). The angle cannot be reduced to 0 because 4, 8, 12, ... are not sums of two real Gaussian primes (real Gaussian primes are +-q, q = 3 mod 4, and q1+q2 is 2 mod 4 or a difference).

Note on strictness: Holben and Jordan write "< 45" and "< 30". Giannitsi et al. restate F with <= pi/6. For 30 degrees the two readings coincide for nonzero integer vectors (tan^2 = 1/3 is not attained by rational vectors); for 45 degrees they differ (angle exactly 45 is possible), so we test the strict form and report where only <= works.

### Mitsui (1960)

T. Mitsui, "On the Goldbach problem in an algebraic number field I", J. Math. Soc. Japan 12(3) (1960), 290-324, doi 10.2969/jmsj/01230290 (part II, same journal, pages not checked). Generalised Vinogradov method in number fields (his "generalised prime number theorem" for primes in sectors and congruence classes is the input). Per Giannitsi et al., Mitsui (sections 11-12) treats the binary and higher cases: all sufficiently large totally positive odd integers are sums of three totally positive odd primes (ternary analogue), with the binary case only conjectured / on average. Knill cites Mitsui as formulating the number-field conjecture, which in the Gaussian case is D. Not read directly here.

### Knill (2016)

O. Knill, "Goldbach for Gaussian, Hurwitz, Octavian and Eisenstein primes", arXiv:1606.05958 (also web page abel.math.harvard.edu/~knill/primes/goldbach.html). Quadrant version, here called Q:

  Every even Gaussian integer a+bi with a > 1, b > 1 is a sum of two Gaussian primes in the open quadrant {x > 0, y > 0}.

Also a ternary quadrant version (every a+bi with a > 2, b > 2 is a sum of three primes in the open quadrant). Remarks: Q implies infinitely many primes n^2+1 (take b = 2: a+2i = (x+i)+(a-x+i)). The paper says "The smallest Gaussian integer which is not the sum of two Gaussian primes is 4+13i. The smallest real one is 29." (This is about odd targets, showing evenness is needed; we recheck it.) No verification range for Q is given, only pictures (matrices of representation counts in a small box).

### Recent analytic work

| paper | result |
|---|---|
| Giannitsi, Krause, Lacey, Mousavi, Rahimi, "Averages over the Gaussian primes: Goldbach's conjecture and improving estimates", arXiv:2309.14249 | For any arc w of directions: (ternary) every odd n of large norm with arg(n) at distance > (log N(n))^-B from the boundary of w is p1+p2+p3 with arg(pj) in w; (binary, density) the set E2(w, N) of even n, N(n) < N, arg(n) in w, not p1+p2 with arg(pj) in w, has size << N/(log N)^B. Restates F as open (Conjecture 1.4). Says no prior sector results. |
| Banerjee, Rahaman, "Representation of even Gaussian integer a la Chen", arXiv:2406.16311 (2024) | Every even Gaussian integer of sufficiently large norm is p + P2 (P2 at most two prime factors), with a lower bound >> S(n) N(n)/(log N(n))^2. No sector condition. Mentions no Gaussian numerical verification. |

### Computational verification already done

None found with a stated bound. Holben and Jordan checked E and F in the box |Re|, |Im| <= 50 (norm up to 5000, complete only for N <= 2500). Knill shows pictures. No OEIS sequence counts Goldbach representations of Gaussian integers that I could find (searched OEIS via web search; A062711 etc. are unrelated). No MathOverflow / Math.SE thread with a computation turned up. The valuation-problems survey's finding stands.

### Knill's odd-target remark, rechecked

An odd z = p1+p2 needs one summand an associate of 1+i, so z is a sum iff some z - (1+i)e is prime. odd_check.py (sympy) confirms: the odd Gaussian integers of smallest norm that are not sums of two Gaussian primes are 4+13i and its 7 images (norm 185), next 12+17i (433); the smallest positive odd real one is 29 (then 31, 33). So Knill's statement is about unrestricted sums of odd targets.

## 2. Variants to verify and method

| variant | targets | condition on z = p1 + p2 | symmetry used |
|---|---|---|---|
| D (H-J, Mitsui) | all even z | none | follows from E-nonstrict for N(z) > 2; 0 = (2+i)+(-2-i), 1+i = (3+2i)+(-2-i) |
| E (H-J) | even, N(z) > 2 | angle(pj, z) < 45 deg, j = 1, 2 | 8 symmetries (units, conjugation): 0 <= b <= a |
| F (H-J) | even, N(z) > 10 | angle(pj, z) < 30 deg | same |
| Q (Knill) | even a+bi, a, b > 1 | p1, p2 in open first quadrant | a <-> b: 2 <= b <= a |

Exact tests. With dot = Re(p1 conj z), cr = Im(p1 conj z), and p2 = z - p1 one has cross(p2, z) = -cr and dot(p2, z) = N(z) - dot. So E strict is dot > |cr| and N(z) - dot > |cr|; F is dot, N(z)-dot > 0 and 3 cr^2 < dot^2, (N(z)-dot)^2 (128-bit). Primality: x+yi with xy != 0 is prime iff x^2+y^2 is a rational prime; on the axes iff |coordinate| is a prime 3 mod 4. So a table of rational primes up to about max N(p2) is the whole Gaussian prime table; no splitting of p = 1 mod 4 is needed.

Search order. E, F: p1 runs over all Gaussian primes (all associates) sorted by norm, so the first hit is the minimal N(p1) over all representations (= min over reps of min(N(p1), N(p2)) by symmetry). The cone test filters most candidates before any table lookup. Q: p1 = x+yi over diagonals x+y = k = 2, 3, ..., so the first hit is the minimal L1 size; this handles the thin rows b = 2, 3 well (for b = 2 only p1 = x+i, p2 = (a-x)+i are possible, a Goldbach problem for primes n^2+1).

Implementation (gg.c). Targets are processed in norm shells [S, S+W) with W = 2^27. A segmented odd-only Eratosthenes sieve covers the shell plus a margin 2|z|max|p1| + max N(p1); any norm outside the segment goes to a small table or to deterministic Miller-Rabin (12 prime bases, exact for 64-bit), so correctness never depends on the margin. Memory is O(W) plus the prime list, independent of N: about 30 MB for every bound up to 10^12. Shell ranges can be run independently ([NMIN, NMAX] arguments), which is how a larger run would be split across cores or machines.

Measured cost (one core, all three variants together, niced, with other jobs running): 4.5 s to 10^8, 54 s to 10^9, about 70 s per further 10^9 window near 10^10. Per variant near 10^9: E about 2.5 s, F 3.5 s, Q 2 s per 10^8 window of norms, roughly 130 ns per target, dominated by table lookups. Targets in the fundamental domain number about pi N / 16.

| bound N | targets per variant (domain) | even z in the whole plane | est. one-core time, E+F+Q | memory |
|---|---|---|---|---|
| 10^8 | 2.0e7 | 1.6e8 | 4.5 s (measured) | 30 MB |
| 10^10 | 2.0e9 | 1.6e10 | about 12 min (measured, see below) | 30 MB |
| 10^12 | 2.0e11 | 1.6e12 | about 25 to 30 h (extrapolated; growth slightly above linear) | 30 MB |

10^12 corresponds to |z| <= 10^6, so all arithmetic fits in 64 bits (norms < 2^41, dot products < 2^31 times 2^20) with 128-bit only for the F squares. Obvious speedups if 10^12 or beyond is wanted: run shells in parallel (trivially independent), smaller shells that fit in L2, a wheel/bucket sieve, and testing the best few p1 candidates for a whole row of z at once.

## 3. Checks

| check | result |
|---|---|
| ref.py: independent numpy full-box brute force (no early exit, cos^2 form of the angle tests) vs gg.c per-target minimal values, all 4013 even z in the domain with N <= 20000 | identical |
| sieve vs pure Miller-Rabin (GGNOSIEVE=1) and three different shell/margin settings, 196335 targets with 9.99e8 < N <= 1e9 | identical per-target output |
| split ranges [0, 1e6) + [1e6, 2e6] vs one run | identical |
| target counts reported by gg.c vs closed-form count in check_top.py | equal for every run and chunk |
| the 12 hardest z per variant in every run: representation re-verified with sympy and minimality proved by exhaustive search over all smaller p1 | all valid and minimal |
