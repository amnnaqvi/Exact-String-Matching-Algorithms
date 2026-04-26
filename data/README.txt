The corpora are stored as plain UTF-8 text files at these paths.

1. Dante's Divina Commedia (Italian, about 550,000 chars)
   - URL: https://www.gutenberg.org/cache/epub/1000/pg1000.txt
   - Save as: data/dante/divina_commedia.txt
   - Used for: Garraoui (2025) FBAS cross-check.

2. Pizza & Chilli English text (English, 100 MB)
   - URL: https://pizzachili.dcc.uchile.cl/texts/nlang/english.100MB.gz
   - Decompress and save as: data/pizza/english.txt
   - Used for: Palmer et al. (2024) HC runtime cross-check.

3. Project Gutenberg English sample/subset
   - Full reference corpus: https://zenodo.org/records/2422561
   - Local path: data/gutenberg/gutenberg_english.txt
   - Used for: the proposal's third-corpus requirement and robustness checks
     on a second English literary corpus.

Notes
-----
- CorpusLoader::load() preserves byte count while replacing CR/LF/NUL with
  spaces for the main all-occurrences benchmark.
- tools/replicate_fbas_paper.cpp uses CorpusLoader::load_raw() for the FBAS
  paper replication because Garraoui's Table II uses raw first-occurrence
  behavior rather than the normalized all-occurrences benchmark.
