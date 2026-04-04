FBAS Implementation — Partner Handoff
======================================

You are implementing FBASMatcher (src/algorithms/FBASMatcher.h).
Everything else is already built and working. Read this before touching any code.


What FBAS is (Garraoui 2025)
-----------------------------
FBAS is BMH with two small changes:

  1. EXTRA PREPROCESSING STEP
     After building the shift table (inherited from BMHMatcher),
     find the "anchor" — the character in the pattern with the
     lowest frequency in the language (using the freq table passed
     to the constructor). Store its index as anchor_pos_.

  2. DIFFERENT SEARCH BEHAVIOUR
     At each window alignment, check the anchor position FIRST
     instead of always checking right-to-left.
       - If text[anchor_offset] != pattern[anchor_pos_]:   1 comparison, shift and move on.
       - If text[anchor_offset] == pattern[anchor_pos_]:   verify the rest of the pattern.

That's it. The shift table is identical to BMH. You are not
changing the shift logic at all.


What you must implement
-----------------------
Open src/algorithms/FBASMatcher.h and complete:

  [ ] preprocess() — add anchor selection after calling BMHMatcher::preprocess().
      Iterate over the pattern, look up each char in freq_table_,
      and set anchor_pos_ to the index of the minimum-frequency character.
      If a character is not in the table, treat its frequency as 1.0.

  [ ] search(text) — replace the placeholder block with anchor-first checking.
      Note: the signature is search(const std::string& text) only — no pattern arg.
      Access the pattern via pattern_ (stored by preprocess()).
      The anchor aligns at:  anchor_text_pos = (i - m + 1) + anchor_pos_
      Check that character first. If it mismatches: 1 comparison, shift, continue.
      If it matches: verify the full pattern and count every comparison.


What NOT to change
------------------
  - Do not touch the shift table logic (shift_table_ is inherited from BMHMatcher).
  - Do not touch MetricsRecorder, BenchmarkRunner, or CorpusLoader.
  - Do not change the CSV schema.
  - Do not change N_RUNS in BenchmarkRunner.h.
  - Keep using metrics_.comparisons++ for every character comparison.


Frequency tables
----------------
The tables are in src/utils/FrequencyTables.h.
  - FrequencyTables::italian()  — use with the Dante corpus
  - FrequencyTables::english()  — use with Pizza & Chilli / Gutenberg

The correct table is passed into FBASMatcher at construction in main.cpp.
You don't need to worry about which table to use — that's already wired up.


Correctness check
-----------------
Run the test suite before anything else:

    make test

The test fbas_matches_bmh_positions checks that FBAS finds the same
match positions as BMH. It must keep passing after your changes.
If positions differ, there is a bug in your window indexing.

The comparison count for FBAS should be <= BMH for rare-anchor patterns.
If it is consistently higher, your anchor-first logic has an issue.


Building
--------
From the project root in Windows terminal / PowerShell:

    make          — builds benchmark.exe
    make test     — builds and runs test_correctness.exe
    make clean    — removes .exe files

Requires g++ (MinGW-W64) in your PATH.


Pattern selection and plotting
------------------------------
Once FBAS is working, your tasks are:

  1. Download the corpora (see data/README.txt).
  2. Run the benchmark:  benchmark.exe
  3. Run the analysis:   python plots/analyse.py
  4. The three figures will appear in plots/.
  5. Use those figures directly in the report and slides.

See plots/analyse.py — the rarity bucketing and all plot logic is already there.
