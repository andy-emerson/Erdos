// test.c: checks every routine in division.s against the exact quotient. Build: gcc -O2 test.c division.s -o test
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
uint64_t long_division(uint64_t a, uint64_t b);
void pair_control(uint64_t a, uint64_t a2, uint64_t b, uint64_t *q);
void pair_32(uint64_t a, uint64_t a2, uint64_t b, uint64_t *q);
void pair_reuse(uint64_t a, uint64_t a2, uint64_t b, uint64_t *q);
void pair_table(uint64_t a, uint64_t a2, uint64_t b, uint64_t *q);
void pair_table_reuse(uint64_t a, uint64_t a2, uint64_t b, uint64_t *q);
static uint64_t s = 88172645463325252ull;
static uint64_t rnd(void) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
int main(void) {
  void (*f[])(uint64_t, uint64_t, uint64_t, uint64_t *) = {pair_control, pair_32, pair_reuse, pair_table, pair_table_reuse};
  const char *name[] = {"control", "32-bit", "reuse", "table", "table and reuse"};
  long n = 0, small = 0;
  for (int it = 0; it < 3000000; it++) {
    int bits = 1 + rnd() % 64;                        // divisor size, 1 to 64 bits
    uint64_t b = rnd() >> (64 - bits);
    if (b == 0) b = 1;
    int len = 64 - __builtin_clzll(b);
    int lim = 64 - len;                               // quotient bits that keep b*q below 2^64
    if (it % 2 && len < 32) lim = 32 - len;           // half the time, everything below 2^32 (the 32-bit path)
    uint64_t q = lim ? rnd() >> (64 - lim) : 0, q2 = lim ? rnd() >> (64 - lim) : 0;
    if (it % 7 == 0) q = 0;                           // zero dividends too
    uint64_t a = b * q, a2 = b * q2, out[2];
    if (((a | a2 | b) >> 32) == 0) small++;
    if (long_division(a, b) != q) { printf("long_division fails\n"); return 1; }
    uint64_t x = rnd(), y = (rnd() >> (rnd() % 64)) | 1;  // long division also on inexact pairs
    if (long_division(x, y) != x / y) { printf("long_division fails on %lu / %lu\n", x, y); return 1; }
    for (int k = 0; k < 5; k++) {
      f[k](a, a2, b, out);
      if (out[0] != q || out[1] != q2) { printf("%s fails: %lu %lu / %lu\n", name[k], a, a2, b); return 1; }
      n += 2;
    }
  }
  printf("every routine gives the exact quotient: %ld exact divisions (%ld pairs all below 2^32); long division also on 3,000,000 inexact ones\n", n, small);
  return 0;
}
