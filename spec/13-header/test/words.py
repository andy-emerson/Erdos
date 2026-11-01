"""words.py: the shared parts of spec 13 §1, word by word at W = 4 bits.

Routines follow Growing (spec 12, src/growing.nim) word by word: carries and borrows as values (s < x), operands read at
their real length, each writing into a destination that checks the spec's bound and its own capacity.
Pools, slots, the free list and the move-down are §1 Pools and §1 Result class.

Memory the model has not written holds POISON (all ones), so a read of an unwritten word shows. ZERO_SLOTS (a
diagnostic, not the spec) makes every allocation zero its slot.
"""
W = 4
M = (1 << W) - 1          # a word's mask; also the free list's end marker (all ones)
POISON = M
ZERO_SLOTS = False        # set by round1.py for the diagnostic run


class SpecError(Exception):
    """an error the spec names (class above 31, cross-axis sum) or a fault the model detects (overflow, index)"""


# ------------------------------------------------------------------------------------------------ classes
def cls(n):
    """the smallest class holding n words (class 0 holds one word)"""
    c = 0
    while (1 << c) < n:
        c += 1
    if c > 31:
        raise SpecError(f"class {c} above 31")
    return c


# ------------------------------------------------------------------------------------------------ destinations
class Dst:
    """a destination: cap words of mem from off (the slot, the one-word local or part of the working buffer).
    A write at or past cap is an overflow (it would hit the next slot); a write at or past bound breaks the routine's
    bound and is recorded."""
    __slots__ = ("mem", "off", "cap", "bound", "breach")

    def __init__(self, mem, off, cap, bound):
        self.mem, self.off, self.cap, self.bound, self.breach = mem, off, cap, bound, None

    def set(self, i, v):
        if i >= self.cap:
            raise SpecError(f"overflow: word {i} written into a destination of {self.cap} words (bound {self.bound})")
        if i >= self.bound and self.breach is None:
            self.breach = f"word {i} written, bound {self.bound}"
        self.mem[self.off + i] = v & M

    def get(self, i):
        return self.mem[self.off + i]


def norm(mem, off, n):
    while n > 0 and mem[off + n - 1] == 0:
        n -= 1
    return n


# ------------------------------------------------------------------------------------------------ routines
def r_add(r, x, xo, nx, y, yo, ny):
    """Growing's opAdd: r = x + y, writes max + 1 words, returns the length"""
    if nx < ny:
        x, xo, nx, y, yo, ny = y, yo, ny, x, xo, nx
    c = 0
    for i in range(ny):
        s = (x[xo + i] + c) & M
        c1 = int(s < c)
        t = (s + y[yo + i]) & M
        r.set(i, t)
        c = c1 + int(t < s)
    for i in range(ny, nx):
        t = (x[xo + i] + c) & M
        r.set(i, t)
        c = int(t < c)
    r.set(nx, c)
    return norm(r.mem, r.off, nx + 1)


def mulw(a, b):
    p = a * b
    return p & M, p >> W


def r_mac(r, nr, x, xo, nx, y, yo, ny):
    """r += x * y (Growing's opMul, accumulating). r holds nr words; words nr .. max(nr, nx + ny) - 1 are set to 0
    first, and a carry past them is written only when nonzero. Returns the length."""
    top = max(nr, nx + ny)
    for k in range(nr, top):
        r.set(k, 0)
    lim = top
    for i in range(nx):
        c = 0
        for j in range(ny):
            lo, hi = mulw(x[xo + i], y[yo + j])
            lo = (lo + c) & M
            hi += int(lo < c)
            t = r.get(i + j)
            lo = (lo + t) & M
            hi += int(lo < t)
            r.set(i + j, lo)
            c = hi & M
            assert hi == c
        k = i + ny
        while c:
            if k == lim:
                r.set(k, c)            # the carry word, only when nonzero
                lim += 1
                c = 0
            else:
                t = (r.get(k) + c) & M
                c = int(t < c)
                r.set(k, t)
                k += 1
    return norm(r.mem, r.off, lim)


def r_sub_into(r, x, xo, la, y, yo, lb):
    """Growing's subInto: r = x - y over max(la, lb) words; returns the borrow"""
    br = 0
    m = min(la, lb)
    for i in range(m):
        p, q = x[xo + i], y[yo + i]
        d = (p - q) & M
        r.set(i, d - br)
        br = int(p < q) + int(d < br)
    if la >= lb:
        for i in range(m, la):
            p = x[xo + i]
            r.set(i, p - br)
            br = int(p < br)
    else:
        for i in range(m, lb):
            q = y[yo + i]
            d = (0 - q) & M
            r.set(i, d - br)
            br = int(q != 0) + int(d < br)
    return br


