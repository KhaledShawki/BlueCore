#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import math
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

import matplotlib.pyplot as plt


TIME_UNIT_TO_NS: dict[str, float] = {
    "ns": 1.0,
    "us": 1_000.0,
    "ms": 1_000_000.0,
    "s": 1_000_000_000.0,
}


@dataclass(frozen=True)
class InputSource:
    label: str
    path: Path


@dataclass(frozen=True)
class BenchmarkRecord:
    source: str
    benchmark: str
    case: str | None
    implementation: str | None
    value_ns: float
    order: int


@dataclass(frozen=True)
class SeriesPoint:
    category: str
    series: str
    value_ns: float


@dataclass(frozen=True)
class BenchmarkProfile:
    path: Path
    title: str
    unit_label: str
    batch_size: int
    case_unit_suffix: str
    case_order: list[str]
    implementation_order: list[str]
    benchmarks: dict[str, dict[str, Any]]

    @staticmethod
    def load(path: Path) -> "BenchmarkProfile":
        payload = load_json(path)

        if not isinstance(payload, dict):
            raise ValueError(f"Benchmark profile must be a JSON object: {path}")

        benchmarks = payload.get("benchmarks")
        if not isinstance(benchmarks, dict) or not benchmarks:
            raise ValueError(f"Benchmark profile must contain a non-empty benchmarks object: {path}")

        batch_size = int(payload.get("batchSize", 1))
        if batch_size <= 0:
            raise ValueError(f"Profile batchSize must be greater than zero: {path}")

        return BenchmarkProfile(
            path=path,
            title=str(payload.get("title", "Benchmark results")),
            unit_label=str(payload.get("unitLabel", "CPU time (ns)")),
            batch_size=batch_size,
            case_unit_suffix=str(payload.get("caseUnitSuffix", "")),
            case_order=list_of_strings(payload.get("caseOrder", []), "caseOrder"),
            implementation_order=list_of_strings(payload.get("implementationOrder", []), "implementationOrder"),
            benchmarks={str(name): ensure_mapping(value, f"benchmarks.{name}") for name, value in benchmarks.items()},
        )

    def benchmark_rule(self, base_name: str) -> dict[str, Any] | None:
        return self.benchmarks.get(base_name)

    def benchmark_order(self, base_name: str) -> int:
        rule = self.benchmark_rule(base_name)
        if rule is None:
            return 10_000
        return int(rule.get("order", 10_000))

    def case_sort_key(self, case: str) -> tuple[int, int, str]:
        if case in self.case_order:
            return (0, self.case_order.index(case), case)

        number = first_integer(case)
        if number is not None:
            return (1, number, case)

        return (2, 0, case)

    def implementation_sort_key(self, implementation: str) -> tuple[int, str]:
        if implementation in self.implementation_order:
            return (self.implementation_order.index(implementation), implementation)
        return (10_000, implementation)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def resolve_path(root: Path, value: str | Path) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return root / path


def list_of_strings(value: Any, label: str) -> list[str]:
    if value is None:
        return []

    if not isinstance(value, list):
        raise ValueError(f"Profile field {label} must be a list")

    return [str(item) for item in value]


def ensure_mapping(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"Profile field {label} must be an object")
    return dict(value)


def parse_input(value: str, root: Path) -> InputSource:
    if "=" not in value:
        raise ValueError(f"--input must use LABEL=PATH format, got: {value}")

    label, path_value = value.split("=", 1)
    label = label.strip()
    path_value = path_value.strip()

    if not label:
        raise ValueError(f"Input label is empty: {value}")

    if not path_value:
        raise ValueError(f"Input path is empty: {value}")

    return InputSource(label=label, path=resolve_path(root, path_value))


def load_json(path: Path) -> Any:
    if not path.is_file():
        raise FileNotFoundError(f"JSON file not found: {path}")

    with path.open("r", encoding="utf-8") as file:
        return json.load(file)


def split_google_benchmark_name(raw_name: str) -> tuple[str, str | None]:
    name = raw_name.removeprefix("BM_")
    if "/" not in name:
        return name, None

    base_name, argument = name.split("/", 1)
    return base_name, argument


def strip_aggregate_suffix(name: str, aggregate_name: str | None) -> str:
    if not aggregate_name:
        return name

    suffix = f"_{aggregate_name}"
    if name.endswith(suffix):
        return name[: -len(suffix)]

    return name


