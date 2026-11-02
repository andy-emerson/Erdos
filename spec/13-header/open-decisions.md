# Open decisions

Kept for the author (Nov 2, 2026). None blocks the notes.

1. **Sign-extending header loads.** The new header add assumes the header byte is loaded sign-extended (same cost as a plain load; only the register changes; subtract then flips with XOR −128). Recommended: adopt. Without it the add is one step longer.
2. **Zero's direction in the packed design.** With the direction in the top bits, multiply and divide can leave a zero in any direction. Consistent with "any zero equals any zero"; matters only if zero needs one standard form (e.g. comparing numbers by their words). Settle with the packed design.
3. **Conditional moves (`cmov`, `csel`).** One instruction does conditional choice on x86-64 and ARM64; base RISC-V lacks it, so the builds avoid it. Question: time an extra row that uses them?
4. **What to do with the complex integer (formerly Design C).** It stays in the notes as one of four designs; its future is open.
5. **Binary or Gray-coded header.** The Gray-coded header (1, i, −1, −i as 0x00, 0x40, 0xFF, 0xBF) makes add 10 steps (complex 16) against 12 (17), but complex multiply and divide 8 and 9 against 2 (prior-art.md, round 2). Better for integers everywhere; for complex numbers only if adds dominate.
