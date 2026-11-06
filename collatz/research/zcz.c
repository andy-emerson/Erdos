/* zcz.c: integer Collatz-type rules at level k (counterpart of gcz for Z).
 *
 * Odd x -> S(x) = (m x + d(x)) / 2^v, 2^v the full power of 2, with d(x) odd,
 * d(x) = -m x mod 2^k, taken from one of two digit sets:
 *   sym : the odd d with |d| < 2^(k-1)  (the minimal symmetric set, d(-x) = -d(x))
 *   pos : the odd d with 0 < d < 2^k    (the "mx+1" style set; k = 1, m = 3 is 3x+1)
 *
 * Modes:
 *   zcz census sym|pos k m B [cap]
 *       Follow every odd start (x in [1,B] for sym, by symmetry; |x| <= B for pos)
 *       to its cycle, with memo over [-B,B] and Brent's method for orbits that stay
 *       out of range.  Cycles are keyed by min |element| (sym, i.e. up to sign) or
 *       by min element (pos).  Prints one line:
 *       k m set B ncyc capped escaped maxlog2 nbig | key:oddlen:basin ...
 *       capped = starts whose orbit hit the step cap without a cycle found
 *       escaped = starts whose orbit passed |x| > 2^2000 (apparent divergence);
 *       excursions above 2^100 are followed exactly in multi-word arithmetic (nbig counts them).
 *   zcz drop sym k m B
 *       Check that every odd x in [3,B] reaches |y| < x (sym only; with S(1) = 1 and
 *       S(-x) = -S(x) this shows every odd x with |x| <= B reaches +-1).
 *       Prints: k m DROP B ok|FAIL first_failure maxsteps
 * All arithmetic is exact (128-bit, with a multi-word fallback above 2^100; escape at 2^2000).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef __int128 ll;
#define ESC (((ll)1) << 100)
static int K, SYM; static ll M, MASK, HALF;

static inline ll S(ll x) {
  ll r = (-(M * x)) & MASK;           /* in [0, 2^k) */
  if (SYM && r > HALF) r -= (MASK + 1);
  ll y = M * x + r;
  uint64_t lo = (uint64_t)y;
  while (lo == 0) { y >>= 64; lo = (uint64_t)y; }
  int t = __builtin_ctzll(lo);
  return y >> t;                      /* arithmetic shift, exact since 2^t | y */
}
static inline ll ab(ll x) { return x < 0 ? -x : x; }
static double lg2(ll x) { return log2((double)ab(x) + 0.0); }

/* Multi-word fallback for excursions above 2^100: sign and magnitude in 64-bit limbs.
 * big_excursion iterates S exactly from x (|x| > ESC) until |x| <= ESC again (returns 0 and
 * the new x), or |x| > 2^BIGBITS (returns -2, escape), or *steps > cap (returns -1). */
#define NL 40
#define BIGBITS 2000
static int big_excursion(ll *xp, long *steps, long cap, double *mx) {
  ll x0 = *xp; int sg = x0 < 0 ? -1 : 1; ll a0 = ab(x0);
  uint64_t w[NL]; int n = 2; memset(w, 0, sizeof w);
  w[0] = (uint64_t)a0; w[1] = (uint64_t)(a0 >> 64);
  for (;;) {
    /* x mod 2^k, then the digit r */
    ll xm = (ll)(w[0] & (uint64_t)MASK); if (sg < 0) xm = (-xm) & MASK;
    ll r = (-(M * xm)) & MASK; if (SYM && r > HALF) r -= (MASK + 1);
    /* magnitude: M |x|, then +r (sg > 0) or -r (sg < 0) */
    unsigned __int128 c = 0;
    for (int i = 0; i < n; i++) { c += (unsigned __int128)w[i] * (uint64_t)M; w[i] = (uint64_t)c; c >>= 64; }
    if (c) { if (n >= NL) return -2; w[n++] = (uint64_t)c; }
    ll add = sg > 0 ? r : -r;
    if (add >= 0) { uint64_t cc = (uint64_t)add; for (int i = 0; i < n && cc; i++) { uint64_t o = w[i]; w[i] += cc; cc = w[i] < o; } if (cc) w[n++] = cc; }
    else { uint64_t bb = (uint64_t)(-add); for (int i = 0; i < n && bb; i++) { uint64_t o = w[i]; w[i] -= bb; bb = w[i] > o; } }
    /* divide out 2s */
    int z = 0; while (w[z] == 0) z++;
    int t = __builtin_ctzll(w[z]);
    if (z) { memmove(w, w + z, (n - z) * sizeof(uint64_t)); memset(w + n - z, 0, z * sizeof(uint64_t)); n -= z; }
    if (t) { for (int i = 0; i < n; i++) w[i] = (w[i] >> t) | (i + 1 < n ? w[i + 1] << (64 - t) : 0); }
    while (n > 1 && w[n - 1] == 0) n--;
    (*steps)++;
    int bits = 64 * (n - 1) + (64 - __builtin_clzll(w[n - 1]));
    if (bits > *mx) *mx = bits;
    if (bits > BIGBITS) return -2;
    if (bits <= 100) { ll v = ((ll)w[1] << 64) | w[0]; *xp = sg * v; return 0; }
    if (*steps > cap) return -1;
  }
}

