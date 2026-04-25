# Makefile for MinGW-W64 on Windows
# Run from the project root: 
# make (builds benchmark)
# make test  (builds and runs tests)
# make clean (removes binaries)

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Windows uses .exe
BENCH_BIN = benchmark.exe
TEST_BIN  = test_correctness.exe
REPL_BIN  = replicate_fbas_paper.exe

# ---- Build the benchmark executable ----
$(BENCH_BIN): src/main.cpp \
              src/algorithms/Matcher.h \
              src/algorithms/BMHMatcher.h \
              src/algorithms/FBASMatcher.h \
              src/algorithms/HCMatcher.h \
              src/benchmark/BenchmarkRunner.h \
              src/utils/CorpusLoader.h \
              src/utils/PatternSampler.h \
              src/utils/FrequencyTables.h \
              src/utils/MetricsRecorder.h
	$(CXX) $(CXXFLAGS) src/main.cpp -o $(BENCH_BIN)
	@echo "Built $(BENCH_BIN)"

replicate: $(REPL_BIN)
	./$(REPL_BIN)

$(REPL_BIN): tools/replicate_fbas_paper.cpp \
             src/algorithms/Matcher.h \
             src/algorithms/BMHMatcher.h \
             src/algorithms/FBASMatcher.h \
             src/benchmark/FirstOccurrenceRunner.h \
             src/utils/CorpusLoader.h \
             src/utils/FrequencyTables.h
	$(CXX) $(CXXFLAGS) -Isrc tools/replicate_fbas_paper.cpp -o $(REPL_BIN)
	@echo "Built $(REPL_BIN)"

# ---- Build and run correctness tests ----
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/test_correctness.cpp \
             src/algorithms/Matcher.h \
             src/algorithms/BMHMatcher.h \
             src/algorithms/FBASMatcher.h \
             src/utils/FrequencyTables.h
	$(CXX) $(CXXFLAGS) -Isrc tests/test_correctness.cpp -o $(TEST_BIN)
	@echo "Built $(TEST_BIN)"

# ---- Clean ----
clean:
	del /Q $(BENCH_BIN) $(TEST_BIN) $(REPL_BIN) 2>nul || true

.PHONY: test replicate clean