def google_benchmark_time_ns(entry: Mapping[str, Any]) -> float:
    unit = str(entry.get("time_unit", "ns"))
    scale = TIME_UNIT_TO_NS.get(unit)
    if scale is None:
        raise ValueError(f"Unsupported Google Benchmark time unit: {unit}")

    cpu_time = entry.get("cpu_time")
    real_time = entry.get("real_time")
    value = cpu_time if isinstance(cpu_time, (int, float)) else real_time

    if not isinstance(value, (int, float)):
        raise ValueError(f"Benchmark entry has no numeric cpu_time or real_time: {entry}")

    return float(value) * scale


def first_integer(value: str) -> int | None:
    for part in value.split():
        if part.isdigit():
            return int(part)
    return None


def comparison_allowed(rule: Mapping[str, Any], comparison: str) -> bool:
    comparisons = rule.get("comparisons")
    if comparisons is None:
        return comparison == "config"

    if not isinstance(comparisons, list):
        raise ValueError("Profile benchmark comparisons field must be a list")

    return comparison in {str(item) for item in comparisons}


def case_from_rule(rule: Mapping[str, Any], argument: str | None, profile: BenchmarkProfile) -> str | None:
    if "case" in rule:
        return str(rule["case"])

    if bool(rule.get("caseFromArgument", False)):
        if argument is None:
            return None

        suffix = profile.case_unit_suffix.strip()
        return f"{argument} {suffix}".strip()

    return None


def benchmark_label_from_rule(rule: Mapping[str, Any], argument: str | None, profile: BenchmarkProfile) -> str:
    label = str(rule.get("label", ""))
    if not label:
        raise ValueError("Profile benchmark rule requires a non-empty label")

    if bool(rule.get("caseFromArgument", False)) and argument is not None:
        suffix = profile.case_unit_suffix.strip()
        argument_label = f"{argument} {suffix}".strip()
        return f"{label} {argument_label}"

    return label


def normalized_value_ns(rule: Mapping[str, Any], raw_value_ns: float, profile: BenchmarkProfile) -> float:
    normalize_by = rule.get("normalizeBy")
    if normalize_by is None:
        return raw_value_ns

    if normalize_by == "batchSize":
        return raw_value_ns / float(profile.batch_size)

    if isinstance(normalize_by, (int, float)):
        divisor = float(normalize_by)
        if divisor <= 0:
            raise ValueError("Profile normalizeBy divisor must be greater than zero")
        return raw_value_ns / divisor

    raise ValueError(f"Unsupported normalizeBy value: {normalize_by}")


def collect_records(profile: BenchmarkProfile, inputs: Sequence[InputSource], aggregate: str) -> list[BenchmarkRecord]:
    records: list[BenchmarkRecord] = []

    for source in inputs:
        payload = load_json(source.path)
        benchmarks = payload.get("benchmarks") if isinstance(payload, dict) else None
        if not isinstance(benchmarks, list):
            raise ValueError(f"Invalid Google Benchmark JSON: missing benchmarks array in {source.path}")

        for entry in benchmarks:
            if not isinstance(entry, dict):
                continue

            aggregate_name = entry.get("aggregate_name")
            if aggregate_name is not None and aggregate_name != aggregate:
                continue

            if aggregate_name is None and entry.get("run_type") == "aggregate":
                continue

            raw_name = str(entry.get("name", ""))
            if not raw_name:
                continue

            normalized_name = strip_aggregate_suffix(raw_name, str(aggregate_name) if aggregate_name else None)
            base_name, argument = split_google_benchmark_name(normalized_name)
            rule = profile.benchmark_rule(base_name)
            if rule is None:
                continue

            value_ns = normalized_value_ns(rule, google_benchmark_time_ns(entry), profile)
            records.append(
                BenchmarkRecord(
                    source=source.label,
                    benchmark=benchmark_label_from_rule(rule, argument, profile),
                    case=case_from_rule(rule, argument, profile),
                    implementation=str(rule.get("implementation")) if "implementation" in rule else None,
                    value_ns=value_ns,
                    order=profile.benchmark_order(base_name),
                )
            )

    if not records:
        raise ValueError("No benchmark records matched the profile and aggregate.")

    return records


def records_for_comparison(records: Sequence[BenchmarkRecord], profile: BenchmarkProfile, comparison: str) -> list[BenchmarkRecord]:
    filtered: list[BenchmarkRecord] = []

    for record in records:
        # Reconstructing from display labels is intentionally avoided. The profile check is done by walking rules
        # and matching display characteristics only for filtering. Unknown profile names were already filtered out.
        for base_name, rule in profile.benchmarks.items():
            if not comparison_allowed(rule, comparison):
                continue

            implementation = str(rule.get("implementation")) if "implementation" in rule else None
            label = str(rule.get("label", ""))

            if record.implementation == implementation and record.benchmark.startswith(label):
                filtered.append(record)
                break

    if not filtered:
        raise ValueError(f"No benchmark records are enabled for comparison mode: {comparison}")

    return filtered


