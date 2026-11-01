# Book

The book: Part 1, the machine; Part 2, the library. The current draft is [v0.1](v0.1.md).

- **v0.1** (Nov 1, 2026): the paper v0.7 (now Part 2) and the machine-level supplement (now Part 1), put together unchanged. Inconsistencies between them are left for later versions.
- **Plan:** Part 1's chapter 1 holds all the mathematics of the number system (magnitude arithmetic mod $2^w$, direction arithmetic mod 4, why splitting them works) and its chapter 2 the number system built, so the mathematics can be checked before the code; later chapters are the operations. Part 2 builds the library on Part 1, with expressions combining numbers into what people use. Once both are correct, the book may be reordered to alternate an idea with its code. Only one of spec 13's two number designs goes in, chosen by its results.
- **Scope (author, Nov 1, 2026):** two parts only. Applications such as korselt (github.com/andy-emerson/korselt, certified searches over factorizations) are out of scope; if korselt later reaches a new bound using this library, the book may end with a short epilogue on it.
- **Checks:** [`verify-part1/`](verify-part1/README.md) (Part 1's claims and machine code) and [`verify-part2/`](verify-part2/README.md) (the paper's checks, reading only Part 2 of the book). Run `NIM=/path/to/nim python3 book/verify-part2/verify.py` from the repository root.
- **Research:** [`research/`](research/), including the survey of AI writing patterns behind RULES.md's prose rules.
- **Earlier:** the paper's versions v0.1 to v0.7 and the supplement are in [`../archive/paper/`](../archive/paper/).
