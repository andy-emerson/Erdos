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
