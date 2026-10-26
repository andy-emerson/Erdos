"""Puts the Q v0.3 benchmark build (and through it, the v0.1 build) on the import path."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
for p in (HERE.parent.parent / "q-bench" / "build", HERE.parent.parent / "exact-bench" / "build"):
    if str(p) not in sys.path:
        sys.path.insert(0, str(p))
