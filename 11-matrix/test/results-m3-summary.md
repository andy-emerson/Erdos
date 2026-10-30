# Round m3 (factored entries): summary

Each cell is the row's cost **× control** (TC = 1.00, lower is better): median [min–max] across jobs.

| Measure | Hash-V-pack | Pack-primes | Pack-coprime |
| --- | --- | --- | --- |
| Build time per input matrix¹ | 1.72 [1.51–2.53] | 2.47 [1.53–10.3] | 10.9 [1.79–237] |
| add time | 2.20 [2.07–2.73] | 8.42 [3.16–14.9] | 257 [4.29–566] |
| mul time | 1.13 [1.11–1.19] | 119 [82.9–169] | 11.6 [7.49–20.5] |
| sop time | 1.13 [1.12–1.17] | 166 [67.1–295] | 16.8 [9.50–30.1] |
| pos time | 1.12 [1.11–1.18] | 199 [128–441] | 14.0 [7.19–23.0] |
| Stable bytes per stored matrix¹ | 0.29 [0.14–0.81] | 0.72 [0.16–2.34] | 0.77 [0.17–2.69] |
| Peak bytes during add | 0.86 [0.68–1.21] | 1.03 [0.71–2.23] | 1.06 [0.72–2.47] |
| Peak bytes during mul | 1.30 [1.18–1.65] | 1.55 [1.22–2.85] | 1.77 [1.41–3.29] |

Source: the `ratio` column of `results-m3-{add,mul,sop,pos}/verdicts.csv`, pairings "Hash-V-pack vs TC", "Pack-primes vs TC" and "Pack-coprime vs TC". The measures are `build`, `operation`, `stable bytes` and `peak bytes`. The time ratios are stored as T(TC)/T(row), so they are inverted here. The byte ratios are row/TC on accounted bytes and are used as stored. The four operation times come from their own runs: add and mul have 12 jobs each, pos has 12, and sop has 9. Peak during add and mul come from the add and mul runs. All 540 per-job values were checked against the tables in each `results.md`, with no mismatch.
¹ The results files report build time and stable bytes per run. Here they are pooled across all four runs (45 jobs). The per-run medians are close, for example Pack-coprime's build time is 8.5–20× and Hash-V-pack's is 1.7–1.9×.

- **Answer: no.** Factored entries do not recover speed. On all four operations, Pack-primes and Pack-coprime lose to both Hash-V-pack and TC in every job. Their build times also lose, except for 6 ties between Pack-primes and Hash-V-pack. Both factored rows do cut gcd work, to 0.6–4.3% of TC's for Pack-primes on mul, sop and pos. The cost of factoring outweighs that saving by far.
- **Surprise: the two factored rows trade places.** On add, Pack-primes is the cheaper one (8×) and Pack-coprime is 257×. On mul, sop and pos, Pack-coprime is 10–14× faster than Pack-primes (median). In every case the cost is the result's factoring, which counts as operation time. The counters show the cause, for example at n = 64: in add job 9, Pack-coprime's eager refinement makes about 4.4M pass-1 gcds against Pack-primes' 0.42M trial-division candidates; in mul job 9, Pack-primes' trial division of the product takes about 1.1G candidates against Pack-coprime's 11M refinement gcds.
- **No row went over the cap.** The rule expected Pack-primes to exceed the 10 s cap in sop at n = 64, but it ran just under (up to about 9.7 s per run). Those runs took 1 rep per launch and carry the "(c)" noise caveat. So did Pack-primes at n = 64 in mul and pos, Pack-primes at n = 32 in sop, and Pack-coprime at n = 64 in sop.
- **Memory:** both factored rows lose stable bytes at ρ = 1 (1.89–2.69× TC, severe in most jobs) and beat TC from ρ = 4 up (0.16–0.91×). Hash-V-pack's own figures match m2 (add 2.20× vs 2.16×, mul 1.13× vs 1.12×, stable 0.29× in both).
- **Effectively unmeasured:** Pack-coprime's split, rebuild and leftover paths. Across all 45 jobs there was one reconciliation split (sop job 4) with one 15-entry rebuild, and no leftover split, because the denominators, which are all {2,3,5}-smooth, arrive already separated (DECISIONS-m3, last section). Also not measured: n = 128, sop at ρ = 16, and factoring time apart from operation time.
