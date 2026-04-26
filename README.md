# Exact String Matching Benchmark

This project compares three exact string matching algorithms:

- Boyer-Moore-Horspool (BMH)
- Frequency-Based Anchor Selection (FBAS)
- Hash Chain (HC)

The benchmark records character comparisons, search runtime, preprocessing time,
matches found, and repeated-run timing variance. The main question is practical:
how much work do these algorithms do when the same patterns are searched in the
same real text corpora?

## Structure

```text
src/
  algorithms/          Algorithm implementations
  benchmark/           Benchmark runners
  utils/               Corpus loading, sampling, metrics, frequency tables
  main.cpp             Main benchmark entry point
tests/
  test_correctness.cpp Correctness and cross-algorithm agreement tests
tools/
  replicate_fbas_paper.cpp
plots/
  analyse.py           Plot and summary generator
data/
  dante/
  pizza/
  gutenberg/
results/
  Generated benchmark CSVs and summaries
```

## Experiment Flow

```text
data/ corpora
  -> CorpusLoader normalises text
  -> PatternSampler selects reproducible patterns
  -> BenchmarkRunner runs each algorithm 30 times
  -> MetricsRecorder writes results/results.csv
  -> plots/analyse.py creates summaries and figures
```

## Algorithms

- **BMH**: Boyer-Moore-Horspool baseline using a bad-character shift table.
- **FBAS**: Extends BMH by checking the rarest character in the pattern first.
- **HC**: Hash Chain filter using q-grams and an extended Bloom-filter style
  table before full verification.

BMH is the baseline story for the project. FBAS is a small, readable change to
BMH: check the rarest pattern character first. HC is a different filtering
approach that often avoids full verification.

Important metric note: HC comparison counts include only final verification
comparisons. Hash/filter work is reflected in `runtime_ms`, so use runtime for
the fairest direct comparison across all three algorithms.

## Main Command

```powershell
mingw32-make pipeline
```

This runs the complete reproducible workflow:

- builds and runs the correctness tests,
- runs the main benchmark,
- runs the FBAS paper replication,
- regenerates the summary CSVs and plots.

## Useful Individual Commands

Use these only when you want to rerun one part of the project:

```powershell
mingw32-make test
mingw32-make benchmark.exe
.\benchmark.exe
mingw32-make replicate
python plots\analyse.py
```

## Validation Checks

- `mingw32-make test` should pass before trusting new results.
- `results/match_sanity.csv` and `plots/fig8_match_sanity.png` show agreement
  on match counts.
- `plots/fig1_*`, `fig3_*`, and `fig5_*` are the strongest result figures for
  comparisons, runtime, and filtering efficiency.

## Dependencies

- MinGW-W64 `g++` with C++17 support
- Python 3
- `pandas`, `matplotlib`, `numpy`

## References

- Horspool, R. N. (1980). Practical fast searching in strings.
- Garraoui (2025). Frequency-Based Anchor Selection.
- Palmer, Faro, and Scafiti (2024). Hash Chain.
