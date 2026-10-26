"""Runs cbench/hensel.c and checks every result against Python's exact Fractions."""
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

BIN = Path(__file__).resolve().parent.parent / "cbench" / "hensel"


def run(method, nums, dens, K, reps=5):
    with tempfile.TemporaryDirectory() as d:
        for name, x in zip(("a", "b", "c"), nums):
            np.ascontiguousarray(x, dtype=np.int64).tofile(Path(d) / f"{name}.bin")
        for name, x in zip(("a_q", "b_q", "c_q"), dens):
            np.ascontiguousarray(x, dtype=np.int64).tofile(Path(d) / f"{name}.bin")
        out = Path(d) / "out.txt"
        r = subprocess.run([str(BIN), method, str(len(nums[0])), str(K), str(reps), d, str(out)],
                           capture_output=True, text=True, check=True)
        med, mn, falls = r.stdout.split()
        return float(med), float(mn), int(falls), [Fraction(s) for s in out.read_text().split()]


def exact(nums, dens, K):
    out = []
    for i in range(len(nums[0])):
        x = Fraction(int(nums[0][i]), int(dens[0][i]))
        b = Fraction(int(nums[1][i]), int(dens[1][i]))
        c = Fraction(int(nums[2][i]), int(dens[2][i]))
        for _ in range(K):
            x = x * b + c
        out.append(x)
    return out
