# The Nim build of round m4: Control and Packed

Oct 30, 2026. Spec [v0.4](../v0.4.md) §17 (amending [v0.2](../v0.2.md) and [v0.1](../v0.1.md)), built in Nim from the
spec, then compared with the C build. Nothing timed; nothing committed. The builder's choices and the structures are
in [DECISIONS.md](DECISIONS.md).

## Files

| File | What |
| --- | --- |
| `src/gcd.nim`, `src/GCD.SHA256SUMS` | the gcd of §17.7, under its own pin |
| `src/core.nim` | the build settings; the driver's C types (taken from its headers); manual allocation and the accounted total; in the test build, 08's own gcd to check the new one against (`include/m4n_gcd08.h`); 08's checked addition and multiplication; one operation's counts; the mark test |
| `src/arith.nim` | the arithmetic both models share: addition, multiplication and entry, with v0.3 §17.3's cases in order, gcd-first, every step checked |
| `src/control.nim` | **Control** (`nctl_*`): r c pairs per matrix; mul summed from the first product; the mark |
| `src/packed.nim` | **Packed** (`npk_*`): the cells and (l); (h) during a build; the pack; mul's row buffer; the mark; figures, counters, audits, test accessors |
| `include/m4n.h`, `include/m4n_marks.h` | the models' C interface; the overflow record |
| `driver/bench_m4n.c`, `driver/api_m4n.h` | copies of `test/bench_m4.c` and `test/api_m4.h` with the two Nim rows and §17.10's driver part (DECISIONS N-12) |
| `build.sh` | checks the pins (the Nim gcd's, 08's, the m4 gcd's, `tc3.c`'s); builds both models in the four builds, links each with the driver, and builds the TC gate |
| `test/tc3gate_m4.c` | the TC gate: `tc3.c` unchanged, with the new gcd and §17.6's summation, up to the first overflow |
| `test/gen.py` | §17.9's workloads |
| `test/sim.py` | the simulator: every value, mark, count, record, Packed structure and accounted figure, from the spec |
| `test/check.py` | both models, every build, against the simulator and the exact references |
| `test/cases.py` | §17.12's crafted tests, as workloads for `check.py` |
| `test/gcdtest.nim` | §17.12's gcd tests |
| `test/alloc.c` | §17.8's structures, block by block |
| `test/ccompare.py`, `test/ir.py` | against the C build: outputs, then callgrind Ir |
| `test/run_tests.sh` | every check above, in order |

## Nim

Nim **2.2.14**, from the GitHub tag `v2.2.14`, commit `c5c2337564e347bb8252a510c79a74817a670231`, bootstrapped by the
tag's `build_all.sh` from `csources_v3` commit `eeab3ac46e93f10efda8e58c4db02b9438319d71`; `bin/nim` sha256
`c844c0fd004a76b1c1a6bc9b2e16a7878b375aba9dd582a29f66e667a17d0441`. To rebuild it:

```sh
git clone --depth 1 --branch v2.2.14 https://github.com/nim-lang/Nim nim-2.2.14
cd nim-2.2.14 && sh build_all.sh            # bin/nim
```

gcc 13.3.0. CPython 3.13.17 (the generator refuses another).

## Build and check

```sh
NIM=/path/to/nim-2.2.14/bin/nim sh build.sh                 # build/bin/bench-{NCTL,NPK}-{timing,counting,test,memory}
NIM=/path/to/nim-2.2.14/bin/nim WORK=/scratch/dir sh test/run_tests.sh
```

Builds: timing (nothing), counting (`-d:skCount`, C `SK_COUNT`), test (`-d:skCount -d:skAudit`, C `SK_COUNT
SK_AUDIT`), memory (`-d:m2Mem`, C `M2_MEM`, linked with the heap wrapper). The binaries take the m3/m4 driver's modes
(`time`, `count`, `gate`, `mem`) and print its lines, plus `marks` and `mrk` (DECISIONS N-12). `check.py` and
`ccompare.py` use the m3 workloads in `test/build/m3/work-<op>/` and the C binaries in `test/build/m4/m4/`, read
only.

## Verification

All on Oct 30, 2026, on the build revised after the coordinator's review (DECISIONS, R); every count below is a
pass with 0 failures.

**Values** (`check.py`): every cell of every result and every intermediate, both models, all four builds:
- m3's 45 workloads (add 12, mul 12, sop 9, pos 12): equal to the simulator and to m3's exact references
  (`job-NN.ref`, `job-NN.<X>.ref`); no mark;
