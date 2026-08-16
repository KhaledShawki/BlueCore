from __future__ import annotations

import argparse
from .core import BlueCliError
from pathlib import Path
from typing import Sequence

SANITIZER_ORDER = ("asan", "ubsan", "tsan")
SANITIZER_NAMES = frozenset(SANITIZER_ORDER)


def parse_sanitizer_set(value: str) -> tuple[str, ...]:
    normalized = value.strip().lower()

    if normalized == "none":
        return ()

    if not normalized:
        raise argparse.ArgumentTypeError("--sanitizer requires one or more values: asan, ubsan, tsan")

    values = [item.strip() for item in normalized.split(",")]

    if any(not item for item in values):
        raise argparse.ArgumentTypeError(
            "--sanitizer contains an empty value; use comma-separated names such as asan,ubsan"
        )

    if "none" in values:
        raise argparse.ArgumentTypeError("'none' cannot be combined with sanitizer names")

    unknown = sorted(set(values) - SANITIZER_NAMES)
    if unknown:
        raise argparse.ArgumentTypeError(
            f"Unknown sanitizer(s): {', '.join(unknown)}. " f"Supported sanitizers: {', '.join(SANITIZER_ORDER)}"
        )

    duplicates = sorted(name for name in SANITIZER_ORDER if values.count(name) > 1)
    if duplicates:
        raise argparse.ArgumentTypeError(f"Duplicate sanitizer(s): {', '.join(duplicates)}")

    return tuple(name for name in SANITIZER_ORDER if name in values)


def sanitizer_value(sanitizers: Sequence[str]) -> str:
    return ",".join(sanitizers) if sanitizers else "none"


def sanitizer_output_name(sanitizers: Sequence[str]) -> str:
    return "-".join(sanitizers) if sanitizers else "none"


def plan_sanitizer_variants(
    sanitizers: Sequence[str],
) -> tuple[tuple[str, ...], ...]:
    requested = tuple(sanitizers)

    if not requested:
        return ((),)

    without_tsan = tuple(name for name in requested if name != "tsan")

    if "asan" in requested and "tsan" in requested:
        return (without_tsan, ("tsan",))

    return (requested,)


def sanitizer_output_root(
    root: Path,
    sanitizers: Sequence[str],
) -> Path:
    output_root = root / "out"

    if not sanitizers:
        return output_root

    return output_root / "sanitizers" / sanitizer_output_name(sanitizers)


def validate_sanitizer_request(
    host: str,
    toolchain: str,
    sanitizers: Sequence[str],
) -> None:
    if not sanitizers:
        return

    if host == "windows":
        if toolchain != "msvc":
            raise BlueCliError("Sanitized Windows builds currently require --toolchain=msvc.")

        unsupported = [name for name in sanitizers if name != "asan"]
        if unsupported:
            raise BlueCliError(
                "MSVC on Windows currently supports only ASan; " f"unsupported sanitizer(s): {', '.join(unsupported)}."
            )

        return

    if host in {"linux", "macos"}:
        if toolchain != "clang":
            raise BlueCliError(f"Sanitized {host} builds currently require --toolchain=clang.")

        return

    raise BlueCliError(f"Sanitizers are not configured for host: {host}")
