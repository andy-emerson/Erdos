# 09-coprime-base/lib: Design 2 (the coprime base)

The C source of Design 2 (the coprime base), as measured in the [partial-sharing baseline](../../partial-sharing-baseline/) and unchanged since: `src/d2.c`, its function-pointer row `src/rows_d2.c`, its header `include/sk_d2.h`, and its unit tests (`tests/unit.c`, with the test hooks in `tests/introspect_d2.h`). It is built against the baseline's shared core, [`shared/partial-sharing-baseline/lib`](../../partial-sharing-baseline/lib/): the integer module, the lookup, the control and the ceiling, with the common headers and the shared test helpers (`tests/t_common.h`, `tests/ref.h`, `tests/diff.c`).

```
make            # libd2.a (core + Design 2) for widths 64 and 32, timing, counting and test builds
make test       # unit tests, and the differential test of the control and Design 2 against the exact reference;
                # the three builds must print the same checksum
make sanitize   # the same tests under ASan + UBSan
```

The API, build settings, decisions and change log are documented for every row in the baseline: [README](../../partial-sharing-baseline/lib/README.md), [DECISIONS](../../partial-sharing-baseline/lib/DECISIONS.md), [CHANGELOG](../../partial-sharing-baseline/lib/CHANGELOG.md). The specification is [v0.1](../v0.1.md).
