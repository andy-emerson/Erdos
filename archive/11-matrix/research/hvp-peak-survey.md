# Hash-V-pack: lowering the peak during mul (research, not a result)

Oct 30, 2026. Read-only on the code; nothing changed or committed. Each candidate is marked **(plain)** = no new
structure, or **(new structure: name)**. Overflow stays an error throughout.

## The baseline, in exact bytes (mul job 10: n = 64, rho = 4)

From `test/results-m3-mul/results.md` (store and pack tables) and `lib/src/mh2.c` `pack()`. KB = 1000 B, as in the
results.

| Live at the peak (inside `pack(c)`, after (l) and (p) are filled, before (h) and (p) are freed) | Bytes |
| --- | --- |
| A packed: 16-bit cells 16,384 + list 1013 × 8 = 8,104 | 24,488 |
| B packed: 16,384 + 1006 × 8 = 8,048 | 24,432 |
| C cells (a), 16-bit build handles, 2 × 4096 × 2 | 16,384 |
| (h) T = 16384 slots × 8 B | 131,072 |
| (p) T × 2 B | 32,768 |
| (l) W = 4189 × 8 B | 33,512 |
| **Total** | **262,656 = 1.336 × TC (196,608)** |

The summary's 1.30 is the median over the 12 mul jobs; job 10 itself is 1.336. The worst n = 64 job is job 9
(rho = 1, larger input lists): 303,472 B = 1.544 ×.

Facts that drive the candidates:

1. **Every mul result is 64-bit.** All 12 mul jobs' C (and every sop/pos product) end at w = 64 with one widening,
   8 → 64, at the first store. Numerators are 43–49 bits.
2. **The "+2" doubles T on this sweep.** `create()` sizes T = 2^⌈log2(2rc + 2)⌉. v0.2 §15.10 item 1 says the bound on W
   is 2rc and the "+2" is kept as headroom from m1 (where 0 and 1 were reserved). At n = 16, 32, 64, 2rc is a power of
   two, so the +2 is exactly what turns T = 2rc into T = 4rc. Without it, T = 8192 at n = 64.
3. **The current rule does not guarantee load ≤ 0.5.** Its worst-case load is 2rc / T, which is 0.5 only when 2rc is a
   power of two. For other n it is anywhere from 0.5 to just under 1 (e.g. n = 45: 2rc + 2 = 4052, T = 4096, worst-case
   load 0.99). The "4rc" size is a property of the sweep, not of the rule.
4. **Actual fill is low.** C's W = 4189 of T = 16384 (25.6%); measured 1.09 probes per store, longest 7.
5. **Handles are slots.** Any scheme that moves an entry after it is stored (Robin Hood, cuckoo, hopscotch, rehashing)
   changes the handles already written in the cells. So it would need a rewrite pass like grow-map's. This rules them
   out unless the handle stops being the slot (candidate G).
6. **The gate reads (h) at the pack hook.** `bench_m3.c on_pack` dumps (h) slot by slot through `t_whole`, just before it
   is freed. Any candidate that overwrites (h)'s values during the pack must call the hook (or take the dump) before
   that, and the audit build's `before` copy must also be taken first. The `m2_pack` record's `bytes_map` would read 0
   or change meaning.
7. **The inputs cannot be freed or reused.** A and B belong to the caller (sop and pos reuse them, and the harness reads
   them). So this saves nothing: 48,920 B is a fixed floor. The floor of any design is I + C's packed size =
   48,920 + 16,384 + 33,512 = 98,816 B (0.503 ×).

## Candidates

Peaks below are at job 10. The sums use I = inputs 48,920, Cc = C's cells 16,384, l = 33,512.

### A. Positions written into (h)'s own slots (plain)
In the copy loop, read v from slot s, append it to (l) at k, then write k into slot s (as a 64-bit value). The rewrite
reads the position from (h) at the handle's slot. There is no separate (p). Every slot a handle names is a used slot,
so EMPTY never has to be told apart.
- Peak I + Cc + h + l = **229,888 B = 1.169 ×** (job 9: 1.377 ×).
- Speed: one malloc/free and one 32 KB write stream fewer; the position goes into a cache line already loaded.
  Neutral to slightly faster (well under 0.1% of mul Ir).
