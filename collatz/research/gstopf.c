/* gstopf.c: faster drop-below-start check for a unit-equivariant Gaussian map given as a gcz
 * rule file (same starts, same criterion and same output line as gstop.c).
 *
 * The rule file must have the shape of every commuting map in this directory: each odd class
 * r has DE = 1 (the step z -> AL_r z + BE_r) and each even class has AL = 1, BE = 0, DE = 1+i
 * (the step z -> z/(1+i)).  The program checks this and refuses other files.
 *
 * Acceleration: after an odd step the raw map T divides by pi = 1+i as long as the value is
 * divisible, so one loop iteration does the odd step and strips the whole power pi^v at once:
 * with m = min(v2(x), v2(y)), x + yi = 2^m (x' + y'i), and pi divides x' + y'i once more iff x', y'
 * are both odd; z / pi^(2m) = (-i)^m (x + yi) / 2^m, and z / pi = ((x + y) + (y - x) i) / 2.
 * The norm only decreases along the divisions, so the end of the run is the least norm of the
 * run and "some iterate of T has smaller norm than the start" is detected exactly when gstop
 * detects it.  Steps are counted in raw T steps up to the first raw step of smaller norm, as in
 * gstop: if the run ends at norm N_e < N(z_0) after v divisions, the first smaller norm comes k
 * divisions before the end, k the largest with N_e 2^k < N(z_0).
 *
 * Arithmetic: int64 while both coordinates are below 2^56 in absolute value (then |AL z + BE| <
 * 2^62 for |AL| entries <= 32, |BE| <= 2^20), with exact norms in __int128; above that, __int128
 * with escape at 2^110, where the norm exceeds 2^112 > any start norm, so no comparison is needed.
 * A start counts as dropping only on an exact comparison N(z_t) < N(z_0); a return to an associate
 * of the start with equal norm is reported as "cycle-min" (as in gstop); cap or escape is FAIL.
 *
 * With the fifth argument "all" the starts are all odd z with B1 < N(z) <= B2 in all four
 * quadrants and a cycle is a return to z itself, as in gstopall.c (for maps that need not commute
 * with the units; soundness catalog V2).
 *
 * usage: gstopf rulefile B1 B2 [cap=10^7] [all]     (cap in raw T steps)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
typedef __int128 L; typedef long long ll;
static ll AL[256][2], BE[256][2]; static int ODD[256]; static int MOD = 4;
#define LIM (((ll)1) << 56)
#define ESC (((L)1) << 110)

static inline int ctz64(unsigned long long x){ return x ? __builtin_ctzll(x) : 64; }
static inline int ctz128(unsigned __int128 x){
  unsigned long long lo = (unsigned long long)x, hi = (unsigned long long)(x >> 64);
  return lo ? __builtin_ctzll(lo) : (hi ? 64 + __builtin_ctzll(hi) : 128); }

/* divide x + yi (nonzero, divisible by pi) by its full power of pi; return v */
static inline int strip64(ll *px, ll *py){
  ll x = *px, y = *py; int m = ctz64((unsigned long long)x); int my = ctz64((unsigned long long)y);
  if (my < m) m = my;
  x >>= m; y >>= m;                       /* exact: divides by 2^m */
  switch (m & 3) {                        /* multiply by (-i)^m */
    case 1: { ll t = x; x = y; y = -t; } break;
    case 2: x = -x; y = -y; break;
    case 3: { ll t = x; x = -y; y = t; } break; }
  int v = 2 * m;
  if ((x & 1) && (y & 1)) { ll u = (x + y) >> 1, w = (y - x) >> 1; x = u; y = w; v++; }
  *px = x; *py = y; return v; }

static inline int strip128(L *px, L *py){
  L x = *px, y = *py; int m = ctz128((unsigned __int128)x); int my = ctz128((unsigned __int128)y);
  if (my < m) m = my;
  x >>= m; y >>= m;
  switch (m & 3) {
    case 1: { L t = x; x = y; y = -t; } break;
    case 2: x = -x; y = -y; break;
    case 3: { L t = x; x = -y; y = t; } break; }
  int v = 2 * m;
  if ((x & 1) && (y & 1)) { L u = (x + y) >> 1, w = (y - x) >> 1; x = u; y = w; v++; }
  *px = x; *py = y; return v; }

