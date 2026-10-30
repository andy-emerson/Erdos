"""Runs every test_* function in the named modules, imported from the current directory.

Usage, from a harness's build directory: python3 path/to/shared/runtests.py test_module [test_module ...]
Exits nonzero if any test fails.
"""
import importlib, sys, time, traceback
sys.path.insert(0, ".")
fails = 0
for modname in sys.argv[1:]:
    m = importlib.import_module(modname)
    for n in sorted(x for x in dir(m) if x.startswith("test_")):
        t = time.time()
        try:
            getattr(m, n)(); print(f"pass {modname}.{n} {time.time()-t:.1f}s")
        except Exception:
            traceback.print_exc(); print(f"FAIL {modname}.{n}"); fails += 1
print("failures:", fails)
sys.exit(1 if fails else 0)
