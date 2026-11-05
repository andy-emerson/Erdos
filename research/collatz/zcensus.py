"""Integer census driver (research/collatz/integer-census.md).

For each level k and each odd m with 2^k < m < 2^(k+1), run `zcz census` for the digit
set given (sym: minimal symmetric, odd |d| < 2^(k-1); pos: odd 0 < d < 2^k) and write
one line per rule to logs/zcensus_<set>_k<lo>-<hi>_B<B>.txt.
Usage: python3 zcensus.py sym|pos klo khi B [cap]
"""
import subprocess, sys, os

here = os.path.dirname(os.path.abspath(__file__))
dset, klo, khi, B = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
cap = sys.argv[5] if len(sys.argv) > 5 else '100000'
out = os.path.join(here, 'logs', f'zcensus_{dset}_k{klo}-{khi}_B{B}.txt')
with open(out, 'w') as f:
    f.write('# k m set B ncycles capped escaped log2maxexcursion | key:oddlength:basin ...\n')
    for k in range(klo, khi + 1):
        for m in range(2**k + 1, 2**(k + 1), 2):
            r = subprocess.run(['nice', os.path.join(here, 'zcz'), 'census', dset, str(k), str(m), str(B), cap],
                               capture_output=True, text=True, check=True)
            f.write(r.stdout); f.flush()
print(out)
