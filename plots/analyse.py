"""
Generate final-demo plots and summary CSVs from results/results.csv.

Run from project root:
    python plots/analyse.py
"""

import os
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
import pandas as pd


CSV_PATH = Path("results/results.csv")
EXACT_CSV = Path("results/fbas_paper_exact.csv")
PLOT_DIR = Path("plots")
RESULT_DIR = Path("results")
PLOT_DIR.mkdir(exist_ok=True)
RESULT_DIR.mkdir(exist_ok=True)

HC_PAPER_TIMES = {8: 17.54, 16: 10.67, 32: 8.08, 64: 6.84, 128: 6.49}
CORPUS_ORDER = ["dante", "pizza", "gutenberg"]

STYLE = {
    "BMH": {"color": "#4C72B0", "marker": "o", "linestyle": "-"},
    "FBAS": {"color": "#DD8452", "marker": "s", "linestyle": "--"},
    "HC": {"color": "#55A868", "marker": "^", "linestyle": "-."},
    "HC_q3a11": {"color": "#55A868", "marker": "^", "linestyle": "-"},
    "HC_q3a12": {"color": "#8172B3", "marker": "v", "linestyle": "--"},
}


def style(algo):
    return STYLE.get(algo, {"color": "grey", "marker": "x", "linestyle": ":"})


def ordered_corpora(values):
    present = set(values)
    ordered = [corpus for corpus in CORPUS_ORDER if corpus in present]
    ordered.extend(sorted(present - set(ordered)))
    return ordered


def clipped_yerr(means, stds):
    means = np.asarray(means, dtype=float)
    stds = np.asarray(stds, dtype=float)
    return np.array([np.minimum(stds, means), stds])


def read_results():
    if not CSV_PATH.exists():
        raise SystemExit(f"{CSV_PATH} not found. Run benchmark.exe first.")

    df = pd.read_csv(CSV_PATH, encoding="latin-1")
    rename = {"occurrences": "matches_found", "run": "run_id"}
    df = df.rename(columns={k: v for k, v in rename.items() if k in df.columns})

    numeric = [
        "length", "comparisons", "runtime_ms", "preprocess_ms",
        "matches_found", "run_id",
    ]
    for col in numeric:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna(subset=["algorithm", "corpus", "length", "runtime_ms"])
    df = df[df["algorithm"] != "algorithm"].copy()
    df["length"] = df["length"].astype(int)
    return df


def select_hc_per_corpus_length(df):
    """Mirror the HC paper's best-variant reporting at each corpus/length."""
    variants = sorted(a for a in df["algorithm"].unique() if str(a).startswith("HC_"))
    if not variants:
        return df.copy(), pd.DataFrame()

    sweep_df = df[df["rarity_bucket"] != "paper_fixed"].copy()
    hc = sweep_df[sweep_df["algorithm"].isin(variants)]
    winners = (
        hc.groupby(["corpus", "length", "algorithm"], as_index=False)["runtime_ms"]
        .mean()
        .sort_values(["corpus", "length", "runtime_ms"])
        .groupby(["corpus", "length"], as_index=False)
        .first()
        .rename(columns={"algorithm": "selected_hc", "runtime_ms": "mean_runtime_ms"})
    )
    winners.to_csv(RESULT_DIR / "hc_variant_selection.csv", index=False)

    selected = []
    for _, row in winners.iterrows():
        keep = (
            (df["corpus"] == row["corpus"]) &
            (df["length"] == row["length"]) &
            (df["algorithm"] == row["selected_hc"])
        )
        selected.append(df[keep].assign(algorithm="HC"))

    # Paper-fixed rows are for the FBAS replication only. Keep BMH/FBAS rows,
    # but do not include HC variants there because they are not part of that
    # first-occurrence comparison.
    non_hc = df[~df["algorithm"].astype(str).str.startswith("HC_")]
    selected_df = pd.concat([non_hc] + selected, ignore_index=True)
    return selected_df, winners