def r_absdiff(r, x, xo, nx, y, yo, ny):
    """Growing's opAbsdiff2: |x - y| over max words, and the borrow (1 when x < y). Returns (length, borrow)."""
    n = max(nx, ny)
    br = r_sub_into(r, x, xo, nx, y, yo, ny)
    m = (0 - br) & M
    c = br
    for i in range(n):
        t = ((r.get(i) ^ m) + c) & M
        c = int(t < c)
        r.set(i, t)
    return norm(r.mem, r.off, n), br


def r_cmp(x, xo, nx, y, yo, ny):
    if nx != ny:
        return -1 if nx < ny else 1
    for i in range(nx - 1, -1, -1):
        if x[xo + i] != y[yo + i]:
            return -1 if x[xo + i] < y[yo + i] else 1
    return 0


def ctz1(w):
    return (w & -w).bit_length() - 1


def ctzw(x, xo):
    i = 0
    while x[xo + i] == 0:
        i += 1
    return i * W + ctz1(x[xo + i])


def shrw(x, xo, n, s):
    """Growing's shrw: x >>= s in place over n words; returns the new length (the top words it vacates are left)"""
    q, b = divmod(s, W)
    m = n - q
    if b == 0:
        for i in range(m):
            x[xo + i] = x[xo + i + q]
    else:
        for i in range(m):
            hi = (x[xo + i + q + 1] << (W - b)) & M if i + q + 1 < n else 0
            x[xo + i] = (x[xo + i + q] >> b) | hi
    n = max(m, 0)
    return norm(x, xo, n)


def inverse(b):
    x, bits = b, 3
    while bits < W:
        x = (x * (2 - b * x)) & M
        bits *= 2
    return x


def r_div_core(q, buf, xo, nx, yo, ny, n1, n2):
    """Growing's opDivexact on the working buffer (x at xo, y at yo, already loaded), writing exactly n1 - n2 + 1
    quotient words (§1: it stops there). Words of x at or past its current length read as 0. Returns the length."""
    s = ctzw(buf, yo)
    nx = shrw(buf, xo, nx, s)
    ny = shrw(buf, yo, ny, s)
    n = max(n1 - n2 + 1, 0)
    inv = inverse(buf[yo])
    for i in range(n):
        xi = buf[xo + i] if i < nx else 0
        qi = (xi * inv) & M
        q.set(i, qi)
        c = br = 0
        j = 0
        while j < ny and i + j < nx:
            lo, hi = mulw(qi, buf[yo + j])
            lo = (lo + c) & M
            hi += int(lo < c)
            t = buf[xo + i + j]
            d = (t - lo) & M
            buf[xo + i + j] = (d - br) & M
            br = int(t < lo) + int(d < br)
            c = hi
            j += 1
        k = i + j
        while k < nx and (c or br):
            t = buf[xo + k]
            d = (t - c) & M
            buf[xo + k] = (d - br) & M
            br = int(t < c) + int(d < br)
            c = 0
            k += 1
    return norm(q.mem, q.off, n)


def r_gcd_core(r, buf, xo, nx, yo, ny):
    """Growing's opGcd (the general loop, without its round-2 one-word special case) on the working buffer; the
    result, y << shift, is written at its own length only. Returns the length."""
    if nx == 0:
        for i in range(ny):
            r.set(i, buf[yo + i])
        return ny
    if ny == 0:
        for i in range(nx):
            r.set(i, buf[xo + i])
        return nx
    shift = min(ctzw(buf, xo), ctzw(buf, yo))
    ny = shrw(buf, yo, ny, ctzw(buf, yo))
    while nx != 0:
        nx = shrw(buf, xo, nx, ctzw(buf, xo))
        if r_cmp(buf, yo, ny, buf, xo, nx) > 0:
            xo, yo, nx, ny = yo, xo, ny, nx
        br = 0
        for i in range(nx):
            p = buf[xo + i]
            qq = buf[yo + i] if i < ny else 0
            d = (p - qq) & M
            buf[xo + i] = (d - br) & M
            br = int(p < qq) + int(d < br)
        nx = norm(buf, xo, nx)
    # y << shift, word by word, into a list first so only the result's length is written
    q, b = divmod(shift, W)
    out = [0] * q
    c = 0
    for i in range(ny):
        w = buf[yo + i]
        out.append(((w << b) & M) | c)
        c = (w >> (W - b)) if b else 0
    out.append(c)
    n = len(out)
    while n > 0 and out[n - 1] == 0:
        n -= 1
    for i in range(n):
        r.set(i, out[i])
    return n