- Needs fact 6 (the hook, and the audit copy, move before the copy loop).

### F. Reorder only: ranks, rewrite, free (p), then allocate (l) (plain)
Pass 1 fills (p) with ranks by scanning (h) (no copy). Pass 2 rewrites the cells. Then (p) is freed, (l) is allocated,
and (h) is scanned again to copy the values.
- Peak I + Cc + h + max(p, l) = **229,888 B = 1.169 ×**.
- Speed: one extra scan of T slots (about 16k slot reads; about +0.03% of mul Ir, a few % of add Ir).
- Same peak as A here. A is never worse (A pays h + l, F pays h + max(p, l)), so F is superseded unless the
  owner prefers not to overwrite (h).

### J. F, then compact (h) in place and shrink it to become (l) (plain)
As F up to the free of (p). Then move each used value from slot s to position k ≤ s in one forward pass (always safe,
since slot k was already read), and `realloc` (h) down to W × 8: that block is (l). No second buffer for the values.
- Peak I + Cc + h + p = **229,144 B = 1.165 ×** at T = 16384 (job 9: 1.373 ×).
- Better than A whenever (p) < (l), i.e. at fill W/T > 1/4 with 16-bit handles and 64-bit values. That is borderline
  now (25.6%) and clear once T is halved (see C1 + J).
- Speed: as F (one extra T scan); the realloc shrink is in place for this block size (glibc trims or mremaps; no copy).
  The accounted peak does not depend on that.
- Excludes A (the map cannot live in the slots while the values are compacted into them).

### B. Pack in order of first appearance, walking the cells (plain; changes the list order)
Walk the 2rc fields in order. At handle s: if slot s still holds a value (≥ 0), append it to (l) at the next k and
overwrite the slot with the mark −(k + 2). Otherwise read k from the mark. Rewrite the field. Values are magnitudes ≥ 0
and EMPTY = −1, so the marks (≤ −2) cannot be confused with them.
- Peak as A: **1.169 ×**.
- Speed: no scan of T at all (the pack becomes O(2rc), not O(T + 2rc)). About −0.03% of mul; on add and build, where
  pack is 11–20% of Ir, it removes most of the T-proportional part.
- **Changes (l)'s order** from slot order (v0.2 §15.4 item 3) to first-appearance order. That is a spec change, and the
  gate's model of (l) has to follow. It is A's idea plus the order change.

### C1. Drop the "+2" headroom: T = 2^⌈log2 2rc⌉ (plain; spec v0.2 §15.10 item 1)
T = 8192 at n = 64, so (h) is 65,536 B and (p) is 16,384 B.
- Peak with the current pack: I + Cc + 65,536 + 16,384 + l = 180,736 B = **0.919 ×**. With A: 164,352 B = **0.836 ×**.
  **With J: 147,224 B = 0.749 ×** (job 9: 188,048 B = 0.956 ×).
- Correctness: W ≤ 2rc ≤ T always holds, so a new value always finds an empty slot. `full()` stays as the guard.
- Speed at the measured fill (51%): linear probing expects about 1.5 probes per hit and 2.5 per miss (Knuth), against
  about 1.2 and 1.4 now. Hashing is under 0.5% of mul Ir, so the change is neutral there. Init, widen and pack loops halve
  (a gain on add and build).
- Risk: the worst-case load becomes 1.0. If a matrix reaches W ≈ 2rc, probe runs grow to O(T) per store. That is a speed
  cliff, not an error. Fact 3: the current rule already has worst-case loads near 1 for some n.

