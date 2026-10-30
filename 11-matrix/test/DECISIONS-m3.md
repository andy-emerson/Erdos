# Builder decisions (11-matrix, round m3: spec v0.3, rule-m3)

Choices made where `v0.3.md` and `research/rule-m3.md` are silent, or where a plain reading needed a detail. Each is the
plainest option found and each is **pending the author's review**. None changes a threshold, band, count or order that
the rule fixes. None adds a structure: the rows hold exactly the structures of spec v0.3 §16.3 (per matrix: TC 1;
Hash-V-pack 2, plus (h) and (p) during a build; Pack-primes 3: (a), (l), (f); Pack-coprime 4: (a), (l), (q), (f), plus (r)
during each step), and the scratch of §16.3 only. m1's and m2's decisions (`DECISIONS.md`, `DECISIONS-m2.md`) stand
where this file says so. The items marked **ambiguous** are also listed at the end.

Files, all new: `lib/src/mh3.c` (the two factored rows), `lib/include/{m3,mh3}.h`, `lib/src/m3common.c`,
`lib/tests/{m3t.h,m3_unit.c,m3_diff.c,m3_alloc.c}`; harness `test/bench_m3.c`, `test/api_m3.h`, `test/m3_common.py`,
`m3_libconf.py`, `m3_model.py`, `m3_gen.py`, `m3_checks.py`, `m3_runner.py`, `m3_analyze.py`, `test_m3_harness.py`.
Nothing of m1 or m2 is changed: m1's sources are pinned at `6e9a85e`, and m2's sources and harness (`mh2.c`,
`m2common.c`, `m2.h`, `mh2.h`, `bench_m2.c`, `api_m2.h`, `tc3gate_m2.c`, m2's C tests and Python modules) at `e6505aa`,
checked before every run; m1's and m2's results folders are never written and are hashed before and after every m3 run.
Nothing in `../10-interning/` is changed.

## The rows (lib)

- **M3-D1. Hash-V-pack is m2's `mh2.c`.** **Ambiguous.** v0.3 §16.11 says "Hash-V-pack is m2's `mhvp_` from
  `lib/src/mh.c`, unchanged". m2 built Hash-V-pack in `mh2.c` (DECISIONS-m2 M2-D1); `mh.c` is m1's and has no pack. So
  Hash-V-pack is `mh2.c` built `-DMH_GROW=0 -DMH_PACK=1`, exactly as m2 built it, unchanged and pinned. `api_m3.h`
  refuses m2's other hash rows.
- **M3-D2. The factored rows: one new source, `mh3.c`.** As v0.3 §16.11: built `-DMH3_COPRIME=0` (`mhpp_`,
  Pack-primes) or `=1` (`mhpc_`, Pack-coprime). It starts from `mh2.c`'s Hash-V-pack path, copied line for line (the
  build table (h), store, widening, packing, entry, operands, comparisons and the arithmetic bodies); a harness test
  checks those functions against `mh2.c` text for text, and the value arithmetic of `add_body` and `mul_cells` step for
  step. New headers `m3.h` (the factoring and factored-arithmetic counters, the factoring record and its hook) and
  `mh3.h`; `m3common.c` holds the hook.