- §17.9's workloads, 24 jobs (jobs 1, 4, 5, 8, 9, 12 of each op: n = 16, 32, 64 at ρ = 1 and 64): every unmarked
  cell equal to Python's `fractions`; every cell whose exact value does not fit 64 bits marked;
- §17.12's 23 crafted cases (`cases.py`).

**Marks against the simulator's rule** (§17.0, §17.10): on all 92 jobs, the set of marked cells per cell (the driver's
scan of the result; the gate's read-out of each intermediate), the counts "new" and "passed on" and the mark tests
per step, equal to the simulator's in both models, and equal between the models. On §17.9's workloads:

| Job (n, ρ) | Step | Marked cells | New | Passed on | Exact value past 64 bits |
| --- | --- | --- | --- | --- | --- |
| mul-5 (32, 1) | C | 32 (3.1%) | 32 | 0 | 32 |
| mul-9 (64, 1) | C | 3,750 (91.6%) | 3,750 | 0 | 3,578 |
| sop-5 (32, 1) | P / Q / R | 24 / 15 / 149 (14.6%) | 24 / 15 / 110 | 0 / 0 / 39 | 22 / 14 / 129 |
| sop-9 (64, 1) | P / Q / R | 3,698 / 3,735 / 4,090 (99.9%) | 3,698 / 3,735 / 38 | 0 / 0 / 4,052 | 3,517 / 3,571 / 4,059 |
| pos-1 (16, 1) | R | 1 (0.4%) | 1 | 0 | 1 |
| pos-5 (32, 1) | R | 457 (44.6%) | 457 | 0 | 426 |
| pos-9 (64, 1) | R | 4,096 (100%) | 4,096 | 0 | 4,092 |
| pos-12 (64, 64) | R | 3 (0.07%) | 3 | 0 | 3 |

Every other step of the 24 jobs, and every add job, has no mark. These rates are far above §17.10's "Expected"
(DECISIONS §4 item 1).

**Arithmetic, call for call:** the per-operation record (case, gcds, gcd work, divisions, multiplications, additions),
record for record, equal to the simulator's and between the models on all 92 jobs; every `ctr`/`xctr` line (core
counts, comparisons, cases per phase) equal to the simulator's. Every case of addition and multiplication is reached.

**The TC gate** (`tc3gate_m4`, on all 92 jobs): `tc3.c` itself, with the new gcd and §17.6's summation. On the 74
jobs the simulator gives no overflow it completes, and Control's per-operation record and read-out equal its own,
operation for operation; on the 18 with overflows it stops at the simulator's first overflowing operation (e.g.
mul-5 at operation 5,273, mul-9 at 34), and Control's records before it equal its own.

**Packed's structures against the simulator:** per matrix, the `stor` and `pack` lines (W, T_fix, w, w_h,build,
w_h,pack, found, new, probes, longest, fields narrowed, the bytes of (l), (h) and the cells) and the `opc` lines (cross
tests, matches, cross reads); in the gate, (h) at each pack slot for slot, (l) entry for entry in arrival order, and
every handle field. The crafted cases add: 100% fill (W = T_fix, n = 1 and 2); (l) at 8, 16, 32 and 64 bits; the
field pairs (8, 8), (16, 8), (16, 16), (32, 8), (32, 16), (32, 32); an overflow in addition (2^63, and −2^63), in addition's
case num with h ≠ 1 at a (t/h) (after both divisions, as `tc3.c`), in a denominator, in a product (first and later term) and in a sum of products (first and later addition); marked operands
in a later step on the left and on the right, one beside an operand whose product would overflow; n = 1
multiplication. In the counting and test builds, Packed's row buffer bytes per mul step (24 n) and its K-Q3 read counter (0). The
test build's audits and checks (DECISIONS N-13) ran on every gate run without a failure.

**The gcd** (`gcdtest.nim`): 339,896 checks of the new gcd and 08's own C `sk_gcd_raw` against Euclid, including both
zero operands, size differences of 8 bits (the remainder-first path counted not taken) and 9 bits (counted taken) on
either side, zero remainders (taken) and 200,000 random pairs; and in the test build every gcd of every job checked
against 08's C gcd. `build.sh` checks the gcd's pin, 08's and the m4 pin, and `tc3.c`'s sha.

