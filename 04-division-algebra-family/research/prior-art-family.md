# Prior art for the division-algebra family (research A)

## Exact complex, quaternion and octonion arithmetic
- Common Lisp (CLtL2, 1984/1990): `complex` with rational parts is a built-in exact type; a complex rational with zero imaginary part is canonicalised to a rational. Exact Gaussian rationals as pairs of rationals.
  https://faculty.cc.gatech.edu/~isbell/references/lisp/clm/node20.html (robots-blocked; standard reference)
- SageMath rational quaternion algebras (e.g. QuaternionAlgebra(QQ, -1, -1)): an element is (x + yi + zj + wk)/d, four integers over one shared denominator (`denominator_and_integer_coefficient_tuple`). Exact, per-element shared denominator.
  https://doc.sagemath.org/html/en/reference/quat_algebras/sage/algebras/quatalg/quaternion_algebra_element.html
- Generic quaternion types over any scalar: Boost.Math `quaternion<T>`, Julia Quaternions.jl, Eigen `Quaternion<Scalar>`: exact if instantiated with a rational type, as four independent rationals.
  https://www.boost.org/doc/libs/1_34_0/doc/html/boost_math/quaternions.html ; https://github.com/mikmoore/Quaternions.jl
- Hurwitz integer quaternions (Python `hurwitz`): exact, integer and half-integer coordinates only. https://pypi.org/project/hurwitz/
- Octonions and Cayley-Dickson algebras: generic libraries (Julia CayleyDickson.jl, JS cayley-dickson, Perl CayleyDickson, Python hypercomplex, Rust `octonion`), mostly floating point or generic.
  https://github.com/meirizarrygelpi/CayleyDickson.jl ; https://docs.rs/octonion ; https://pypi.org/project/hypercomplex
- Numerical quaternion linear algebra (QuatIca, 2026): floating point. https://arxiv.org/abs/2603.24074

## Fixed-width integers of any size (the uN idea)
- C23 `_BitInt(N)`: fixed-width integers of any width up to BITINT_MAXWIDTH; clang 18 here reports 8,388,608 bits. LLVM's iN types underneath.
  https://llvm.googlesource.com/llvm-project/clang/+/refs/heads/main/test/Sema/large-bit-int.c
- 256-bit integers are standard in blockchain software (Solidity `uint256`, Rust `primitive-types::U256`).
- FLINT 3.4 `nfixed`: fixed-point numbers of 2 to 8 limbs with specialised add, multiply, dot products and matrix multiplication (for floating-point matrix code). https://fossies.org/dox/flint-3.4.0/nfixed_8c.html

## Units and division algebras
- Hurwitz's theorem (1898): the reals, complex numbers, quaternions and octonions are the only normed division algebras over the reals. The signed basis units: {+-1} (group of order 2), {+-1, +-i} (cyclic of order 4), {+-1, +-i, +-j, +-k} (the quaternion group, called Q8), and the 16 signed octonion units (a Moufang loop, not a group: multiplication is not associative).
- C99 `_Imaginary` (Kahan): a separate imaginary type, so real + imaginary = complex.

## What appears new
Nothing found combines (1) a per-value tag naming one signed basis unit of a division algebra, packed with the slash of a floating-slash rational into one byte, (2) one family across the four division algebras sized so the tag and slash fill a byte, and (3) a shared-denominator vector form across elements. Exact quaternions with a shared denominator per quaternion exist (Sage); exact complex rationals exist (Common Lisp); parametric fixed-width integers exist (C23).
