from __future__ import annotations

import importlib.util
import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

BLUE_SCRIPT = Path(__file__).resolve().parents[1] / "blue.py"
SPEC = importlib.util.spec_from_file_location("blue_cli", BLUE_SCRIPT)
assert SPEC is not None
assert SPEC.loader is not None
blue = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = blue
SPEC.loader.exec_module(blue)


class BlueCliTests(unittest.TestCase):
    def test_detect_host_normalizes_supported_platforms(self) -> None:
        self.assertEqual(blue.detect_host("Windows"), "windows")
        self.assertEqual(blue.detect_host("Linux"), "linux")
        self.assertEqual(blue.detect_host("Darwin"), "macos")

    def test_detect_host_rejects_unknown_platform(self) -> None:
        with self.assertRaisesRegex(blue.BlueCliError, "Unsupported host platform"):
            blue.detect_host("FreeBSD")

    def test_premake_executable_uses_bundled_host_binary(self) -> None:
        root = Path("/repo")

        self.assertEqual(
            blue.premake_executable(root, "windows"),
            root / "tools" / "premake" / "windows" / "premake5.exe",
        )
        self.assertEqual(
            blue.premake_executable(root, "linux"),
            root / "tools" / "premake" / "linux" / "premake5",
        )
        self.assertEqual(
            blue.premake_executable(root, "macos"),
            root / "tools" / "premake" / "macos" / "premake5",
        )

    def test_backend_resolution_uses_host_and_linkage_defaults(self) -> None:
        self.assertEqual(blue.resolve_test_backend("linux", "static", None), "ninja")
        self.assertEqual(blue.resolve_test_backend("macos", "static", None), "ninja")
        self.assertEqual(blue.resolve_test_backend("linux", "shared", None), "gmake")
        self.assertEqual(blue.resolve_test_backend("macos", "shared", None), "gmake")
        self.assertEqual(blue.resolve_test_backend("windows", "static", None), "vs2026")
        self.assertEqual(blue.resolve_test_backend("windows", "shared", None), "vs2026")

    def test_backend_resolution_rejects_invalid_host_backend_combinations(self) -> None:
        with self.assertRaisesRegex(blue.BlueCliError, "Ninja does not support"):
            blue.resolve_test_backend("linux", "shared", "ninja")

        with self.assertRaisesRegex(blue.BlueCliError, "not available on macos"):
            blue.resolve_test_backend("macos", "static", "vs2026")

        with self.assertRaisesRegex(blue.BlueCliError, "not configured for Windows"):
            blue.resolve_test_backend("windows", "static", "gmake")

    def test_toolchain_resolution_uses_host_and_backend_defaults(self) -> None:
        self.assertEqual(blue.resolve_test_toolchain("macos", "ninja", None), "clang")
        self.assertEqual(blue.resolve_test_toolchain("macos", "gmake", None), "clang")
        self.assertEqual(blue.resolve_test_toolchain("linux", "ninja", None), "clang")
        self.assertEqual(blue.resolve_test_toolchain("linux", "gmake", None), "gcc")
        self.assertEqual(blue.resolve_test_toolchain("windows", "vs2026", None), "msvc")

    def test_toolchain_resolution_rejects_invalid_host_combinations(self) -> None:
        with self.assertRaisesRegex(blue.BlueCliError, "not available for macOS"):
            blue.resolve_test_toolchain("macos", "gmake", "gcc")

        with self.assertRaisesRegex(blue.BlueCliError, "not configured for Windows"):
            blue.resolve_test_toolchain("windows", "vs2026", "clang")

        with self.assertRaisesRegex(blue.BlueCliError, "not available for Linux"):
            blue.resolve_test_toolchain("linux", "gmake", "msvc")

    def test_resolve_test_request_normalizes_all_build_axes(self) -> None:
        args = blue.parse_test_args(
            [
                "--config=Release",
                "--backend=gmake2",
                "--toolchain=clang",
                "--linkage=shared",
                "--memory-backend=mimalloc",
            ]
        )

        request = blue.resolve_test_request("linux", args)

        self.assertEqual(request.host, "linux")
        self.assertEqual(request.configuration, "Release")
        self.assertEqual(request.backend, "gmake")
        self.assertEqual(request.toolchain, "clang")
        self.assertEqual(request.linkage, "shared")
        self.assertEqual(request.memory_backend, "mimalloc")
        self.assertEqual(request.build_platform, "x64_DLL")

    def test_build_platform_maps_linkage_for_supported_backends(self) -> None:
        self.assertEqual(blue.build_platform_for("ninja", "static"), "x64")
        self.assertEqual(blue.build_platform_for("gmake", "static"), "x64")
        self.assertEqual(blue.build_platform_for("gmake", "shared"), "x64_DLL")
        self.assertEqual(blue.build_platform_for("vs2026", "static"), "x64")
        self.assertEqual(blue.build_platform_for("vs2026", "shared"), "x64_DLL")

        with self.assertRaisesRegex(blue.BlueCliError, "Ninja only supports"):
            blue.build_platform_for("ninja", "shared")

    def test_dispatch_maps_public_command_to_existing_premake_action(self) -> None:
        root = Path("/repo")

        with mock.patch.object(blue, "run_premake", return_value=0) as run_premake:
            result = blue.dispatch(
                [
                    "add-file",
                    "--blue-project=BlueSystem",
                    "--blue-kind=source",
                    "--blue-path=Log/FileLogger.cpp",
                ],
                root=root,
                host="linux",
            )

        self.assertEqual(result, 0)
        run_premake.assert_called_once_with(
            root,
            "linux",
            [
                "blue-add-file",
                "--blue-project=BlueSystem",
                "--blue-kind=source",
                "--blue-path=Log/FileLogger.cpp",
            ],
        )

    def test_dispatch_premake_forwards_arguments_unchanged(self) -> None:
        root = Path("/repo")
        arguments = ["ninja", "--toolchain=clang", "--blue-platforms=linux"]

        with mock.patch.object(blue, "run_premake", return_value=0) as run_premake:
            result = blue.dispatch(["premake", *arguments], root=root, host="linux")

        self.assertEqual(result, 0)
        run_premake.assert_called_once_with(root, "linux", arguments)

    def test_test_arguments_are_semantic_and_case_insensitive(self) -> None:
        args = blue.parse_test_args(
            [
                "--config=release",
                "--linkage=STATIC",
                "--backend=GMAKE2",
                "--toolchain=CLANG",
                "--memory-backend=MIMALLOC",
            ]
        )

        self.assertEqual(args.config, "Release")
        self.assertEqual(args.linkage, "static")
        self.assertEqual(args.backend, "gmake")
        self.assertEqual(args.toolchain, "clang")
        self.assertEqual(args.memory_backend, "mimalloc")

    def test_legacy_test_arguments_remain_compatible_with_old_wrappers(self) -> None:
        static_args = blue.parse_test_args(["Release_x64", "--memory-backend=mimalloc"])
        shared_args = blue.parse_test_args(["Debug_x64", "x64_DLL"])

        self.assertEqual(static_args.config, "Release")
        self.assertEqual(static_args.linkage, "static")
        self.assertEqual(static_args.memory_backend, "mimalloc")
        self.assertEqual(shared_args.config, "Debug")
        self.assertEqual(shared_args.linkage, "shared")

    def test_registered_test_manifest_is_authoritative_and_ordered(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            manifest = blue.test_manifest_path(root)
            manifest.parent.mkdir(parents=True)
            manifest.write_text(
                '{"tests":[{"name":"BlueSystemAtomicTests"},{"name":"BlueSystemThreadingTests"}]}',
                encoding="utf-8",
            )

            self.assertEqual(
                blue.load_registered_test_names(root),
                ["BlueSystemAtomicTests", "BlueSystemThreadingTests"],
            )

    def test_registered_test_manifest_rejects_duplicate_names(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            manifest = blue.test_manifest_path(root)
            manifest.parent.mkdir(parents=True)
            manifest.write_text(
                '{"tests":[{"name":"BlueSystemAtomicTests"},{"name":"BlueSystemAtomicTests"}]}',
                encoding="utf-8",
            )

            with self.assertRaisesRegex(blue.BlueCliError, "duplicate test name"):
                blue.load_registered_test_names(root)

    def test_gmake2_backend_name_is_a_compatibility_alias(self) -> None:
        self.assertEqual(blue.parse_test_args(["--backend=gmake"]).backend, "gmake")
        self.assertEqual(blue.parse_test_args(["--backend=GMAKE2"]).backend, "gmake")

    def test_gmake_shared_tests_use_x64_dll_configuration(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            build_root = root / "out" / "build" / "gmake"
            bin_dir = root / "out" / "bin" / "linux" / "x64_DLL" / "Debug"
            build_root.mkdir(parents=True)
            bin_dir.mkdir(parents=True)

            manifest = blue.test_manifest_path(root)
            manifest.parent.mkdir(parents=True)
            manifest.write_text('{"tests":[{"name":"BlueSystemAtomicTests"}]}', encoding="utf-8")

            runner = bin_dir / "BlueRunTests"
            test_executable = bin_dir / "BlueSystemAtomicTests"
            for path in (runner, test_executable):
                path.write_text("", encoding="utf-8")
                path.chmod(path.stat().st_mode | stat.S_IXUSR)

            with (
                mock.patch.object(blue, "require_command", return_value="/usr/bin/tool"),
                mock.patch.object(blue, "run_premake", return_value=0) as run_premake,
                mock.patch.object(blue, "run_command", return_value=0) as run_command,
                mock.patch.object(blue.os, "cpu_count", return_value=4),
                mock.patch("builtins.print"),
            ):
                request = blue.TestRequest(
                    host="linux",
                    configuration="Debug",
                    backend="gmake",
                    toolchain="clang",
                    linkage="shared",
                    memory_backend="system",
                    build_platform="x64_DLL",
                )
                result = blue.run_gmake_tests(root, request)

        self.assertEqual(result, 0)
        premake_args = run_premake.call_args.args[2]
        self.assertEqual(premake_args[0], "gmake")
        self.assertIn("--toolchain=clang", premake_args)
        self.assertIn("--blue-build-platforms=x64_DLL", premake_args)
        self.assertIn("--blue-test-manifest=out/metadata/BlueTests.json", premake_args)
        self.assertIn("--blue-test-postbuild=off", premake_args)

        make_command = run_command.call_args_list[0].args[0]
        self.assertIn("config=debug_x64_dll", make_command)
        self.assertIn("BlueRunTests", make_command)
        self.assertNotIn("BlueTests", make_command)

        runner_command = run_command.call_args_list[1].args[0]
        self.assertEqual(runner_command[0], str(runner))
        self.assertEqual(runner_command[1], "--jobs=auto")
        self.assertEqual(runner_command[2:], [str(test_executable)])

    def test_linux_toolchain_environment_checks_selected_compiler(self) -> None:
        root = Path("/repo")

        with mock.patch.object(blue, "require_command", return_value="/usr/bin/tool") as require_command:
            blue.prepare_unix_toolchain_environment(root, "linux", "gcc")

        require_command.assert_called_once_with(
            "g++",
            "Install GCC and ensure g++ is available in PATH.",
        )

    def test_macos_clang_environment_creates_xcrun_wrappers(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)

            with (
                mock.patch.object(blue, "require_command", return_value="/usr/bin/xcrun"),
                mock.patch.object(blue, "run_command", return_value=0),
                mock.patch.dict(os.environ, {"PATH": "/usr/bin"}, clear=True),
            ):
                env = blue.make_macos_clang_environment(root)

            wrapper_dir = root / "out" / "tools" / "macos-clang"
            clang = wrapper_dir / "clang"
            clangxx = wrapper_dir / "clang++"

            self.assertTrue(clang.is_file())
            self.assertTrue(clangxx.is_file())
            self.assertTrue(clang.stat().st_mode & stat.S_IXUSR)
            self.assertTrue(clangxx.stat().st_mode & stat.S_IXUSR)
            self.assertEqual(env["PATH"].split(os.pathsep)[0], str(wrapper_dir))
            self.assertIn("xcrun --sdk macosx clang", clang.read_text(encoding="utf-8"))
            self.assertIn("xcrun --sdk macosx clang++", clangxx.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
