# Open decisions

Kept for the author (Nov 2, 2026). None blocks the notes.

1. **Sign-extending header loads.** The new header add assumes the header byte is loaded sign-extended (same cost as a plain load; only the register changes; subtract then flips with XOR −128). Recommended: adopt. Without it the add is one step longer.
2. **Zero's direction in the packed design.** With the direction in the top bits, multiply and divide can leave a zero in any direction. Consistent with "any zero equals any zero"; matters only if zero needs one standard form (e.g. comparing numbers by their words). Settle with the packed design.
3. **Conditional moves (`cmov`, `csel`).** One instruction does conditional choice on x86-64 and ARM64; base RISC-V lacks it, so the builds avoid it. Question: time an extra row that uses them?
4. **What to do with the complex integer (formerly Design C).** It stays in the notes as one of four designs; its future is open.
5. **Binary or Gray-coded header.** The Gray-coded header (1, i, −1, −i as 0x00, 0x40, 0xFF, 0xBF) makes add 10 steps (complex 16) against 12 (17), but complex multiply and divide 8 and 9 against 2 (prior-art.md, round 2). Better for integers everywhere; for complex numbers only if adds dominate.

## Ideas to return to

- **Idea 6, the leak as a parity detector** (Nov 2, 2026). In complex rationals with the axis in the denominator's top bit, raw products put parity information into the top bit. Reducing a fraction needs to know when both parts are even; the leak may supply that for free. Waits until reduction is in scope.
- **Ideas 4 + 6, the direction counter on the odd part** (shelved Nov 2, 2026). Correct for every denominator by strip-and-rebuild (test/playground/order4_rebuild.py), but about 20–25 instructions per add or multiply, against about 10–12 for the signed-numerator, complement-axis design. The cost is structural: every result needs its common factors of 2 removed and its counter moved.
- **Edge correctness for complex rationals** (Nov 2, 2026; not speed, for after the main problem). Free tags: in the negated-denominator layout a denominator word of 0 or −2^63 is never valid, so either can mark overflow or a big number at no cost until checked. Exclude −2^63 from numerators so every numerator can be negated (catching a result that lands there belongs with overflow).
- **Library-level tricks for later** (from book/v0.1.md Part 2 and spec 11): special cases (branches at machine level, so only where predictable), dividing out first (Henrici; needs the gcd), delayed reduction (needs the gcd), the three-word (p + qi)/D form, Gauss's three-multiplication product for a + bi, collapsing a pair when a part is zero.