def sanity_tables(raw_df):
    per_algo = (
        raw_df.groupby(["corpus", "pattern", "length", "rarity_bucket", "algorithm"])
        ["matches_found"]
        .nunique()
        .reset_index(name="distinct_match_counts")
    )
    unstable = per_algo[per_algo["distinct_match_counts"] > 1]

    match_counts = (
        raw_df.groupby(["corpus", "pattern", "length", "rarity_bucket", "algorithm"])
        ["matches_found"]
        .first()
        .reset_index()
    )
    pivot = match_counts.pivot_table(
        index=["corpus", "pattern", "length", "rarity_bucket"],
        columns="algorithm",
        values="matches_found",
        aggfunc="first",
    ).reset_index()
    algo_cols = [c for c in pivot.columns if c not in {
        "corpus", "pattern", "length", "rarity_bucket"
    }]
    pivot["all_algorithms_agree"] = pivot[algo_cols].nunique(axis=1, dropna=True) == 1
    pivot.to_csv(RESULT_DIR / "match_sanity.csv", index=False)

    if len(unstable):
        unstable.to_csv(RESULT_DIR / "match_count_unstable_runs.csv", index=False)

    return pivot


def save_runtime_summary(df):
    df = df[df["rarity_bucket"] != "paper_fixed"].copy()

    summary = (
        df.groupby(["algorithm", "corpus", "length"])["runtime_ms"]
        .agg(mean_ms="mean", std_ms="std", runs="count")
        .round(5)
        .reset_index()
    )
    summary.to_csv(RESULT_DIR / "runtime_summary.csv", index=False)

    cmp_summary = (
        df.groupby(["algorithm", "corpus", "length"])["comparisons"]
        .agg(mean="mean", std="std")
        .round(2)
        .reset_index()
    )
    cmp_summary.to_csv(RESULT_DIR / "comparison_summary.csv", index=False)


def plot_lines(df, value, ylabel, title, filename, include_error=True):
    corpora = ordered_corpora(df["corpus"].unique())
    algorithms = [a for a in ["BMH", "FBAS", "HC"] if a in set(df["algorithm"])]

    fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4.5))
    if len(corpora) == 1:
        axes = [axes]

    for ax, corpus in zip(axes, corpora):
        sub = df[df["corpus"] == corpus]
        for algo in algorithms:
            a = sub[sub["algorithm"] == algo]
            grouped = a.groupby("length")[value]
            means = grouped.mean()
            stds = grouped.std().fillna(0)
            s = style(algo)
            yerr = clipped_yerr(means.values, stds.values) if include_error else None
            ax.errorbar(
                means.index, means.values, yerr=yerr, label=algo, capsize=4,
                color=s["color"], marker=s["marker"],
                linestyle=s["linestyle"], linewidth=1.8,
            )
        ax.set_title(corpus.capitalize())
        ax.set_xlabel("Pattern length m")
        ax.set_ylabel(ylabel)
        ax.set_ylim(bottom=0)
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend()

    fig.suptitle(title, fontsize=10, y=1.03)
    fig.tight_layout()
    fig.savefig(PLOT_DIR / filename, dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_rarity(df):
    df8 = df[(df["length"] == 8) & (df["rarity_bucket"] != "paper_fixed")].copy()
    corpora = ordered_corpora(df8["corpus"].unique())
    algorithms = [a for a in ["BMH", "FBAS", "HC"] if a in set(df8["algorithm"])]
    rarity_order = ["common", "medium", "rare"]

    fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4.5))
    if len(corpora) == 1:
        axes = [axes]

    for ax, corpus in zip(axes, corpora):
        sub = df8[df8["corpus"] == corpus]
        x = np.arange(len(rarity_order))
        width = 0.8 / max(len(algorithms), 1)
        for idx, algo in enumerate(algorithms):
            vals = []
            errs = []
            a = sub[sub["algorithm"] == algo]
            for bucket in rarity_order:
                b = a[a["rarity_bucket"] == bucket]["comparisons"]
                vals.append(b.mean() if len(b) else 0)
                errs.append(b.std() if len(b) > 1 else 0)
            offset = (idx - len(algorithms) / 2) * width + width / 2
            s = style(algo)
            ax.bar(
                x + offset, vals, width, yerr=clipped_yerr(vals, errs),
                label=algo, color=s["color"], capsize=4, alpha=0.86,
                error_kw={"ecolor": "black"},
            )
        ax.set_title(corpus.capitalize())
        ax.set_xlabel("Least-frequent character bucket (m = 8)")
        ax.set_ylabel("Mean character comparisons")
        ax.set_xticks(x)
        ax.set_xticklabels(rarity_order)
        ax.set_ylim(bottom=0)
        ax.grid(True, axis="y", linestyle="--", alpha=0.4)
        ax.legend()

    fig.suptitle(
        "Fig 2 - Comparisons vs character rarity\n"
        "BMH/FBAS count scanning comparisons; HC counts verification-only comparisons.",
        fontsize=10, y=1.03,
    )
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig2_comparisons_vs_rarity.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def add_filter_efficiency(df):
    sweep = df[df["rarity_bucket"] != "paper_fixed"].copy()
    grouped = (
        sweep.groupby(["algorithm", "corpus", "pattern", "length", "rarity_bucket"])
        ["comparisons"]
        .mean()
        .reset_index()
    )
    base = grouped[grouped["algorithm"] == "BMH"][
        ["corpus", "pattern", "length", "rarity_bucket", "comparisons"]
    ].rename(columns={"comparisons": "bmh_comparisons"})
    merged = grouped.merge(
        base, on=["corpus", "pattern", "length", "rarity_bucket"], how="left"
    )
    merged["comparison_reduction_pct"] = (
        100.0 * (merged["bmh_comparisons"] - merged["comparisons"])
        / merged["bmh_comparisons"].replace(0, np.nan)
    )
    merged.to_csv(RESULT_DIR / "filter_efficiency.csv", index=False)
    return merged


