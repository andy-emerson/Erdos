# Open decisions

Kept for the author (Nov 2, 2026). None blocks the notes.

1. **Sign-extending header loads.** The new header add assumes the header byte is loaded sign-extended (same cost as a plain load; only the register changes; subtract then flips with XOR −128). Recommended: adopt. Without it the add is one step longer.
2. **Zero's direction in the packed design.** With the direction in the top bits, multiply and divide can leave a zero in any direction. Consistent with "any zero equals any zero"; matters only if zero needs one standard form (e.g. comparing numbers by their words). Settle with the packed design.
3. **Conditional moves (`cmov`, `csel`).** One instruction does conditional choice on x86-64 and ARM64; base RISC-V lacks it, so the builds avoid it. Question: time an extra row that uses them?
4. **What to do with the complex integer (formerly Design C).** It stays in the notes as one of four designs; its future is open.