### C3. Any T, with a load cap, through a multiply-high reduction (plain; hash formula changes)
`home(v) = (hi32(v·φ) × T) >> 32` (Lemire's "fastrange") instead of the top t bits, so T need not be a power of two.
The probe's wrap becomes a compare instead of a mask.
- With a cap of 0.75 at worst (T = ⌈2rc / 0.75⌉ = 10,923): (h) 87,384 B and (p) 21,846 B. Peak with the current pack
  208,046 B = 1.058 ×; with A 186,200 B = 0.947 ×; **with J 174,534 B = 0.888 ×**.
- With a cap of 0.5 (T = 4rc) it equals the current table on this sweep. Off the sweep it makes the load rule mean what
  the owner thinks it means (fact 3).
- Speed: one more multiply per home; neutral. It guards the worst case better than C1.
- Choose C1 or C3, not both.

### G. The table holds list positions; values are stored once, in a dense build list (new structure: index table + dense list, "compact dict")
The build list (l) is allocated at the worst case, 2rc entries, and a new value is appended. (h) holds 16-bit positions
(EMPTY = 0xFFFF), and the probe compares v with l[h[s]]. **The handle is the position**, so there is no (p), no rewrite
and no T scan at the end. The pack is: free (h), realloc (l) down to W entries, and (only if W ≤ 128) narrow the fields
in place. Widening applies to (l) instead of (h). This is CPython's 3.6+ dict layout (Hettinger 2012).
- Peak I + Cc + 32,768 (index, T = 16384) + 65,536 (l at 2rc × 8) = 163,608 B = **0.832 ×**. **With C1:
  147,224 B = 0.749 ×** (job 9: 0.956 ×). The same as C1 + J.
- Speed: one extra dependent load per probe (negligible in mul). It removes pack's map, scan and rewrite (a clear gain
  on add and build).
- (l) is in insertion order (a spec change, like B). Because entries no longer are handles, Robin Hood or a high load
  would become possible on the index. But the index is only 16–32 KB, so it is not worth it.
- Replaces A, B, F and J. It combines with C1/C3, L and M.

### H. Sort-based dedup, no hash (new structure: raw value array + sort buffer)
The build writes each cell's magnitude and tag as 64-bit values (V, 2rc × 8). At the end: copy V, heapsort the copy in
place, dedup it, and realloc it to W × 8 = (l). Each field = binary search in (l) (handles 16-bit). Then free V.
- Peak I + V + copy = 179,992 B = **0.915 ×**.
- Speed: heapsort plus 8192 binary searches, about 1–2 M Ir. That is under 1% of mul, but roughly +100% on add
  (TC add is 1 M Ir).
- (l) ends in sorted order (a spec change). It excludes all the hash candidates. It is dominated by C1 + J on peak.

### D. Separate numerator and denominator tables (new structure: two tables and two lists)
Each table's bound is rc. Denominators (≤ 19 bits here) stay at 32-bit slots, numerators at 64.
- (h) 65,536 + 32,768 = 98,304 B. Peak about 229,900 B (1.17 ×) with the current pack; about 196,400 B (1.00 ×) with
  J. Read path: two value bases per cell.
- Dominated by C1 on peak, with no new structure there. Not recommended.

### K. Narrower slot storage (plain, but a new width case)
- "Narrow when values fit": **0 at this job**, since every mul result needs 64 bits (fact 1).
- An odd width (7-byte slots via memcpy, since 49-bit magnitudes plus the sign bit need 50 bits; 6 bytes is too
  narrow): (h) 114,688 B and (l) 29,323 B, about 1.23 × with the current pack. Unaligned access on every probe and
  read. That is clever engineering for about 0.1 ×. Not recommended.

### Rejected
- **E. Freeing or reusing the inputs' buffers:** 0. They are caller-owned and alive after mmul (fact 7).
- **Sizing from the inputs' list sizes:** no sound bound. Result values are sums of products, so W_C is not bounded by
  W_A, W_B. Using it would need growth, which was tested and set aside in m1.
- **Robin Hood / cuckoo on (h) as is:** they move stored entries and so break slot = handle (fact 5).
- **In-place compaction with the map inside (h):** the 64-bit slot cannot hold both a value up to 2^63 − 1 and a
  position. That would need bit-packing (clever) or cycle-following permutation (clever). J gets the same effect plainly.

## The two approved speed changes

