# 09-coprime-base/lib: Design 2 (the coprime base) of spec 09-coprime-base v0.1, built against the shared core of the partial-sharing
# baseline (../../shared/partial-sharing-baseline/lib: the integer module, the lookup, the control and the ceiling).
# Split out of the joint skeletons library on Oct 29, 2026; the sources are unchanged.  Targets:
#   make              every library: build/w{64,32}/{timing,counting,test}/libd2.a (core + Design 2)
#   make test         Design 2's unit tests, and the differential test of the control and Design 2 against the
#                     exact reference, test build, at widths 64 and 32; then the same differential run in the timing,
#                     counting and test builds must print the same result checksum (spec §4.6 item 4)
#   make sanitize     the same tests under AddressSanitizer + UBSan (test build, both widths)
#   make check        test + sanitize
#   make clean
#
# One compiler, one set of flags, as for every row of the baseline (spec §4.6 item 2).  The three builds differ only
# in -D settings (spec §4.6 item 3); the width is -DSK_WIDTH=32 (default 64, spec §4.3 item 1).  Design 1, in
# its own folder, is not built here.

CORE     = ../../shared/partial-sharing-baseline/lib

CC       = gcc
CSTD     = -std=c11
WARN     = -Wall -Wextra -Werror
OPT      = -O2
SAN      = -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all

V_timing   =
V_counting = -DSK_COUNT
V_test     = -DSK_COUNT -DSK_AUDIT
W_64       =
W_32       = -DSK_WIDTH=32

vpath %.c src $(CORE)/src
SRC  = common.c ctl.c ceil.c rows.c d2.c rows_d2.c
INC  = -Iinclude -I$(CORE)/include -I$(CORE)/src
HDR  = include/sk_d2.h $(wildcard $(CORE)/include/*.h) $(CORE)/src/sk_internal.h
TINC = -Itests -I$(CORE)/tests
THDR = tests/introspect_d2.h $(CORE)/tests/t_common.h $(CORE)/tests/ref.h

WIDTHS   = 64 32
VARIANTS = timing counting test
LIBS     = $(foreach w,$(WIDTHS),$(foreach v,$(VARIANTS),build/w$(w)/$(v)/libd2.a))

.PHONY: all test sanitize check clean
all: $(LIBS)

# $(1) dir, $(2) flags
define CONFIG
$(1)/obj/%.o: %.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) $(INC) -c $$< -o $$@
$(1)/libd2.a: $(patsubst %.c,$(1)/obj/%.o,$(SRC))
	ar rcs $$@ $$^
$(1)/unit: tests/unit.c $(THDR) $(1)/libd2.a
	$(CC) $(CSTD) $(WARN) $(2) $(INC) $(TINC) $$< $(1)/libd2.a -o $$@
$(1)/diff: $(CORE)/tests/diff.c $(THDR) $(1)/libd2.a
	$(CC) $(CSTD) $(WARN) $(2) -DSK_DIFF_D2 $(INC) $(TINC) $$< $(1)/libd2.a -o $$@
endef
$(foreach w,$(WIDTHS),$(foreach v,$(VARIANTS),\
  $(eval $(call CONFIG,build/w$(w)/$(v),$(OPT) $(V_$(v)) $(W_$(w))))))
$(foreach w,$(WIDTHS),$(eval $(call CONFIG,build/w$(w)/asan,$(SAN) $(V_test) $(W_$(w)))))

DIFF_ARGS ?= --seeds 40

test: $(LIBS) $(foreach w,$(WIDTHS),build/w$(w)/test/unit $(foreach v,$(VARIANTS),build/w$(w)/$(v)/diff))
	@for w in $(WIDTHS); do \
	  echo "== width $$w: unit (test build)"; ./build/w$$w/test/unit || exit 1; \
	  echo "== width $$w: differential (test build, audits on the gate schedule)"; \
	  ./build/w$$w/test/diff $(DIFF_ARGS) > build/w$$w/test/diff.out || { cat build/w$$w/test/diff.out; exit 1; }; \
	  tail -n 4 build/w$$w/test/diff.out; \
	  for v in timing counting; do \
	    ./build/w$$w/$$v/diff $(DIFF_ARGS) > build/w$$w/$$v/diff.out || { cat build/w$$w/$$v/diff.out; exit 1; }; \
	  done; \
	  a=$$(grep '^checksum' build/w$$w/timing/diff.out); b=$$(grep '^checksum' build/w$$w/counting/diff.out); \
	  c=$$(grep '^checksum' build/w$$w/test/diff.out); \
	  if [ "$$a" != "$$b" ] || [ "$$a" != "$$c" ]; then echo "builds differ: $$a / $$b / $$c"; exit 1; fi; \
	  echo "== width $$w: timing, counting and test builds agree ($$a)"; \
	done

sanitize: $(foreach w,$(WIDTHS),build/w$(w)/asan/unit build/w$(w)/asan/diff)
	@for w in $(WIDTHS); do \
	  echo "== width $$w: ASan + UBSan"; \
	  ASAN_OPTIONS=detect_leaks=1 ./build/w$$w/asan/unit || exit 1; \
	  ASAN_OPTIONS=detect_leaks=1 ./build/w$$w/asan/diff --seeds 8 > build/w$$w/asan/diff.out || \
	    { cat build/w$$w/asan/diff.out; exit 1; }; tail -n 2 build/w$$w/asan/diff.out; \
	done

check: test sanitize

clean:
	rm -rf build
