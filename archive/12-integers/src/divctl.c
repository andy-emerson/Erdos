// divctl.c: control in C, built with clang: the language's own division, over divbench2's operands (divbench2 dump),
// timed the same way (reps of at least 50 ms, 15 reps, median per division) and checksummed the same way.
//   divctl <operand file> <set>     set 32 and 64: uint64_t a / b; set 128: unsigned __int128 a / b
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1024
typedef unsigned __int128 u128;
static uint64_t B[N], Alo[N], Ahi[N], A2lo[N], A2hi[N], R[N], R2[N];

static int64_t now(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (int64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
#define barrier() __asm__ __volatile__("" ::: "memory")

__attribute__((noinline)) static void c64(int64_t k) {
  for (int64_t it = 0; it < k; it++) {
    for (int j = 0; j < N; j++) { R[j] = Alo[j] / B[j]; R2[j] = A2lo[j] / B[j]; }
    barrier();
  }
}
__attribute__((noinline)) static void c128(int64_t k) {
  for (int64_t it = 0; it < k; it++) {
    for (int j = 0; j < N; j++) {
      R[j] = (uint64_t)((((u128)Ahi[j] << 64) | Alo[j]) / B[j]);
      R2[j] = (uint64_t)((((u128)A2hi[j] << 64) | A2lo[j]) / B[j]);
    }
    barrier();
  }
}
static int cmp(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

int main(int argc, char **argv) {
  FILE *f = fopen(argv[1], "r");
  int set = atoi(argv[2]);
  for (int j = 0; j < N; j++)
    if (fscanf(f, "%lu %lu %lu %lu %lu", &B[j], &Alo[j], &Ahi[j], &A2lo[j], &A2hi[j]) != 5) return 1;
  void (*fn)(int64_t) = set == 128 ? c128 : c64;
  int64_t k = 1;
  for (;;) { int64_t t0 = now(); fn(k); if (now() - t0 >= 50000000) break; k *= 2; }
  double ts[15];
  for (int r = 0; r < 15; r++) { int64_t t0 = now(); fn(k); ts[r] = (double)(now() - t0) / (double)(k * 2 * N); }
  qsort(ts, 15, sizeof(double), cmp);
  uint64_t h = 0;
  for (int j = 0; j < N; j++) h = (h ^ R[j] ^ (R2[j] << 1)) * 0x9E3779B97F4A7C15ull;
  printf("clang %d %.4f %.4f %016lX\n", set, ts[7], (ts[14] - ts[0]) / ts[7], h);
  return 0;
}
