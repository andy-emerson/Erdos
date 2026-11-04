# Gaussian Goldbach: literature and a first verification

Working notes. Code in this directory. Status: first verification done (all even z with N(z) <= 10^10).

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
| 10^10 | 2.0e9 | 1.6e10 | 10.3 min (measured) | 30 MB |
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

## 4. Results: first bound N(z) <= 10^10

Runs: run_1e8.txt, run_1e9.txt (norms [0, 1e9]) and chunks/run_ke9.txt (norms (k e9, (k+1) e9], k = 1..9), merged in merged_1e10.txt. 1,963,555,659 targets for E in the fundamental domain, i.e. all 15,707,962,441 even Gaussian integers of norm <= 10^10 in the plane. 619 s CPU in total.

| variant | targets checked (domain) | exceptions | hardest z (N) | its smallest summand |
|---|---|---|---|---|
| E, angles <= 45 | 1,963,555,659 | none | 28708+28708i (1.65e9) | N(p1) = 1301, p1 = 25+26i |
| E, angles < 45 (literal) | same | exactly 2 and 3+i (with all their unit multiples and conjugates); both work with exactly 45 | | |
| F, angles < 30 (= <= 30) | 1,963,555,656 | none | 57874 (3.35e9) | N(p1) = 1949, p1 = 43-10i |
| Q, open quadrant | 1,963,455,660 | none | 90232+2i (8.14e9) | p1 = 1106+i (L1 1107) |
| D, unrestricted | all even z | none | follows from E | |

So, for every even Gaussian integer z with N(z) <= 10^10: Holben-Jordan D holds; E holds with "<= 45 degrees" for N(z) > 2 (and with "< 45" for N(z) > 10); F holds for N(z) > 10; Knill's quadrant form holds for a, b > 1. The E strict exceptions match Holben and Jordan's own remark that 2 and 3+i "require 45 degrees", so their E is best read with <=, or with threshold N > 10.

Growth of the hardest case (max over z in a dyadic norm bin of the minimal summand; bins_1e9.txt to 1e9, merged run beyond):

| N(z) up to | E: max min N(p1) | F: max min N(p1) | Q: max min L1 |
|---|---|---|---|
| 2^12 | 97 | 113 | 25 |
| 2^16 | 157 | 397 | 85 |
| 2^20 | 281 | 421 | 265 |
| 2^24 | 701 | 797 | 265 |
| 2^30 (1.07e9) | 941 | 1549 | 785 |
| 10^10 | 1301 | 1949 | 1107 |

E and F grow roughly like (log N)^2 (E: 1301 / (ln 1e10)^2 = 2.5). All the hardest Q cases are on the row b = 2, where the only representations are (x+i) + (a-x+i) with x^2+1 and (a-x)^2+1 both prime: Q at b = 2 is the binary Goldbach problem for the primes n^2+1, which is why Knill notes Q implies Landau's problem. Rows b >= 3 are much easier.

Representation counts (ordered pairs (p1, p2), from ref.py, counts_small.txt has N <= 400):

| z | N | E (< 45) | E (<= 45) | F (< 30) | Q |
|---|---|---|---|---|---|
| 1+i | 2 | 0 | 0 | 0 | n/a |
| 2 | 4 | 0 | 2 | 0 | n/a |
| 2+2i | 8 | 1 | 1 | 1 | 1 |
| 3+i | 10 | 0 | 2 | 0 | n/a |
| 4 | 16 | 2 | 2 | 2 | n/a |
| 3+3i | 18 | 2 | 4 | 2 | 2 |
| 4+2i | 20 | 3 | 3 | 1 | 1 |
| 6 | 36 | 7 | 7 | 5 | n/a |
| 10+10i | 200 | 20 | 28 | 16 | 20 |

(2+2i = (1+i)+(1+i) is the only target with a single ordered pair; F at 4+2i has the one pair (2+i)+(2+i).)

## 5. What a publishable verification would need

1. Bound. 10^10 is already far beyond Holben and Jordan's box (norm about 2500) and is, as far as the literature search shows, the first stated bound. A paper would want 10^12 (|z| <= 10^6, 1.6e12 even targets): about 25 to 30 core-hours with gg.c as is, so an overnight run on a modest multicore machine, or less after the easy speedups in section 2. 10^14 would need a few thousand core-hours and the optimised sieve.
2. Two independent implementations, as in Oliveira e Silva, Herzog and Pardi (Math. Comp. 2014): here gg.c plus a second program with a different primality source (for example primesieve, or a Gaussian sieve that splits p = 1 mod 4 via Cornacchia and marks multiples in Z[i]) and a different search order, compared on per-shell checksums of the minimal-summand values.
3. Certificates: for each shell, store a hash of the per-target minimal summand and the full records (hardest z per dyadic bin), so anyone can recheck a shell; the record z and summands are tiny to publish.
4. Exact statements: say which reading of E (strict or not) and which thresholds; report the strict-45 exceptions 2, 3+i; state Q separately because of its Landau-type row b = 2 (worth its own table: the n^2+1 Goldbach records for a up to 10^6).
5. Optional extras that make it more than a number: minimal angle needed as a function of N (how far below 30 degrees one can go, Holben and Jordan ask for the threshold angle), comparison of representation counts with the Hardy-Littlewood type singular series for Z[i] (the S1 of Banerjee and Rahaman), and the sector version of Giannitsi et al. for narrow fixed arcs.

## Files

| file | purpose |
|---|---|
| gg.c / gg | verifier (E, F, Q; shells; GGDUMP=1 dumps per-target values; GGNOSIEVE=1 uses Miller-Rabin only) |
| gg_bins.c / gg_bins | same plus dyadic-bin maxima |
| ref.py | independent numpy brute force and representation counts |
| check_top.py | target counts by formula; hardest reps re-verified and proved minimal (sympy) |
| merge.py | merges runs over disjoint norm ranges |
| odd_check.py | Knill's odd-target remark (4+13i, 29) |
| run10.sh | the 1e9..1e10 chunked run |
