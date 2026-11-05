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
 *       k m set B ncyc capped escaped maxlog2 | key:oddlen:basin ...
 *       capped = starts whose orbit hit the step cap without a cycle found
 *       escaped = starts whose orbit passed |x| > 2^100 (apparent divergence).
 *   zcz drop sym k m B
 *       Check that every odd x in [3,B] reaches |y| < x (sym only; with S(1) = 1 and
 *       S(-x) = -S(x) this shows every odd x with |x| <= B reaches +-1).
 *       Prints: k m DROP B ok|FAIL first_failure maxsteps
 * All arithmetic is exact (128-bit, escape at 2^100).
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
    if (ab(hare) > ESC) return -2;
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
    long worst = 0;
    for (long s = 3; s <= B; s += 2) {
      ll y = s; long n = 0;
      while (ab(y) >= s) {
        y = S(y); n++;
        if (n > cap || ab(y) > ESC) { printf("%d %lld DROP %ld FAIL %ld %ld\n", K, (long long)M, B, s, n); return 0; }
      }
      if (n > worst) worst = n;
    }
    printf("%d %lld DROP %ld ok - %ld\n", K, (long long)M, B, worst); return 0;
  }
  /* memo over [-B, B]: 0 unset, >0 cycle id + 1, <0 -(tag) in progress, INT32_MIN capped, INT32_MIN+1 escaped */
  int32_t *memo = calloc(2 * B + 1, sizeof(int32_t));
  ll *path = malloc(sizeof(ll) * (cap + 2));
  long capped = 0, escaped = 0; double mx = 0;
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
      if (ab(x) > ESC) { id = -2; break; }
      double l = lg2(x); if (l > mx) mx = l;
      x = S(x);
      if (++steps > cap) { id = brent(x, 10 * cap, &mx); break; }
    }
    int32_t mark = id >= 0 ? id + 1 : (id == -2 ? INT32_MIN + 1 : INT32_MIN);
    for (long i = 0; i < np; i++) memo[(long)path[i] + B] = mark;
    if (id >= 0) cbasin[id]++; else if (id == -2) escaped++; else capped++;
  }
  printf("%d %lld %s %ld %d %ld %ld %.1f |", K, (long long)M, SYM ? "sym" : "pos", B, nc, capped, escaped, mx);
  for (int i = 0; i < nc; i++) {
    long long kk = (long long)ckey[i];
    printf(" %lld:%ld:%ld", kk, clen[i], cbasin[i]);
  }
  printf("\n"); return 0;
}
