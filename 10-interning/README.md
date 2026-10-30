# 10: Interning (Design 3, layer 1)

Exact sharing of whole numbers, as in [`../paper/v0.4.md`](../paper/v0.4.md) §5. Every distinct whole number is stored once. Each rational is a pair of 16-bit handles to whole numbers, with a sign tag on the numerator, and is always kept in lowest terms. Every operation compares indices first and takes a shortcut from the paper's §5.2 table when one applies. Otherwise it uses the gcd-first method. In round 3, each Q32 row has exactly two structures: the rationals, and the store of whole numbers. In Q32-index that store is a list in arrival order, searched top to bottom, and a handle is an index into it. In Q32-hash it is a fixed hash table of 32,768 slots, and a handle is a slot.

## Round 3: two-structure Q32 (approved Oct 29, 2026)

**Status: approved by the author, Oct 29, 2026; amended Oct 29, 2026 before any measurement (v0.3 §17: 0 and 1 at fixed handles 0 and 1; the special cases of a denominator of 1 and a numerator of 0, ordered by the cost of their result); built and tested untimed, not run.** An independent review found the draft sound after fixes, which are applied. The author then approved every choice (R3-1 to R3-5) and every new threshold of the rule, which is now fixed. Next come the build, its review, and the run.

**Why round 3.** Round 2's Q32 had three structures: the rationals, the list of whole numbers, and a separate hash lookup from value to index. The author's model has two. Round 2's result stays on record as **"three-structure Q32 (retired)"** in [`test/results-r2/`](test/results-r2/results.md).

The round has three rows, with every structure counted:
- **TC** has 1 structure and is unchanged from round 2.
- **Q32-index** has 2 structures: the rationals, with 16-bit index handles, and a list of whole numbers in arrival order. The list is searched top to bottom, with no hash and no lookup.
- **Q32-hash** has 2 structures: the rationals, with 16-bit slot handles, and a fixed hash table of 32,768 slots, with multiplicative hashing and linear probing. It has no separate list and no fill limit. Only a truly full table is an error.

The arithmetic is identical in all three rows, call for call. Creation time and operation time each get their own verdict. Each job's fill and probes are reported, with speed shown against fill.

The documents:
- **[v0.3](v0.3.md):** the specification.
- **[research/rule-r3.md](research/rule-r3.md):** the rule. It uses round 2's jobs and timing method, with each threshold either carried over from round 2 or new and approved by the author, Oct 29, 2026. The rule is fixed: the harness pins its sha256 (`f4671f1fbc53273a193e63fc1e1c13e116aa0d7d6a06647aadbf6890407f2cf9`) and refuses a measured run if the file differs or still holds a mark of a proposal.

## Round 2: two textbook baselines (approved Oct 29, 2026), three-structure Q32 (retired)

**Status: approved by the author, Oct 29, 2026; built and reviewed (below), not run.** An independent review (Oct 29, 2026) accepted the choices R2-1 to R2-10, two of them on conditions that the revision met (v0.2 §17). The author then approved every choice and every threshold, with gcd work as a check, not a verdict, and added the ±1 operand shortcut. The rule is fixed. Next come the build, its review, and the run.

