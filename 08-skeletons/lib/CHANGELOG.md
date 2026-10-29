# Change log: fixes of plain inefficiencies (spec §11)

Every fix of a plain inefficiency inside the skeleton made while building, with its reason and why it changes no structure and no result. The fixes the spec already makes (§15.1: D2-a to D2-f, D1-a to D1-c) are built as written and are not repeated here.

## CL-1 (Design 1, §7.5 add): no division by h when g is 1

**Pseudocode.** `if g is 1 then set h to 1 … return store((t / h) over (r_x · (d_j / h)))` divides t and d_j by 1, a constant the code has just set.

**Built.** When g is 1, `store(t over r_x · d_j)`.

**No change.** t/1 = t and d_j/1 = d_j, so the stored value is identical. No structure is read or written differently; only the two divisions by the literal 1 are gone. Test: `test_d1_counting`, which shows 0 core divisions for 1/5 + 1/7.

**For the author.** The same pattern, a division by a gcd that turns out to be 1, appears elsewhere: the control's `t/h`, `m_x/g_1` and so on, Design 2's multiply `m / h`, and entry's division by `gcd(m, d)`. There the divisor is a computed gcd, not a constant, so skipping it would need a test, which no pseudocode states, and it would change every row's division counts. Those are built literally. Design 2's addition has the test (`if h is not 1`) and is built with it, as written.

## CL-2 (ceiling, §9 item 4 add/subtract): E / E_x is E_y / g, already computed

**Pseudocode.** `E = E_x · (E_y / g)`, then `n_k = x_k · (E / E_x) ± y_k · (E / E_y)`.

**Built.** `f_x = E_y / g` (one division), `E = E_x · f_x`, `f_y = E / E_y` (one division), then `n_k = x_k · f_x ± y_k · f_y`. Each factor is computed once per operation, not once per element; computing it per element would be the larger, plainer inefficiency. E / E_x is not divided again, because it equals the quotient E_y / g just formed.

**No change.** E / E_x = E_x · (E_y / g) / E_x = E_y / g exactly, so every n_k, E and the content step are identical. The ceiling has no other structure. Test: the ceiling differential run (`run_ceil`).

## CL-3 (Design 1, §7.1 pair set): no membership search before an insertion

**Pseudocode.** §7.3's `compare` tests "(i, j) is in the pair set" and returns if it is; otherwise it later says "insert (i, j) into the pair set". The first build's insertion searched the set for the key again before placing it.

**Built.** The insertion hashes the key and probes to the first empty slot, with no membership test: `compare` has just found the pair absent, and nothing inserts it in between (DEC-47).

**No change.** The set holds the same keys in the same slots, since the search for an absent key stops at the same empty slot that the insertion uses. Every stored value and pair is the same; only the second search's `lookup_search` and `lookup_probe` counts are gone. A duplicate insertion would make the count of keys differ from the keys held, which the full audit checks. Tests: `test_d1_pair_set`, and the differential run, whose checksums are unchanged.
