# 08-common-divisors/lib: Design 1 (common divisors)

The C source of Design 1 (common divisors), as measured in the [partial-sharing baseline](../../partial-sharing-baseline/) and unchanged since: `src/d1.c`, its function-pointer row `src/rows_d1.c`, its header `include/sk_d1.h`, and its unit tests (`tests/unit.c`, with the test hooks in `tests/introspect_d1.h`). It is built against the baseline's shared core, [`shared/partial-sharing-baseline/lib`](../../partial-sharing-baseline/lib/): the integer module, the lookup, the control and the ceiling, with the common headers and the shared test helpers (`tests/t_common.h`, `tests/ref.h`, `tests/diff.c`).

```
make            # libd1.a (core + Design 1) for widths 64 and 32, timing, counting and test builds
make test       # unit tests, and the differential test of the control and Design 1 against the exact reference;
                # the three builds must print the same checksum
make sanitize   # the same tests under ASan + UBSan
```

The API, build settings, decisions and change log are documented for every row in the baseline: [README](../../partial-sharing-baseline/lib/README.md), [DECISIONS](../../partial-sharing-baseline/lib/DECISIONS.md), [CHANGELOG](../../partial-sharing-baseline/lib/CHANGELOG.md). The specification is [v0.1](../v0.1.md).
