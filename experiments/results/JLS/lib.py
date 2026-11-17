"""Helpers for the JLS results: load the TSV runs, add the metrics, build the tables and write them to Excel.

Used by `load.ipynb` (results_tsv -> results_csv) and `analysis_tables.ipynb` (results_csv -> results_xlsx).
"""
import re
from pathlib import Path
from typing import Dict, Iterable, List, Optional

import pandas as pd

HERE = Path(__file__).resolve().parent
TSV_DIR = HERE / "results_tsv"
CSV_DIR = HERE / "results_csv"
XLSX_DIR = HERE / "results_xlsx"

# columns of the TSV files (no header line; the last tab is followed by nothing)
TSV_HEADER = ["execid", "benchmark", "instance", "executable", "output", "status", "exit_code",
              "real", "time", "user", "system", "memory", "problem", "cost", "_empty"]
KEY = ["bench", "instance"]                         # an instance (bench: the short name, the same across renamings)
RUN = KEY + ["solver"]    

EXIT_OPTIMUM = 30                                   # clingo/amoclingo: optimum found (and proven)

# solvers shown first in the tables, in this order; the others follow sorted
BASELINES = ["clingo"]


def solver_order(solvers: Iterable[str]) -> List[str]:
    solvers = list(dict.fromkeys(solvers))
    return [s for s in BASELINES if s in solvers] + sorted(s for s in solvers if s not in BASELINES)

# short names of the benchmarks
BENCHMARK_NAMES = {
    "CombinatorialAuctionsAmomaximize": "CA",
    "CombinatorialAuctionsOpt": "CA",
    "KnapsackAmomaximize": "K",
    "KnapsackOpt": "K",
    "WeightedGraphColouringAmomaximize": "WGC",
    "GraphColouringOpt": "WGC",
    "TSP": "TSP",
    "TspOpt": "TSP",
    "GroupAssignmentOpt": "GA",
    "WOD": "WOD",
}

# family of an instance, by benchmark: the first group of the regex on the instance name (no match: "-")
FAMILY_REGEX = {
    "CombinatorialAuctionsAmomaximize": r"^cat_([a-z]+)_",          # paths / sched
    "WeightedGraphColouringAmomaximize": r"graph_colouring-(\d+)-",  # nodes: 125 ... 150
    "WOD": r"cp(\d_\d)\.asp$",                                       # 0_2 / 0_5 / 0_7
    "KnapsackAmomaximize": r"(type\d+)\.asp$",
}


# --------------------------------------------------------------------------------------------------------------------
# loading
# --------------------------------------------------------------------------------------------------------------------

def solver_name(executable: str) -> str:
    """amoclingo-reas=ijca-lazy=fals-prog=1-opt=su -> amo[ijca,lazy=fals,prog=1,opt=su]; clingo-* -> clingo"""
    if executable.startswith("clingo"):
        return "clingo"
    m = re.match(r"^amoclingo-(.*)$", executable)
    if not m:
        return executable
    opts = dict(kv.split("=", 1) for kv in m.group(1).split("-") if "=" in kv)
    name = "amomaximize-" + opts.get("reas", "?")
    if opts.get("lazy", "fals").startswith("t"):
        name += "-lazy"
    if opts.get("prog", "1") != "1":                # progression, default 1
        name += "-p" + opts["prog"]
    if opts.get("opt", "su") != "su":               # opt strategy, default sat-unsat
        name += "-" + opts["opt"]
    return name


def read_tsv(path: Path) -> pd.DataFrame:
    df = pd.read_csv(path, sep="\t", names=TSV_HEADER, index_col=False, dtype={"cost": "string"})
    df = df.drop(columns="_empty")
    df["tsv"] = Path(path).name
    return df


def load_tsv(files: Optional[Iterable[Path]] = None, tsv_dir: Path = TSV_DIR) -> pd.DataFrame:
    """All the TSV files of tsv_dir (or the given ones), concatenated."""
    files = sorted(Path(tsv_dir).glob("*.tsv")) if files is None else [Path(f) for f in files]
    if not files:
        raise FileNotFoundError(f"no .tsv in {tsv_dir}")
    return pd.concat([read_tsv(f) for f in files], ignore_index=True)


def filter_df(df: pd.DataFrame, keep: Optional[List[Dict[str, str]]] = None,
              drop: Optional[List[Dict[str, str]]] = None) -> pd.DataFrame:
    """keep / drop: lists of {column: regex}; a dict matches when all its regexes match (search)."""
    def mask(f: Dict[str, str]) -> pd.Series:
        m = pd.Series(True, index=df.index)
        for col, regex in f.items():
            m &= df[col].astype(str).str.contains(regex, regex=True)
        return m

    for f in keep or []:
        df = df[mask(f)]
    for f in drop or []:
        df = df[~mask(f)]
    return df


