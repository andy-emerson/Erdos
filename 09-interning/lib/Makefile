# layer 1 (spec 09-interning v0.1).  Targets:
#   make              build/{timing,counting,test}/liblayer1.a: L1 (l1_*), L1-opt (l1o_*), and the 08 module and
#                     control (vendor08/, copied unchanged from 08-skeletons at commit 08bbfbe)
#   make test         unit and differential tests (test build)
#   make sanitize     the same under AddressSanitizer + UBSan
#   make check        test + sanitize, and the sha256 of every vendor08 file against 08's
#   make clean
#
# One compiler, one set of flags (spec §4.6 item 2); the builds differ only in -D settings (spec §4.6 item 3).
# L1-opt is the same source compiled with -DL1_OPT (spec §17 item 2).

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
$(1)/obj/common.o: vendor08/common.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/obj/ctl.o: vendor08/ctl.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/liblayer1.a: $(1)/obj/l1.o $(1)/obj/l1o.o $(1)/obj/common.o $(1)/obj/ctl.o
	ar rcs $$@ $$^
$(1)/unit: tests/unit.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
$(1)/diff: tests/diff.c $(THDR) $(1)/liblayer1.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -Itests $$< $(1)/liblayer1.a -o $$@
endef

$(eval $(call CONFIG,build/timing,$(OPT) $(V_timing)))
$(eval $(call CONFIG,build/counting,$(OPT) $(V_counting)))
$(eval $(call CONFIG,build/test,$(OPT) $(V_test)))
$(eval $(call CONFIG,build/asan,$(SAN) $(V_test)))

DIFF_SEEDS ?= 200

test: build/test/unit build/test/diff
	./build/test/unit
	./build/test/diff --seeds $(DIFF_SEEDS)

sanitize: build/asan/unit build/asan/diff
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/unit
	ASAN_OPTIONS=detect_leaks=1 ./build/asan/diff --seeds 40

# spec §4.3 item 4, rule item 2: the 08 files are unchanged (sha256 as at 08-skeletons commit 08bbfbe)
hashes:
	cd vendor08 && sha256sum -c SHA256SUMS

check: hashes test sanitize

clean:
	rm -rf build
