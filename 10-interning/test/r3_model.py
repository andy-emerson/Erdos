"""A model of spec v0.3 in exact Python integers, for the generator's predicted figures (spec v0.3 §12.1 item 2).

It is round 2's model (r2_model.py, unchanged and imported: the cases, every K counter, the reads, the comparisons, the
zero results and the base arithmetic, which v0.3 keeps for all three rows, call for call) with the whole-number store
replaced by the two stores of round 3, simulated exactly and side by side, since both Q32 rows call intern on the
same values in the same order (spec v0.3 §10 item 3):
  - Q32-index (§7): a list in arrival order searched top to bottom. A value at index k costs k + 1 entries examined;
    a new value costs W (the whole list), then is appended. Each distinct whole number is stored once, so the search's
    result is the value's position, kept here in a dict (the model's own bookkeeping, not the row's structure).
  - Q32-hash (§8): a table of 32,768 slots, -1 empty; the home slot h(v) is the top 15 bits of v * phi mod 2^64, and
    linear probing goes to the next slot, wrapping. Probes: the slots read by one store, the home slot included.
The model's own handles are the list's indices; a comparison of handles is a comparison of values in either row,
since each whole number is stored once in each (spec v0.3 §3, `same`).

Round 2's lookup counter (lookup_search) is not counted: no round-3 row has a lookup.
"""
import r2_model
import r3_common as C

EMPTY = -1


class Model(r2_model.Model):
    def __init__(self):
        super().__init__()
        self.T = [EMPTY] * C.SLOTS            # Q32-hash's table
        self.slot = {}                        # value -> its slot (for the model's checks only)
        self.tot = {"found": 0, "new": 0, "examined": 0, "probes": 0, "longest": 0}

    def longest(self, n):
        key = (self.ph, "longest")
        if n > self.cnt.get(key, 0):
            self.cnt[key] = n
        self.tot["longest"] = max(self.tot["longest"], n)

    def intern(self, v):
        k = self.where.get(v)
        # Q32-index: the entries examined by the search
        ex = k + 1 if k is not None else len(self.W)
        self.C("examined", ex)
        self.tot["examined"] += ex
        # Q32-hash: the probe sequence from the home slot
        s, n = C.home(v), 1
        while self.T[s] != v and self.T[s] != EMPTY:
            s = (s + 1) % C.SLOTS
            n += 1
            assert n <= C.SLOTS, "Q32-hash: the table is full"
        self.C("probes", n)
        self.tot["probes"] += n
        self.longest(n)
        if k is not None:
            assert self.T[s] == v and self.slot[v] == s, "the two stores disagree: found in the list, not the table"
            self.C("intern_found")
            self.tot["found"] += 1
            return k
        assert self.T[s] == EMPTY, "the two stores disagree: new to the list, found in the table"
        k = len(self.W)
        assert k < C.Q32_CAPACITY, "Q32 capacity exceeded"
        self.W.append(v)
        self.where[v] = k
        self.T[s] = v
        self.slot[v] = s
        self.C("intern_appended")
        self.tot["new"] += 1
        return k


def predict(ops, a, b):
    """Run the model over the calls of a workload (op codes, operands). Returns a dict: the values, the cases (indices
    into r3_common.CASES, -1 for an entry), W after the last entry and at the end, the counters per phase, Q32-index's
    list in index order, Q32-hash's table slot by slot, and the storage totals after the last entry and at the end."""
    M = Model()
    cases, values = [], []
    last_entry = max(i for i, o in enumerate(ops) if o == 0)
    W_after_entry, stor_after_entry = None, None
    for i, o in enumerate(ops):
        if o == 0:
            M.enter(a[i], b[i])
            cases.append(-1)
        else:
            _, k = M.operate(o, a[i], b[i])
            cases.append(C.CASE[k])
        values.append(M.value(i))
        if i == last_entry:
            W_after_entry = len(M.W)
            stor_after_entry = dict(M.tot)
    return {"values": values, "cases": cases, "W_after_entry": W_after_entry, "W_end": len(M.W),
            "counters": M.per_phase(), "list": list(M.W), "table": list(M.T),
            "stor_after_entry": stor_after_entry, "stor_end": dict(M.tot)}
