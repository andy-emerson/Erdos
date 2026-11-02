# Spec 13 v0.2, round 2: results

Nov 1, 2026. Spec §6 for Control, A, A's branch-free row, B and C (C added by the coordinator during the round), on integers and on complex numbers on the axes. The Lean is in [`lean/`](lean/) (Lean 4.35.0, core only); its README lists every theorem. No timed runs were made.

## 1. The instruction set

`lean/Header/Isa.lean` defines 36 instructions and an interpreter for them. A register holds a word, a natural number below $W = 2^{64}$; register 0 reads 0. Memory maps an address to a cell: a word cell for `ld`/`sd`, a byte cell for `lbu`/`sb` (read mod 256, written mod 256). The model does not split a word into its bytes; no program has a byte and a word at overlapping addresses. An address is a register plus a constant.

The instructions are `add sub neg and or xor` (register), `addi andi ori xori` (constant), `slli srli srai` and `sll srl sra` (shifts), `sltu sltiu` (set-less-than-unsigned), `li mv`, `mul` (low 64 bits), `mulhu` (high 64 bits, unsigned), `divu` and `div` (64/64, unsigned and signed, truncating), `ld lbu sd sb`, the compare-and-branches `beq bne bltu bgeu beqz bnez`, `j`, and `trap` (exit to the error handler). There is no flag, no conditional move and no 128/64 divide. A borrow is a value (`sltu`) or the condition of a compare-and-branch. Signed division follows `Int.tdiv`; on a zero divisor the model gives 0 (the machines differ, and no program divides by zero).

The interpreter runs a program from instruction 0 and records the index of every instruction executed (the trace) and every store as an (address, value) pair. A run ends normally at one past the last instruction, or with an error at `trap`. Every branch and jump of every program goes forward (`all_forward`), so a program runs each instruction at most once.

Placement is the same rule for every design: the caller passes one register per list per number, holding the address of that number's entry. Control, B and C have one list, so registers 1, 2 and 3 hold the addresses of `x`, `y` and the result `z`. A has two parallel lists, so registers 1 to 6 hold the addresses of `x`'s header and word, `y`'s, and `z`'s. Inside an entry, Control's complex number is (real, imaginary) at +0 and +8; B's integer is P at +0 and N at +8; B's complex number has slot k at +8k; C's number, integer or complex, is one word. On every path every load comes before every store (`all_loads_first`), so a result may share storage with an operand.

### Steps that are not one instruction everywhere

RISC-V (RV64IM) takes every step as one instruction. ARM64 and x86-64 do not, for five kinds of step:

| Step | x86-64 | ARM64 | RV64IM |
| --- | --- | --- | --- |
| `sltu rd, a, b` | `cmp; setb; movzx` (3); `setb; movzx` (2) right after `sub` of the same registers | `cmp; cset` (2); `cset` (1) after `subs` | `sltu` |
| `bltu/bgeu/beq/bne a, b` | `cmp; jcc` (2, one fused uop); `jcc` (1) right after `sub` of the same registers | `cmp; b.cond` (2); `b.cond` after `subs` | 1 |
| `beqz/bnez r` | `test; jcc` (2, fused); `jcc` (1) after an ALU instruction that wrote `r` | `cbz/cbnz` (1) | 1 |
| `divu`, `div` | `xor edx, edx; div` and `cqo; idiv` (2), dividend and quotient in `rax` | `udiv`, `sdiv` | `divu`, `div` |
| `rd ← rs1 op rs2` with `rs1` still needed | `mov; op` (2), or `lea` for an add | 1 | 1 |

Folding goes the other way: an `sltiu` or `andi` whose only use is the zero-test right after it becomes the branch's `cmp`/`test`, and a shift left whose only use is the next `add` disappears into it (x86-64 `lea` with scale 2, 4 or 8; ARM64 `add rd, rn, rm, lsl #k`, any k). C's division ends in `slli; add`, which is one `lea` on x86-64 and one shifted `add` on ARM64. `Counts.lean` (`costAt`) states these rules, and the x86-64 and ARM64 columns below are computed with them. x86-64 can also fold a load into an ALU instruction (`add rdx, [mem]`); the column does not, and §7 lists where gcc does.

The full mapping:

| Step | x86-64 | ARM64 | RV64IM |
| --- | --- | --- | --- |
| `add`, `addi` | `add` or `lea` | `add` | `add`, `addi` |
| `sub` | `sub` | `sub` (`subs` when a branch reuses it) | `sub` |
| `neg` | `neg` | `neg` | `neg` (`sub rd, x0, rs`) |
| `and or xor`, `andi ori xori` | `and or xor` (imm8 or imm32) | `and orr eor` (bitmask immediates; 0xC0, 0x80, 0x40, 1, 3 and ~1 all encode) | same names |
| `slli srli srai` | `shl shr sar` | `lsl lsr asr` | same names |
| `sltu`, `sltiu` | `cmp; setb; movzx` | `cmp; cset lo` | `sltu`, `sltiu` |
| `li`, `mv` | `mov` | `mov` | `li`, `mv` |
| `mul` | `imul r64, r64` | `mul` | `mul` |
| `mulhu` | `mul` (rdx:rax) or `mulx` | `umulh` | `mulhu` |
| `divu`, `div` | `xor edx,edx; div` / `cqo; idiv` | `udiv`, `sdiv` | `divu`, `div` |
| `ld`, `lbu` | `mov`, `movzx` | `ldr`, `ldrb` | `ld`, `lbu` |
| `sd`, `sb` | `mov` (`mov qword [m], 0` for register 0) | `str`, `strb` (`xzr` for 0) | `sd`, `sb` (`x0` for 0) |
| `beqz bnez` | `test; jz/jnz` | `cbz cbnz` | `beqz bnez` |
| `bltu bgeu beq bne` | `cmp; jb/jae/je/jne` | `cmp; b.lo/b.hs/b.eq/b.ne` | same names |
| `j` | `jmp` | `b` | `j` |
| `trap` | `call` to the handler | `bl` | `jal` |

## 2. The programs

Forty-six programs, listed in full in the appendix and defined in `Control.lean`, `ProgA.lean`, `ProgB.lean` and `ProgC.lean`. They follow §2 to §4a and the notes' tree:

- Control: the machine's signed arithmetic. Complex division by a number on an axis branches once, on whether the divisor's imaginary part is 0: a real divisor divides both parts, an imaginary one `s·i` gives `(a_im / s, −(a_re / s))`.
- A, integers: the direction check `t = (h1 xor h2) and D`, one branch to same or opposite. A, complex: the notes' whole tree, testing same direction, then different axes (`t and 0x40`), then the borrow; on different axes `m1 = 0`, then `m2 = 0`, else `trap`. Subtraction is the add program with `xori h2, h2, 0x80` after the loads. Negate, multiply and divide have no branch and serve integers and complex numbers alike.
- The opposite-direction path is written both ways, as the coordinator asked: the notes' form (`sub`, branch on the borrow, `neg` and take `h2` on a borrow) and the per-arm form (`bltu m1, m2`, then `m1 − m2` or `m2 − m1`, no `neg`). So are B's collapse and B's division.
- A's branch-free row: `o = t >> 7` (integers) or `o = (t xor 0x80) <u 1` (complex), masks `0 − o` and `0 − b` with `b = (m1 <u m2) and o`; the opposite difference is negated by `(d xor mb) − mb` and selected by `s xor ((s xor d) and mo)`; direction `(h1 + (b shl 7)) and D`. The complex row also forms `a` (different axes), `z1`, `z2`, replaces `h1`'s direction by `h2`'s when `m1 = 0` across axes, and leaves the error flag `a and not (z1 or z2)` in register 21 for the caller. It stores on every input, the error case included.
- B: part by part for add, subtract (crossed), negate (swap or rotate by 2) and multiply (4 or 16 products; a complex slot is stored as soon as it is done). Collapse per axis, as in A's add. Division collapses both operands (borrows kept as values by `sltu` for the direction), divides, and routes the quotient by address: it stores 0 to every part, then the quotient to part `(b1 − b2) and 1` (integers) or slot `(d1 − d2) and 3` (complex). So it stores 3 or 5 times and writes one part twice.
- C: add and subtract branch on the axis bit, then on `w2 <u 2` and `w1 <u 2`, then `trap`; the cross-axis results (`w1`, `w2`, or the negation of `w2`) are formed before the zero test that selects them. Multiply negates by the mask `0 − (w1 and w2 and 1)` instead of a branch. Division is the current rule step for step. One program per operation serves integers and complex numbers.

## 3. Correctness

Proved for every input, with no sampling (`Control.lean`, `CorrectA.lean`, `CorrectB.lean`, `CorrectC.lean`). Each `_run` theorem runs a program from the start and states how it ends, its exact trace, and its exact stores, in terms of round 1's definitions:

