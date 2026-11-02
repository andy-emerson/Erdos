"""asmcheck.py: the machine code of the timed loops, from build/asm/<b>.s (objdump of loop_* in each binary).

For every loop_<kind>_<op> it finds the element loop (the smallest natural loop that stores to memory not on the stack:
one operation per pass through it) and lists, inside it:
- the operation's conditional branches (not the loop's own test), each with its condition;
- calls (the error path), conditional moves (cmov), vector-register instructions (xmm/ymm/zmm), and setcc (a compare
  written as a value).
Rule 6 gate: no conditional move and no vector instruction in any loop, and in the branch-free rows (abf, amul, alg) no
conditional branch of the operation's own. Exit 1 if the gate fails.

  python3 test/asmcheck.py [-v]     -v prints each element loop's instructions
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
ASM = HERE / "build" / "asm"
BUILDS = ("control", "a", "abf", "anm", "amul", "alg", "b")
INS = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
FUN = re.compile(r"^([0-9a-f]+) <(\S+)>:$")


def functions(path):
    funs, cur = {}, None
    for line in path.read_text().splitlines():
        m = FUN.match(line)
        if m:
            cur = funs.setdefault(m.group(2), [])
            continue
        m = INS.match(line)
        if m and cur is not None:
            ops = re.sub(r"\s+#.*$", "", m.group(3)).strip()
            cur.append((int(m.group(1), 16), m.group(2), ops))
    return funs


def target(ins):
    m = re.match(r"([0-9a-f]+)\s+<", ins[2])
    return int(m.group(1), 16) if m else None


def element_loop(ins):
    addr = [i[0] for i in ins]
    idx = {a: k for k, a in enumerate(addr)}
    succ = []
    for k, (a, mn, ops) in enumerate(ins):
        s = []
        t = target(ins[k]) if mn.startswith("j") else None
        if t is not None and t in idx:
            s.append(idx[t])
        if mn != "jmp" and not mn.startswith("ret") and k + 1 < len(ins):
            s.append(k + 1)
        succ.append(s)
    pred = [[] for _ in ins]
    for k, s in enumerate(succ):
        for t in s:
            pred[t].append(k)
    n = len(ins)
    reach, stack = {0}, [0]                        # reachable from the entry (padding is not)
    while stack:
        for t in succ[stack.pop()]:
            if t not in reach:
                reach.add(t)
                stack.append(t)
    pred = [[p for p in ps if p in reach] for ps in pred]
    dom = [set(range(n)) for _ in range(n)]       # dominators from the entry, iteratively
    dom[0] = {0}
    changed = True
    while changed:
        changed = False
        for k in range(1, n):
            if k not in reach:
                continue
            ps = [dom[p] for p in pred[k]]
            d = (set.intersection(*ps) if ps else set()) | {k}
            if d != dom[k]:
                dom[k], changed = d, True
    loops = []
    for k, s in enumerate(succ):
        if k not in reach:
            continue
        # back edges k -> h, h dominating k; the natural loop of each header
        for h in s:
            if h in dom[k]:
                body, stack = {h, k}, [k]
                while stack:
                    x = stack.pop()
                    for p in pred[x]:
                        if p not in body:
                            body.add(p)
                            stack.append(p)
                loops.append((h, body))
    merged = {}
    for h, body in loops:
        merged.setdefault(h, set()).update(body)
    store = re.compile(r",\s*-?(0x[0-9a-f]+)?\((?!%rsp)[^)]*\)$")
    cands = [(len(b), h, b) for h, b in merged.items()
             if any(store.search(ins[k][2]) and ins[k][1].startswith("mov") for k in b)]
    if not cands:
        return None, None
    _, h, body = min(cands)
    return h, sorted(body)


def inventory(ins):
    h, body = element_loop(ins)
    if body is None:
        return None
    inside = set(body)
    branches, calls, cmov, vec, setcc = [], [], [], [], []
    for k in body:
        a, mn, ops = ins[k]
        if mn.startswith("j") and mn != "jmp":
            t = target(ins[k])
            tk = next((x for x in range(len(ins)) if ins[x][0] == t), None)
            exits = tk not in inside or (k + 1) not in inside
            if not exits and tk != h:
                prev = ins[k - 1]
                branches.append(f"{mn} after {prev[1]} {prev[2]}")
        if mn.startswith("call"):
            calls.append(ops)
        if mn.startswith("cmov"):
            cmov.append(f"{mn} {ops}")
        if re.search(r"%[xyz]mm", ops):
            vec.append(f"{mn} {ops}")
        if mn.startswith("set"):
            setcc.append(mn)
    return {"n": len(body), "branches": branches, "calls": calls, "cmov": cmov, "vec": vec, "setcc": setcc,
            "body": [ins[k] for k in body]}


def main():
    verbose = "-v" in sys.argv
    ok = True
    for b in BUILDS:
        funs = functions(ASM / f"{b}.s")
        for name in sorted(funs):
            if not name.startswith("loop_"):
                continue
            inv = inventory(funs[name])
            if inv is None:
                print(f"{b:8} {name:18} no element loop found")
                ok = False
                continue
            bad = inv["cmov"] or inv["vec"] or (b in ("abf", "amul", "alg") and inv["branches"])
            ok &= not bad
            print(f"{b:8} {name:18} {inv['n']:3} instructions; operation branches {len(inv['branches'])}"
                  + (f" ({'; '.join(inv['branches'])})" if inv["branches"] else "")
                  + (f"; calls {inv['calls']}" if inv["calls"] else "")
                  + (f"; setcc {' '.join(inv['setcc'])}" if inv["setcc"] else "")
                  + (f"; CMOV {inv['cmov']}" if inv["cmov"] else "") + (f"; VECTOR {inv['vec']}" if inv["vec"] else "")
                  + ("   <-- FAIL" if bad else ""))
            if verbose:
                for a, mn, ops in inv["body"]:
                    print(f"      {a:x}: {mn} {ops}")
    print("asmcheck: " + ("PASS" if ok else "FAILED"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
