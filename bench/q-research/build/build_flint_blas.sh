#!/bin/sh
# Builds FLINT 3.0.1 with BLAS for section G, against OpenBLAS from the scipy-openblas32 wheel. The wheel's symbols
# carry a scipy_ prefix, so a small shim library provides cblas_dgemm, cblas_sgemm and openblas_set_num_threads.
set -e
R=${1:-/home/claude/flintblas}
mkdir -p "$R" && cd "$R"
git clone -q --depth 1 --branch v3.0.1 https://github.com/flintlib/flint.git src
pip download scipy-openblas32==0.3.34.237.2 --no-deps -d wh -q
(cd wh && python3 -m zipfile -e scipy_openblas32-*.whl x)
S="$R/shim"; O="$R/wh/x/scipy_openblas32"
mkdir -p "$S/include" "$S/lib"
cp "$O"/include/*.h "$S/include/"
sed -i 's/scipy_cblas_/cblas_/g; s/scipy_openblas_/openblas_/g' "$S/include/cblas.h"
cp "$O/lib/libscipy_openblas.so" "$S/lib/"
cat > "$S/shim.c" <<'C'
void scipy_cblas_dgemm(int, int, int, int, int, int, double, const double *, int, const double *, int, double, double *, int);
void scipy_cblas_sgemm(int, int, int, int, int, int, float, const float *, int, const float *, int, float, float *, int);
void scipy_openblas_set_num_threads(int);
void cblas_dgemm(int o, int a, int b, int m, int n, int k, double al, const double *A, int la, const double *B, int lb, double be, double *C, int lc) { scipy_cblas_dgemm(o, a, b, m, n, k, al, A, la, B, lb, be, C, lc); }
void cblas_sgemm(int o, int a, int b, int m, int n, int k, float al, const float *A, int la, const float *B, int lb, float be, float *C, int lc) { scipy_cblas_sgemm(o, a, b, m, n, k, al, A, la, B, lb, be, C, lc); }
void openblas_set_num_threads(int t) { scipy_openblas_set_num_threads(t); }
C
gcc -O2 -shared -fPIC -o "$S/lib/libopenblas.so" "$S/shim.c" -L"$S/lib" -lscipy_openblas -Wl,-rpath,"$S/lib"
cd src && ./bootstrap.sh
./configure --prefix="$R/inst" --with-blas-include="$S/include" --with-blas-lib="$S/lib" --disable-static \
    LDFLAGS="-Wl,-rpath,$S/lib"
grep -q "define FLINT_USES_BLAS 1" src/flint-config.h
make -j2 && make install