def plot_filter_efficiency(eff):
    eff = eff[eff["algorithm"].isin(["FBAS", "HC"])].copy()
    corpora = ordered_corpora(eff["corpus"].unique())

    fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4.5))
    if len(corpora) == 1:
        axes = [axes]

    for ax, corpus in zip(axes, corpora):
        sub = eff[eff["corpus"] == corpus]
        for algo in ["FBAS", "HC"]:
            a = sub[sub["algorithm"] == algo]
            grouped = a.groupby("length")["comparison_reduction_pct"]
            means = grouped.mean()
            stds = grouped.std().fillna(0)
            s = style(algo)
            ax.errorbar(
                means.index, means.values,
                yerr=clipped_yerr(means.values, stds.values),
                label=algo, capsize=4, color=s["color"],
                marker=s["marker"], linestyle=s["linestyle"], linewidth=1.8,
            )
        ax.axhline(0, color="black", linewidth=0.8)
        ax.set_title(corpus.capitalize())
        ax.set_xlabel("Pattern length m")
        ax.set_ylabel("Comparison reduction vs BMH (%)")
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend()

    fig.suptitle(
        "Fig 5 - Filtering efficiency vs pattern length\n"
        "For HC this is verification-work reduction, not total hash-operation reduction.",
        fontsize=10, y=1.03,
    )
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig5_filter_efficiency_vs_length.png", dpi=150, bbox_inches="tight")
    plt.close(fig)

    df8 = eff[eff["length"] == 8]
    if df8.empty:
        return
    rarity_order = ["common", "medium", "rare"]
    fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4.5))
    if len(corpora) == 1:
        axes = [axes]
    for ax, corpus in zip(axes, corpora):
        sub = df8[df8["corpus"] == corpus]
        x = np.arange(len(rarity_order))
        width = 0.35
        for idx, algo in enumerate(["FBAS", "HC"]):
            a = sub[sub["algorithm"] == algo]
            vals = [a[a["rarity_bucket"] == b]["comparison_reduction_pct"].mean()
                    for b in rarity_order]
            offset = (idx - 0.5) * width
            s = style(algo)
            ax.bar(x + offset, vals, width, label=algo, color=s["color"], alpha=0.86)
        ax.axhline(0, color="black", linewidth=0.8)
        ax.set_title(corpus.capitalize())
        ax.set_xlabel("Rarity bucket (m = 8)")
        ax.set_ylabel("Comparison reduction vs BMH (%)")
        ax.set_xticks(x)
        ax.set_xticklabels(rarity_order)
        ax.grid(True, axis="y", linestyle="--", alpha=0.4)
        ax.legend()

    fig.suptitle("Fig 6 - Filtering efficiency vs character rarity", fontsize=10, y=1.03)
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig6_filter_efficiency_vs_rarity.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_hc_variants(raw_df):
    hc = raw_df[raw_df["algorithm"].astype(str).str.startswith("HC_")]
    if hc.empty:
        return
    corpora = ordered_corpora(hc["corpus"].unique())
    fig, axes = plt.subplots(1, len(corpora), figsize=(6 * len(corpora), 4.5))
    if len(corpora) == 1:
        axes = [axes]

    for ax, corpus in zip(axes, corpora):
        sub = hc[hc["corpus"] == corpus]
        for algo in sorted(sub["algorithm"].unique()):
            grouped = sub[sub["algorithm"] == algo].groupby("length")["runtime_ms"]
            means = grouped.mean()
            stds = grouped.std().fillna(0)
            s = style(algo)
            ax.errorbar(
                means.index, means.values,
                yerr=clipped_yerr(means.values, stds.values),
                label=algo, capsize=4, color=s["color"],
                marker=s["marker"], linestyle=s["linestyle"], linewidth=1.8,
            )
        ax.set_title(corpus.capitalize())
        ax.set_xlabel("Pattern length m")
        ax.set_ylabel("Mean runtime (ms)")
        ax.set_ylim(bottom=0)
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend()

    fig.suptitle("Fig 7 - HC parameter comparison (q=3, alpha=11 vs 12)", fontsize=10, y=1.03)
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig7_hc_variant_runtime.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_match_sanity(match_sanity):
    by_corpus = (
        match_sanity.groupby("corpus")["all_algorithms_agree"]
        .agg(total="count", agree="sum")
        .reset_index()
    )
    by_corpus["disagree"] = by_corpus["total"] - by_corpus["agree"]

    fig, ax = plt.subplots(figsize=(7, 4))
    x = np.arange(len(by_corpus))
    ax.bar(x, by_corpus["agree"], label="agree", color="#55A868")
    ax.bar(
        x, by_corpus["disagree"], bottom=by_corpus["agree"],
        label="disagree", color="#C44E52",
    )
    ax.set_xticks(x)
    ax.set_xticklabels(by_corpus["corpus"].str.capitalize())
    ax.set_ylabel("Pattern cases")
    ax.set_title("Fig 8 - Matches-found sanity check across algorithms")
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)
    ax.legend()
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig8_match_sanity.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_hc_paper(df):
    pizza = df[(df["corpus"] == "pizza") & (df["algorithm"] == "HC")]
    if pizza.empty:
        return
    means = pizza.groupby("length")["runtime_ms"].mean()
    stds = pizza.groupby("length")["runtime_ms"].std().fillna(0)

    paper_lengths = sorted(HC_PAPER_TIMES)
    paper_times = [HC_PAPER_TIMES[m] for m in paper_lengths]

    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.errorbar(
        means.index, means.values, yerr=clipped_yerr(means.values, stds.values),
        label="HC (our selected q3 alpha variant)", capsize=4,
        color="#55A868", marker="^", linestyle="-.", linewidth=1.8,
    )
    ax.plot(
        paper_lengths, paper_times,
        label="HC best variant - Palmer et al. Table 3",
        color="black", marker="D", linestyle=":", linewidth=1.5,
    )
    ax.set_xlabel("Pattern length m")
    ax.set_ylabel("Search runtime (ms), Pizza 100 MB English")
    ax.set_ylim(bottom=0)
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.legend(fontsize=9)
    ax.set_title(
        "Fig 9 - HC runtime cross-check\n"
        "Paper uses 500 runs; our CSV reports mean and std dev from local runs.",
        fontsize=10,
    )
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig9_hc_paper_runtime.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def plot_fbas_paper_exact():
    if not EXACT_CSV.exists():
        print(f"{EXACT_CSV} not found; skipping Fig 10/11.")
        return
    de = pd.read_csv(EXACT_CSV)
    labels = de["pattern"].tolist()
    x = np.arange(len(labels))

    fig, ax = plt.subplots(figsize=(max(13, len(labels) * 1.1), 5.5))
    width = 0.2
    ax.bar(x - 1.5 * width, de["our_bmh"], width, label="BMH our C++", color="#4C72B0")
    ax.bar(x - 0.5 * width, de["paper_bmh"], width, label="BMH paper", color="#4C72B0", alpha=0.35, hatch="//")
    ax.bar(x + 0.5 * width, de["our_fbas"], width, label="FBAS our C++", color="#DD8452")
    ax.bar(x + 1.5 * width, de["paper_fbas"], width, label="FBAS paper", color="#DD8452", alpha=0.35, hatch="//")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("Character comparisons (first occurrence)")
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)
    ax.legend(fontsize=9)
    ax.set_title("Fig 10 - FBAS paper replication: absolute comparison counts", fontsize=10)
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig10_fbas_exact_table2.png", dpi=150, bbox_inches="tight")
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(max(12, len(labels) * 1.1), 5))
    width = 0.35
    ax.bar(x - width / 2, de["our_improvement_pct"], width, label="Our C++", color="#55A868")
    ax.bar(x + width / 2, de["paper_improvement_pct"], width, label="Garraoui Table II", color="#55A868", alpha=0.35, hatch="//")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("FBAS improvement over BMH (%)")
    ax.grid(True, axis="y", linestyle="--", alpha=0.4)
    ax.legend(fontsize=9)
    ax.set_title("Fig 11 - FBAS paper replication: improvement percentage", fontsize=10)
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "fig11_fbas_exact_improvement.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


