Data download instructions
==========================

The corpora are not included in the repo (large files).
Place them as plain UTF-8 text files at the paths below.


1. Dante's Divina Commedia (Italian, ~550,000 chars)
   - URL: https://www.gutenberg.org/cache/epub/1000/pg1000.txt
   - Save as: data/dante/divina_commedia.txt
   - Used in: Garraoui (2025) — allows direct cross-check of FBAS results.

2. Pizza & Chilli English text (English, 100 MB)
   - URL: https://pizzachili.dcc.uchile.cl/texts/nlang/english.100MB.gz
   - Decompress and save as: data/pizza/english.txt
   - Used in: Palmer et al. (2024) — allows direct cross-check of HC results.

3. Project Gutenberg English subset (optional for checkpoint 2)
   - URL: https://zenodo.org/record/2422560 (Gerlach & Font-Clos 2020)
   - Save as: data/gutenberg/gutenberg_english.txt
   - Used in: proposal's full experimental plan (final checkpoint).


Notes
-----
- The benchmark skips any corpus whose file is missing and prints a warning.
- All files must be plain text (no HTML, no XML).
- For Dante: the Project Gutenberg file includes a header/footer.
  You can leave it in — it won't affect results meaningfully.
- For Pizza & Chilli: the .gz file is ~35 MB; decompressed ~100 MB.
  Use 7-Zip on Windows to extract it.
