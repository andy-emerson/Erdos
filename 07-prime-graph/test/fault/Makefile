# test/fault: the fault-injection test of spec v0.3 section 10 (see fault_bench.py).
#   make            the trial drivers build/drv_{N,O1,O2,O3,J} and overhead benches build/ovh_{N,O1,O2,O3}
#   make test       python3 test_fault.py (injectors and classification)
#   make clean
CC    ?= gcc
CFLAGS = -std=c11 -D_GNU_SOURCE -Wall -Wextra -Wshadow -Wno-unused-function -Wno-format-truncation -O3 -march=native -g -Iinclude
SRC    = src/mem.c src/factor.c src/graph.c src/value.c src/array.c src/audit.c
HDR    = include/primegraph.h src/pg_internal.h src/kernels.h src/pgfault.h src/fault_impl.h
VARS   = N O1 O2 O3
POL_N = 0
POL_O1 = 1
POL_O2 = 2
POL_O3 = 3
POL_J = 4

all: $(foreach v,$(VARS),build/drv_$(v) build/ovh_$(v)) build/drv_J

build/drv_%: driver.c $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) -DPGF_POLICY=$(POL_$*) driver.c $(SRC) -o $@ -lpthread

build/ovh_%: ovh.c $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) -DPGF_POLICY=$(POL_$*) ovh.c $(SRC) -o $@ -lpthread

test: all
	python3 test_fault.py

clean:
	rm -rf build

.PHONY: all test clean