**Build (Oct 29, 2026): built and reviewed, not run.** The build review found it ready after one fix (the profile's
M per operation, now divided by the runs per rep) and two minor ones; every untimed test passes. **For the author to
confirm:** R2-H7 in [`test/DECISIONS.md`](test/DECISIONS.md): batch times exist per part only (entry, operations,
read-out), so the profile gives estimated ns per operation per part, as the spec gives them, and per job phase and
per case only shares and mean corrected times (spec §13.4 item 7 speaks of each job phase's batch time).

The author's decisions (Oct 29, 2026):
- **Only two rows:**
  - **Q32**: layer 1 with 16-bit index fields (a 15-bit index plus the sign bit), capacity 32,768 whole numbers;
  - **TC**: the textbook control, plain rationals as two 64-bit values in lowest terms, by the gcd-first method.
- **TC takes every value-comparison shortcut** of the paper's §5.2 table, in each of its three algorithms (addition and subtraction, multiplication, division). It has no structures, caching or lookups.
- **Both rows follow the same principle: "correct programming, not clever engineering."** Work whose answer the operation already knows is not done in either row. That covers known indices, known 1s and x − x. The other former tricks stay out.
- **A profiling build** breaks each row's time down by phase: operand fetch, shortcut checks, arithmetic, interning, memory management. It is there for the systematic optimization that comes later. It probes a sample of the calls (one in K), times every part in batches, and has a limit on its slowdown; its per-phase differences between the rows are indicative only.
- **The ±1 operand shortcut**, added by the author, in both rows: multiplying or dividing by ±1 gives the other operand, with its sign changed for −1, and a dividend of ±1 gives the divisor's reciprocal. It is checked by value in TC and by index in Q32.
- **Retired:** L1-opt and its tricks, the Q64 rows, FLINT `fmpq`, GMP `mpq` and the FLINT ceiling. Their code and results stay in the history and the archives, and they are not run.

The specification and the rule:
- **[v0.2](v0.2.md):** the specification. It covers:
  - the two rows, as new sources `lib/src/q32.c` and `lib/src/tc.c`, leaving every retired file unchanged;
  - one pseudocode per algorithm, followed by both rows;
  - every known-answer skip (§11);
  - the profiling build (§13.4);
  - the retirement (§16).

  Both rows make the same arithmetic, call for call. They differ only in structure: index comparisons, lookups and indirection against value comparisons and pairs. §17 lists the choices, each approved by the author.
- **[research/rule-r2.md](research/rule-r2.md):** the measurement rule. It covers:
  - the 8 jobs that fit Q32 (1–4 and 8–11; jobs 5, 6 and 7 are over capacity and not run);
  - per-measure verdicts on time and bytes per rational, with gcd work reported as a check (equal by construction) and the profile reported and not judged;
  - round 1's timing method;
  - a hard stop at 30 minutes, with a pilot that includes a profiling run and projects the run's time;
  - **nothing of the runner's own running during timed phases**: timed programs started by `fork` and `exec`, pinned before `exec`; a blocking wait; a kernel timer for the hard stop; and an operator who runs nothing until the exit notice. This is checked after each timed block by every process's ticks, its waited-for children's included, and by the kernel's fork counter, with an allowance for the host processes that the pilot measures.

  Every threshold is approved by the author, Oct 29, 2026, and the rule is fixed. The harness pins its sha256, and refuses a measured run if the file differs or still holds a mark of a proposal.

## Round 1 (archived)

Round 1 measured L1 and L1-opt against the 08 control, FLINT `fmpq`, GMP `mpq` and a FLINT shared-denominator ceiling. Its later Q32 run added L1-Q32 and L1-opt-Q32. The record is kept unchanged:
- **The specification:** [v0.1](v0.1.md), with §17 (L1-opt) and §18 (the Q32 rows).
- **The rule:** [research/rule.md](research/rule.md), pinned by hash in the results.
- **The code:** [`lib/`](lib/) ([`lib/DECISIONS.md`](lib/DECISIONS.md)) and [`test/`](test/) ([`test/DECISIONS.md`](test/DECISIONS.md)).
- **The results:**
  - [`test/results-q64-b3b88b6/`](test/results-q64-b3b88b6/results.md): the official Q64 verdicts;
  - [`test/results-q32/`](test/results-q32/results.md): the Q32 run;
  - [`test/results/`](test/results/results.md): the original `b3b88b6` output.

**Reuses from 08,** kept in round 2 unchanged and hash-checked: the shared integer module (`lib/vendor08/sk_internal.h` and `common.c`, copied from [`shared/partial-sharing-baseline`](../shared/partial-sharing-baseline/)). The 08 control (`ctl.c`) is kept but is no longer a row.

**Order of work:** paper → spec and rule → review → build → review → run → results.