- Control: the stored words are `ofInt` of the true integer or Gaussian-integer result, so read as signed they are the result whenever it fits (`toInt_ofInt`). Division: when `a = q·b`, all parts fit, and (complex) `b` is a nonzero number on an axis, the stores are `q`.
- A: `intAdd_run` (both forms, add and subtract, integer directions) and `cxAdd_run` (both forms, all directions) store round 1's `A.add W x y` (subtraction: `A.add W x (A.neg y)`, which is `A.sub`), as header `64·d` and word; `cxAdd_run` ends at `trap` exactly when `A.add` gives an expression. `neg_run`, `mul_run`, `div_run` store `A.neg`, `(d1 + d2, m1·m2 mod 2^64)` and `A.div`. `bfInt_run` gives the same stores as the tree for integers. The complex branch-free row's correctness (`bfCx_run`: the tree's stores, and register 21 equal to 1 exactly when `A.add` gives an expression) is not yet in the build: its proof reaches the end of every case in about 10 seconds per direction pair, but Lean then spends more than 50 minutes finishing each quarter of it, so the eight files are parked in `lean/Pending/` and it remains to be done. Its counts and its absence of branches are proved (§4). Round 1's `add_value`, `sub_value`, `val_mul` and `val_div_mul` then give the values under their no-overflow hypotheses.
- B: add, subtract and multiply store the round 1 parts mod 2^64 (`add_value`, `sub_value`, `mul_value` give `B.add`, `B.sub`, `B.mul` under no overflow; `cmul_slots` writes out `B4.mul`); negate stores `B.neg`; collapse stores `B.collapse` per axis, both forms; integer division stores `(q, 0)` or `(0, q)` by the signs, and `div_value` proves the stored pair collapsed and times `y` equal to `x`; complex division, for operands on axes, stores round 1's `A.div` of the two operands read as A numbers into its slot, and `cdiv_value` proves its value times `y` is `x`.
- C: `add_run`, `sub_run`, `neg_run`, `mul_run`, `div_run` store round 1's `C.add`, `C.sub`, `C.neg`, `C.mul`, `C.div` (the current rule), as `BitVec 64` words; add and subtract end at `trap` exactly when round 1 gives an expression. A bridge section in `ProgC.lean` proves each instruction computes, on `toNat`, the `BitVec` operation of the same name (`div_bv` through `BitVec.toInt_sdiv`, `sra1_bv` through `BitVec.toInt_sshiftRight`).

No sorry, admit or native_decide; `#print axioms` on every round 2 theorem shows only `propext`, `Quot.sound` and `Classical.choice` (§9).

## 4. Counts, path by path

`Counts.table_eq` proves the whole table below by evaluation, from the programs: for every path, the steps executed, conditional branches and how many were taken, unconditional jumps, loads, stores, the machine instructions on each family by the rules of §1, and the branch log (each conditional branch by instruction index, T taken, N not). The paths are the traces the `_run` theorems name, as functions of the inputs, and those theorems hold for every input, so the listed paths cover every input. `bf_no_branch` proves that the four programs of the branch-free row, and A's negate, multiply and divide that it shares, have no conditional branch and no jump; `straight_line` proves the same for the other one-path programs.

The paths of B's complex division are the nine whose operands lie on axes (at most one borrow per operand); the program has sixteen branch combinations in all, and the other seven need an operand with a nonzero part on both axes, outside the precondition. A path name "borrows x ab" gives the real and imaginary borrows of `x`.

| Design | Operation | Path | Steps | Branches (taken) | Jumps | Loads | Stores | x86-64 | ARM64 | RV64 | Branch log |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Control | int add | one | 4 | 0 (0) | 0 | 2 | 1 | 4 | 4 | 4 |  |
| Control | int sub | one | 4 | 0 (0) | 0 | 2 | 1 | 4 | 4 | 4 |  |
| Control | int neg | one | 3 | 0 (0) | 0 | 1 | 1 | 3 | 3 | 3 |  |
| Control | int mul | one | 4 | 0 (0) | 0 | 2 | 1 | 4 | 4 | 4 |  |
| Control | int div | one | 4 | 0 (0) | 0 | 2 | 1 | 5 | 4 | 4 |  |
| Control | cx add | one | 8 | 0 (0) | 0 | 4 | 2 | 8 | 8 | 8 |  |
| Control | cx sub | one | 8 | 0 (0) | 0 | 4 | 2 | 8 | 8 | 8 |  |
| Control | cx neg | one | 6 | 0 (0) | 0 | 2 | 2 | 6 | 6 | 6 |  |
| Control | cx mul | one | 12 | 0 (0) | 0 | 4 | 2 | 14 | 12 | 12 |  |
| Control | cx div | real divisor | 10 | 1 (0) | 1 | 4 | 2 | 13 | 10 | 10 | 4 N |
| Control | cx div | imag divisor | 10 | 1 (1) | 0 | 4 | 2 | 13 | 10 | 10 | 4 T |
| A | int add (notes) | same | 11 | 1 (1) | 0 | 4 | 2 | 11 | 11 | 11 | 6 T |
| A | int add (notes) | opp, no borrow | 12 | 2 (1) | 0 | 4 | 2 | 13 | 12 | 12 | 6 N, 8 T |
| A | int add (notes) | opp, borrow | 15 | 2 (0) | 1 | 4 | 2 | 16 | 15 | 15 | 6 N, 8 N |
| A | int sub (notes) | same | 12 | 1 (1) | 0 | 4 | 2 | 12 | 12 | 12 | 7 T |
| A | int sub (notes) | opp, no borrow | 13 | 2 (1) | 0 | 4 | 2 | 14 | 13 | 13 | 7 N, 9 T |
| A | int sub (notes) | opp, borrow | 16 | 2 (0) | 1 | 4 | 2 | 17 | 16 | 16 | 7 N, 9 N |
| A | int add (perArm) | same | 11 | 1 (1) | 0 | 4 | 2 | 11 | 11 | 11 | 6 T |
| A | int add (perArm) | opp, no borrow | 13 | 2 (0) | 1 | 4 | 2 | 14 | 14 | 13 | 6 N, 7 N |
| A | int add (perArm) | opp, borrow | 14 | 2 (1) | 1 | 4 | 2 | 15 | 15 | 14 | 6 N, 7 T |
| A | int sub (perArm) | same | 12 | 1 (1) | 0 | 4 | 2 | 12 | 12 | 12 | 7 T |
| A | int sub (perArm) | opp, no borrow | 14 | 2 (0) | 1 | 4 | 2 | 15 | 15 | 14 | 7 N, 8 N |
| A | int sub (perArm) | opp, borrow | 15 | 2 (1) | 1 | 4 | 2 | 16 | 16 | 15 | 7 N, 8 T |
| A | cx add (notes) | same | 11 | 1 (1) | 0 | 4 | 2 | 11 | 11 | 11 | 6 T |
| A | cx add (notes) | opp, no borrow | 14 | 3 (1) | 0 | 4 | 2 | 15 | 14 | 14 | 6 N, 8 N, 10 T |
| A | cx add (notes) | opp, borrow | 17 | 3 (0) | 1 | 4 | 2 | 18 | 17 | 17 | 6 N, 8 N, 10 N |
| A | cx add (notes) | cross, x = 0 | 15 | 3 (2) | 0 | 4 | 2 | 16 | 15 | 15 | 6 N, 8 T, 14 T |
| A | cx add (notes) | cross, y = 0 | 15 | 4 (2) | 0 | 4 | 2 | 17 | 15 | 15 | 6 N, 8 T, 14 N, 15 T |
| A | cx add (notes) | cross, expression | 12 | 4 (1) | 0 | 4 | 0 | 14 | 12 | 12 | 6 N, 8 T, 14 N, 15 N |
| A | cx sub (notes) | same | 12 | 1 (1) | 0 | 4 | 2 | 12 | 12 | 12 | 7 T |
| A | cx sub (notes) | opp, no borrow | 15 | 3 (1) | 0 | 4 | 2 | 16 | 15 | 15 | 7 N, 9 N, 11 T |
| A | cx sub (notes) | opp, borrow | 18 | 3 (0) | 1 | 4 | 2 | 19 | 18 | 18 | 7 N, 9 N, 11 N |
| A | cx sub (notes) | cross, x = 0 | 16 | 3 (2) | 0 | 4 | 2 | 17 | 16 | 16 | 7 N, 9 T, 15 T |
| A | cx sub (notes) | cross, y = 0 | 16 | 4 (2) | 0 | 4 | 2 | 18 | 16 | 16 | 7 N, 9 T, 15 N, 16 T |
| A | cx sub (notes) | cross, expression | 13 | 4 (1) | 0 | 4 | 0 | 15 | 13 | 13 | 7 N, 9 T, 15 N, 16 N |
| A | cx add (perArm) | same | 11 | 1 (1) | 0 | 4 | 2 | 11 | 11 | 11 | 6 T |
| A | cx add (perArm) | opp, no borrow | 15 | 3 (0) | 1 | 4 | 2 | 16 | 16 | 15 | 6 N, 8 N, 9 N |
| A | cx add (perArm) | opp, borrow | 16 | 3 (1) | 1 | 4 | 2 | 17 | 17 | 16 | 6 N, 8 N, 9 T |
| A | cx add (perArm) | cross, x = 0 | 15 | 3 (2) | 0 | 4 | 2 | 16 | 15 | 15 | 6 N, 8 T, 15 T |
| A | cx add (perArm) | cross, y = 0 | 15 | 4 (2) | 0 | 4 | 2 | 17 | 15 | 15 | 6 N, 8 T, 15 N, 16 T |
| A | cx add (perArm) | cross, expression | 12 | 4 (1) | 0 | 4 | 0 | 14 | 12 | 12 | 6 N, 8 T, 15 N, 16 N |
| A | cx sub (perArm) | same | 12 | 1 (1) | 0 | 4 | 2 | 12 | 12 | 12 | 7 T |
| A | cx sub (perArm) | opp, no borrow | 16 | 3 (0) | 1 | 4 | 2 | 17 | 17 | 16 | 7 N, 9 N, 10 N |
| A | cx sub (perArm) | opp, borrow | 17 | 3 (1) | 1 | 4 | 2 | 18 | 18 | 17 | 7 N, 9 N, 10 T |
| A | cx sub (perArm) | cross, x = 0 | 16 | 3 (2) | 0 | 4 | 2 | 17 | 16 | 16 | 7 N, 9 T, 16 T |
| A | cx sub (perArm) | cross, y = 0 | 16 | 4 (2) | 0 | 4 | 2 | 18 | 16 | 16 | 7 N, 9 T, 16 N, 17 T |
| A | cx sub (perArm) | cross, expression | 13 | 4 (1) | 0 | 4 | 0 | 15 | 13 | 13 | 7 N, 9 T, 16 N, 17 N |
| A | neg | one | 5 | 0 (0) | 0 | 2 | 2 | 5 | 5 | 5 |  |
| A | mul | one | 9 | 0 (0) | 0 | 4 | 2 | 9 | 9 | 9 |  |
| A | div | one | 9 | 0 (0) | 0 | 4 | 2 | 10 | 9 | 9 |  |
| A-bf | int add | one | 23 | 0 (0) | 0 | 4 | 2 | 27 | 22 | 23 |  |
| A-bf | int sub | one | 24 | 0 (0) | 0 | 4 | 2 | 28 | 23 | 24 |  |
| A-bf | cx add | one | 35 | 0 (0) | 0 | 4 | 2 | 47 | 37 | 35 |  |
| A-bf | cx sub | one | 36 | 0 (0) | 0 | 4 | 2 | 48 | 38 | 36 |  |
| B | int add | one | 8 | 0 (0) | 0 | 4 | 2 | 8 | 8 | 8 |  |
| B | int sub | one | 8 | 0 (0) | 0 | 4 | 2 | 8 | 8 | 8 |  |
| B | int neg | one | 4 | 0 (0) | 0 | 2 | 2 | 4 | 4 | 4 |  |
| B | int mul | one | 12 | 0 (0) | 0 | 4 | 2 | 14 | 12 | 12 |  |
| B | int collapse (notes) | borrow 0 | 6 | 1 (1) | 0 | 2 | 2 | 7 | 6 | 6 | 3 T |
| B | int collapse (notes) | borrow 1 | 8 | 1 (0) | 1 | 2 | 2 | 9 | 8 | 8 | 3 N |
| B | int collapse (perArm) | borrow 0 | 7 | 1 (0) | 1 | 2 | 2 | 8 | 8 | 7 | 2 N |
| B | int collapse (perArm) | borrow 1 | 6 | 1 (1) | 0 | 2 | 2 | 7 | 7 | 6 | 2 T |
| B | int div (notes) | borrows x 0, y 0 | 18 | 2 (2) | 0 | 4 | 3 | 24 | 19 | 18 | 6 T, 10 T |
| B | int div (notes) | borrows x 0, y 1 | 19 | 2 (1) | 0 | 4 | 3 | 25 | 20 | 19 | 6 T, 10 N |
| B | int div (notes) | borrows x 1, y 0 | 19 | 2 (1) | 0 | 4 | 3 | 25 | 20 | 19 | 6 N, 10 T |
| B | int div (notes) | borrows x 1, y 1 | 20 | 2 (0) | 0 | 4 | 3 | 26 | 21 | 20 | 6 N, 10 N |
| B | int div (perArm) | borrows x 0, y 0 | 20 | 2 (0) | 2 | 4 | 3 | 26 | 23 | 20 | 5 N, 10 N |
| B | int div (perArm) | borrows x 0, y 1 | 19 | 2 (1) | 1 | 4 | 3 | 25 | 22 | 19 | 5 N, 10 T |
| B | int div (perArm) | borrows x 1, y 0 | 19 | 2 (1) | 1 | 4 | 3 | 25 | 22 | 19 | 5 T, 10 N |
| B | int div (perArm) | borrows x 1, y 1 | 18 | 2 (2) | 0 | 4 | 3 | 24 | 21 | 18 | 5 T, 10 T |
| B | cx add | one | 16 | 0 (0) | 0 | 8 | 4 | 16 | 16 | 16 |  |
| B | cx sub | one | 16 | 0 (0) | 0 | 8 | 4 | 16 | 16 | 16 |  |
| B | cx neg | one | 8 | 0 (0) | 0 | 4 | 4 | 8 | 8 | 8 |  |
| B | cx mul | one | 40 | 0 (0) | 0 | 8 | 4 | 52 | 40 | 40 |  |
| B | cx collapse (notes) | borrows re 0, im 0 | 12 | 2 (2) | 0 | 4 | 4 | 14 | 12 | 12 | 5 T, 13 T |
| B | cx collapse (notes) | borrows re 0, im 1 | 14 | 2 (1) | 1 | 4 | 4 | 16 | 14 | 14 | 5 T, 13 N |
| B | cx collapse (notes) | borrows re 1, im 0 | 14 | 2 (1) | 1 | 4 | 4 | 16 | 14 | 14 | 5 N, 13 T |
| B | cx collapse (notes) | borrows re 1, im 1 | 16 | 2 (0) | 2 | 4 | 4 | 18 | 16 | 16 | 5 N, 13 N |
| B | cx collapse (perArm) | borrows re 0, im 0 | 14 | 2 (0) | 2 | 4 | 4 | 16 | 16 | 14 | 4 N, 12 N |
| B | cx collapse (perArm) | borrows re 0, im 1 | 13 | 2 (1) | 1 | 4 | 4 | 15 | 15 | 13 | 4 N, 12 T |
| B | cx collapse (perArm) | borrows re 1, im 0 | 13 | 2 (1) | 1 | 4 | 4 | 15 | 15 | 13 | 4 T, 12 N |
| B | cx collapse (perArm) | borrows re 1, im 1 | 12 | 2 (2) | 0 | 4 | 4 | 14 | 14 | 12 | 4 T, 12 T |
| B | cx div (notes) | borrows x 00, y 00 | 40 | 4 (4) | 0 | 8 | 5 | 56 | 45 | 40 | 10 T, 14 T, 23 T, 27 T |
| B | cx div (notes) | borrows x 00, y 01 | 41 | 4 (3) | 0 | 8 | 5 | 57 | 46 | 41 | 10 T, 14 T, 23 T, 27 N |
| B | cx div (notes) | borrows x 00, y 10 | 41 | 4 (3) | 0 | 8 | 5 | 57 | 46 | 41 | 10 T, 14 T, 23 N, 27 T |
| B | cx div (notes) | borrows x 01, y 00 | 41 | 4 (3) | 0 | 8 | 5 | 57 | 46 | 41 | 10 T, 14 N, 23 T, 27 T |
| B | cx div (notes) | borrows x 01, y 01 | 42 | 4 (2) | 0 | 8 | 5 | 58 | 47 | 42 | 10 T, 14 N, 23 T, 27 N |
| B | cx div (notes) | borrows x 01, y 10 | 42 | 4 (2) | 0 | 8 | 5 | 58 | 47 | 42 | 10 T, 14 N, 23 N, 27 T |
| B | cx div (notes) | borrows x 10, y 00 | 41 | 4 (3) | 0 | 8 | 5 | 57 | 46 | 41 | 10 N, 14 T, 23 T, 27 T |
| B | cx div (notes) | borrows x 10, y 01 | 42 | 4 (2) | 0 | 8 | 5 | 58 | 47 | 42 | 10 N, 14 T, 23 T, 27 N |
| B | cx div (notes) | borrows x 10, y 10 | 42 | 4 (2) | 0 | 8 | 5 | 58 | 47 | 42 | 10 N, 14 T, 23 N, 27 T |
| B | cx div (perArm) | borrows x 00, y 00 | 44 | 4 (0) | 4 | 8 | 5 | 60 | 53 | 44 | 9 N, 14 N, 24 N, 29 N |
| B | cx div (perArm) | borrows x 00, y 01 | 43 | 4 (1) | 3 | 8 | 5 | 59 | 52 | 43 | 9 N, 14 N, 24 N, 29 T |
| B | cx div (perArm) | borrows x 00, y 10 | 43 | 4 (1) | 3 | 8 | 5 | 59 | 52 | 43 | 9 N, 14 N, 24 T, 29 N |
| B | cx div (perArm) | borrows x 01, y 00 | 43 | 4 (1) | 3 | 8 | 5 | 59 | 52 | 43 | 9 N, 14 T, 24 N, 29 N |
| B | cx div (perArm) | borrows x 01, y 01 | 42 | 4 (2) | 2 | 8 | 5 | 58 | 51 | 42 | 9 N, 14 T, 24 N, 29 T |
| B | cx div (perArm) | borrows x 01, y 10 | 42 | 4 (2) | 2 | 8 | 5 | 58 | 51 | 42 | 9 N, 14 T, 24 T, 29 N |
| B | cx div (perArm) | borrows x 10, y 00 | 43 | 4 (1) | 3 | 8 | 5 | 59 | 52 | 43 | 9 T, 14 N, 24 N, 29 N |
| B | cx div (perArm) | borrows x 10, y 01 | 42 | 4 (2) | 2 | 8 | 5 | 58 | 51 | 42 | 9 T, 14 N, 24 N, 29 T |
| B | cx div (perArm) | borrows x 10, y 10 | 42 | 4 (2) | 2 | 8 | 5 | 58 | 51 | 42 | 9 T, 14 N, 24 T, 29 N |
| C | add | same axis | 8 | 1 (1) | 0 | 2 | 1 | 9 | 8 | 8 | 4 T |
| C | add | cross, y = 0 | 9 | 2 (1) | 0 | 2 | 1 | 10 | 9 | 9 | 4 N, 7 T |
| C | add | cross, x = 0 | 12 | 3 (1) | 0 | 2 | 1 | 13 | 12 | 12 | 4 N, 7 N, 10 T |
| C | add | cross, expression | 12 | 3 (0) | 0 | 2 | 0 | 13 | 12 | 12 | 4 N, 7 N, 10 N |
| C | sub | same axis | 8 | 1 (1) | 0 | 2 | 1 | 9 | 8 | 8 | 4 T |
| C | sub | cross, y = 0 | 9 | 2 (1) | 0 | 2 | 1 | 10 | 9 | 9 | 4 N, 7 T |
| C | sub | cross, x = 0 | 14 | 3 (1) | 0 | 2 | 1 | 16 | 14 | 14 | 4 N, 7 N, 12 T |
| C | sub | cross, expression | 14 | 3 (0) | 0 | 2 | 0 | 16 | 14 | 14 | 4 N, 7 N, 12 N |
| C | neg | one | 5 | 0 (0) | 0 | 1 | 1 | 6 | 5 | 5 |  |
| C | mul | one | 14 | 0 (0) | 0 | 2 | 1 | 17 | 14 | 14 |  |
| C | div | one | 14 | 0 (0) | 0 | 2 | 1 | 16 | 13 | 14 |  |

In brief, on the common paths (steps on RV64IM; x86-64 in parentheses): Control integer operations 3 or 4 steps (3 to 5); A add 11 on the same-direction path (11), 12 to 16 on the opposite paths; A negate 5, multiply and divide 9; A's branch-free add 23 for integers and 35 for complex numbers (27 and 47); B add, subtract 8, multiply 12 (14), collapse 6 to 8, integer division 18 to 20 (24 to 26); B complex multiply 40 (52); C add and subtract 8 on the same axis (9), negate 5, multiply 14 (17), divide 14 (16).

## 5. Branch inventory

Every conditional branch, by program and instruction index (subtraction programs have the same branches one index later, and compare `y`'s direction turned by 2). "Taken" is proved by the branch logs of §4 together with the `_run` theorems' path functions.

| Program | Branch | Condition | Taken exactly when | Reached when |
| --- | --- | --- | --- | --- |
| Control complex div | 4 `bnez b_im` | divisor's imaginary part nonzero | the divisor is imaginary | always |
| A int add, notes | 6 `beqz t` | `t = 0` | `d1 = d2` | always |
| | 8 `bgeu m1, m2` | no borrow | `m2 ≤ m1` | `d2 = d1 + 2` |
| A int add, per arm | 6 `beqz t` | `t = 0` | `d1 = d2` | always |
| | 7 `bltu m1, m2` | borrow | `m1 < m2` | `d2 = d1 + 2` |
| A complex add, notes | 6 `beqz t` | `t = 0` | `d1 = d2` | always |
| | 8 `bnez (t and 0x40)` | axis bits differ | different axes | `d1 ≠ d2` |
| | 10 `bgeu m1, m2` | no borrow | `m2 ≤ m1` | opposite directions |
| | 14 `beqz m1` | `m1 = 0` | `m1 = 0` | different axes |
| | 15 `beqz m2` | `m2 = 0` | `m2 = 0` | different axes, `m1 ≠ 0` |
| A complex add, per arm | 6, 8, 15, 16 | as above (indices 6, 8, 15, 16) | as above | as above |
| | 9 `bltu m1, m2` | borrow | `m1 < m2` | opposite directions |
| B int collapse, notes | 3 `bgeu P, N` | no borrow | `N ≤ P` | always |
| B int collapse, per arm | 2 `bltu P, N` | borrow | `P < N` | always |
| B int div, notes | 6 `bgeu P1, N1`, 10 `bgeu P2, N2` | no borrow | `x ≥ 0`, `y ≥ 0` | always |
| B int div, per arm | 5 `bltu P1, N1`, 10 `bltu P2, N2` | borrow | `x < 0`, `y < 0` | always |
| B complex collapse, notes | 5 `bgeu m0, m2`, 13 `bgeu m1, m3` | no borrow on that axis | real part ≥ 0, imaginary part ≥ 0 | always |
| B complex collapse, per arm | 4 `bltu m0, m2`, 12 `bltu m1, m3` | borrow on that axis | real part < 0, imaginary part < 0 | always |
| B complex div, notes | 10, 14 (x), 23, 27 (y) `bgeu` | no borrow on that axis | that part ≥ 0 | always |
| B complex div, per arm | 9, 14 (x), 24, 29 (y) `bltu` | borrow on that axis | that part < 0 | always |
| C add | 4 `beqz t` | axis bits equal | same axis | always |
| | 7 `bnez (w2 <u 2)` | `v2 = 0` | `y` is zero | different axes |
| | 10 `bnez (w1 <u 2)` | `v1 = 0` | `x` is zero | different axes, `y ≠ 0` |
| C sub | 4, 7, 12 | as C add (12 is `w1 <u 2`) | as C add | as C add |

Every other program has no conditional branch: Control's integer operations and complex add, subtract, negate, multiply; A's negate, multiply, divide; the whole branch-free row; B's add, subtract, negate, multiply (both kinds); C's negate, multiply, divide.

When each is predictable depends on the data, so this part is prose, under the round 3 mixes of §7 (sign mix: all positive, 1/16 negative, random; order mix for add and subtract: |a| ≥ |b| always, 15/16, half; complex operands on a random axis, cross-axis pairs replaced by one with a zero):

- A's direction branch (`beqz t`). Integers: for add, all positive makes every pair same-direction, so it is always taken and predictable; 1/16 negative gives opposite directions with probability 2·(1/16)·(15/16), about 12 percent, so mostly predicted; random signs make it a coin toss. Subtraction turns `y` by 2, so the roles swap: all positive is always opposite. Complex numbers on random axes: the axes agree half the time, and then the sign mix decides as for integers, so the branch is taken about half the time under all positive and less otherwise; it is poorly predictable in every complex mix.
- A's axis branch (`bnez (t and 0x40)`, complex only): taken when the axes differ, half the time on random axes; unpredictable.
- A's borrow branch (`bgeu`/`bltu`), reached on opposite directions only: under |a| ≥ |b| always it never borrows (predictable); 15/16 mispredicts about 1 in 16; half is unpredictable. The per-arm form has the same condition with the opposite polarity, so the same predictability.
- A's zero branches (`m1 = 0`, `m2 = 0`) on different axes: §7 replaces a nonzero cross pair by one with a zero, and which operand is zero is a property of the generated data; predictable only if the generator always zeroes the same side.
- B's collapse and division borrow branches are taken by sign: never under all positive (predictable), 1/16 of the time per operand under 1/16 negative (mostly predicted), half the time under random signs (unpredictable). Collapsed operands (§7) do not change this, since the borrow is the sign. Complex: one axis is zero for operands on an axis, so its branch always goes the no-borrow way (predictable), and the other follows the sign mix.
- Control's complex division branch follows the divisor's axis: half the time imaginary on random axes, so unpredictable; integers have no branch.
- C's axis branch: integers always take it (predictable); complex numbers on random axes, half the time; unpredictable. C's zero branches as A's.

## 6. Cycle predictions (claims about a model)

The machine is an Intel Xeon reporting family 6, model 173 (Granite Rapids, Redwood Cove cores). uops.info does not list Granite Rapids; its nearest core is Meteor Lake-P (Redwood Cove), whose figures I fetched for: `ADD r64, r64` latency 1, throughput 0.20 (port p0156B, so 5 ALU ports); `IMUL r64, r64` latency 3, throughput 1.00 (p1); `DIV r64` latency 14, throughput 10.00, 3 uops on p1; `IDIV r64` latency 14 to 18, throughput 10.00. Not fetched, and assumed: load-to-use latency 5 cycles on an L1 hit, 3 loads and 2 stores per cycle, a 6-wide rename, `setcc` latency 1, one taken branch per cycle, macro-fusion of `cmp/test` with `jcc`, correctly predicted branches. These predictions are claims about that model, not measurements.

The latency column is one operation alone, from the loads to the last stored value (every branch predicted). The throughput column is the cycles per operation of a long run of independent operations (round 3's 1024 per row): the largest of slots/6, ALU uops/5, `imul` count plus 10 per divide (port p1), loads/3, stores/2, and taken branches. "x86 slots" counts fused-domain uops (a fused compare-and-branch is one; a divide is 1 plus 3).

| Design | Operation | Path | x86 slots | Latency | Throughput bound |
| --- | --- | --- | --- | --- | --- |
| Control | int add | one | 4 | 6 | 0.67 |
| Control | int sub | one | 4 | 6 | 0.67 |
| Control | int neg | one | 3 | 6 | 0.5 |
| Control | int mul | one | 4 | 8 | 1 |
| Control | int div | one | 7 | 19 | 10 |
| Control | cx add | one | 8 | 6 | 1.33 |
| Control | cx sub | one | 8 | 6 | 1.33 |
| Control | cx neg | one | 6 | 6 | 1.0 |
| Control | cx mul | one | 14 | 9 | 4 |
| Control | cx div | real divisor | 16 | 19 | 20 |
| Control | cx div | imag divisor | 16 | 20 | 20 |
| A | int add (notes) | same | 11 | 6 | 1.83 |
| A | int add (notes) | opp, no borrow | 13 | 6 | 2.17 |
| A | int add (notes) | opp, borrow | 16 | 7 | 2.67 |
| A | int sub (notes) | same | 12 | 6 | 2.0 |
| A | int sub (notes) | opp, no borrow | 14 | 6 | 2.33 |
| A | int sub (notes) | opp, borrow | 17 | 8 | 2.83 |
| A | int add (perArm) | same | 11 | 6 | 1.83 |
| A | int add (perArm) | opp, no borrow | 13 | 6 | 2.17 |
| A | int add (perArm) | opp, borrow | 14 | 7 | 2.33 |
| A | int sub (perArm) | same | 12 | 6 | 2.0 |
| A | int sub (perArm) | opp, no borrow | 14 | 6 | 2.33 |
| A | int sub (perArm) | opp, borrow | 15 | 8 | 2.5 |
| A | cx add (notes) | same | 11 | 6 | 1.83 |
| A | cx add (notes) | opp, no borrow | 15 | 6 | 2.5 |
| A | cx add (notes) | opp, borrow | 18 | 7 | 3.0 |
| A | cx add (notes) | cross, x = 0 | 15 | 7 | 2.5 |
| A | cx add (notes) | cross, y = 0 | 15 | 6 | 2.5 |
| A | cx add (notes) | cross, expression | 12 | 0 | 2.0 |
| A | cx sub (notes) | same | 12 | 6 | 2.0 |
| A | cx sub (notes) | opp, no borrow | 16 | 6 | 2.67 |
| A | cx sub (notes) | opp, borrow | 19 | 8 | 3.17 |
| A | cx sub (notes) | cross, x = 0 | 16 | 8 | 2.67 |
| A | cx sub (notes) | cross, y = 0 | 16 | 6 | 2.67 |
| A | cx sub (notes) | cross, expression | 13 | 0 | 2.17 |
| A | cx add (perArm) | same | 11 | 6 | 1.83 |
| A | cx add (perArm) | opp, no borrow | 15 | 6 | 2.5 |
| A | cx add (perArm) | opp, borrow | 16 | 7 | 2.67 |
| A | cx add (perArm) | cross, x = 0 | 15 | 7 | 2.5 |
| A | cx add (perArm) | cross, y = 0 | 15 | 6 | 2.5 |
| A | cx add (perArm) | cross, expression | 12 | 0 | 2.0 |
| A | cx sub (perArm) | same | 12 | 6 | 2.0 |
| A | cx sub (perArm) | opp, no borrow | 16 | 6 | 2.67 |
| A | cx sub (perArm) | opp, borrow | 17 | 8 | 2.83 |
| A | cx sub (perArm) | cross, x = 0 | 16 | 8 | 2.67 |
| A | cx sub (perArm) | cross, y = 0 | 16 | 6 | 2.67 |
| A | cx sub (perArm) | cross, expression | 13 | 0 | 2.17 |
| A | neg | one | 5 | 6 | 1.0 |
| A | mul | one | 9 | 8 | 1.5 |
| A | div | one | 12 | 19 | 10 |
| A-bf | int add | one | 27 | 15 | 4.5 |
| A-bf | int sub | one | 28 | 16 | 4.67 |
| A-bf | cx add | one | 47 | 17 | 8.2 |
| A-bf | cx sub | one | 48 | 18 | 8.4 |
| B | int add | one | 8 | 6 | 1.33 |
| B | int sub | one | 8 | 6 | 1.33 |
| B | int neg | one | 4 | 5 | 1.0 |
| B | int mul | one | 14 | 9 | 4 |
| B | int collapse (notes) | borrow 0 | 7 | 6 | 1.17 |
| B | int collapse (notes) | borrow 1 | 9 | 7 | 1.5 |
| B | int collapse (perArm) | borrow 0 | 7 | 6 | 1.17 |
| B | int collapse (perArm) | borrow 1 | 6 | 6 | 1.0 |
| B | int div (notes) | borrows x 0, y 0 | 24 | 20 | 10 |
| B | int div (notes) | borrows x 0, y 1 | 25 | 21 | 10 |
| B | int div (notes) | borrows x 1, y 0 | 25 | 21 | 10 |
| B | int div (notes) | borrows x 1, y 1 | 26 | 21 | 10 |
| B | int div (perArm) | borrows x 0, y 0 | 26 | 20 | 10 |
| B | int div (perArm) | borrows x 0, y 1 | 25 | 20 | 10 |
| B | int div (perArm) | borrows x 1, y 0 | 25 | 20 | 10 |
| B | int div (perArm) | borrows x 1, y 1 | 24 | 20 | 10 |
| B | cx add | one | 16 | 6 | 2.67 |
| B | cx sub | one | 16 | 6 | 2.67 |
| B | cx neg | one | 8 | 5 | 2.0 |
| B | cx mul | one | 52 | 11 | 16 |
| B | cx collapse (notes) | borrows re 0, im 0 | 14 | 6 | 2.33 |
| B | cx collapse (notes) | borrows re 0, im 1 | 16 | 7 | 2.67 |
| B | cx collapse (notes) | borrows re 1, im 0 | 16 | 7 | 2.67 |
| B | cx collapse (notes) | borrows re 1, im 1 | 18 | 7 | 3.0 |
| B | cx collapse (perArm) | borrows re 0, im 0 | 14 | 6 | 2.33 |
| B | cx collapse (perArm) | borrows re 0, im 1 | 13 | 6 | 2.17 |
| B | cx collapse (perArm) | borrows re 1, im 0 | 13 | 6 | 2.17 |
| B | cx collapse (perArm) | borrows re 1, im 1 | 12 | 6 | 2.0 |
| B | cx div (notes) | borrows x 00, y 00 | 54 | 21 | 10 |
| B | cx div (notes) | borrows x 00, y 01 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 00, y 10 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 01, y 00 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 01, y 01 | 56 | 22 | 10 |
| B | cx div (notes) | borrows x 01, y 10 | 56 | 22 | 10 |
| B | cx div (notes) | borrows x 10, y 00 | 55 | 22 | 10 |
| B | cx div (notes) | borrows x 10, y 01 | 56 | 22 | 10 |
| B | cx div (notes) | borrows x 10, y 10 | 56 | 22 | 10 |
| B | cx div (perArm) | borrows x 00, y 00 | 58 | 21 | 10 |
| B | cx div (perArm) | borrows x 00, y 01 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 00, y 10 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 01, y 00 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 01, y 01 | 56 | 21 | 10 |
| B | cx div (perArm) | borrows x 01, y 10 | 56 | 21 | 10 |
| B | cx div (perArm) | borrows x 10, y 00 | 57 | 21 | 10 |
| B | cx div (perArm) | borrows x 10, y 01 | 56 | 21 | 10 |
| B | cx div (perArm) | borrows x 10, y 10 | 56 | 21 | 10 |
| C | add | same axis | 9 | 7 | 1.5 |
| C | add | cross, y = 0 | 10 | 6 | 1.67 |
| C | add | cross, x = 0 | 13 | 6 | 2.17 |
| C | add | cross, expression | 13 | 0 | 2.17 |
| C | sub | same axis | 9 | 7 | 1.5 |
| C | sub | cross, y = 0 | 10 | 6 | 1.67 |
| C | sub | cross, x = 0 | 16 | 8 | 2.67 |
| C | sub | cross, expression | 16 | 0 | 2.67 |
| C | neg | one | 6 | 8 | 1.0 |
| C | mul | one | 17 | 12 | 2.83 |
| C | div | one | 18 | 24 | 10 |

On this model the throughput bound on most paths is the rename width (slots/6) or the stores; divides set it at 10 cycles; B's complex multiply is bound by its 16 `imul`s on p1. A mispredicted branch costs about 17 cycles on Golden Cove-class cores (an assumption, not fetched), so in round 3 an unpredictable branch adds roughly 8 cycles per operation on average, more than the whole of most paths above.

## 7. Check against the Nim builds' disassembly

The builds in `build/bin/` (from `src/build.sh`, gcc -O2 with if-conversion off), disassembled in `build/asm/` and walked with `test/asmcheck.py -v`. The gcc loops include the loop's own index increment, compare and branch (3 instructions, 4 or 5 with pointer increments), which I subtract. No loop has a conditional move, `adc` or `sbb` (grep, and asmcheck's gate). Every mismatch with the x86-64 column:

1. Loads folded into memory operands, everywhere gcc can: Control integer add, subtract, multiply (3 instructions against my 4) and divide (`idiv [mem]`, 4 against 5); Control complex add and subtract (7 against 8); A's multiply and divide fold the header load into the header add or subtract (8 against 9, 9 against 10); B integer add and subtract (6 against 8), B complex add and subtract (12 against 16). Loads are the same in number; only the instruction count differs.
2. A uses one program for integers and complex numbers (the builds have no separate complex loop for A or the branch-free row), the whole tree, so integers pay for the complex tests. Its test order differs from mine: `t = 0` falls through to same direction (then `jmp` to the shared stores), then `cmp t, 0x80; je` for opposite, else the different-axes block. My complex program tests same, then the axis bit, then the borrow; my same-direction path is one taken branch and no jump. Same-direction path: gcc 13 instructions (one `mov` to copy `h1` for the XOR, one `jmp`), mine 11.
3. A's opposite path in the build is the per-arm form (`cmp m1, m2; jb`, one `sub` per arm, the borrow arm adding `-0x80` to the header), as the builder reported; no `setb` or `adc` for the borrow. gcc: 18 instructions without borrow, 19 with; my per-arm complex form 16 and 17 (its integer form 14 and 15). The difference is gcc's `cmp $0x80` branch and `mov` copies.
4. A's error path saves and restores 7 registers around `call specError` (16 instructions) where my program has one `trap`. It is never taken in round 3.
5. The branch-free row in the build differs in method from mine: it adds `m1 + (m2 xor s) + o` (one add serves both sum and difference), negates with `(r xor mk) + b`, keeps the error flag sticky (`err or …`) in a register, and forms its compare values with `setne`, `sete`, `setb` (a compare written as a value, the `sltu` step; no flag is consumed by arithmetic). It has no conditional branch and no conditional move. 47 instructions per operation, the same count as my complex row (47) by coincidence; my integer row (27) has no counterpart in the build.
6. B's collapse in the build is the per-arm form. It makes the zero part with `xor reg, reg` and stores the register, where I store the constant (one more instruction per path), and its borrow path is the fall-through with a `jmp`, the opposite of my per-arm layout. gcc 8 instructions without a borrow and 9 with; mine (per arm) 8 and 7. Complex collapse likewise, per axis.
7. B's integer division in the build computes each borrow with `setb` and branches on the same flags with `jb`/`jae` (one `cmp` serving both), subtracts per arm, stores `0` to both parts and the quotient by a scaled index (`mov [rdi + rsi·8], rax`, my `slli; add; sd` in one instruction). 23 instructions on the no-borrow path against my per-arm 26 (notes form 24). The `setb` here is the `sltu` step, not a borrow taken from a subtraction's carry.
8. B's complex division in the build forms the direction differently (`lea 1(b, b)`, masks with `neg`, `and 3`), and stores 4 zeros and the quotient by scaled index, as mine does. About 62 instructions on the all-no-borrow path against my per-arm 60 (notes form 56).
9. Control's complex division: gcc moves each dividend into `rax` (2 or 3 extra `mov`s per path, which my divide rule assumes free), and loads the divisor's real part only on the real path (3 loads on the imaginary path against my 4). Real path 16 instructions against 13, imaginary 15 against 13. The branch is the same (`test; jne` on the imaginary part).
10. Matches: Control negate (3), complex negate (6), complex multiply (14); A negate (5); B negate (4, 8), integer multiply (14), complex multiply (about 52).

## 8. Findings

1. "Each step one instruction on x86-64, ARM64 and RV64IM" holds only for RISC-V. On the other two, `sltu`, a two-register compare-and-branch and (x86-64) a divide or a three-operand op with a live source take two or three instructions; flag reuse, folding into `cmp`/`test`, `lea` and shifted adds take some back. The tables give both the step count and the per-family counts.
2. With the class code always 0, every `and D` in A is redundant: the headers are `64·d`, their XOR is again of that form, and the result is stored as a byte, so `(h1 + h2)`, `(h1 − h2)` and `(h1 + (b shl 7))` mod 256 already have bits 0 to 5 clear. The programs keep them, as §3 writes them: one step on every add, subtract, multiply and divide path, and one more for `t`.
3. Control's complex division has a data-dependent branch (the divisor's axis), which A, B and C do not have in division. Under round 3's random axes it is unpredictable.
4. B's division, routed by address, stores 3 (integers) or 5 (complex) words and writes one part twice. A branch on the sign would store 2 or 4 but add an unpredictable branch under random signs.
5. The per-arm form saves the `neg` and the `mv` on the borrow path but, with the stores shared at the end, puts a jump on both opposite paths; the notes' form reaches the stores from the no-borrow path by its taken branch. Per path: A integer add 13/14 steps (per arm) against 12/15 (notes).
6. A's branch-free row stores on every input, the error case included, and leaves the error flag for the caller; the build's row keeps it sticky across a pass.
7. C's multiply is written with a mask, not the branch its round 1 definition states; `mul_mask` proves the two equal on every pair of words.
8. Lean: the symbolic runs are heavy. A's proofs are split over several files (`CorrectA.lean` and the four `CA_cxAdd_*.lean`) and B's complex division over two (`CB_cdiv_*.lean`), since one file per design ran out of memory when built beside another (Lean exited with code 137 on this 8 GB machine). A `variable … include` section with many memory hypotheses made the symbolic runs stall (well past 10 minutes for 8 goals that take 3 seconds as plain theorem binders); the files use plain binders. The complex branch-free row's proof stalls the same way after its tactics finish and is parked (§3).

## 9. Checks

`lake build` builds every module with no errors. No file of round 2 contains `sorry`, `admit` or `native_decide`. `#print axioms` was run on every round 2 theorem; see `lean/README.md`.

## Appendix: the programs

Each program as its steps, index first (registers as `rN`, `zero` for register 0; branch targets are indices; the end is one past the last index).

```text
### Control int add
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: add r12, r10, r11
3: sd r12, 0(r3)
### Control int sub
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: sub r12, r10, r11
3: sd r12, 0(r3)
### Control int neg
0: ld r10, 0(r1)
1: neg r12, r10
2: sd r12, 0(r3)
### Control int mul
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: mul r12, r10, r11
3: sd r12, 0(r3)
### Control int div
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: div r12, r10, r11
3: sd r12, 0(r3)
### Control cx add
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: add r14, r10, r12
5: add r15, r11, r13
6: sd r14, 0(r3)
7: sd r15, 8(r3)
### Control cx sub
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: sub r14, r10, r12
5: sub r15, r11, r13
6: sd r14, 0(r3)
7: sd r15, 8(r3)
### Control cx neg
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: neg r14, r10
3: neg r15, r11
4: sd r14, 0(r3)
5: sd r15, 8(r3)
### Control cx mul
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: mul r14, r10, r12
5: mul r15, r11, r13
6: sub r14, r14, r15
7: mul r16, r10, r13
8: mul r17, r11, r12
9: add r16, r16, r17
10: sd r14, 0(r3)
11: sd r16, 8(r3)
### Control cx div
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: bnez r13, 8
5: div r14, r10, r12
6: div r15, r11, r12
7: j 11
8: div r14, r11, r13
9: div r15, r10, r13
10: neg r15, r15
11: sd r14, 0(r3)
12: sd r15, 8(r3)
### A int add (notes)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xor r14, r10, r11
5: andi r14, r14, 192
6: beqz r14, 12
7: sub r15, r12, r13
8: bgeu r12, r13, 13
9: neg r15, r15
10: mv r10, r11
11: j 13
12: add r15, r12, r13
13: andi r10, r10, 192
14: sb r10, 0(r5)
15: sd r15, 0(r6)
### A int sub (notes)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xori r11, r11, 128
5: xor r14, r10, r11
6: andi r14, r14, 192
7: beqz r14, 13
8: sub r15, r12, r13
9: bgeu r12, r13, 14
10: neg r15, r15
11: mv r10, r11
12: j 14
13: add r15, r12, r13
14: andi r10, r10, 192
15: sb r10, 0(r5)
16: sd r15, 0(r6)
### A int add (perArm)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xor r14, r10, r11
5: andi r14, r14, 192
6: beqz r14, 13
7: bltu r12, r13, 10
8: sub r15, r12, r13
9: j 14
10: sub r15, r13, r12
11: mv r10, r11
12: j 14
13: add r15, r12, r13
14: andi r10, r10, 192
15: sb r10, 0(r5)
16: sd r15, 0(r6)
### A int sub (perArm)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xori r11, r11, 128
5: xor r14, r10, r11
6: andi r14, r14, 192
7: beqz r14, 14
8: bltu r12, r13, 11
9: sub r15, r12, r13
10: j 15
11: sub r15, r13, r12
12: mv r10, r11
13: j 15
14: add r15, r12, r13
15: andi r10, r10, 192
16: sb r10, 0(r5)
17: sd r15, 0(r6)
### A cx add (notes)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xor r14, r10, r11
5: andi r14, r14, 192
6: beqz r14, 18
7: andi r16, r14, 64
8: bnez r16, 14
9: sub r15, r12, r13
10: bgeu r12, r13, 19
11: neg r15, r15
12: mv r10, r11
13: j 19
14: beqz r12, 17
15: beqz r13, 18
16: trap
17: mv r10, r11
18: add r15, r12, r13
19: andi r10, r10, 192
20: sb r10, 0(r5)
21: sd r15, 0(r6)
### A cx sub (notes)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xori r11, r11, 128
5: xor r14, r10, r11
6: andi r14, r14, 192
7: beqz r14, 19
8: andi r16, r14, 64
9: bnez r16, 15
10: sub r15, r12, r13
11: bgeu r12, r13, 20
12: neg r15, r15
13: mv r10, r11
14: j 20
15: beqz r12, 18
16: beqz r13, 19
17: trap
18: mv r10, r11
19: add r15, r12, r13
20: andi r10, r10, 192
21: sb r10, 0(r5)
22: sd r15, 0(r6)
### A cx add (perArm)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xor r14, r10, r11
5: andi r14, r14, 192
6: beqz r14, 19
7: andi r16, r14, 64
8: bnez r16, 15
9: bltu r12, r13, 12
10: sub r15, r12, r13
11: j 20
12: sub r15, r13, r12
13: mv r10, r11
14: j 20
15: beqz r12, 18
16: beqz r13, 19
17: trap
18: mv r10, r11
19: add r15, r12, r13
20: andi r10, r10, 192
21: sb r10, 0(r5)
22: sd r15, 0(r6)
### A cx sub (perArm)
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xori r11, r11, 128
5: xor r14, r10, r11
6: andi r14, r14, 192
7: beqz r14, 20
8: andi r16, r14, 64
9: bnez r16, 16
10: bltu r12, r13, 13
11: sub r15, r12, r13
12: j 21
13: sub r15, r13, r12
14: mv r10, r11
15: j 21
16: beqz r12, 19
17: beqz r13, 20
18: trap
19: mv r10, r11
20: add r15, r12, r13
21: andi r10, r10, 192
22: sb r10, 0(r5)
23: sd r15, 0(r6)
### A neg
0: lbu r10, 0(r1)
1: ld r12, 0(r2)
2: xori r10, r10, 128
3: sb r10, 0(r5)
4: sd r12, 0(r6)
### A mul
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: mul r15, r12, r13
5: add r10, r10, r11
6: andi r10, r10, 192
7: sb r10, 0(r5)
8: sd r15, 0(r6)
### A div
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: divu r15, r12, r13
5: sub r10, r10, r11
6: andi r10, r10, 192
7: sb r10, 0(r5)
8: sd r15, 0(r6)
### A-bf int add
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xor r14, r10, r11
5: andi r14, r14, 192
6: srli r16, r14, 7
7: neg r17, r16
8: add r15, r12, r13
9: sub r18, r12, r13
10: sltu r19, r12, r13
11: and r19, r19, r16
12: neg r20, r19
13: xor r18, r18, r20
14: sub r18, r18, r20
15: xor r18, r18, r15
16: and r18, r18, r17
17: xor r15, r15, r18
18: slli r19, r19, 7
19: add r10, r10, r19
20: andi r10, r10, 192
21: sb r10, 0(r5)
22: sd r15, 0(r6)
### A-bf int sub
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xori r11, r11, 128
5: xor r14, r10, r11
6: andi r14, r14, 192
7: srli r16, r14, 7
8: neg r17, r16
9: add r15, r12, r13
10: sub r18, r12, r13
11: sltu r19, r12, r13
12: and r19, r19, r16
13: neg r20, r19
14: xor r18, r18, r20
15: sub r18, r18, r20
16: xor r18, r18, r15
17: and r18, r18, r17
18: xor r15, r15, r18
19: slli r19, r19, 7
20: add r10, r10, r19
21: andi r10, r10, 192
22: sb r10, 0(r5)
23: sd r15, 0(r6)
### A-bf cx add
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xor r14, r10, r11
5: andi r14, r14, 192
6: xori r16, r14, 128
7: sltiu r16, r16, 1
8: srli r22, r14, 6
9: andi r22, r22, 1
10: sltiu r23, r12, 1
11: sltiu r24, r13, 1
12: or r21, r23, r24
13: xori r21, r21, 1
14: and r21, r21, r22
15: and r23, r23, r22
16: neg r23, r23
17: and r23, r23, r14
18: xor r10, r10, r23
19: neg r17, r16
20: add r15, r12, r13
21: sub r18, r12, r13
22: sltu r19, r12, r13
23: and r19, r19, r16
24: neg r20, r19
25: xor r18, r18, r20
26: sub r18, r18, r20
27: xor r18, r18, r15
28: and r18, r18, r17
29: xor r15, r15, r18
30: slli r19, r19, 7
31: add r10, r10, r19
32: andi r10, r10, 192
33: sb r10, 0(r5)
34: sd r15, 0(r6)
### A-bf cx sub
0: lbu r10, 0(r1)
1: lbu r11, 0(r3)
2: ld r12, 0(r2)
3: ld r13, 0(r4)
4: xori r11, r11, 128
5: xor r14, r10, r11
6: andi r14, r14, 192
7: xori r16, r14, 128
8: sltiu r16, r16, 1
9: srli r22, r14, 6
10: andi r22, r22, 1
11: sltiu r23, r12, 1
12: sltiu r24, r13, 1
13: or r21, r23, r24
14: xori r21, r21, 1
15: and r21, r21, r22
16: and r23, r23, r22
17: neg r23, r23
18: and r23, r23, r14
19: xor r10, r10, r23
20: neg r17, r16
21: add r15, r12, r13
22: sub r18, r12, r13
23: sltu r19, r12, r13
24: and r19, r19, r16
25: neg r20, r19
26: xor r18, r18, r20
27: sub r18, r18, r20
28: xor r18, r18, r15
29: and r18, r18, r17
30: xor r15, r15, r18
31: slli r19, r19, 7
32: add r10, r10, r19
33: andi r10, r10, 192
34: sb r10, 0(r5)
35: sd r15, 0(r6)
### B int add
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: add r14, r10, r12
5: add r15, r11, r13
6: sd r14, 0(r3)
7: sd r15, 8(r3)
### B int sub
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: add r14, r10, r13
5: add r15, r11, r12
6: sd r14, 0(r3)
7: sd r15, 8(r3)
### B int neg
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: sd r11, 0(r3)
3: sd r10, 8(r3)
### B int mul
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: mul r14, r10, r12
5: mul r15, r11, r13
6: add r14, r14, r15
7: mul r16, r10, r13
8: mul r17, r11, r12
9: add r16, r16, r17
10: sd r14, 0(r3)
11: sd r16, 8(r3)
### B int collapse (notes)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: sub r14, r10, r11
3: bgeu r10, r11, 8
4: neg r14, r14
5: sd zero, 0(r3)
6: sd r14, 8(r3)
7: j 10
8: sd r14, 0(r3)
9: sd zero, 8(r3)
### B int collapse (perArm)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: bltu r10, r11, 7
3: sub r14, r10, r11
4: sd r14, 0(r3)
5: sd zero, 8(r3)
6: j 10
7: sub r14, r11, r10
8: sd zero, 0(r3)
9: sd r14, 8(r3)
### B int div (notes)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: sub r14, r10, r11
5: sltu r16, r10, r11
6: bgeu r10, r11, 8
7: neg r14, r14
8: sub r15, r12, r13
9: sltu r17, r12, r13
10: bgeu r12, r13, 12
11: neg r15, r15
12: divu r18, r14, r15
13: sub r19, r16, r17
14: andi r19, r19, 1
15: slli r19, r19, 3
16: add r19, r19, r3
17: sd zero, 0(r3)
18: sd zero, 8(r3)
19: sd r18, 0(r19)
### B int div (perArm)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 0(r2)
3: ld r13, 8(r2)
4: sltu r16, r10, r11
5: bltu r10, r11, 8
6: sub r14, r10, r11
7: j 9
8: sub r14, r11, r10
9: sltu r17, r12, r13
10: bltu r12, r13, 13
11: sub r15, r12, r13
12: j 14
13: sub r15, r13, r12
14: divu r18, r14, r15
15: sub r19, r16, r17
16: andi r19, r19, 1
17: slli r19, r19, 3
18: add r19, r19, r3
19: sd zero, 0(r3)
20: sd zero, 8(r3)
21: sd r18, 0(r19)
### B cx add
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: ld r20, 0(r2)
5: ld r21, 8(r2)
6: ld r22, 16(r2)
7: ld r23, 24(r2)
8: add r30, r10, r20
9: add r31, r11, r21
10: add r32, r12, r22
11: add r33, r13, r23
12: sd r30, 0(r3)
13: sd r31, 8(r3)
14: sd r32, 16(r3)
15: sd r33, 24(r3)
### B cx sub
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: ld r20, 0(r2)
5: ld r21, 8(r2)
6: ld r22, 16(r2)
7: ld r23, 24(r2)
8: add r30, r10, r22
9: add r31, r11, r23
10: add r32, r12, r20
11: add r33, r13, r21
12: sd r30, 0(r3)
13: sd r31, 8(r3)
14: sd r32, 16(r3)
15: sd r33, 24(r3)
### B cx neg
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: sd r12, 0(r3)
5: sd r13, 8(r3)
6: sd r10, 16(r3)
7: sd r11, 24(r3)
### B cx mul
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: ld r20, 0(r2)
5: ld r21, 8(r2)
6: ld r22, 16(r2)
7: ld r23, 24(r2)
8: mul r30, r10, r20
9: mul r31, r11, r23
10: add r30, r30, r31
11: mul r31, r12, r22
12: add r30, r30, r31
13: mul r31, r13, r21
14: add r30, r30, r31
15: sd r30, 0(r3)
16: mul r30, r10, r21
17: mul r31, r11, r20
18: add r30, r30, r31
19: mul r31, r12, r23
20: add r30, r30, r31
21: mul r31, r13, r22
22: add r30, r30, r31
23: sd r30, 8(r3)
24: mul r30, r10, r22
25: mul r31, r11, r21
26: add r30, r30, r31
27: mul r31, r12, r20
28: add r30, r30, r31
29: mul r31, r13, r23
30: add r30, r30, r31
31: sd r30, 16(r3)
32: mul r30, r10, r23
33: mul r31, r11, r22
34: add r30, r30, r31
35: mul r31, r12, r21
36: add r30, r30, r31
37: mul r31, r13, r20
38: add r30, r30, r31
39: sd r30, 24(r3)
### B cx collapse (notes)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: sub r14, r10, r12
5: bgeu r10, r12, 10
6: neg r14, r14
7: sd zero, 0(r3)
8: sd r14, 16(r3)
9: j 12
10: sd r14, 0(r3)
11: sd zero, 16(r3)
12: sub r14, r11, r13
13: bgeu r11, r13, 18
14: neg r14, r14
15: sd zero, 8(r3)
16: sd r14, 24(r3)
17: j 20
18: sd r14, 8(r3)
19: sd zero, 24(r3)
### B cx collapse (perArm)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: bltu r10, r12, 9
5: sub r14, r10, r12
6: sd r14, 0(r3)
7: sd zero, 16(r3)
8: j 12
9: sub r14, r12, r10
10: sd zero, 0(r3)
11: sd r14, 16(r3)
12: bltu r11, r13, 17
13: sub r14, r11, r13
14: sd r14, 8(r3)
15: sd zero, 24(r3)
16: j 20
17: sub r14, r13, r11
18: sd zero, 8(r3)
19: sd r14, 24(r3)
### B cx div (notes)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: ld r20, 0(r2)
5: ld r21, 8(r2)
6: ld r22, 16(r2)
7: ld r23, 24(r2)
8: sub r14, r10, r12
9: sltu r16, r10, r12
10: bgeu r10, r12, 12
11: neg r14, r14
12: sub r15, r11, r13
13: sltu r17, r11, r13
14: bgeu r11, r13, 16
15: neg r15, r15
16: or r14, r14, r15
17: or r16, r16, r17
18: sltu r17, zero, r15
19: slli r16, r16, 1
20: or r16, r16, r17
21: sub r24, r20, r22
22: sltu r26, r20, r22
23: bgeu r20, r22, 25
24: neg r24, r24
25: sub r25, r21, r23
26: sltu r27, r21, r23
27: bgeu r21, r23, 29
28: neg r25, r25
29: or r24, r24, r25
30: or r26, r26, r27
31: sltu r27, zero, r25
32: slli r26, r26, 1
33: or r26, r26, r27
34: divu r18, r14, r24
35: sub r19, r16, r26
36: andi r19, r19, 3
37: slli r19, r19, 3
38: add r19, r19, r3
39: sd zero, 0(r3)
40: sd zero, 8(r3)
41: sd zero, 16(r3)
42: sd zero, 24(r3)
43: sd r18, 0(r19)
### B cx div (perArm)
0: ld r10, 0(r1)
1: ld r11, 8(r1)
2: ld r12, 16(r1)
3: ld r13, 24(r1)
4: ld r20, 0(r2)
5: ld r21, 8(r2)
6: ld r22, 16(r2)
7: ld r23, 24(r2)
8: sltu r16, r10, r12
9: bltu r10, r12, 12
10: sub r14, r10, r12
11: j 13
12: sub r14, r12, r10
13: sltu r17, r11, r13
14: bltu r11, r13, 17
15: sub r15, r11, r13
16: j 18
17: sub r15, r13, r11
18: or r14, r14, r15
19: or r16, r16, r17
20: sltu r17, zero, r15
21: slli r16, r16, 1
22: or r16, r16, r17
23: sltu r26, r20, r22
24: bltu r20, r22, 27
25: sub r24, r20, r22
26: j 28
27: sub r24, r22, r20
28: sltu r27, r21, r23
29: bltu r21, r23, 32
30: sub r25, r21, r23
31: j 33
32: sub r25, r23, r21
33: or r24, r24, r25
34: or r26, r26, r27
35: sltu r27, zero, r25
36: slli r26, r26, 1
37: or r26, r26, r27
38: divu r18, r14, r24
39: sub r19, r16, r26
40: andi r19, r19, 3
41: slli r19, r19, 3
42: add r19, r19, r3
43: sd zero, 0(r3)
44: sd zero, 8(r3)
45: sd zero, 16(r3)
46: sd zero, 24(r3)
47: sd r18, 0(r19)
### C add
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: xor r12, r10, r11
3: andi r12, r12, 1
4: beqz r12, 12
5: mv r14, r10
6: sltiu r15, r11, 2
7: bnez r15, 14
8: mv r14, r11
9: sltiu r15, r10, 2
10: bnez r15, 14
11: trap
12: andi r13, r11, 18446744073709551614
13: add r14, r10, r13
14: sd r14, 0(r3)
### C sub
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: xor r12, r10, r11
3: andi r12, r12, 1
4: beqz r12, 14
5: mv r14, r10
6: sltiu r15, r11, 2
7: bnez r15, 16
8: andi r16, r11, 1
9: slli r16, r16, 1
10: sub r14, r16, r11
11: sltiu r15, r10, 2
12: bnez r15, 16
13: trap
14: andi r13, r11, 18446744073709551614
15: sub r14, r10, r13
16: sd r14, 0(r3)
### C neg
0: ld r10, 0(r1)
1: andi r12, r10, 1
2: slli r12, r12, 1
3: sub r14, r12, r10
4: sd r14, 0(r3)
### C mul
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: srai r12, r10, 1
3: andi r13, r11, 18446744073709551614
4: mul r12, r12, r13
5: and r14, r10, r11
6: andi r14, r14, 1
7: neg r14, r14
8: xor r12, r12, r14
9: sub r12, r12, r14
10: xor r15, r10, r11
11: andi r15, r15, 1
12: or r12, r12, r15
13: sd r12, 0(r3)
### C div
0: ld r10, 0(r1)
1: ld r11, 0(r2)
2: andi r12, r10, 18446744073709551614
3: andi r13, r11, 18446744073709551614
4: div r12, r12, r13
5: xor r14, r10, r11
6: andi r14, r14, 1
7: and r15, r14, r11
8: neg r15, r15
9: xor r12, r12, r15
10: sub r12, r12, r15
11: slli r12, r12, 1
12: add r12, r12, r14
13: sd r12, 0(r3)
```
