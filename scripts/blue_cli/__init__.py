"""BlueCore cross-platform developer tooling."""

from .build import (
    default_build_backend,
    default_build_toolchain,
    normalize_premake_options,
    parse_build_args,
    parse_regenerate_args,
    resolve_build_request,
    run_build,
    run_regenerate,
)
from .cli import dispatch, main
from .core import (
    BlueCliError,
    detect_host,
    premake_executable,
    repo_root,
    require_command,
    run_command,
    run_premake,
)
from .formatting import (
    collect_format_files,
    parse_format_args,
    run_format,
)
from .toolchains import make_macos_clang_environment, prepare_unix_toolchain_environment
from .testing import (
    TestRequest,
    build_platform_for,
    load_registered_test_names,
    parse_test_args,
    resolve_test_backend,
    resolve_test_request,
    resolve_test_toolchain,
    run_gmake_tests,
    run_ninja_tests,
    run_tests,
    run_windows_tests,
    test_manifest_path,
)

__all__ = [name for name in globals() if not name.startswith("_")]