def build_config_points(records: Sequence[BenchmarkRecord]) -> list[SeriesPoint]:
    return [SeriesPoint(category=record.benchmark, series=record.source, value_ns=record.value_ns) for record in records]


def build_implementation_points(records: Sequence[BenchmarkRecord]) -> list[SeriesPoint]:
    points: list[SeriesPoint] = []

    for record in records:
        if record.case is None or record.implementation is None:
            continue
        points.append(SeriesPoint(category=record.case, series=record.implementation, value_ns=record.value_ns))

    if not points:
        raise ValueError("Implementation comparison requires profile rules with case and implementation fields.")

    return points


def sorted_categories(points: Sequence[SeriesPoint], profile: BenchmarkProfile, mode: str, records: Sequence[BenchmarkRecord]) -> list[str]:
    categories = {point.category for point in points}

    if mode == "implementation":
        return sorted(categories, key=profile.case_sort_key)

    order_by_category: dict[str, int] = {}
    for record in records:
        order_by_category[record.benchmark] = min(record.order, order_by_category.get(record.benchmark, record.order))

    return sorted(categories, key=lambda category: (order_by_category.get(category, 10_000), profile.case_sort_key(category), category))


def sorted_series(points: Sequence[SeriesPoint], profile: BenchmarkProfile, mode: str) -> list[str]:
    series = {point.series for point in points}

    if mode == "implementation":
        return sorted(series, key=profile.implementation_sort_key)

    return sorted(series)


def write_csv(points: Sequence[SeriesPoint], output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with output_path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["category", "series", "time_ns_normalized"])

        for point in sorted(points, key=lambda item: (item.category, item.series)):
            writer.writerow([point.category, point.series, f"{point.value_ns:.6f}"])


