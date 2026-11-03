# Spec 14: needs, worked back from cryptology

Draft, Nov 3, 2026. Level 1 is what cryptology over the Gaussian integers needs, ranked by how often the papers need it (from `research/gaussian-crypto-survey.md`, section 5). Level 2 is the number-theory operations Part 1 must provide for Level 1, with their status. Operations that number theory needs but cryptology does not are listed last, as deviations to discuss.

Status: **decided** (a textbook answer is decided), **studied** (built and measured, not yet classified as textbook), **missing**.

## Level 1: what cryptology needs

1. Arithmetic modulo a Gaussian modulus: a Gaussian prime π, a rational integer N, or a product of Gaussian primes. Reduce, add, multiply, square.
2. Powers modulo it, with exponents of 1 to 2 times the modulus size.
3. Inverses modulo it.
4. Sizes of 1,024 to 4,096 bits, fixed for a whole computation, in constant time.
5. The gcd and the extended gcd.
6. Gaussian primality testing and prime generation.
7. The Chinese remainder theorem over the Gaussian integers.
8. Square roots modulo a Gaussian prime.
9. Division with remainder, exact division, norms, standard associates.
10. Later: polynomials over the Gaussian integers mod q (NTRU-like schemes), the quartic residue symbol.

## Level 2: the number-theory operations behind them

| Operation | Serves | Status |
|---|---|---|
| Creating numbers, reading parts, equality, is zero | all | simple, not yet written down |
| Add, subtract, negate, conjugate, multiply at 32-bit halves | all | decided |
| The same at 1,024 to 4,096 bits (32 to 128 pieces per half), fixed width | 4 | missing |
| Reduction mod an integer n: Barrett at one piece | 1 | decided (38) |
| Reduction mod an integer n at many pieces | 1, 4 | missing |
| Reduction mod a Gaussian modulus | 1 | missing; key simplification below |
| Fused modular multiply and square | 1, 2 | missing |
| Modular power, constant time | 2, 6, 8 | missing |
| Inverse mod n: norm route with Euclid | 3 | decided, not constant time |
| Inverse mod n in constant time (Bernstein–Yang for the odd part, Newton mod 2^k, combined by CRT) | 3, 4 | research (53) |
| gcd | 5, 6 | decided (31, 39), not constant time |
| Extended gcd | 5, 7 | studied in plain registers (14); vector version missing |
| Chinese remainder theorem | 7 | missing |
| Primality: test the norm (split primes) or the rational prime (inert primes) | 6 | missing; uses modular power on ordinary integers held as b = 0 |
| Prime generation: a prime p ≡ 1 mod 4, a square root of −1 mod p, then gcd(p, t + i) | 6 | missing; needs modular power, square root, gcd |
| Square roots mod p and in the field with p² elements | 8 | missing; built on modular power |
| Random residues for keys | 4, 6 | missing; needs a randomness source outside the library |
| Division with remainder, exact division, norm, standard associate | 9 | decided (28, 29, 44, 45, 52; associate in 41) |

**The key simplification.** Arithmetic modulo a Gaussian prime collapses to ordinary modular arithmetic. For π with norm p ≡ 1 (mod 4), the ring Z[i]/(π) is the integers mod p, through a + bi ↦ a + b·t (mod p), with t² ≡ −1 (mod p) (studies 5 and 13). For an inert prime q ≡ 3 (mod 4), Z[i]/(q) is Gaussian integers mod the rational integer q, which is our mod n with n = q. Products of such primes follow by the Chinese remainder theorem. So "arithmetic mod a Gaussian modulus" may need no new reduction method, only the mapping, mod n at many pieces, and the CRT.

## Deviations: needed by number theory, not by cryptology

| Operation | Serves | Status |
|---|---|---|
| Batched small-norm primality and enumeration (norms up to 2^64 to 2^128) | the Gaussian moat (Erdős #952), prime distribution | missing; the clearest fit for NEON and GPUs |
| Factorization (reduces to factoring the norm) | the Gaussian Lehmer totient problem, φ over Z[i] | missing; low priority for cryptology |
| Quartic residue symbol | reciprocity research | missing; niche |
