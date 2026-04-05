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
# df = pd.read_csv(CSV_PATH)
df = pd.read_csv(CSV_PATH, encoding='latin-1')

# Consistent colour + marker per algorithm so all three figures match
STYLE = {
    "BMH":  {"color": "#4C72B0", "marker": "o", "linestyle": "-"},
    "FBAS": {"color": "#DD8452", "marker": "s", "linestyle": "--"},
    "HC":   {"color": "#55A868", "marker": "^", "linestyle": "-."},
}

algorithms = df["algorithm"].unique()
corpora    = df["corpus"].unique()


# ================================================================
#  Figure 1 — character comparisons vs pattern length
#  One subplot per corpus, one line per algorithm.
#  Addresses the proposal's "length sweep" experiment directly.
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
        s = STYLE.get(algo, {})
        ax.errorbar(means.index, means.values, yerr=stds.values,
                    label=algo, capsize=4,
                    color=s.get("color"), marker=s.get("marker"),
                    linestyle=s.get("linestyle"), linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Avg character comparisons")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle("Character comparisons vs pattern length", fontsize=13, y=1.02)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig1_comparisons_vs_length.png", dpi=150, bbox_inches="tight")
print("Saved fig1_comparisons_vs_length.png")


# ================================================================
#  Figure 2 — character comparisons vs rarity bucket (m=8 only)
#  This isolates the core FBAS advantage: rarer anchor = fewer
#  comparisons. Directly maps to the proposal's rarity sweep.
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
            stds.append(vals.std()  if len(vals) > 1 else 0)

        offset = (idx - len(algorithms) / 2) * bar_width + bar_width / 2
        s = STYLE.get(algo, {})
        ax.bar(x + offset, means, bar_width, yerr=stds,
               label=algo, color=s.get("color"), capsize=4, alpha=0.85)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern rarity (m=8)")
    ax.set_ylabel("Avg character comparisons")
    ax.set_xticks(x)
    ax.set_xticklabels(rarity_order)
    ax.legend()
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)

fig.suptitle("Comparisons vs character rarity (pattern length = 8)", fontsize=13, y=1.02)
fig.tight_layout()
fig.savefig(f"{PLOT_DIR}/fig2_comparisons_vs_rarity.png", dpi=150, bbox_inches="tight")
print("Saved fig2_comparisons_vs_rarity.png")


# ================================================================
#  Figure 3 — runtime (ms) vs pattern length with error bars
#  Addresses the instructor's statistical rigour comment directly.
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
        s = STYLE.get(algo, {})
        ax.errorbar(means.index, means.values, yerr=stds.values,
                    label=algo, capsize=4,
                    color=s.get("color"), marker=s.get("marker"),
                    linestyle=s.get("linestyle"), linewidth=1.8)

    ax.set_title(corpus.capitalize(), fontsize=12)
    ax.set_xlabel("Pattern length (m)")
    ax.set_ylabel("Avg runtime (ms) ± std dev")
    ax.legend()
    ax.grid(True, linestyle="--", alpha=0.4)

fig.suptitle("Search runtime vs pattern length (mean ± std dev, 5 runs)", fontsize=13, y=1.02)
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
