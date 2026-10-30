# layer 1 (spec 10-interning v0.1).  Targets:
#   make              build/{timing,counting,test}/liblayer1.a: L1 (l1_*), L1-opt (l1o_*), L1-Q32 (l1q_*), L1-opt-Q32
#                     (l1oq_*), the lookup gate's build (l1g_*), the parameterized lookup of L1-Q32 and of the gate
#                     (src/plk.c, its own object: plkq.o, plkg.o), and the 08 module and control (vendor08/, copied
#                     unchanged from 08-skeletons, now shared/partial-sharing-baseline, at commit 08bbfbe)
#   make test         unit and differential tests at both widths, and the Q32 tests (test build)
#   make sanitize     the same under AddressSanitizer + UBSan
#   make check        test + sanitize, and the sha256 of every vendor08 file against 08's
#   make clean
#
# One compiler, one set of flags (spec §4.6 item 2); the builds differ only in -D settings (spec §4.6 item 3).
# L1-opt is the same source compiled with -DL1_OPT (spec §17 item 2); the Q32 rows add -DL1_Q32 (spec §18.2).
# L1-Q32's lookup, like L1-Q64's (sk_lookup in vendor08/common.c), is a separate object called from l1.c, never
# inlined into it: src/plk.c with -DL1_Q32 (plkq.o), and with -DL1_LKGATE for the gate's build (plkg.o); D-25.
# The unit and differential tests are compiled twice: as written (the Q64 rows) and with -DT_Q32 (the same tests
# driving the Q32 rows).  tests/q32.c checks the Q32 rows against the Q64 rows, capacity, and the lookup gate.

CC    = gcc
CSTD  = -std=c11
WARN  = -Wall -Wextra -Werror
OPT   = -O2
SAN   = -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
INC   = -Iinclude -Ivendor08

V_timing   =
V_counting = -DSK_COUNT
V_test     = -DSK_COUNT -DSK_AUDIT

HDR  = include/l1.h vendor08/sk_internal.h vendor08/sk_common.h vendor08/sk_ctl.h
PHDR = src/plk.h
THDR = tests/t_common.h

.PHONY: all test sanitize check hashes clean
all: build/timing/liblayer1.a build/counting/liblayer1.a build/test/liblayer1.a