# ------------------------------------------------------------------------------------------------ pools
class Pool:
    """class c: slots of 2^c words. mem is replaced on growth (pointers do not survive; the old list is poisoned)."""
    __slots__ = ("c", "size", "mem", "cap", "next", "head")

    def __init__(self, c):
        self.c, self.size, self.mem, self.cap, self.next, self.head = c, 1 << c, [], 0, 0, M

    def alloc(self):
        if self.head != M:                          # pop the free list
            i = self.head
            self.head = self.mem[i * self.size]
        else:
            if self.next == self.cap:               # double, from one slot at first use
                old = self.mem
                self.cap = max(1, 2 * self.cap)
                self.mem = old + [POISON] * (self.cap * self.size - len(old))
                for k in range(len(old)):
                    old[k] = POISON
            i = self.next
            self.next += 1
        if i > M:
            raise SpecError(f"slot index {i} does not fit a {W}-bit word (pool {self.c})")
        if ZERO_SLOTS:
            for k in range(self.size):
                self.mem[i * self.size + k] = 0
        return i

    def release(self, i):
        self.mem[i * self.size] = self.head
        self.head = i

    def check(self):
        """every slot taken is on the free list, once; returns an error string or None"""
        seen = set()
        i = self.head
        while i != M:
            if i in seen:
                return f"pool {self.c}: slot {i} twice on the free list"
            if i >= self.next:
                return f"pool {self.c}: free list holds slot {i}, never taken"
            seen.add(i)
            i = self.mem[i * self.size]
        if len(seen) != self.next:
            return f"pool {self.c}: {self.next - len(seen)} of {self.next} slots not on the free list"
        return None


class Pools:
    def __init__(self):
        self.p = [None] + [Pool(c) for c in range(1, 32)]

    def ptr(self, c, i):
        """a pointer: (the pool's current list, the slot's offset)"""
        pl = self.p[c]
        return pl.mem, i * pl.size

    def check(self):
        for pl in self.p[1:]:
            e = pl.check()
            if e:
                return e
        return None

    def max_slots(self):
        return max(pl.next for pl in self.p[1:])


# ------------------------------------------------------------------------------------------------ parts (class, word)
def part_len(pools, c, w):
    """the real length of a magnitude (class c, word w): scan to the top nonzero word"""
    if c == 0:
        return 1 if w else 0
    mem, off = pools.ptr(c, w)
    return norm(mem, off, 1 << c)


def part_ptr(pools, c, w):
    """(mem, off, n) for reading a magnitude"""
    if c == 0:
        return [w], 0, (1 if w else 0)
    mem, off = pools.ptr(c, w)
    return mem, off, norm(mem, off, 1 << c)


def part_value(pools, c, w):
    mem, off, n = part_ptr(pools, c, w)
    v = 0
    for i in range(n - 1, -1, -1):
        v = (v << W) | mem[off + i]
    return v


def part_canonical(pools, c, w):
    """None if (c, w) is canonical: the smallest class that holds it, zero as class 0 word 0"""
    n = part_len(pools, c, w)
    if c != cls(n):
        return f"class {c} holds {n} words (smallest {cls(n)})"
    return None


def part_release(pools, c, w):
    if c:
        pools.p[c].release(w)


def part_from_int(pools, v):
    """(class, word) for magnitude v: test setup, outside the operations"""
    ws = []
    while v:
        ws.append(v & M)
        v >>= W
    n = len(ws)
    c = cls(n)
    if c == 0:
        return 0, (ws[0] if ws else 0)
    i = pools.p[c].alloc()
    mem, off = pools.ptr(c, i)
    for k in range(1 << c):
        mem[off + k] = ws[k] if k < n else 0
    return c, i


# ------------------------------------------------------------------------------------------------ result slots
class Res:
    """a result destination: class from the bound (a one-word local for class 0, else a slot allocated now)"""
    __slots__ = ("c", "i", "local", "bound")

    def __init__(self, pools, bound):
        self.bound = bound
        self.c = cls(bound)
        self.i = None
        self.local = None
        if self.c == 0:
            self.local = [POISON]
        else:
            self.i = pools.p[self.c].alloc()

    def dst(self, pools):
        """a pointer to it, taken after all allocations"""
        if self.c == 0:
            return Dst(self.local, 0, 1, self.bound)
        mem, off = pools.ptr(self.c, self.i)
        return Dst(mem, off, 1 << self.c, self.bound)

    def finish(self, pools, n):
        """the result of length n: stays, or moves down to the smallest class holding it (the slot released).
        Returns (class, word)."""
        t = cls(n)
        if self.c == 0:
            return 0, (self.local[0] if n == 1 else 0)
        if t == self.c:
            return self.c, self.i
        if t == 0:
            mem, off = pools.ptr(self.c, self.i)
            w = mem[off] if n == 1 else 0
            pools.p[self.c].release(self.i)
            return 0, w
        j = pools.p[t].alloc()
        src, so = pools.ptr(self.c, self.i)
        dst, do = pools.ptr(t, j)
        for k in range(n):
            dst[do + k] = src[so + k]
        pools.p[self.c].release(self.i)
        return t, j
