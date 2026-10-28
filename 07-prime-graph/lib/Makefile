# primegraph M1.  Targets:
#   make            the static library build/libprimegraph.a
#   make test       unit tests and a short differential run against Python's fractions (release build)
#   make check      test + the same under the debug build (an audit after every operation) and ASan/UBSan (gcc)
#   make debug      build/debug/: library and tests with -DPG_AUDIT_EVERY_OP -O1 -g
#   make asan       build/asan/:  library and tests with -fsanitize=address,undefined (gcc)
#   make fuzz       the differential driver in a loop (FUZZ_SECONDS, default 600; FUZZ_BUILD=debug|asan|release)
#   make bench      build/bench/pg_bench and a short comparison with round three's R0 and D1 binaries
#   make clean

CC      ?= cc
BENCHCC ?= clang
STD      = -std=c11 -D_GNU_SOURCE
WARN     = -Wall -Wextra -Wshadow -Wno-unused-function
OPT      = -O3 -march=native
BOPT     = -O3 -march=native $(if $(findstring clang,$(BENCHCC)),-Wno-invalid-feature-combination,)
PY      ?= python3
SAN      = -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
FUZZ_SECONDS ?= 600
FUZZ_BUILD   ?= debug

SRC  = src/mem.c src/factor.c src/graph.c src/value.c src/array.c src/audit.c
HDR  = include/primegraph.h src/pg_internal.h src/kernels.h

.PHONY: all lib test check debug asan fuzz bench clean

all: lib
lib: build/libprimegraph.a

# ---------------------------------------------------------------- one build directory per configuration
define CONFIG
$(1)/obj/%.o: src/%.c $(HDR)
	@mkdir -p $(1)/obj
	$(2) $(STD) $(WARN) $(3) -Iinclude -c $$< -o $$@
$(1)/libprimegraph.a: $(patsubst src/%.c,$(1)/obj/%.o,$(SRC))
	ar rcs $$@ $$^
$(1)/unit: tests/unit.c $(1)/libprimegraph.a
	$(2) $(STD) $(WARN) $(3) -Iinclude $$< $(1)/libprimegraph.a -o $$@ -lpthread
$(1)/diff: tests/diff.c $(1)/libprimegraph.a
	$(2) $(STD) $(WARN) $(3) -Iinclude $$< $(1)/libprimegraph.a -o $$@ -lpthread
endef

$(eval $(call CONFIG,build,$(CC),$(OPT)))
$(eval $(call CONFIG,build/debug,$(CC),-O1 -g -DPG_AUDIT_EVERY_OP))
$(eval $(call CONFIG,build/asan,gcc,$(SAN)))
$(eval $(call CONFIG,build/bench,$(BENCHCC),$(BOPT)))

debug: build/debug/unit build/debug/diff
asan: build/asan/unit build/asan/diff

test: build/unit build/diff
	./build/unit
	$(PY) tests/diff.py --bin build/diff --rounds 40

check: test debug asan
	./build/debug/unit
	$(PY) tests/diff.py --bin build/debug/diff --rounds 20 --audit
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 ./build/asan/unit
	ASAN_OPTIONS=detect_leaks=1 $(PY) tests/diff.py --bin build/asan/diff --rounds 20 --audit

fuzz: build/unit build/diff debug asan
	$(PY) tests/diff.py --bin build/$(if $(filter release,$(FUZZ_BUILD)),,$(FUZZ_BUILD)/)diff --fuzz $(FUZZ_SECONDS) \
		$(if $(filter release,$(FUZZ_BUILD)),,--audit)

build/bench/pg_bench: bench/pg_bench.c build/bench/libprimegraph.a
	$(BENCHCC) -std=c2x -D_GNU_SOURCE $(WARN) $(BOPT) -Iinclude $< build/bench/libprimegraph.a -o $@ -lpthread

bench: build/bench/pg_bench
	$(PY) bench/compare.py --bin build/bench/pg_bench

clean:
	rm -rf build