# $(1) dir, $(2) flags
define CONFIG
$(1)/obj/l1.o: src/l1.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/l1o.o: src/l1.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_OPT $(INC) -c $$< -o $$@
$(1)/obj/l1q.o: src/l1.c $(HDR) $(PHDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_Q32 $(INC) -c $$< -o $$@
$(1)/obj/plkq.o: src/plk.c $(HDR) $(PHDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_Q32 $(INC) -c $$< -o $$@
$(1)/obj/plkg.o: src/plk.c $(HDR) $(PHDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_LKGATE $(INC) -c $$< -o $$@
$(1)/obj/l1oq.o: src/l1.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_OPT -DL1_Q32 $(INC) -c $$< -o $$@
$(1)/obj/l1g.o: src/l1.c $(HDR) $(PHDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_LKGATE $(INC) -c $$< -o $$@
$(1)/obj/common.o: vendor08/common.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/ctl.o: vendor08/ctl.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/liblayer1.a: $(1)/obj/l1.o $(1)/obj/l1o.o $(1)/obj/l1q.o $(1)/obj/l1oq.o $(1)/obj/l1g.o $(1)/obj/plkq.o \
		$(1)/obj/plkg.o $(1)/obj/common.o $(1)/obj/ctl.o
	ar rcs $$@ $$^
$(1)/unit: tests/unit.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
$(1)/unitq: tests/unit.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) -DT_Q32 $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
$(1)/diff: tests/diff.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
$(1)/diffq: tests/diff.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) -DT_Q32 $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
$(1)/q32: tests/q32.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
endef

$(eval $(call CONFIG,build/timing,$(OPT) $(V_timing)))
$(eval $(call CONFIG,build/counting,$(OPT) $(V_counting)))
$(eval $(call CONFIG,build/test,$(OPT) $(V_test)))
$(eval $(call CONFIG,build/asan,$(SAN) $(V_test)))

DIFF_SEEDS ?= 200

test: build/test/unit build/test/unitq build/test/diff build/test/diffq build/test/q32
	./build/test/unit
	./build/test/unitq
	./build/test/diff --seeds $(DIFF_SEEDS)
	./build/test/diffq --seeds $(DIFF_SEEDS)
	./build/test/q32

sanitize: build/asan/unit build/asan/unitq build/asan/diff build/asan/diffq build/asan/q32
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/unit
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/unitq
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/diff --seeds 40
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/diffq --seeds 40
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/q32

# spec §4.3 item 4, rule item 2: the 08 files are unchanged (sha256 as at commit 08bbfbe of 08-skeletons, now shared/partial-sharing-baseline)
hashes:
	cd vendor08 && sha256sum -c SHA256SUMS

check: hashes test sanitize

clean:
	rm -rf build

# ======================================================================================================== round 2
# Spec v0.2 (Q32 and TC, the two textbook baselines).  Targets:
#   make r2            build/r2/{timing,counting,test,profiling}/libr2.a: Q32 (src/q32.c, q32_*), its lookup (src/plk.c
#                      with -DL1_Q32, unchanged), TC (src/tc.c, tc_*), the profiling clock (src/r2prof.c) and the 08
#                      module (vendor08/common.c, unchanged); four builds differing only in -D settings (spec §6 item 4)
#   make r2-test       unit tests and the differential test of both rows (test build)
#   make r2-sanitize   the same under AddressSanitizer + UBSan
#   make r2-check      hashes + r2-test + r2-sanitize
# The v0.1 targets above are retired and unchanged (spec §16); round 2 builds none of their objects.

V_profiling = -DSK_PROFILE
R2HDR = include/r2.h include/q32.h include/tc.h src/r2int.h src/plk.h vendor08/sk_internal.h vendor08/sk_common.h
R2THDR = tests/r2t.h

.PHONY: r2 r2-test r2-sanitize r2-check
r2: build/r2/timing/libr2.a build/r2/counting/libr2.a build/r2/test/libr2.a build/r2/profiling/libr2.a

# $(1) dir, $(2) flags
define R2CONFIG
$(1)/obj/q32.o: src/q32.c $(R2HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/plk.o: src/plk.c $(R2HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -DL1_Q32 $(INC) -c $$< -o $$@
$(1)/obj/tc.o: src/tc.c $(R2HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/r2prof.o: src/r2prof.c $(R2HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/common.o: vendor08/common.c $(R2HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/libr2.a: $(1)/obj/q32.o $(1)/obj/plk.o $(1)/obj/tc.o $(1)/obj/r2prof.o $(1)/obj/common.o
	ar rcs $$@ $$^
$(1)/r2_unit: tests/r2_unit.c $(R2THDR) $(1)/libr2.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/libr2.a -o $$@
$(1)/r2_diff: tests/r2_diff.c $(R2THDR) $(1)/libr2.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/libr2.a -o $$@
endef

$(eval $(call R2CONFIG,build/r2/timing,$(OPT) $(V_timing)))
$(eval $(call R2CONFIG,build/r2/counting,$(OPT) $(V_counting)))
$(eval $(call R2CONFIG,build/r2/test,$(OPT) $(V_test)))
$(eval $(call R2CONFIG,build/r2/profiling,$(OPT) $(V_profiling)))
$(eval $(call R2CONFIG,build/r2/asan,$(SAN) $(V_test)))

R2_SEEDS ?= 300

r2-test: build/r2/test/r2_unit build/r2/test/r2_diff
	./build/r2/test/r2_unit
	./build/r2/test/r2_diff --seeds $(R2_SEEDS)

r2-sanitize: build/r2/asan/r2_unit build/r2/asan/r2_diff
	ASAN_OPTIONS=detect_leaks=1 ./build/r2/asan/r2_unit
	ASAN_OPTIONS=detect_leaks=1 ./build/r2/asan/r2_diff --seeds 60

r2-check: hashes r2-test r2-sanitize

# ======================================================================================================== round 3
# Spec v0.3 (TC, Q32-index, Q32-hash: two-structure Q32).  Targets:
#   make r3            build/r3/{timing,counting,test,profiling}/libr3.a: Q32-index (src/q32i.c, q32i_*), Q32-hash
#                      (src/q32h.c, q32h_*), TC (src/tc3.c, tc_*: round 2's tc.c with the integer cases of the
#                      amendment of Oct 29, 2026, v0.3 §17; tc.c itself is round 2's, unchanged and not built here),
#                      the profiling clock (src/r2prof.c, unchanged) and the 08 module (vendor08/common.c, unchanged);
#                      four builds differing only in -D settings (v0.3 §6 item 3).  Round 2's q32.c and plk.c are
#                      retired and not built (v0.3 §14).
#   make r3-test       unit tests and the differential test of the three rows (test build), and the allocation check
#                      (timing, counting and profiling builds: each row allocates only its named structures)
#   make r3-sanitize   the unit and differential tests under AddressSanitizer + UBSan
#   make r3-check      hashes + r3-test + r3-sanitize
# The round-1 and round-2 targets above are unchanged.

R3HDR = include/r2.h include/r3.h include/q32i.h include/q32h.h include/tc.h src/r2int.h vendor08/sk_internal.h \
	vendor08/sk_common.h
R3THDR = tests/r3t.h
WRAPALLOC = -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

.PHONY: r3 r3-test r3-sanitize r3-check
r3: build/r3/timing/libr3.a build/r3/counting/libr3.a build/r3/test/libr3.a build/r3/profiling/libr3.a

# $(1) dir, $(2) flags
define R3CONFIG
$(1)/obj/q32i.o: src/q32i.c $(R3HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/q32h.o: src/q32h.c $(R3HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/tc.o: src/tc3.c $(R3HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/r2prof.o: src/r2prof.c $(R3HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/common.o: vendor08/common.c $(R3HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/libr3.a: $(1)/obj/q32i.o $(1)/obj/q32h.o $(1)/obj/tc.o $(1)/obj/r2prof.o $(1)/obj/common.o
	ar rcs $$@ $$^
$(1)/r3_unit: tests/r3_unit.c $(R3THDR) $(1)/libr3.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/libr3.a -o $$@
$(1)/r3_diff: tests/r3_diff.c $(R3THDR) $(1)/libr3.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/libr3.a -o $$@
$(1)/r3_alloc: tests/r3_alloc.c $(1)/libr3.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) $$< $(1)/libr3.a $(WRAPALLOC) -o $$@
endef

$(eval $(call R3CONFIG,build/r3/timing,$(OPT) $(V_timing)))
$(eval $(call R3CONFIG,build/r3/counting,$(OPT) $(V_counting)))
$(eval $(call R3CONFIG,build/r3/test,$(OPT) $(V_test)))
$(eval $(call R3CONFIG,build/r3/profiling,$(OPT) $(V_profiling)))
$(eval $(call R3CONFIG,build/r3/asan,$(SAN) $(V_test)))

R3_SEEDS ?= 300

r3-test: build/r3/test/r3_unit build/r3/test/r3_diff build/r3/timing/r3_alloc build/r3/counting/r3_alloc \
		build/r3/profiling/r3_alloc
	./build/r3/test/r3_unit
	./build/r3/test/r3_diff --seeds $(R3_SEEDS)
	./build/r3/timing/r3_alloc
	./build/r3/counting/r3_alloc
	./build/r3/profiling/r3_alloc

r3-sanitize: build/r3/asan/r3_unit build/r3/asan/r3_diff
	ASAN_OPTIONS=detect_leaks=1 ./build/r3/asan/r3_unit
	ASAN_OPTIONS=detect_leaks=1 ./build/r3/asan/r3_diff --seeds 60

r3-check: hashes r3-test r3-sanitize