- **M3-D3. Where gcds come from, and what is counted where.** Each operation's gcd is obtained from the
  factorizations (§16.5): by a merge, by divisibility tests (Pack-primes) or by one per-piece Euclid gcd
  `gcd(t, q^nu)` (Pack-coprime), or known to be 1. Pack-coprime's per-piece gcds go through the 08 module's `sk_gcd`, so
  they are the core's gcds and gcd work and enter G (§16.5's cost list puts them with TC's Euclid gcds). The entry's
  gcd (reducing an entered `m/d`) is Hash-V-pack's, unchanged: an input is factored only after its pack. All other
  factor work (trial division, pass 1 and 2, the tests, the merges, forming a gcd's value, the splits) uses the raw
  forms (`sk_gcd_raw`, `%`, `/`, `sk_mul_raw`) and its own counters, so the core's exact divisions, multiplications and
  additions stay TC's, as §16.5 requires for the adapted check.
- **M3-D4. Counting the factor work** (§16.10 names the counters, not their units). A merge counts one entry compared
  per comparison of two factors. A divisibility test is one division with remainder (in a reduction; and, in
  Pack-coprime, in checking that `hq` is a power of `q`). A multiplication forming a value counts every multiply,
  starting from 1: `g = prod q^e` counts `sum e`, `q^nu` counts `nu`, and `h` one per factor (Pack-primes: one per
  successful test). Trial division counts candidates (each `d` whose loop ran), divisions with remainder (each `m mod d`)
  and exact divisions (each `m <- m/d`), as the pseudocode writes them. Also counted and reported, though §16.10 does
  not name them: the comparisons made while forming a temporary's list (`lwalk`), the splits' own gcds and divisions,
  and the obtained gcds the test build checks (`gchecked`).
- **M3-D5. The coprime base "by gcds alone"** (notation note §4, which leaves the order open): repeat until nothing
  changes: take the first pair `i < j` in order that shares a factor; put their gcd at `i`, remove `j`, append the two
  residuals; drop 1s and later duplicates. The result is deterministic, and the model runs the same order.
- **M3-D6. Pass 1's split** (§16.4 item 4): `S`, the coprime base of `{h, q/h}`, all of whose elements divide `q`;
  `S[0]` takes `q`'s index and the rest are appended in `S`'s order. `q/h` is counted as one of the split's divisions.
- **M3-D7. Reconciliation** (§16.7). **Ambiguous** in three details, each taken literally:
  - "every matrix of the step" is the step's two operands (the result has no base before its own factoring);
  - a piece's split leaves a matrix unchanged when its pieces from `S` are the piece itself (as `q = 2` against
    `r = 6`): no entry is changed and no (f) is rebuilt; `q`'s pieces are the elements of `S` with `nu_s(q) >= 1`, in
    `S`'s order, their exponents found by divisibility tests;
  - when `refine(L, D_R)` splits a piece that is also in `D_L`, `D_L` stays as it was made for that pass (the
    pseudocode updates only the `D` being scanned); `refine(R, D_L)` then works against the coarser stale entry, and
    the next pass remakes both lists from (f). A pass that splits nothing ends the loop, as written.
- **M3-D8. Leftovers** (§16.8, inside a step): **ambiguous** ("split as at reconciliation" names no second number).
  Only when the sum is a temporary, each `hq = gcd(t, q^nu)` is tested to be a power of `q` by divisibility tests; if
  not, `q` is split into the pieces of the coprime base of `{q, hq}` that divide `q` (`hq` is then a product of them,
  checked), in each operand that holds `q`, in the live lists of `s` and `p`, and in the gcd list's entry, which keeps
  `h`'s exponent at each new piece for forming the result's list. A stored sum makes no check and no split (§16.8,
  "Leftovers in stored results").