def main():
    raw_df = read_results()
    selected_df, winners = select_hc_per_corpus_length(raw_df)
    sweep = selected_df[selected_df["rarity_bucket"] != "paper_fixed"].copy()

    print(f"Loaded {len(raw_df)} rows from {CSV_PATH}")
    print(f"Corpora: {ordered_corpora(raw_df['corpus'].unique())}")
    if not winners.empty:
        print("HC variant selections written to results/hc_variant_selection.csv")

    save_runtime_summary(selected_df)
    match_sanity = sanity_tables(raw_df)
    disagreements = int((~match_sanity["all_algorithms_agree"]).sum())
    print(f"Match-count sanity disagreements: {disagreements}")

    plot_lines(
        sweep, "comparisons", "Mean character comparisons per search",
        "Fig 1 - Character comparisons vs pattern length\n"
        "Error bars = standard deviation across sampled patterns and timed runs.",
        "fig1_comparisons_vs_length.png",
    )
    plot_rarity(selected_df)
    plot_lines(
        sweep, "runtime_ms", "Mean search runtime (ms)",
        "Fig 3 - Search runtime vs pattern length\n"
        "Wall-clock time includes hashing, shifting, scanning, and verification.",
        "fig3_runtime_vs_length.png",
    )
    plot_lines(
        sweep, "preprocess_ms", "Preprocessing time (ms)",
        "Fig 4 - Preprocessing time vs pattern length",
        "fig4_preprocess_vs_length.png",
        include_error=False,
    )

    efficiency = add_filter_efficiency(selected_df)
    plot_filter_efficiency(efficiency)
    plot_hc_variants(raw_df)
    plot_match_sanity(match_sanity)
    plot_hc_paper(selected_df)
    plot_fbas_paper_exact()

    print("Summary CSVs written to results/.")
    print("Plots written to plots/.")
    print("Note: HC comparison counts are verification-only; use runtime for direct HC/BMH/FBAS comparison.")


if __name__ == "__main__":
    main()