int main(int argc, char **argv){
  if (argc < 4) { fprintf(stderr, "usage: gstopf rulefile B1 B2 [cap]\n"); return 1; }
  FILE *f = fopen(argv[1], "r"); if (!f) { perror("rule"); return 1; }
  if (fscanf(f, "MOD %d", &MOD) != 1) MOD = 4;
  for (int k = 0; k < MOD * MOD; k++) { int ra, rb; ll a1, a2, b1, b2, d1, d2;
    if (fscanf(f, "%d %d %lld %lld %lld %lld %lld %lld", &ra, &rb, &a1, &a2, &b1, &b2, &d1, &d2) != 8) return 1;
    int r = ra * MOD + rb; int odd = (ra + rb) & 1;
    if (odd) { if (d1 != 1 || d2 != 0) { fprintf(stderr, "odd class with DE != 1\n"); return 2; }
      if (llabs(a1) > 32 || llabs(a2) > 32 || llabs(b1) > (1 << 20) || llabs(b2) > (1 << 20)) { fprintf(stderr, "entries too large\n"); return 2; } }
    else if (!(a1 == 1 && a2 == 0 && b1 == 0 && b2 == 0 && d1 == 1 && d2 == 1)) { fprintf(stderr, "even class is not z/(1+i)\n"); return 2; }
    ODD[r] = odd; AL[r][0] = a1; AL[r][1] = a2; BE[r][0] = b1; BE[r][1] = b2; }
  fclose(f);
  ll B1 = atoll(argv[2]), B2 = atoll(argv[3]); ll cap = argc > 4 ? atoll(argv[4]) : 10000000LL;
  int allq = argc > 5 && argv[5][0] == 'a'; int nq = allq ? 4 : 1;
  ll amax = (ll)floor(sqrt((double)B2)); while (amax * amax > B2) amax--; while ((amax + 1) * (amax + 1) <= B2) amax++;
  ll n = 0, fails = 0, cycles = 0, maxs = 0, msa = 0, msb = 0; double maxe = 0; ll mea = 0, meb = 0; double sums = 0;
  const int M1 = MOD - 1;
  clock_t t0 = clock();
  for (ll a = 1; a <= amax; a++) {
    ll b2max = B2 - a * a; ll bmax = (ll)floor(sqrt((double)b2max)); while (bmax * bmax > b2max) bmax--; while ((bmax + 1) * (bmax + 1) <= b2max) bmax++;
    ll bmin = 0; ll lo = B1 - a * a; if (lo >= 0) { bmin = (ll)floor(sqrt((double)lo)); while (bmin * bmin <= lo) bmin++; while (bmin > 0 && (bmin - 1) * (bmin - 1) > lo) bmin--; }
    for (ll b = bmin; b <= bmax; b++) {
      if (((a + b) & 1) == 0) continue;
      for (int q = 0; q < nq; q++) {
      ll a0 = q == 0 ? a : q == 1 ? -b : q == 2 ? -a : b, b0 = q == 0 ? b : q == 1 ? a : q == 2 ? -b : -a;
      n++;
      L n0 = (L)a * a + (L)b * b; ll x = a0, y = b0; ll s = 0; double mx = 0; int ok = 0;
      while (s < cap) {
        /* x + yi is odd here, |x|, |y| < LIM */
        int r = (int)((x & M1) * MOD + (y & M1));
        ll u = AL[r][0] * x - AL[r][1] * y + BE[r][0], w = AL[r][0] * y + AL[r][1] * x + BE[r][1];
        double du = (double)u * u + (double)w * w; if (du > mx) mx = du;
        int v = strip64(&u, &w); s += 1 + v;
        x = u; y = w;
        if (x < LIM && x > -LIM && y < LIM && y > -LIM) {
          L nn = (L)x * x + (L)y * y;
          if (nn < n0) { int k = 0; while ((nn << (k + 1)) < n0) k++; s -= k; ok = 1; break; }
          if (nn == n0 && (allq ? (x == a0 && y == b0) : ((x == a && y == b) || (x == -b && y == a) || (x == -a && y == -b) || (x == b && y == -a)))) { ok = 2; break; }
          continue; }
        /* wide path */
        L X = x, Y = y; int esc = 0;
        while (s < cap) {
          int r2 = (int)((X & M1) * MOD + (Y & M1));
          L U = (L)AL[r2][0] * X - (L)AL[r2][1] * Y + BE[r2][0], W = (L)AL[r2][0] * Y + (L)AL[r2][1] * X + BE[r2][1];
          if (U > ESC || U < -ESC || W > ESC || W < -ESC) { esc = 1; break; }
          double dU = (double)U * (double)U + (double)W * (double)W; if (dU > mx) mx = dU;
          v = strip128(&U, &W); s += 1 + v;
          X = U; Y = W;
          if (X < LIM && X > -LIM && Y < LIM && Y > -LIM) break; }
        if (esc) break;
        if (!(X < LIM && X > -LIM && Y < LIM && Y > -LIM)) break;   /* cap reached in wide path */
        x = (ll)X; y = (ll)Y;
        { L nn = (L)x * x + (L)y * y;
          if (nn < n0) { int k = 0; while ((nn << (k + 1)) < n0) k++; s -= k; ok = 1; break; }
          if (nn == n0 && (allq ? (x == a0 && y == b0) : ((x == a && y == b) || (x == -b && y == a) || (x == -a && y == -b) || (x == b && y == -a)))) { ok = 2; break; } }
      }
      if (ok == 2) { cycles++; printf("cycle-min %lld %lld steps %lld\n", a0, b0, s); continue; }
      if (!ok) { fails++; printf("FAIL %lld %lld steps %lld\n", a0, b0, s); fflush(stdout); continue; }
      sums += s; if (s > maxs) { maxs = s; msa = a0; msb = b0; }
      double e = log2(mx / (double)n0); if (e > maxe) { maxe = e; mea = a0; meb = b0; }
      }
    }
  }
  printf("range (%lld,%lld] starts %lld fails %lld cycles %lld maxstop %lld at %lld %lld meanstop %.2f maxexc_log2 %.2f at %lld %lld cpu %.1f\n",
    B1, B2, n, fails, cycles, maxs, msa, msb, sums / (double)(n > 0 ? n : 1), maxe, mea, meb, (double)(clock() - t0) / CLOCKS_PER_SEC);
  return 0; }
