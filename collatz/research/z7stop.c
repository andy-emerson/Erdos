/* z7stop.c: drop-below-start check for Barina's 7x +- 1 on positive integers, as fast as a simple
 * sieve allows, to estimate the cost of extending Barina's verification (cycle-bounds.md, Section 7).
 *
 * f(n) = 7n + 1 (n = 1 mod 4), 7n - 1 (n = 3 mod 4), n/2 (n even).  For every odd n in (lo, hi] we
 * check that some iterate is smaller than n; with every n <= lo known to reach 1, induction gives
 * every n <= hi reaches 1 (even n halve at once).
 *
 * Two exclusions, both exact:
 * 1. Predecessor: if n = 2 or 5 (mod 7), m = (4n -+ 1)/7 is odd with f^3(m) = n (digit +1 for
 *    n = 2, -1 for n = 5; the digit condition m = 1 resp. 3 mod 4 holds automatically) and m < n.
 *    The orbit of m passes through n and later goes below m (m is a start too, or m <= lo), and
 *    that later point lies in the orbit of n and is < m < n.  So n needs no check.
 *    (If m <= lo, m reaches 1 and so does n's orbit, which then passes below n.)
 * 2. Sieve mod 2^k: for each odd residue r mod 2^k the first steps of n = 2^k t + r are the same
 *    for all t as long as the residues needed (mod 4 before an odd step, mod 2 before a halving)
 *    are determined; then the iterate is M t + x with M = 7^a 2^(k-b).  If at some point
 *    M < 2^k and M t + x < 2^k t + r for every t >= t0 (t0 from lo), the class drops and is
 *    skipped; otherwise it is a survivor, iterated from the state M t + x (a later point of the
 *    same orbit; a drop found from there is a drop of n).
 * Survivors are iterated in __int128 (odd step, then the whole power of 2 by ctz); a start whose
 * iterate passes 2^120 is reported (FAIL big) and would need multiword arithmetic.
 *
 * usage: z7stop lo hi k [survivor-file]   (lo a multiple of 2^k; prints counts, survivors fraction,
 *        time; with a file name also writes the surviving residues, for z7test.py)
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef unsigned __int128 U; typedef __int128 L; typedef unsigned long long ull; typedef long long ll;

static inline int ctz128(U x){ ull lo = (ull)x; return lo ? __builtin_ctzll(lo) : 64 + __builtin_ctzll((ull)(x >> 64)); }

int main(int argc, char **argv){
  if (argc < 4) { fprintf(stderr, "usage: z7stop lo hi k\n"); return 1; }
  ull lo = strtoull(argv[1], 0, 10), hi = strtoull(argv[2], 0, 10); int k = atoi(argv[3]);
  ull K = 1ULL << k; if (lo % K) { fprintf(stderr, "lo must be a multiple of 2^k\n"); return 1; }
  ull t0 = lo / K, t1 = (hi + K - 1) / K;        /* t in [t0, t1) covers (lo, hi] */
  clock_t c0 = clock();
  /* sieve */
  ull *sr = malloc(sizeof(ull) * (K / 2)); U *sM = malloc(sizeof(U) * (K / 2)); L *sx = malloc(sizeof(L) * (K / 2));
  ull ns = 0;
  for (ull r = 1; r < K; r += 2) {
    U M = K; L x = r; int bits = k; int drop = 0;
    for (;;) {
      if (x & 1) { if (bits < 2) break; int c = (int)(x & 3); x = 7 * x + (c == 1 ? 1 : -1); M = 7 * M; }
      else { if (bits < 1) break; x >>= 1; M >>= 1; bits--; }
      /* the iterate is M t + x; M = 7^a 2^bits.  Drop for all t >= t0 if M < K and (K - M) t0 > x - r */
      if (M < K) { L lhs = (L)(K - M) * (L)t0; if (lhs > x - (L)r) { drop = 1; break; } }
    }
    if (!drop) { sr[ns] = r; sM[ns] = M; sx[ns] = x; ns++; }
  }
  double tsieve = (double)(clock() - c0) / CLOCKS_PER_SEC;
  if (argc > 4) { FILE *df = fopen(argv[4], "w"); for (ull i = 0; i < ns; i++) fprintf(df, "%llu\n", sr[i]); fclose(df); }
  ull odd = 0, skip7 = 0, checked = 0, big = 0; ull steps = 0, maxsteps = 0, argmax = 0;
  const U LIM = ((U)1) << 120;
  for (ull t = t0; t < t1; t++) {
    for (ull i = 0; i < ns; i++) {
      ull n0 = t * K + sr[i];
      if (n0 <= lo || n0 > hi) continue;
      unsigned m7 = (unsigned)(n0 % 7); if (m7 == 2 || m7 == 5) { skip7++; continue; }
      checked++;
      U x = sM[i] * (U)t + (U)sx[i];             /* x > 0 */
      ull s = 0;
      while (x >= n0) {
        if (!(x & 1)) { x >>= ctz128(x); if (x < n0) break; }
        x = 7 * x + (((x & 3) == 1) ? 1 : (U)-1); s++;
        if (x > LIM) { big++; printf("FAIL big %llu\n", n0); break; }
        if (s > 100000000ULL) { big++; printf("FAIL cap %llu\n", n0); break; }
      }
      steps += s; if (s > maxsteps) { maxsteps = s; argmax = n0; }
    }
  }
  odd = (hi - lo) / 2;
  double tt = (double)(clock() - c0) / CLOCKS_PER_SEC;
  printf("z7stop (%llu,%llu] k=%d survivors %llu of %llu classes (%.4f); odd numbers %llu, skipped mod 7 %llu, iterated %llu (%.4f of odd); big %llu; odd steps after sieve mean %.2f max %llu at %llu; sieve %.2f s, total %.2f s, %.3g odd numbers per s\n",
    lo, hi, k, ns, K / 2, (double)ns / (K / 2), odd, skip7, checked, (double)checked / odd, big,
    (double)steps / (checked ? checked : 1), maxsteps, argmax, tsieve, tt, odd / (tt - tsieve));
  return 0; }
