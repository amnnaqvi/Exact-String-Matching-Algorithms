"""
analyse.py — reads results/results.csv and produces four figures
for the final report comparing BMH, FBAS, and HC.

Run from the project root:
    python plots/analyse.py

Outputs:
    plots/fig1_comparisons_vs_length.png   — character comparisons vs m
    plots/fig2_comparisons_vs_rarity.png   — comparisons vs rarity (m=8)
    plots/fig3_runtime_vs_length.png       — wall-clock runtime vs m
    plots/fig4_preprocess_vs_length.png    — preprocessing time vs m

Requires: pip install pandas matplotlib numpy

================================================================
NOTE ON HC COMPARISON METRIC
================================================================
HC only counts character comparisons made during full pattern
verification (the final check after the filter passes). BMH and
FBAS count every character comparison made during window scanning.
This means HC comparisons are NOT directly comparable to BMH/FBAS
comparisons — HC's comparisons measure verification work only,
while BMH/FBAS measure total scanning work.

Fig 1 and Fig 2 show this honestly: HC comparisons will look very
low because it skips most windows without any character comparison
at all.  The runtime plots (Fig 3) are the fair head-to-head.

We include Fig 1 and Fig 2 because they still illustrate the
filtering mechanism's effectiveness, but the report should clearly
explain this counting difference.
================================================================
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import os

CSV_PATH = "results/results.csv"
PLOT_DIR = "plots"
os.makedirs(PLOT_DIR, exist_ok=True)

# ---- Load data ----
COLUMNS = [
    "algorithm",
    "corpus",
    "pattern",
    "length",
    "rarity_bucket",
    "comparisons",
    "runtime_ms",
    "preprocess_ms",
    "occurrences",
    "run",
]

df = pd.read_csv(CSV_PATH, encoding="latin-1", header=None, names=COLUMNS)

# Sanitize numeric columns (in case of any quoted whitespace)
for col in ["length", "comparisons", "runtime_ms", "preprocess_ms", "occurrences", "run"]:
    df[col] = pd.to_numeric(df[col], errors="coerce")

df = df.dropna(subset=["algorithm", "runtime_ms", "comparisons"])
print(f"Loaded {len(df)} rows from {CSV_PATH}")
# ================================================================
#  HC parameter selection:
#  We benchmarked HC_q3a11 and HC_q3a12. For fair comparison,
#  pick the configuration that was faster on average across all
#  (corpus, length) combinations — report the winner in the plots.
#  This mirrors how Palmer et al. (2024) report "best variant".
# ================================================================
hc_variants = [a for a in df["algorithm"].unique() if a.startswith("HC_")]

if hc_variants:
    # For each HC variant, compute mean runtime across everything
    hc_mean_rt = (
        df[df["algorithm"].isin(hc_variants)]
        .groupby("algorithm")["runtime_ms"]
        .mean()
    )
    best_hc = hc_mean_rt.idxmin()   # variant with lowest mean runtime
    print(f"Best HC variant by mean runtime: {best_hc}")

    # Rename the best HC variant to "HC" in the dataframe for clean plots
    df["algorithm"] = df["algorithm"].replace(best_hc, "HC")
    # Drop other HC variants
    df = df[~df["algorithm"].str.startswith("HC_")]
else:
    print("No HC variants found in CSV — plotting BMH and FBAS only.")

# ---- Final algorithm list ----
algorithms = sorted(df["algorithm"].unique())
corpora    = sorted(df["corpus"].unique())

print(f"Algorithms in data: {algorithms}")
print(f"Corpora in data:    {corpora}")

# ---- Consistent style per algorithm ----
STYLE = {
    "BMH":  {"color": "#4C72B0", "marker": "o", "linestyle": "-"},
    "FBAS": {"color": "#DD8452", "marker": "s", "linestyle": "--"},
    "HC":   {"color": "#55A868", "marker": "^", "linestyle": "-."},
}

def style(algo):
    return STYLE.get(algo, {"color": "grey", "marker": "x", "linestyle": ":"})


def clipped_yerr(means, stds):
    """
    Asymmetric error bars clipped so the lower bound never goes below zero.
    Returns (2, N) array for matplotlib's yerr parameter.
    """
    means  = np.asarray(means, dtype=float)
    stds   = np.asarray(stds,  dtype=float)
    err_lo = np.minimum(stds, means)
    err_hi = stds
    return np.array([err_lo, err_hi])


# ================================================================
#  Figure 1 — character comparisons vs pattern length
#
#  IMPORTANT: HC comparisons = verification-only (chars compared
#  after the filter passes). BMH/FBAS = all chars compared during
#  scanning. The gap is intentional and meaningful — it shows HC
#  rarely needs to fall back to verification.
#
#  Error bars = std dev across the 20 sampled patterns per bucket.
# ================================================================
n_corpora = len(corpora)
fig, axes = plt.subplots(1, n_corpora,
                          figsize=(6 * n_corpora, 4.5), sharey=False)
if n_corpora == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df[df["corpus"] == corpus]
    for algo in algorithms:
        a = sub[sub["algorithm"] == algo]
        grouped = a.groupby("length")["comparisons"]
        means   = grouped.mean()
        stds    = grouped.std().fillna(0)
        yerr    = clipped_yerr(means.values, stds.values)
        s = style(algo)
        ax.errorbar(means.index, means.values, yerr=yerr,
                    label=algo, capsize=4,
                    color=s["color"], marker=s["marker"],
                    linestyle=s["linestyle"], linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Avg character comparisons")
    ax.set_ylim(bottom=0)
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle(
    "Character comparisons vs pattern length\n"
    "(HC counts verification-only; BMH/FBAS count all scanning comparisons)\n"
    "Error bars = std dev across 20 sampled patterns",
    fontsize=11, y=1.04
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig1_comparisons_vs_length.png",
            dpi=150, bbox_inches="tight")
print("Saved fig1_comparisons_vs_length.png")
plt.close(fig)


# ================================================================
#  Figure 2 — character comparisons vs rarity bucket (m=8 only)
#
#  Rarity buckets (defined in PatternSampler.h):
#    rare   — min char freq < 0.005  (e.g. 'x', 'z', 'q')
#    medium — min char freq < 0.040  (e.g. 'b', 'v', 'k')
#    common — everything else
#
#  Error bars = std dev across sampled patterns per bucket.
# ================================================================
df8 = df[df["length"] == 8].copy()
rarity_order = ["common", "medium", "rare"]

fig, axes = plt.subplots(1, n_corpora,
                          figsize=(6 * n_corpora, 4.5), sharey=False)
if n_corpora == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df8[df8["corpus"] == corpus]
    x   = np.arange(len(rarity_order))
    bar_width = 0.8 / max(len(algorithms), 1)

    for idx, algo in enumerate(algorithms):
        a = sub[sub["algorithm"] == algo]
        means, stds = [], []
        for bucket in rarity_order:
            vals = a[a["rarity_bucket"] == bucket]["comparisons"]
            means.append(vals.mean() if len(vals) > 0 else 0)
            stds.append(vals.std()   if len(vals) > 1 else 0)

        means = np.array(means)
        stds  = np.array(stds)
        yerr  = clipped_yerr(means, stds)
        offset = (idx - len(algorithms) / 2) * bar_width + bar_width / 2
        s = style(algo)
        ax.bar(x + offset, means, bar_width, yerr=yerr,
               label=algo, color=s["color"], capsize=4, alpha=0.85,
               error_kw={"ecolor": "black"})

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern rarity (m=8)")
    ax.set_ylabel("Avg character comparisons")
    ax.set_ylim(bottom=0)
    ax.set_xticks(x)
    ax.set_xticklabels(rarity_order)
    ax.legend()
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)

fig.suptitle(
    "Comparisons vs character rarity (pattern length = 8)\n"
    "(HC counts verification-only; BMH/FBAS count all scanning comparisons)\n"
    "Error bars = std dev across sampled patterns per bucket",
    fontsize=11, y=1.04
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig2_comparisons_vs_rarity.png",
            dpi=150, bbox_inches="tight")
print("Saved fig2_comparisons_vs_rarity.png")
plt.close(fig)


# ================================================================
#  Figure 3 — search runtime (ms) vs pattern length  [FAIR COMPARISON]
#
#  This is the primary head-to-head metric. Wall-clock time includes
#  all overhead (hash lookups, filter checks, shifts, verification)
#  and is directly comparable across all three algorithms.
#
#  Error bars = std dev across 5 timed runs, averaged over 20 patterns.
# ================================================================
fig, axes = plt.subplots(1, n_corpora,
                          figsize=(6 * n_corpora, 4.5), sharey=False)
if n_corpora == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df[df["corpus"] == corpus]
    for algo in algorithms:
        a = sub[sub["algorithm"] == algo]
        grouped = a.groupby("length")["runtime_ms"]
        means   = grouped.mean()
        stds    = grouped.std().fillna(0)
        yerr    = clipped_yerr(means.values, stds.values)
        s = style(algo)
        ax.errorbar(means.index, means.values, yerr=yerr,
                    label=algo, capsize=4,
                    color=s["color"], marker=s["marker"],
                    linestyle=s["linestyle"], linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Avg runtime (ms)")
    ax.set_ylim(bottom=0)
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle(
    "Search runtime vs pattern length  [primary fair comparison]\n"
    "(mean ± std dev across 5 timed runs, averaged over 20 patterns)",
    fontsize=11, y=1.03
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig3_runtime_vs_length.png",
            dpi=150, bbox_inches="tight")
print("Saved fig3_runtime_vs_length.png")
plt.close(fig)


# ================================================================
#  Figure 4 — preprocessing time vs pattern length
#
#  BMH and FBAS have O(m) preprocessing (shift table).
#  HC has O(m*q) preprocessing (building the hash chain / filter).
#  This figure shows the cost each algorithm pays before search begins.
#
#  Preprocessing is done ONCE per pattern (not per run), so these
#  values are deterministic and have no timing spread.
# ================================================================
fig, axes = plt.subplots(1, n_corpora,
                          figsize=(6 * n_corpora, 4.5), sharey=False)
if n_corpora == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df[df["corpus"] == corpus]
    for algo in algorithms:
        a = sub[sub["algorithm"] == algo]
        # Preprocess is same across all 5 runs — just take mean
        grouped = a.groupby("length")["preprocess_ms"]
        means   = grouped.mean()
        s = style(algo)
        ax.plot(means.index, means.values,
                label=algo,
                color=s["color"], marker=s["marker"],
                linestyle=s["linestyle"], linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Preprocessing time (ms)")
    ax.set_ylim(bottom=0)
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle(
    "Preprocessing time vs pattern length\n"
    "(HC builds a hash filter: O(m·q); BMH/FBAS build a shift table: O(m))",
    fontsize=11, y=1.03
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig4_preprocess_vs_length.png",
            dpi=150, bbox_inches="tight")
print("Saved fig4_preprocess_vs_length.png")
plt.close(fig)


# ================================================================
#  Console summary table — paste into the report
# ================================================================
print("\n--- Runtime summary (mean ± std ms) ---")
summary = (
    df.groupby(["algorithm", "corpus", "length"])["runtime_ms"]
      .agg(mean="mean", std="std")
      .round(4)
      .reset_index()
)
print(summary.to_string(index=False))

print("\n--- Comparisons summary (mean across patterns) ---")
cmp_summary = (
    df.groupby(["algorithm", "corpus", "length"])["comparisons"]
      .agg(mean="mean", std="std")
      .round(1)
      .reset_index()
)
print(cmp_summary.to_string(index=False))

print("\n--- Note on comparisons metric ---")
print("HC comparisons = chars compared in verification phase only.")
print("BMH/FBAS comparisons = all chars compared during window scanning.")
print("Use runtime (Fig 3) for the primary fair performance comparison.")