- **M3-D9. The scratch of §16.3.** (t): each temporary's list, 15 entries (one spare slot); `s`, `p`, and the sum's
  new list, formed and then copied to `s` (it is the result temporary's (t)). The merge's gcd list: each common
  factor's smaller exponent, whether the exponents are equal, and `h`'s part at it; multiplication's general case has
  two merges, so two such lists at once. Refine's working set `S` (32 entries), with the pieces of `q` and of `r`
  selected from it and their exponents (read as part of the working set). No other array exists; a harness test lists
  every allocation of `mh3.c`.
- **M3-D10. Growth of (q) and (r).** v0.3 says (q) grows by doubling but not from what: from 16 pieces. (q) is shrunk
  at the end of the factoring and at the end of each step in which an operand's base grew. (r), the two lists `D_L` and
  `D_R`, 64-bit, grows by doubling from 16 entries, is allocated at the step's reconciliation and freed at the end of
  the step, before the result's pack and factoring. The accounted total counts their capacity.
- **M3-D11. (f) and (f').** (f) starts at `2W` entries and doubles (§16.4 item 2), then shrinks to `E`. A rebuild
  (§16.8) writes (f') in one pass into a new allocation that starts at `E` plus the number of new pieces and doubles,
  then shrinks; old and new coexist only inside it. An array with no entries (every number 0 or 1) is freed rather than
  kept at zero size, so every accounted byte is a block (the allocation test checks block by block); (q) likewise.
- **M3-D12. The test build's checks** (§16.9 items 3, 5): every obtained gcd against `sk_gcd_raw` on the same operands
  (the build dies on a difference); every pair a merge compares equal or coprime; after reconciliation, every piece of
  each operand equal or coprime to every entry of the other's `D`; every temporary list sorted and multiplying back to
  its denominator; a read of 0's list dies. The audit checks (f) multiplies back, is sorted, has exponents >= 1 and
  empty lists for 0 and 1, and (Pack-coprime) that every factor is a piece; it checks the base pairwise coprime by all
  pairs up to 1,024 pieces (larger bases, up to about 4,600 at n = 64, are compared piece for piece with the model by
  the gates; the unit tests check all pairs).
- **M3-D13. The new sub-cases** are counted wherever they occur: in additions of cells (add, pos's S and U, sop's R)
  and of temporaries (multiplication's sums).
- **M3-D14. Lists formed only where needed** (§16.5, "the last column ... needed only when the result is a
  temporary"): an addition of two cells forms no list; a product and a sum of temporaries form theirs.
- **M3-D15. The memory report** names (f)'s three arrays and (q) as items, so the memory run's stable bytes (measure 6)
  include them exactly, and the allocation test can match each block.
- **M3-D16. The audits and introspection** (`t_nfac`, `t_fac`, `t_piece`, `t_coprime`, `t_corrupt_fac`) are the test
  build's only, as m2's `t_` functions.

## The harness (test)

- **M3-H1. Pins.** `m3_common.RULE_SHA256` is the sha256 of `research/rule-m3.md` as approved and committed at
  `dcee959` (`c3379520…51004fd`); a measured run is refused if the file differs or holds the word "PROPOSED". The 08
  module, tc3.c, its headers and tcm.c's recorded diff are pinned as in m1 and m2; m1's sources at `6e9a85e`; m2's
  sources and harness at `e6505aa` (`m3_libconf.M2_SOURCES_SHA256`).
- **M3-H2. One command per run** (DECISIONS-m2 M2-H2, carried over): `--op add`, `mul`, `sop`, `pos`, started one after
  another; no option runs the four.
- **M3-H3. Step 0 against m1 and m2.** **Ambiguous** (as M2-H3). rule-m3: add and mul's step-0 files MUST equal m1's,
  sop and pos's m2's, by sha256. The workloads and references (`job-NN.wl`, `job-NN.ref`, and sop's and pos's
  `job-NN.<X>.ref`) are compared with the hashes those runs recorded in their `state.json`; the predictions are this
  round's own (`job-NN.m3.json`) and are not compared. Toy runs are not compared. The generator draws with m1's and
  m2's own functions, so the files are theirs by construction (a harness test checks jobs 1 and 4 of every set).
- **M3-H4. Each timed quantity its own minimum** of the kept reps (DECISIONS-m2 M2-H4, carried over).
- **M3-H5. The pilot's projection** (rule-m3, Run plan step 3). **Ambiguous** in which rows the sums run over: the
  `K + 6 P` term runs over the five rows (the null row has no calibration launch, `K = 0`, since it takes Hash-V-pack's
  r); the `4 t` term over the four builds (the null row has no gate, counting or memory run of its own). `t` is the
  calibration's last rep over its r (an over-cap row's too: its side runs cost that much). `O_c` is the pilot block's
  wall time, its idle and disturbance checks included, less its launches. A toy run without a size's pilot job uses its
  largest pilot block for that size.
- **M3-H6. Over cap** (rule-m3 item 1): the magnitude shown is the calibration rep that passed 10 s (its total over its
  r, which is 1), on both time measures of every pairing that has the row; its memory verdicts stand. The analysis
  prints, for example, "over cap (calibration rep Pack-primes 14.2 s)".
- **M3-H7. The adapted call-for-call check** (v0.3 §16.9 item 3): the counting run's per-operation record with the
  gcd and gcd-work fields cleared, compared with TC's by sha256 (so the case, exact divisions, multiplications and
  additions are equal per operation); Hash-V-pack's whole record equal to TC's and G(row)/G(TC) = 1 (carried over);
  each factored row against Hash-V-pack (§16.9 item 4): every counter of every matrix but the operations' gcds and gcd
  work, and every store's W and found/new, equal. G(row)/G(TC) of the factored rows is reported per part (entry, the
  operations' additions, their multiplications) beside their factor work.
- **M3-H8. Toy runs skip the two large trial divisions** of the unit tests (`m3_unit --quick`): a toy run is a harness
  test, and the measured runs' step 2 runs them in full (about 7 s in the test build here, 20 s under the
  sanitizers). The harness's own C-suite test runs them in full.
- **M3-H9. The model** (step 0): Pack-primes' factorizations by `sympy.factorint`, their trial-division counts by the
  closed form of §16.4 item 3 (checked against a plain loop by a harness test); Pack-coprime's pass 1, pass 2,
  reconciliation, splits, rebuilds and leftovers run in Python integers in the same order of work as `mh3.c`. The
  model predicts `gchecked` for the test build; the counting build prints 0. On this machine the model takes about 12 s
  for an n = 64 multiplication job.
- **M3-H10. The TC gate** is m2's `tc3gate_m2.c`, unchanged (the same job sets), built into m3's bin directory.
- **M3-H11. `--rerun TC|HASH|FAC`**: TC; the three hash rows and the null row; or the two factored rows.
- **M3-H12. Toy sizes** for harness tests only: 4, 6, 8 (m1's first three, M1-H9). A toy run writes only under
  `test/build/m3toy/`.
- **M3-H13. Also written:** `factoring.csv` beside m2's CSVs: the factored rows' counts per factoring, per matrix at its
  end, and per step (rule-m3 item 13).
- **M3-H14. Self-tests** (`test_m3_harness.py`; untimed, nothing measured): the pins (m1's and m2's included); the
  forbidden places; the workloads on the grid (step 0's files equal m1's and m2's); the model by hand; the sources
  (mh2.c's code and value arithmetic in mh3.c; one source for both factored rows; schoolbook trial division; no
  reserved slot; every allocation named); the C suites; every toy gate, counting and memory run of every set, and the
  splitting examples, against the model; the exact checks catching a difference; the runner's untimed steps; the
  statistics, the adapted call-for-call record, the pilot's projection and stops; m1's process and disturbance tests
  (unchanged); the timed block with a stand-in driver; the analysis; a toy run of each set end to end (`add` with
  `--pilot-only` then `--resume`). `--full` adds every gate and counting run at full size.

## Ambiguous (for the author)

- **M3-D1.** Hash-V-pack is `mh2.c`, not `mh.c` as §16.11 says.
- **M3-D7.** Reconciliation: the step's matrices are its operands; a no-change split rebuilds nothing; the other `D`
  list stays stale within a pass.
- **M3-D8.** A leftover splits `q` into the coprime base of `{q, hq}`.
- **M3-H3.** Step 0: only workloads and references are compared with m1's and m2's.
- **M3-H5.** The projection's row sums: the null row in `K + 6P` (with `K = 0`), not in `4t`.

## Found in the spec and rule (for the author)

- v0.3 §16.11: "Hash-V-pack is m2's `mhvp_` from `lib/src/mh.c`": it is in `mh2.c` (M3-D1; v0.2 §15.8 had the same
  slip, DECISIONS-m2 M2-D1).
- v0.3 §16.3: "Refine's working set S ... at most 32 entries" holds for the final base; while the gcd-only refinement
  runs, the list can in principle hold more (it is bounded by about log2 of the product). `mh3.c` stops with an error
  past 32; no job of the grid, the unit tests or the differential test reaches it.
- v0.3 §16.6 item 1: "at most nu_q(g) divisions, then one failed test": when all nu divisions succeed no failed test
  follows (the loop is "divide while divisible, up to nu times", §16.5 case 8). The rows and the model do the latter.
- v0.3 §16.8, "Inside a step, a leftover": the split's second number is not named (M3-D8).
- v0.3 §16.7: the pseudocode leaves the other `D` stale after a split within a pass (M3-D7); correct by the repeat.
- On every job of the grid (all 45, by the model, checked by the full-size gates), no reconciliation split and no
  leftover split occurs: every input's base already
  holds 2, 3 and 5 as separate pieces, because some numerator of each input is divisible by one of them alone (|D| = 3
  on both sides). Those paths are exercised by the unit tests, the differential test (2,796 reconciliation splits and
  106 leftover splits over 3,000 random jobs) and the splitting examples of the harness test, each checked against the
  model.
