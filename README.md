# Exact String Matching Benchmark

This project compares three exact string matching algorithms:

- Boyer-Moore-Horspool (BMH)
- Frequency-Based Anchor Selection (FBAS)
- Hash Chain (HC)

The benchmark records character comparisons, search runtime, preprocessing
time, matches found, and repeated-run timing variance.

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

## Build And Test

```powershell
mingw32-make test
```

Or compile directly:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/test_correctness.cpp -o test_correctness.exe
.\test_correctness.exe
```

## Run Benchmarks

```powershell
mingw32-make benchmark.exe
.\benchmark.exe
python plots\analyse.py
```

For the FBAS first-occurrence replication:

```powershell
mingw32-make replicate
```

## Dependencies

- MinGW-W64 `g++` with C++17 support
- Python 3
- `pandas`, `matplotlib`, `numpy`

## References

- Horspool, R. N. (1980). Practical fast searching in strings.
- Garraoui (2025). Frequency-Based Anchor Selection.
- Palmer, Faro, and Scafiti (2024). Hash Chain.
