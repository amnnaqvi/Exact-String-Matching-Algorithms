ADA Project — Exact String Matching Comparison
Group 47: Amn Naqvi (sn08776) & Hamayel Mashkoor (hm08937)

Algorithms: BMH (Horspool 1980), FBAS (Garraoui 2025), HC (Palmer et al. 2024)


Project structure
-----------------
src/
  algorithms/
    Matcher.h           — abstract base class (interface all algorithms share)
    BMHMatcher.h        — Boyer-Moore-Horspool implementation  [Person A - DONE]
    FBASMatcher.h       — FBAS stub + partner TODO             [Person B - TODO]
  benchmark/
    BenchmarkRunner.h   — runs N_RUNS per config, writes CSV   [Person A - DONE]
  utils/
    CorpusLoader.h      — loads text files                     [Person A - DONE]
    PatternSampler.h    — samples patterns, assigns rarity     [Person A - DONE]
    FrequencyTables.h   — English + Italian freq tables        [Person A - DONE]
    MetricsRecorder.h   — appends rows to results CSV          [Person A - DONE]
  main.cpp              — benchmark entry point                [Person A - DONE]

tests/
  test_correctness.cpp  — 13 correctness tests for BMH + FBAS [Person A - DONE]

plots/
  analyse.py            — reads CSV, produces 3 figures        [Person B - runs this]

data/
  README.txt            — corpus download instructions
  dante/                — place divina_commedia.txt here
  pizza/                — place english.txt here
  gutenberg/            — optional (final checkpoint)

results/
  results.csv           — auto-generated when benchmark runs


Quick start
-----------
1. Download corpora (see data/README.txt).

2. Build and test:
     make test
     OR: 
     g++ -std=c++17 -O2 -Wall -Wextra -Isrc tests/test_correctness.cpp -o test_correctness.exe
     .\test_correctness.exe

3. Run benchmark:
     make
     benchmark.exe
     OR:
     g++ -std=c++17 -O2 -Wall -Wextra src/main.cpp -o benchmark.exe
    .\benchmark.exe


4. Generate plots (after benchmark):
     python plots/analyse.py

5. Plots appear in plots/  — use them in the report and slides.


Build requirements
------------------
- g++ (MinGW-W64) with C++17 support  — g++ --version to confirm
- Python 3 with pandas and matplotlib — pip install pandas matplotlib


References
----------
- Horspool (1980) — BMH algorithm (Algorithm SBM in the paper)
- Garraoui (2025) — FBAS algorithm (arXiv:2601.03271)
- Palmer et al. (2024) — HC algorithm (SEA 2024, LIPIcs vol. 301)