**Structures and accounting events** (§17.8, §17.11):
- `alloc.c`, block by block, timing and memory builds: Packed (273,414 and 273,530 checks), at n = 1, 2, 8, 9, 16,
  129 and four value counts each: `create` allocates exactly the header, the cells, (h) and (l) (2rc × 8 bytes); no
  `enter` allocates; the pack frees (h), then reallocs (l) to W w/8 bytes, then the cells if narrowed; `madd`
  allocates nothing else; `mmul` allocates one n × 24-byte row buffer and frees it before the pack; `destroy` frees
  the rest. Control (35 checks): its header and 16 rc bytes, nothing else. In the memory build the accounted total
  equals the live structures' bytes after every call.
- `check.py`, memory build, every job: every accounted memrun figure (each input's bytes, base and build peak; after
  creation; the operation's peak; the end) equal to the simulator's event-by-event total.

## Against C

**Outputs** (`ccompare.py`, m3's 45 workloads, counting, test and memory builds): Nim Control against `tcm.c` with
the m4 gcd, Nim Packed against `mh4.c`. Read-outs and intermediates byte-equal; `stor`, `pack` and `opc` lines equal;
counters and per-operation records equal once C's n² first additions per mul step (`0/1 + p`, case `add_zero_x`, 8
comparisons each, which §17.6 removes) and C's K-n counters (DECISIONS §3) are set aside; accounted figures equal,
except each mul step's peak, higher by the row buffer's n × 24 bytes when the peak falls while it exists (§17.8). No
other difference.

**Instruction counts** (`ir.py`; callgrind Ir, one run at a time, timing build, `time <wl> <ro> 1 1`; build = per
input, enter plus its share of create; op = madd/mmul plus the results' share of create):

| Job (n) | Control build Nim/C | Control op Nim/C | Packed build Nim/C | Packed op Nim/C |
| --- | --- | --- | --- | --- |
| add-2 / 6 / 10 (16 / 32 / 64) | 1.087 / 1.073 / 1.078 | 1.024 / 1.014 / 1.017 | 1.041 / 1.012 / 1.012 | 0.922 / 0.920 / 0.918 |
| mul-2 / 6 / 10 | 1.069 / 1.084 / 1.081 | 0.984 / 0.991 / 0.991 | 1.035 / 1.016 / 1.014 | 0.894 / 0.901 / 0.901 |
| sop-2 / 6 / 10 | 1.077 / 1.080 / 1.078 | 0.986 / 0.989 / 0.988 | 1.038 / 1.014 / 1.012 | 0.895 / 0.900 / 0.900 |
| pos-2 / 6 / 10 | 1.076 / 1.079 / 1.079 | 0.967 / 0.972 / 0.969 | 1.037 / 1.014 / 1.013 | 0.897 / 0.899 / 0.898 |

- **Where the algorithms coincide** (build; add's op): Control's build costs 7–9% more Ir in Nim, and its add 1–2%
  more (the two mark tests per cell); Packed's build 1–4% more, and its add 8% less. The Nim gcd and arithmetic are
  plain Nim (DECISIONS N-3); the C code is shaped for conditional moves.
- **Where they differ by the spec** (mul, sop, pos: n − 1 additions per cell, the mark tests, and Packed's row buffer
  on the heap), Nim makes 1–3% fewer Ir in Control and 10–11% fewer in Packed.
- Reading each operand's numerator only after the mark tests (review R-5) raised Packed's add op by about 3% (0.891
  to 0.918 of C at n = 64); mul, sop and pos moved by under 1%.
- Packed / Control at n = 64 (job 10): C 2.25 (add), 1.13 (mul, sop, pos); Nim 2.03, 1.03, 1.03, 1.05.

**Peak bytes** (memory build, n = 64, job 10; accounted / heap):

| | Control Nim | Control C | Packed Nim | Packed C |
| --- | --- | --- | --- | --- |
| an input's build peak (accounted) | 65,536 | 65,536 | 98,304 | 98,304 |
| add op peak | 196,608 / 196,752 | 196,608 / 196,848 | 135,112 / 135,536 | 135,112 / 135,536 |
| mul op peak | 196,608 / 196,752 | 196,608 / 196,848 | 136,646 / 137,080 | 135,110 / 135,536 |
| sop op peak | 458,752 / 459,088 | 458,752 / 459,312 | 271,764 / 272,704 | 271,764 / 272,704 |
| pos op peak | 458,752 / 459,088 | 458,752 / 459,312 | 238,242 / 239,208 | 236,706 / 237,664 |

The accounted figures are equal but for the row buffer (1,536 bytes at n = 64). Control's heap is 32–224 bytes
smaller in Nim (its header is smaller than C's); Packed's is equal but for the row buffer.
