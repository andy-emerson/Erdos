# skeletons (spec 08-skeletons v0.1).  Targets:
#   make              every library: build/w{64,32}/{timing,counting,test}/libskeletons.a
#   make test         unit and differential tests, test build, at widths 64 and 32; then the same differential run
#                     in the timing, counting and test builds must print the same result checksum (spec §4.6 item 4)
#   make sanitize     unit and differential tests under AddressSanitizer + UBSan (test build, both widths)
#   make check        test + sanitize
#   make clean
#
# One compiler, one set of flags for every row (spec §4.6 item 2).  The three builds differ only in -D settings
# (spec §4.6 item 3); the width is -DSK_WIDTH=32 (default 64, spec §4.3 item 1).

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

SRC  = src/common.c src/ctl.c src/d1.c src/d2.c src/ceil.c src/rows.c
HDR  = include/sk_common.h include/sk_ctl.h include/sk_d1.h include/sk_d2.h include/sk_ceil.h include/skeletons.h \
       src/sk_internal.h
THDR = tests/introspect.h tests/ref.h

WIDTHS   = 64 32
VARIANTS = timing counting test
LIBS     = $(foreach w,$(WIDTHS),$(foreach v,$(VARIANTS),build/w$(w)/$(v)/libskeletons.a))

.PHONY: all test sanitize check clean
all: $(LIBS)

# $(1) dir, $(2) flags
define CONFIG
$(1)/obj/%.o: src/%.c $(HDR)
	@mkdir -p $(1)/obj
	$(CC) $(CSTD) $(WARN) $(2) -Iinclude -c $$< -o $$@
$(1)/libskeletons.a: $(patsubst src/%.c,$(1)/obj/%.o,$(SRC))
	ar rcs $$@ $$^
$(1)/unit: tests/unit.c $(THDR) $(1)/libskeletons.a
	$(CC) $(CSTD) $(WARN) $(2) -Iinclude -Itests $$< $(1)/libskeletons.a -o $$@
$(1)/diff: tests/diff.c $(THDR) $(1)/libskeletons.a
	$(CC) $(CSTD) $(WARN) $(2) -Iinclude -Itests $$< $(1)/libskeletons.a -o $$@
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
