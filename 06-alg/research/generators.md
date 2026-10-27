# How many generators should the unit byte hold?

Research note for the planned `Alg<T>` specification (no spec text yet). Question: the unit of `Alg` is a Clifford basis blade with a sign. How many generators should the format support, which algebras does each choice bring in, and what does it cost?

## The unit

- A unit is a sign bit plus an n-bit blade mask (bit i set ⇔ generator eᵢ is in the blade): **n + 1 bits**, leaving **7 − n** of the byte.
- The signature Cl(p, q, r) — how many generators square to +1, −1 and 0 — is part of the type, not the value. It costs nothing per value.
- Product of two units: mask = a ⊕ b; sign = parity of the swaps needed to reorder the generators, times the squares of the shared generators (a ∧ b). The result is zero when a shared generator squares to 0, i.e. `(a & b & null_mask) != 0`.
- The sign can be computed (a few popcounts) or read from a table of 4ⁿ bits per signature: 32 bytes at n = 4, 128 B at n = 5, 2 KB at n = 7. All fit in L1.
- Zero: with a coefficient, a zero product puts 0 in the coefficient. The bare `Alg<One>` has no coefficient, so a degenerate signature (r > 0) needs one reserved zero pattern there.

## The key point: the format commits to n_max, not to a list of algebras

Because the signature lives in the type, **every Cl(p, q, r) with p + q + r ≤ n_max comes for free**. The decision is n_max (and whether the spare bits get a fixed meaning), not which algebras to include.

## What each generator count brings in

| n | Algebras newly reachable | Main uses | Bits left |
|---|---|---|---|
| 1 | Cl(0,1) complex numbers; Cl(1,0) split-complex (hyperbolic) numbers; Cl(0,0,1) dual numbers | everything complex; 1+1 Lorentz boosts; forward automatic differentiation, one direction [5][6] | 6 |
| 2 | Cl(0,2) quaternions; Cl(2,0) plane geometric algebra (≅ 2×2 real matrices); Cl(1,1) | 3D rotations; 2D geometry | 5 |
| 3 | Cl(3,0) 3D geometric algebra (Pauli algebra; its rotors are the quaternions); Cl(2,0,1) 2D projective geometric algebra; Cl(0,3) | 3D vectors, rotations and reflections in one algebra; 2D rigid motions | 4 |
| 4 | Cl(3,0,1) 3D projective geometric algebra (its even part is the dual quaternions); Cl(1,3) spacetime algebra (Dirac algebra); Cl(3,1) 2D conformal algebra | 3D rigid motions in graphics and robotics, with libraries such as Klein and others listed at bivector.net [1][2]; special relativity and the Dirac equation | 3 |
| 5 | Cl(4,1) 3D conformal geometric algebra | spheres, circles and their intersections as single objects; robotics and vision (GAFRO, Gaalop) [3][4] | 2 |
| 6 | Cl(3,3) line geometry (Plücker coordinates) [7] | kinematics of lines and screws; niche | 1 |
| 7 | Cl(0,7), Cl(4,3), … | mostly theoretical | 0 |
| > 7 | Cl(4,4) "mother algebra"; quadric algebra Cl(9,6) (n = 15) | research | out of reach in a byte |

## Costs that grow with n

| n | Dense sum (one coefficient per blade) | Dense × dense product (coefficient multiplications) | Sign table |
|---|---|---|---|
| 1 | 2 | 4 | 4 bits |
| 2 | 4 | 16 | 16 bits |
| 3 | 8 | 64 | 64 bits |
| 4 | 16 | 256 | 32 B |
| 5 | 32 | 1,024 | 128 B |
| 6 | 64 | 4,096 | 512 B |
| 7 | 128 | 16,384 | 2 KB |

The single-term `Alg` costs the same at every n (one mask XOR, one sign, one coefficient product). The growth only hits the dense sum container; sparse sums pay per nonzero term. Real applications rarely fill all blades (a 3D PGA motor uses 8 of 16, a CGA point 5 of 32).

## Two caveats found

1. **Octonions are not in any Clifford algebra.** Clifford algebras are associative; octonions are not. Octonions would need their own small type (decided earlier).
2. **Clifford infinitesimals anticommute, so they give first derivatives but not second.**
   - With one null generator, Cl(0,0,1) is exactly the dual numbers.
   - With k null generators, first-order derivatives in k directions are correct, read from the grade-1 part.
   - Hyper-dual numbers, used for exact second derivatives, need ε₁ε₂ = ε₂ε₁. In a Clifford algebra ε₁ε₂ = −ε₂ε₁, so the mixed second-derivative terms cancel.
   - So automatic differentiation beyond first order is not covered. It would need a separate commutative type, like octonions.

## Options

- **A. n_max = 4, 3 spare bits.** Covers complex numbers, quaternions, dual numbers, 3D geometric algebra, 2D and 3D projective geometric algebra (rigid motions), spacetime algebra and 2D conformal geometric algebra. Loses 3D conformal geometric algebra.
- **B. n_max = 5, 2 spare bits.** Adds 3D conformal geometric algebra, the main model in geometric algebra robotics and vision work. Dense sums are at most 32 coefficients.
- **C. n_max = 7, 0 spare bits.** Everything a byte can hold, with nothing left for flags.
- **D. n as a type parameter, like the signature.** The byte layout is always sign plus mask in the low bits, so the byte supports any n ≤ 7, and the spare bits are whatever each type leaves. Nothing about n is fixed globally. The catch is that a feature using spare bits only exists for types small enough to leave them. A rule like "spare-bit features use at most 2 bits" keeps them available up to n = 5.

**Lean: D, with any spare-bit features limited to 2 bits.** That gives B's guarantee (everything up to 3D conformal algebra keeps the features) without forbidding n = 6 or 7 for those who don't need them.

## Sources

1. [Klein: 3D projective geometric algebra library (C++)](https://github.com/jeremyong/Klein)
2. [bivector.net: geometric algebra libraries](https://bivector.net/lib.html)
3. [Löw, Abbet, Calinon (2024): GAFRO, geometric algebra for robotics](https://calinon.ch/paper4065.htm)
4. [Hildenbrand, *Introduction to Geometric Algebra Computing* (Gaalop)](https://bvbvf1.bib-bvb.de/vufind/Record/DE-604.BV047442411/TOC)
5. [ForwardModeAD: dual numbers](https://forwardmodead.readthedocs.io/en/latest/background/dualnumbers.html)
6. [Baydin et al., Automatic differentiation in machine learning: a survey](https://arxiv.org/pdf/1502.05767)
7. [Klawitter, A Clifford algebraic approach to line geometry](https://arxiv.org/pdf/1311.0131)
