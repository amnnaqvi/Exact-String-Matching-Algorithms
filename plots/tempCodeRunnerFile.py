"""
analyse.py — reads results/results.csv and produces the three figures
required for checkpoint 2.

Run from the project root:
    python plots/analyse.py

Outputs:
    plots/fig1_comparisons_vs_length.png
    plots/fig2_comparisons_vs_rarity.png
    plots/fig3_runtime_vs_length.png

Requires: pip install pandas matplotlib
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

CSV_PATH  = "results/results.csv"
PLOT_DIR  = "plots"
os.makedirs(PLOT_DIR, exist_ok=True)

# ---- Load data ----
df = pd.read_csv(CSV_PATH, encoding='latin-1')

# Consistent colour + marker per algorithm so all three figures match
STYLE = {
    "BMH":  {"color": "#4C72B0", "marker": "o", "linestyle": "-"},
    "FBAS": {"color": "#DD8452", "marker": "s", "linestyle": "--"},
    "HC":   {"color": "#55A868", "marker": "^", "linestyle": "-."},
}

algorithms = df["algorithm"].unique()
corpora    = df["corpus"].unique()


def clipped_yerr(means, stds):
    """
    Return asymmetric error bars clipped so the lower bound never goes
    below zero.  Character comparisons and runtimes are non-negative.

    Returns (err_low, err_high) arrays suitable for the yerr parameter
    when passed as a (2, N) array.
    """
    means  = np.asarray(means, dtype=float)
    stds   = np.asarray(stds,  dtype=float)
    err_lo = np.minimum(stds, means)   # never pull bar below zero
    err_hi = stds
    return np.array([err_lo, err_hi])


# ================================================================
#  Figure 1 — character comparisons vs pattern length
#
#  NOTE ON ERROR BARS: character comparisons are deterministic for a
#  fixed (text, pattern) pair.  The spread shown here is the std dev
#  *across the 20 sampled patterns* in each length bucket, not
#  measurement noise across repeated runs.
# ================================================================
fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4),
                         sharey=False)
if len(corpora) == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df[df["corpus"] == corpus]
    for algo in algorithms:
        a = sub[sub["algorithm"] == algo]
        grouped = a.groupby("length")["comparisons"]
        means   = grouped.mean()
        stds    = grouped.std().fillna(0)
        yerr    = clipped_yerr(means.values, stds.values)
        s = STYLE.get(algo, {})
        ax.errorbar(means.index, means.values, yerr=yerr,
                    label=algo, capsize=4,
                    color=s.get("color"), marker=s.get("marker"),
                    linestyle=s.get("linestyle"), linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Avg character comparisons")
    ax.set_ylim(bottom=0)
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle(
    "Character comparisons vs pattern length\n"
    "(error bars = std dev across 20 sampled patterns)",
    fontsize=12, y=1.03
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig1_comparisons_vs_length.png", dpi=150, bbox_inches="tight")
print("Saved fig1_comparisons_vs_length.png")


# ================================================================
#  Figure 2 — character comparisons vs rarity bucket (m=8 only)
#
#  NOTE ON ERROR BARS: same as Fig 1 — spread is across sampled
#  patterns within each rarity bucket, not across runs.
# ================================================================
df8 = df[df["length"] == 8]
rarity_order = ["common", "medium", "rare"]

fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4),
                         sharey=False)
if len(corpora) == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df8[df8["corpus"] == corpus]
    x   = np.arange(len(rarity_order))
    bar_width = 0.8 / max(len(algorithms), 1)

    for idx, algo in enumerate(algorithms):
        a = sub[sub["algorithm"] == algo]
        means = []
        stds  = []
        for bucket in rarity_order:
            vals = a[a["rarity_bucket"] == bucket]["comparisons"]
            means.append(vals.mean() if len(vals) > 0 else 0)
            stds.append(vals.std()   if len(vals) > 1 else 0)

        means = np.array(means)
        stds  = np.array(stds)
        yerr  = clipped_yerr(means, stds)
        offset = (idx - len(algorithms) / 2) * bar_width + bar_width / 2
        s = STYLE.get(algo, {})
        ax.bar(x + offset, means, bar_width, yerr=yerr,
               label=algo, color=s.get("color"), capsize=4, alpha=0.85,
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
    "(error bars = std dev across sampled patterns per bucket)",
    fontsize=12, y=1.03
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig2_comparisons_vs_rarity.png", dpi=150, bbox_inches="tight")
print("Saved fig2_comparisons_vs_rarity.png")


# ================================================================
#  Figure 3 — runtime (ms) vs pattern length
#
#  NOTE ON ERROR BARS: runtime IS measured across 5 repeated runs per
#  (pattern, corpus) pair, so these bars are genuine timing std devs,
#  averaged across the 20 sampled patterns.
# ================================================================
fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4),
                         sharey=False)
if len(corpora) == 1:
    axes = [axes]

for ax, corpus in zip(axes, corpora):
    sub = df[df["corpus"] == corpus]
    for algo in algorithms:
        a = sub[sub["algorithm"] == algo]
        grouped = a.groupby("length")["runtime_ms"]
        means   = grouped.mean()
        stds    = grouped.std().fillna(0)
        yerr    = clipped_yerr(means.values, stds.values)
        s = STYLE.get(algo, {})
        ax.errorbar(means.index, means.values, yerr=yerr,
                    label=algo, capsize=4,
                    color=s.get("color"), marker=s.get("marker"),
                    linestyle=s.get("linestyle"), linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Avg runtime (ms)")
    ax.set_ylim(bottom=0)
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle(
    "Search runtime vs pattern length\n"
    "(mean ± std dev across 5 timed runs, averaged over 20 patterns)",
    fontsize=12, y=1.03
)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig3_runtime_vs_length.png", dpi=150, bbox_inches="tight")
print("Saved fig3_runtime_vs_length.png")


# ================================================================
#  Console summary table — paste into the report (Table 1)
# ================================================================
print("\n--- Runtime summary table (mean ± std, ms) ---")
summary = (df.groupby(["algorithm", "corpus", "length"])["runtime_ms"]
             .agg(mean="mean", std="std")
             .round(4)
             .reset_index())
print(summary.to_string(index=False))