def clean(df: pd.DataFrame, dedup: bool = True) -> pd.DataFrame:
    """Types, solver/benchmark/family names; duplicated runs (same benchmark, instance, executable) keep the last one:
    a rerun is appended after the run it replaces (e.g. 2026-11-10: amo-ijca on WGC, the first run had 10 s timeouts)."""
    df = df.copy()

    def extract_date(exec_id: pd.Series):
        return pd.to_datetime(exec_id.str.extract(r"(\d{4}-\d{2}-\d{2}-\d{2}-\d{2}).*")[0], format="%Y-%m-%d-%H-%M")
    df = df.assign(_date=extract_date(df["execid"])).sort_values("_date", kind="stable")

    cost = pd.to_numeric(df["cost"].astype(str).str.replace(",", ""), errors="coerce")
    
    df["solver"] = df["executable"].map(solver_name)
    df["bench"] = df["benchmark"].map(lambda b: BENCHMARK_NAMES.get(b, b))
    df["family"] = [_family(b, i) for b, i in zip(df["benchmark"], df["instance"])]
    
    if dedup:
        dup = df.duplicated(subset=RUN, keep=False)
        if dup.any():
            print(f"dropped {dup.sum() - df[dup].drop_duplicates(subset=RUN).shape[0]} duplicated runs, keeping the last:")
            print(df[dup].groupby(["benchmark", "executable"]).size().to_string())
        df = df.drop_duplicates(subset=RUN, keep="last")
    for c in ["real", "time", "user", "system", "memory"]:
        df[c] = pd.to_numeric(df[c].astype(str).str.replace(",", ""), errors="coerce")
    df["exit_code"] = df["exit_code"].astype(int)
    # cost only when a model was found ("Decision": no model)
    df["cost"] = cost.where(df["problem"] == "Optimization")
    df.drop(columns="_date", inplace=True)
    return df.reset_index(drop=True)


def _family(benchmark: str, instance: str) -> str:
    regex = FAMILY_REGEX.get(benchmark)
    m = re.search(regex, instance) if regex else None
    return m.group(1) if m else "-"


# --------------------------------------------------------------------------------------------------------------------
# metrics
# --------------------------------------------------------------------------------------------------------------------

def add_metrics(df: pd.DataFrame) -> pd.DataFrame:
    """Maximization, over the solvers in df:
    - optimum: the solver proved the optimum (exit code 30)
    - best:    the best cost of the instance among the solvers (NaN if no solver found a model)
    - win:     the solver found the best cost
    - score:   (cost + 1) / (best + 1) in [0, 1] (the +1 handles best = 0); 0 without a model
    """
    df = df.copy()
    df["solved"] = df["cost"].notna()
    df["optimum"] = df["exit_code"] == EXIT_OPTIMUM
    df["best"] = df.groupby(KEY)["cost"].transform("max")
    df["win"] = df["solved"] & (df["cost"] == df["best"])
    score = (df["cost"] + 1) / (df["best"] + 1)
    df["score"] = score.fillna(0.0)
    return df


def check_complete(df: pd.DataFrame) -> pd.DataFrame:
    """Instances that don't have a run of every solver (the comparison is unfair on them)."""
    solvers = df["solver"].nunique()
    n = df.groupby(KEY)["solver"].nunique()
    return n[n != solvers].reset_index(name="solvers")


# --------------------------------------------------------------------------------------------------------------------
# tables
# --------------------------------------------------------------------------------------------------------------------

METRIC_AGG = {"optimum": "sum", "win": "sum", "score": "mean"}


def metrics_table(df: pd.DataFrame, by: List[str]) -> pd.DataFrame:
    """Long table: one row per (by..., solver) with instances, solved, optimum, win, score."""
    g = df.groupby(by + ["solver"], dropna=False)
    t = g.agg(instances=("instance", "size"), solved=("solved", "sum"), optimum=("optimum", "sum"),
              win=("win", "sum"), score=("score", "mean"))
    t["score"] = t["score"].round(3)
    t = t.reset_index()
    rank = {s: i for i, s in enumerate(solver_order(t["solver"]))}
    return t.sort_values(by + ["solver"], key=lambda c: c.map(rank) if c.name == "solver" else c,
                         kind="stable").reset_index(drop=True)