#define MAXC 200000
static ll ckey[MAXC]; static long clen[MAXC], cbasin[MAXC]; static int nc = 0;

static int reg_cycle(ll x0) {         /* x0 lies on a cycle */
  ll key = SYM ? ab(x0) : x0, x = x0; long len = 0;
  do { ll a = SYM ? ab(x) : x; if (a < key) key = a; x = S(x); len++; } while (x != x0);
  for (int i = 0; i < nc; i++) if (ckey[i] == key) return i;
  if (nc >= MAXC) { fprintf(stderr, "too many cycles\n"); exit(1); }
  ckey[nc] = key; clen[nc] = len; cbasin[nc] = 0; return nc++;
}

/* Brent from x; returns cycle id, -1 cap, -2 escape */
static int brent(ll x, long cap, double *mx) {
  ll tort = x, hare = S(x); long power = 1, lam = 1, steps = 0;
  while (tort != hare) {
    if (ab(hare) > ESC) {           /* excursion: follow it exactly, then restart Brent */
      int st = big_excursion(&hare, &steps, cap, mx); if (st) return st;
      tort = hare; hare = S(hare); power = 1; lam = 1; continue;
    }
    double l = lg2(hare); if (l > *mx) *mx = l;
    if (power == lam) { tort = hare; power *= 2; lam = 0; }
    hare = S(hare); lam++;
    if (++steps > cap) return -1;
  }
  return reg_cycle(hare);
}

int main(int argc, char **argv) {
  if (argc < 6) { fprintf(stderr, "usage: zcz census|drop sym|pos k m B [cap]\n"); return 1; }
  int drop = !strcmp(argv[1], "drop"); SYM = !strcmp(argv[2], "sym");
  K = atoi(argv[3]); M = atoll(argv[4]); long B = atol(argv[5]);
  long cap = argc > 6 ? atol(argv[6]) : 100000;
  MASK = (((ll)1) << K) - 1; HALF = ((ll)1) << (K - 1);
  if (drop) {
    long worst = 0, bigx = 0; double dmx = 0;
    for (long s = 3; s <= B; s += 2) {
      ll y = s; long n = 0;
      while (ab(y) >= s) {
        y = S(y); n++;
        if (ab(y) > ESC) { int st = big_excursion(&y, &n, cap, &dmx); if (st) { printf("%d %lld DROP %ld FAIL %ld %ld status %d\n", K, (long long)M, B, s, n, st); return 0; } bigx++; continue; }
        if (n > cap) { printf("%d %lld DROP %ld FAIL %ld %ld\n", K, (long long)M, B, s, n); return 0; }
      }
      if (n > worst) worst = n;
    }
    printf("%d %lld DROP %ld ok - %ld excursions_above_2^100 %ld peak_bits %.0f\n", K, (long long)M, B, worst, bigx, dmx); return 0;
  }
  /* memo over [-B, B]: 0 unset, >0 cycle id + 1, <0 -(tag) in progress, INT32_MIN capped, INT32_MIN+1 escaped */
  int32_t *memo = calloc(2 * B + 1, sizeof(int32_t));
  ll *path = malloc(sizeof(ll) * (cap + 2));
  long capped = 0, escaped = 0, nbig = 0; double mx = 0;
  int32_t tag = 0;
  for (long s0 = (SYM ? 1 : -B); s0 <= B; s0++) {
    if (!(s0 & 1)) continue;
    if (memo[s0 + B] != 0) { int v = memo[s0 + B]; if (v > 0) cbasin[v - 1]++; else if (v == INT32_MIN) capped++; else if (v == INT32_MIN + 1) escaped++; continue; }
    tag++; long np = 0; ll x = s0; int id = -3; long steps = 0;
    for (;;) {
      if (ab(x) <= B) {
        int32_t v = memo[(long)x + B];
        if (v > 0) { id = v - 1; break; }
        if (v == INT32_MIN) { id = -1; break; }
        if (v == INT32_MIN + 1) { id = -2; break; }
        if (v == -tag) { id = reg_cycle(x); break; }
        memo[(long)x + B] = -tag; path[np++] = x;
      }
      if (ab(x) > ESC) { int st = big_excursion(&x, &steps, cap, &mx); if (st) { id = st; break; } nbig++; continue; }
      double l = lg2(x); if (l > mx) mx = l;
      x = S(x);
      if (++steps > cap) { id = brent(x, 10 * cap, &mx); break; }
    }
    int32_t mark = id >= 0 ? id + 1 : (id == -2 ? INT32_MIN + 1 : INT32_MIN);
    for (long i = 0; i < np; i++) memo[(long)path[i] + B] = mark;
    if (id >= 0) cbasin[id]++; else if (id == -2) { if (escaped < 5 && getenv("ZCZ_SHOWESC")) fprintf(stderr, "escape %ld\n", s0); escaped++; } else capped++;
  }
  printf("%d %lld %s %ld %d %ld %ld %.1f %ld |", K, (long long)M, SYM ? "sym" : "pos", B, nc, capped, escaped, mx, nbig);
  for (int i = 0; i < nc; i++) {
    long long kk = (long long)ckey[i];
    printf(" %lld:%ld:%ld", kk, clen[i], cbasin[i]);
  }
  printf("\n"); return 0;
}
