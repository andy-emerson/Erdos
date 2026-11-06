/* gstopcone.c: drop-below-start check for a fundamental-domain map at modulus (1+i)^2 on the
 * cone C = {a > 0, -a < b <= a} (fundamental-domain.md, Sections 6 and 7.3), faster than
 * fdsearch mode 1 and without its O(B) memory.
 *
 * Map: S(z) = rep_C((alpha z + d(z)) / (1+i)^v) on odd z in C, with d(z) = d1 on the class of 1
 * (a odd, b even) and d(z) = di on the class of i (a even, b odd) modulo 2, v the full power of
 * 1+i, rep_C the rotation by a power of i into C.  Starts: every odd z in C with B1 < N(z) <= B2.
 * A start passes when some iterate has smaller norm (exact comparison in __int128); it FAILs on
 * escape (a coordinate beyond 2^60, as fdsearch) or at the cap of odd steps.  Rotation does not
 * change the norm, so this is the stage-2 test of fdsearch.c (same starts, same S, same criterion).
 * Output: one line with starts, fails, the longest stopping time in odd steps and the largest
 * excursion in bits of |z| (0.5 log2 of the norm ratio after the division, as fdsearch prints).
 *
 * usage: gstopcone ar ai d1r d1i dir dii B1 B2 [cap=10^6]
 * the cone map 2+i of fundamental-domain.md 7.3: gstopcone 2 1 0 -1 -1 0 B1 B2
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
typedef __int128 L; typedef long long ll;
#define ESC (((L)1) << 60)

int main(int argc, char **argv){
  if (argc < 9) { fprintf(stderr, "usage: gstopcone ar ai d1r d1i dir dii B1 B2 [cap]\n"); return 1; }
  ll AR = atoll(argv[1]), AI = atoll(argv[2]);
  ll D1R = atoll(argv[3]), D1I = atoll(argv[4]), DIR = atoll(argv[5]), DII = atoll(argv[6]);
  ll B1 = atoll(argv[7]), B2 = atoll(argv[8]); ll cap = argc > 9 ? atoll(argv[9]) : 1000000LL;
  ll amax = (ll)floor(sqrt((double)B2)) + 1;
  ll n = 0, fails = 0, maxs = 0, msa = 0, msb = 0; double maxe = 0, sums = 0;
  clock_t t0 = clock();
  for (ll a = 1; a <= amax; a++) {
    /* b in (-a, a], B1 < a^2 + b^2 <= B2 */
    ll r2 = B2 - a * a; if (r2 < 0) break;
    ll bm = (ll)floor(sqrt((double)r2)); while (bm * bm > r2) bm--; while ((bm + 1) * (bm + 1) <= r2) bm++;
    ll bhi = bm < a ? bm : a, blo = -bm > -a + 1 ? -bm : -a + 1;
    ll l2 = B1 - a * a; ll inner = -1;             /* |b| <= inner is excluded */
    if (l2 >= 0) { inner = (ll)floor(sqrt((double)l2)); while (inner * inner > l2) inner--; while ((inner + 1) * (inner + 1) <= l2) inner++; }
    for (ll b = blo; b <= bhi; b++) {
      if (b >= -inner && b <= inner) { b = inner; continue; }
      if (((a + b) & 1) == 0) continue;
      n++;
      L x = a, y = b, n0 = (L)a * a + (L)b * b, top = n0; ll s = 0; int ok = 0;
      while (s < cap) {
        int c1 = (int)(x & 1);                     /* a odd: class of 1 */
        L u = AR * x - AI * y + (c1 ? D1R : DIR), w = AR * y + AI * x + (c1 ? D1I : DII);
        if (u == 0 && w == 0) break;
        /* strip (1+i): z/(1+i) = ((u + w) + (w - u) i)/2 */
        int m = 0; { unsigned long long lu = (unsigned long long)u, lw = (unsigned long long)w;
          unsigned long long o = lu | lw; m = o ? __builtin_ctzll(o) : 0; }
        if (m) { u >>= m; w >>= m;
          switch (m & 3) { case 1: { L t = u; u = w; w = -t; } break; case 2: u = -u; w = -w; break; case 3: { L t = u; u = -w; w = t; } break; } }
        if ((u & 1) && (w & 1)) { L p = (u + w) >> 1, q = (w - u) >> 1; u = p; w = q; }
        if (u > ESC || u < -ESC || w > ESC || w < -ESC) break;
        /* rep_C */
        for (int k = 0; k < 4; k++) { if (u > 0 && -u < w && w <= u) break; L t = u; u = -w; w = t; }
        x = u; y = w; s++;
        L nn = x * x + y * y; if (nn > top) top = nn;
        if (nn < n0) { ok = 1; break; }
      }
      if (!ok) { fails++; printf("FAIL %lld %lld odd steps %lld\n", a, b, s); fflush(stdout); continue; }
      sums += s; if (s > maxs) { maxs = s; msa = a; msb = b; }
      double e = 0.5 * log2((double)top / (double)n0); if (e > maxe) maxe = e;
    }
  }
  printf("range (%lld,%lld] starts %lld fails %lld cycles 0 maxstop %lld at %lld %lld meanstop %.2f maxexc_bits %.2f cpu %.1f\n",
    B1, B2, n, fails, maxs, msa, msb, sums / (double)(n ? n : 1), maxe, (double)(clock() - t0) / CLOCKS_PER_SEC);
  return 0; }
