from __future__ import annotations

from dataclasses import dataclass, replace
from typing import Sequence

from .core import BlueCliError

BUILD_PLATFORMS = ("x64", "x64_DLL")


def linkage_for_build_platform(build_platform: str) -> str:
    if build_platform == "x64":
        return "static"
    if build_platform == "x64_DLL":
        return "shared"

    raise BlueCliError(f"Unsupported Blue build platform: {build_platform}")


def build_platform_for_linkage(linkage: str, architecture: str = "x64") -> str:
    if linkage == "static":
        return architecture
    if linkage == "shared":
        return f"{architecture}_DLL"

    raise BlueCliError(f"Unsupported Blue linkage: {linkage}")


@dataclass(frozen=True)
class BuildContext:
    host: str
    configuration: str
    linkage: str
    toolchain: str
    memory_backend: str
    sanitizer: tuple[str, ...] = ()
    architecture: str = "x64"

    @property
    def build_platform(self) -> str:
        return build_platform_for_linkage(self.linkage, self.architecture)

    def with_sanitizer(self, sanitizers: Sequence[str]) -> "BuildContext":
        return replace(self, sanitizer=tuple(sanitizers))