def metrics_pivot(df: pd.DataFrame, by: List[str], metrics=("optimum", "win", "score"),
                  total: bool = True, rows: str = "solver") -> pd.DataFrame:
    """rows="solver": rows the solvers, columns (by..., metric), a Total column when total.
    rows="by":       rows by..., columns (solver, metric), a Total row when total."""
    metrics = list(metrics)
    agg = {m: METRIC_AGG[m] for m in metrics}
    if rows == "solver":
        p = df.pivot_table(index="solver", columns=by, values=metrics, aggfunc=agg, margins=total,
                           margins_name="Total")
        p = p.drop(index="Total", errors="ignore")
        p = p.reindex(solver_order(p.index))
        p = p.reorder_levels(list(range(1, len(by) + 1)) + [0], axis=1)        # (metric, by...) -> (by..., metric)
        groups = sorted(dict.fromkeys(c[:-1] for c in p.columns), key=lambda k: (k[0] == "Total", k))
        p = p[[g + (m,) for g in groups for m in metrics]]
        p.columns.names = list(by) + ["metric"]
    else:
        p = df.pivot_table(index=by, columns="solver", values=metrics, aggfunc=agg, margins=total,
                           margins_name="Total")
        p = p.drop(columns="Total", level=1, errors="ignore")
        p = p.reorder_levels([1, 0], axis=1)
        p = p.reindex(columns=pd.MultiIndex.from_product([solver_order(df["solver"]), metrics]))
        p.columns.names = ["solver", "metric"]
    for col in p.columns:
        p[col] = p[col].round(3) if col[-1] == "score" else p[col].fillna(0).astype(int)
    return p


def status_pivot(df: pd.DataFrame, by: List[str] = ("bench",), rows: str = "solver") -> pd.DataFrame:
    """How the runs ended, counts.
    rows="solver": rows the solvers, columns (by..., status), a Total group of columns.
    rows="by":     rows by..., columns (solver, status), a Total row."""
    by = list(by)
    if rows == "solver":
        p = df.groupby(["solver"] + by + ["status"]).size().unstack(by + ["status"], fill_value=0).sort_index(axis=1)
        total = df.groupby(["solver", "status"]).size().unstack("status", fill_value=0)
        total.columns = pd.MultiIndex.from_tuples([("Total",) + ("",) * (len(by) - 1) + (s,) for s in total.columns])
        p = pd.concat([p, total], axis=1).fillna(0).astype(int)
        p = p.reindex(solver_order(p.index))
        p.columns.names = by + ["status"]
        return p
    p = df.groupby(by + ["solver", "status"]).size().unstack(["solver", "status"], fill_value=0)
    p = p.sort_index(axis=1)
    p = p[[c for s in solver_order(p.columns.get_level_values(0)) for c in p.columns if c[0] == s]]
    total = p.sum().to_frame().T
    total.index = pd.MultiIndex.from_tuples([("Total",) + ("",) * (len(by) - 1)]) if len(by) > 1 else ["Total"]
    return pd.concat([p, total]).astype(int)


def bold_best_latex(pivot: pd.DataFrame, float_format: str = "{:.2f}") -> str:
    """LaTeX of a metrics pivot, the best value in bold: of each column when the rows are the solvers, else of each
    metric in a row."""
    out = pivot.copy().astype(object)
    by_column = pivot.index.name == "solver"
    for c in pivot.columns:
        metric = c[-1]
        if by_column:
            best = pd.Series(pivot[c].max(), index=pivot.index)
        else:
            best = pivot[[k for k in pivot.columns if k[-1] == metric]].max(axis=1)
        fmt = (lambda v: float_format.format(v)) if metric == "score" else str
        out[c] = [rf"\textbf{{{fmt(v)}}}" if v == b and v != 0 else fmt(v) for v, b in zip(pivot[c], best)]
    return out.to_latex(multicolumn=True, multicolumn_format="c", escape=False)


# --------------------------------------------------------------------------------------------------------------------
# Excel
# --------------------------------------------------------------------------------------------------------------------

def to_xlsx(name: str, table: pd.DataFrame, pivot: Optional[pd.DataFrame] = None,
            xlsx_dir: Path = XLSX_DIR) -> Path:
    """results_xlsx/<name>.xlsx with the sheet "table" and, when given, the sheet "pivot"."""
    xlsx_dir = Path(xlsx_dir)
    xlsx_dir.mkdir(parents=True, exist_ok=True)
    path = xlsx_dir / f"{name}.xlsx"
    with pd.ExcelWriter(path, engine="openpyxl") as w:
        table.to_excel(w, sheet_name="table", index=not isinstance(table.index, pd.RangeIndex))
        if pivot is not None:
            pivot.to_excel(w, sheet_name="pivot")
    return path
