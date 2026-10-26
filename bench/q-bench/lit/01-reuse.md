# Reusing earlier work

Infrastructure that does not depend on the number format is imported from the v0.1 build: the timing harness (interleaved rounds, median and spread), the data generators, the best-approximation routine (used only to measure rounding), the 64×64→128-bit multiply for Numba, the exact solver chain, and the reference pipelines. This file only puts that build on the import path.

```python {file=build/reuse.py}
import sys
from pathlib import Path

V01_BUILD = Path(__file__).resolve().parents[2] / "exact-bench" / "build"
if not (V01_BUILD / "core.py").exists():
    raise SystemExit(f"the v0.1 build is needed at {V01_BUILD}; run its tangle.py first")
if str(V01_BUILD) not in sys.path:
    sys.path.append(str(V01_BUILD))
```