- **L. mul's result table at 64 bits from the start:** no peak change at job 10, or at any mul job of the sweep, because
  every C already widens 8 → 64 at its first store (fact 1). It removes that widening's transient
  (I + Cc + 16,384 + 131,072 = 212,760 B, below the pack peak). It would raise the peak only for a future mul whose
  results all fit in 32 bits ((h) 2–8 × larger). Under G, it fixes (l)'s worst-case at 2rc × 8 from the start, which is
  what G's peak already assumes.
- **M. One decoded row of A as scratch:** n entries. As two 64-bit magnitudes plus a sign per entry, that is about
  64 × 17 B ≈ 1.1 KB (+0.006 ×) if it is heap, accounted and still alive at the pack. 0 if it is freed before `pack(c)`
  (or kept on the stack). So: free it before the pack.

## Summary table (job 10; TC = 196,608 B; current 262,656 B = 1.336 ×)

| Candidate | What it does | Peak (× TC) | Plain / new | Speed effect (Ir) | Combines with |
| --- | --- | --- | --- | --- | --- |
| A | positions into (h)'s slots; no (p) | 1.169 | plain | neutral, slightly less | B, C1/C3, L, M; not J, G |
| F | rank → rewrite → free (p) → alloc (l) | 1.169 | plain | +1 T scan (~0.03% mul) | C1/C3, L, M; superseded by A or J |
| J | F, then compact (h) in place, realloc → (l) | 1.165 | plain | +1 T scan | C1/C3, L, M; not A, B, G |
| B | pack by first appearance, marks in slots | 1.169 | plain (list order changes) | removes T scan; gain on add/build | A (it contains it), C1/C3, L, M |
| C1 | drop "+2": T = 2rc | 0.919 alone; 0.836 + A; **0.749 + J** | plain (spec §15.10) | neutral on mul; worst-case load 1.0 | A/B/F/J/G, L, M; not C3 |
| C3 | any T, load cap 0.75, fastrange hash | 1.058 alone; 0.947 + A; 0.888 + J | plain | neutral | A/B/F/J/G, L, M; not C1 |
| G | table of positions + dense list (2rc) | 0.832; **0.749 + C1** | new: index table + dense list | +1 load per probe; pack nearly free | C1/C3, L, M; replaces A/B/F/J |
| H | sort-based dedup | 0.915 | new: value array + sort buffer | <1% mul, ~+100% add | none of the hash rows |
| D | split num/den tables | ~1.17; ~1.00 + J | new: two tables, two lists | neutral | A/J, C1 |
| K | 7-byte slots | ~1.23 | plain (new width case) | slower reads | all; low value |
| L | mul table 64-bit from start | 1.336 (no change) | plain (approved) | removes 1 widening | all |
| M | decoded row of A | +0.006 if alive at pack, else 0 | plain (approved) | approved gain | all (free before pack) |

**Recommendation.** Take C1 + J: plain, about 0.75 × TC at job 10 (0.956 × at job 9). It needs one spec ruling, that the
"+2" headroom goes, so T = 2rc and the worst-case load is 1.0, plus the hook moved before (h) is compacted. If a
worst-case load of 1.0 is unacceptable, use C3 (cap 0.75) + J for about 0.89 ×.

A alone (1.17 ×) is the smallest code change if the sizing rule must stay as it is. G reaches the same 0.75 × as C1 + J
but needs approval of a new structure, so prefer C1 + J.

## Sources

- Compact (index + dense entries) dict: [R. Hettinger, "More compact dictionaries with faster iteration", python-dev, Dec 2012](https://mail.python.org/pipermail/python-dev/2012-December/123028.html).
- Multiply-high range reduction: [D. Lemire, fastrange](https://github.com/lemire/fastrange); [Lemire, "Performance of ranged accesses into arrays: modulo, multiply-shift and masks"](https://eklausmeier.neocities.org/lemire/blog/2018/08-20-performance-of-ranged-accesses-into-arrays-modulo-multiply-shift-and-masks).
- From my own knowledge (not fetched): Knuth, TAOCP vol. 3 §6.4, linear probing's expected probes, ½(1 + 1/(1−α)) for a hit and
  ½(1 + 1/(1−α)²) for a miss; Robin Hood hashing (Celis 1986) moves entries on insert.