def append_history(points: Sequence[SeriesPoint], history_path: Path, commit: str) -> None:
    history_path.parent.mkdir(parents=True, exist_ok=True)

    file_exists = history_path.is_file()
    timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds")

    with history_path.open("a", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        if not file_exists:
            writer.writerow(["timestamp_utc", "commit", "category", "series", "time_ns_normalized"])

        for point in sorted(points, key=lambda item: (item.category, item.series)):
            writer.writerow([timestamp, commit, point.category, point.series, f"{point.value_ns:.6f}"])


def write_grouped_bar_chart(
    points: Sequence[SeriesPoint],
    records: Sequence[BenchmarkRecord],
    profile: BenchmarkProfile,
    comparison: str,
    output_path: Path,
    title: str | None,
) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    categories = sorted_categories(points, profile, comparison, records)
    series_names = sorted_series(points, profile, comparison)
    values = {(point.category, point.series): point.value_ns for point in points}

    figure_height = max(6.5, len(categories) * 0.62)
    figure_width = 15.5 if len(series_names) <= 3 else 16.5
    fig, ax = plt.subplots(figsize=(figure_width, figure_height))

    y_positions = list(range(len(categories)))
    group_height = 0.82
    bar_height = group_height / max(len(series_names), 1)

    for series_index, series in enumerate(series_names):
        offset = -group_height / 2.0 + (series_index + 0.5) * bar_height
        positions = [position + offset for position in y_positions]
        series_values = [values.get((category, series), math.nan) for category in categories]

        bars = ax.barh(positions, series_values, height=bar_height, label=series)
        for bar in bars:
            width = bar.get_width()
            if math.isnan(width):
                continue
            ax.text(width + 0.08, bar.get_y() + bar.get_height() / 2.0, f"{width:.2f}", va="center", fontsize=8)

    max_value = max(point.value_ns for point in points)
    chart_title = title or default_title(profile, comparison, series_names)

    ax.set_title(chart_title)
    ax.set_xlabel(profile.unit_label)
    ax.set_ylabel("Case" if comparison == "implementation" else "Benchmark")
    ax.set_yticks(y_positions)
    ax.set_yticklabels(categories)
    ax.set_xlim(0.0, max_value * 1.16)
    ax.invert_yaxis()
    ax.grid(axis="x", alpha=0.25)
    ax.legend(loc="upper left", bbox_to_anchor=(1.01, 1.0), borderaxespad=0.0)

    fig.tight_layout(rect=(0.0, 0.0, 0.82, 1.0))
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def default_title(profile: BenchmarkProfile, comparison: str, series_names: Sequence[str]) -> str:
    if comparison == "implementation":
        return f"{profile.title} - implementation comparison"

    if comparison == "backend":
        return f"{profile.title} - backend comparison"

    if len(series_names) > 1:
        return f"{profile.title} - configuration comparison"

    return profile.title


def write_history_line_chart(history_path: Path, output_path: Path, title: str, unit_label: str) -> None:
    if not history_path.is_file():
        return

    with history_path.open("r", encoding="utf-8") as file:
        rows = list(csv.DictReader(file))

    if not rows:
        return

    output_path.parent.mkdir(parents=True, exist_ok=True)

    series: dict[tuple[str, str], list[tuple[str, float]]] = defaultdict(list)
    for row in rows:
        key = (row.get("series", "unknown"), row.get("category", "unknown"))
        commit = row.get("commit", "unknown")
        value = float(row.get("time_ns_normalized", "nan"))
        series[key].append((commit, value))

    fig, ax = plt.subplots(figsize=(14.0, 8.0))

    for (series_name, category), values in sorted(series.items()):
        if len(values) < 2:
            continue

        ax.plot(
            [item[0] for item in values],
            [item[1] for item in values],
            marker="o",
            linewidth=1.5,
            label=f"{series_name} - {category}",
        )

    ax.set_title(title)
    ax.set_xlabel("Commit")
    ax.set_ylabel(unit_label)
    ax.tick_params(axis="x", rotation=35)
    ax.grid(axis="y", alpha=0.25)

    if ax.lines:
        ax.legend(fontsize="x-small", ncol=2)

    fig.tight_layout()
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot Google Benchmark JSON output using a benchmark profile.")

    parser.add_argument(
        "--profile",
        required=True,
        help="Benchmark profile JSON file.",
    )

    parser.add_argument(
        "--comparison",
        default="config",
        choices=["config", "backend", "implementation"],
        help="Comparison mode.",
    )

    parser.add_argument(
        "--input",
        action="append",
        required=True,
        help="Benchmark JSON input in LABEL=PATH format. Can be passed multiple times except with implementation mode.",
    )

    parser.add_argument(
        "--aggregate",
        default="median",
        choices=["mean", "median"],
        help="Aggregate result to plot from Google Benchmark JSON.",
    )

    parser.add_argument(
        "--bar-output",
        default="out/benchmarks/graphs/benchmark_results.png",
        help="Output PNG path for the grouped bar chart.",
    )

    parser.add_argument(
        "--csv-output",
        default=None,
        help="Optional flat CSV output path derived from the benchmark JSON.",
    )

    parser.add_argument(
        "--history-csv",
        default=None,
        help="Optional history CSV path. Results are appended to this file.",
    )

    parser.add_argument(
        "--history-output",
        default=None,
        help="Optional history line chart PNG path.",
    )

    parser.add_argument(
        "--commit",
        default="unknown",
        help="Commit label used when appending history.",
    )

    parser.add_argument(
        "--title",
        default=None,
        help="Optional chart title override.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = repo_root()

    profile = BenchmarkProfile.load(resolve_path(root, args.profile))
    inputs = [parse_input(value, root) for value in args.input]

    if args.comparison == "implementation" and len(inputs) != 1:
        raise ValueError("--comparison implementation expects exactly one --input LABEL=PATH")

    all_records = collect_records(profile, inputs, args.aggregate)
    selected_records = records_for_comparison(all_records, profile, args.comparison)

    if args.comparison == "implementation":
        points = build_implementation_points(selected_records)
    else:
        points = build_config_points(selected_records)

    bar_output = resolve_path(root, args.bar_output)
    write_grouped_bar_chart(points, selected_records, profile, args.comparison, bar_output, args.title)

    if args.csv_output:
        write_csv(points, resolve_path(root, args.csv_output))

    if args.history_csv:
        history_csv = resolve_path(root, args.history_csv)
        append_history(points, history_csv, args.commit)

        if args.history_output:
            write_history_line_chart(
                history_csv,
                resolve_path(root, args.history_output),
                args.title or f"{profile.title} - history",
                profile.unit_label,
            )

    print(f"Bar chart written to: {display_path(bar_output, root)}")

    if args.csv_output:
        print(f"CSV written to: {display_path(resolve_path(root, args.csv_output), root)}")

    if args.history_csv:
        print(f"History CSV updated: {display_path(resolve_path(root, args.history_csv), root)}")

    if args.history_output:
        print(f"History chart written to: {display_path(resolve_path(root, args.history_output), root)}")

    return 0


def display_path(path: Path, root: Path) -> Path:
    try:
        return path.relative_to(root)
    except ValueError:
        return path


if __name__ == "__main__":
    raise SystemExit(main())
