"""Puts the Q v0.3 benchmark build (and through it, the v0.1 build) on the import path."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]  # build, q-research, test, 03-q
for p in (REPO / "03-q" / "test" / "q-bench" / "build", REPO / "01-number-system" / "test" / "build"):
    if str(p) not in sys.path:
        sys.path.insert(0, str(p))
