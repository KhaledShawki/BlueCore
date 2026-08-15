from __future__ import annotations

import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPTS_DIR = Path(__file__).resolve().parents[1]
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import blue_cli as blue
from blue_cli import cli as cli_module
from blue_cli import build as build_module
from blue_cli import formatting as formatting_module
from blue_cli import testing as testing_module
from blue_cli import toolchains as toolchains_module


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

        with mock.patch.object(cli_module, "run_premake", return_value=0) as run_premake:
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

        with mock.patch.object(cli_module, "run_premake", return_value=0) as run_premake:
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

    def test_registered_test_manifest_rejects_missing_and_malformed_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            with self.assertRaisesRegex(blue.BlueCliError, "manifest was not generated"):
                blue.load_registered_test_names(root)

            manifest = blue.test_manifest_path(root)
            manifest.parent.mkdir(parents=True)
            manifest.write_text("{not-json", encoding="utf-8")
            with self.assertRaisesRegex(blue.BlueCliError, "Could not read registered test manifest"):
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
                mock.patch.object(testing_module, "require_gnu_make", return_value="/usr/bin/gmake"),
                mock.patch.object(testing_module, "run_premake", return_value=0) as run_premake,
                mock.patch.object(testing_module, "run_command", return_value=0) as run_command,
                mock.patch.object(testing_module.os, "cpu_count", return_value=4),
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

    def test_generic_build_toolchain_uses_generic_environment_variable(self) -> None:
        with mock.patch.dict(os.environ, {"BLUE_TOOLCHAIN": "gcc"}, clear=True):
            self.assertEqual(blue.default_build_toolchain("linux"), "gcc")

        with mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(blue.default_build_toolchain("linux"), "clang")

    def test_clean_is_configuration_scoped_and_rejects_target_argument(self) -> None:
        args = blue.parse_build_args(
            ["--config=Release", "--platform=x64", "--backend=gmake", "--toolchain=clang"],
            command="clean",
        )
        self.assertIsNone(args.target)

        with (
            mock.patch("sys.stderr"),
            self.assertRaises(SystemExit),
        ):
            blue.parse_build_args(["BlueTests"], command="clean")

    def test_windows_build_supports_static_shared_and_configuration_clean(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            build_root = root / "out" / "build" / "vs2026"
            build_root.mkdir(parents=True)
            solution = build_root / "Blue.slnx"
            solution.write_text("", encoding="utf-8")

            for platform in ("x64", "x64_DLL"):
                with self.subTest(platform=platform):
                    args = blue.parse_build_args(
                        [
                            "BlueTests",
                            "--config=Release",
                            f"--platform={platform}",
                            "--backend=vs2026",
                            "--toolchain=msvc",
                        ],
                        command="build",
                    )
                    with (
                        mock.patch.object(build_module, "require_command", return_value=r"C:\VS\MSBuild.exe"),
                        mock.patch.object(build_module, "run_premake", return_value=0),
                        mock.patch.object(build_module, "run_command", return_value=0) as run_command,
                    ):
                        result = blue.run_build(root, "windows", args)

                    self.assertEqual(result, 0)
                    command = run_command.call_args.args[0]
                    self.assertEqual(command[0], r"C:\VS\MSBuild.exe")
                    self.assertIn("/t:BlueTests", command)
                    self.assertIn("/p:Configuration=Release", command)
                    self.assertIn(f"/p:Platform={platform}", command)

            clean_args = blue.parse_build_args(
                ["--config=Release", "--platform=x64_DLL", "--backend=vs2026", "--toolchain=msvc"],
                command="clean",
            )
            with (
                mock.patch.object(build_module, "require_command", return_value=r"C:\VS\MSBuild.exe"),
                mock.patch.object(build_module, "run_premake", return_value=0),
                mock.patch.object(build_module, "run_command", return_value=0) as run_command,
            ):
                result = blue.run_build(root, "windows", clean_args, clean=True)

            self.assertEqual(result, 0)
            command = run_command.call_args.args[0]
            self.assertIn("/t:Clean", command)
            self.assertNotIn("/t:BlueTests", command)

    def test_windows_test_orchestration_uses_resolved_msbuild_for_static_and_shared(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            build_root = root / "out" / "build" / "vs2026"
            runner_project = build_root / "BlueRunTests" / "BlueRunTests.vcxproj"
            test_project = build_root / "BlueSystemAtomicTests" / "BlueSystemAtomicTests.vcxproj"
            runner_project.parent.mkdir(parents=True)
            test_project.parent.mkdir(parents=True)
            runner_project.write_text("", encoding="utf-8")
            test_project.write_text("", encoding="utf-8")

            manifest = blue.test_manifest_path(root)
            manifest.parent.mkdir(parents=True)
            manifest.write_text('{"tests":[{"name":"BlueSystemAtomicTests"}]}', encoding="utf-8")

            for build_platform, linkage in (("x64", "static"), ("x64_DLL", "shared")):
                with self.subTest(build_platform=build_platform):
                    bin_dir = root / "out" / "bin" / "windows" / build_platform / "Debug"
                    bin_dir.mkdir(parents=True, exist_ok=True)
                    runner = bin_dir / "BlueRunTests.exe"
                    test_executable = bin_dir / "BlueSystemAtomicTests.exe"
                    runner.write_text("", encoding="utf-8")
                    test_executable.write_text("", encoding="utf-8")

                    request = blue.TestRequest(
                        host="windows",
                        configuration="Debug",
                        backend="vs2026",
                        toolchain="msvc",
                        linkage=linkage,
                        memory_backend="system",
                        build_platform=build_platform,
                    )
                    with (
                        mock.patch.object(testing_module, "require_command", return_value=r"C:\VS\MSBuild.exe"),
                        mock.patch.object(testing_module, "run_premake", return_value=0),
                        mock.patch.object(testing_module, "run_command", return_value=0) as run_command,
                        mock.patch("builtins.print"),
                    ):
                        result = blue.run_windows_tests(root, request)

                    self.assertEqual(result, 0)
                    self.assertEqual(run_command.call_count, 3)
                    self.assertEqual(run_command.call_args_list[0].args[0][0], r"C:\VS\MSBuild.exe")
                    self.assertEqual(run_command.call_args_list[1].args[0][0], r"C:\VS\MSBuild.exe")
                    self.assertEqual(run_command.call_args_list[2].args[0][0], str(runner))
                    self.assertIn(f"/p:Platform={build_platform}", run_command.call_args_list[0].args[0])

    def test_build_stops_when_premake_generation_fails(self) -> None:
        root = Path("/repo")
        args = blue.parse_build_args(
            ["BlueTests", "--backend=ninja", "--toolchain=clang"],
            command="build",
        )
        with (
            mock.patch.object(build_module, "require_command", return_value="/usr/bin/ninja"),
            mock.patch.object(build_module, "prepare_unix_toolchain_environment", return_value={}),
            mock.patch.object(build_module, "run_premake", return_value=7),
            mock.patch.object(build_module, "run_command", return_value=0) as run_command,
        ):
            result = blue.run_build(root, "linux", args)

        self.assertEqual(result, 7)
        run_command.assert_not_called()

    def test_test_orchestration_stops_when_native_build_fails(self) -> None:
        root = Path("/repo")
        request = blue.TestRequest(
            host="linux",
            configuration="Debug",
            backend="ninja",
            toolchain="clang",
            linkage="static",
            memory_backend="system",
            build_platform="x64",
        )
        with (
            mock.patch.object(testing_module, "require_command", return_value="/usr/bin/ninja"),
            mock.patch.object(testing_module, "prepare_unix_toolchain_environment", return_value={}),
            mock.patch.object(testing_module, "run_premake", return_value=0),
            mock.patch.object(testing_module, "run_command", return_value=13) as run_command,
            mock.patch("builtins.print"),
        ):
            result = blue.run_ninja_tests(root, request)

        self.assertEqual(result, 13)
        run_command.assert_called_once()
        self.assertEqual(run_command.call_args.args[0][0], "/usr/bin/ninja")

    def test_linux_toolchain_environment_checks_selected_compiler(self) -> None:
        root = Path("/repo")

        with mock.patch.object(toolchains_module, "require_command", return_value="/usr/bin/tool") as require_command:
            blue.prepare_unix_toolchain_environment(root, "linux", "gcc")

        require_command.assert_called_once_with(
            "g++",
            "Install GCC and ensure g++ is available in PATH.",
        )

    def test_macos_clang_environment_creates_xcrun_wrappers(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)

            with (
                mock.patch.object(toolchains_module, "require_command", return_value="/usr/bin/xcrun"),
                mock.patch.object(toolchains_module, "run_command", return_value=0),
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

    def test_regenerate_normalizes_split_options_and_updates_stale_token(self) -> None:
        root = Path("/repo")
        action, options = blue.parse_regenerate_args(
            ["ninja", "--toolchain", "clang", "--blue-platforms=linux"], host="linux"
        )

        self.assertEqual(action, "ninja")
        self.assertEqual(options, ["--toolchain=clang", "--blue-platforms=linux"])

        with mock.patch.object(build_module, "run_premake", side_effect=[2, 0, 0]) as run_premake:
            result = blue.run_regenerate(root, "linux", action, options)

        self.assertEqual(result, 0)
        self.assertEqual(
            run_premake.call_args_list,
            [
                mock.call(root, "linux", ["--regen-action=ninja", *options, "check-regeneration"]),
                mock.call(root, "linux", [*options, "ninja"]),
                mock.call(root, "linux", ["--regen-action=ninja", *options, "update-build-token"]),
            ],
        )

    def test_regenerate_skips_when_token_is_current(self) -> None:
        root = Path("/repo")
        with mock.patch.object(build_module, "run_premake", return_value=0) as run_premake:
            result = blue.run_regenerate(root, "macos", "ninja", ["--toolchain=clang"])

        self.assertEqual(result, 0)
        run_premake.assert_called_once_with(
            root,
            "macos",
            ["--regen-action=ninja", "--toolchain=clang", "check-regeneration"],
        )

    def test_regenerate_propagates_unexpected_check_failure(self) -> None:
        root = Path("/repo")
        with mock.patch.object(build_module, "run_premake", return_value=9) as run_premake:
            result = blue.run_regenerate(root, "linux", "ninja", ["--toolchain=clang"])

        self.assertEqual(result, 9)
        run_premake.assert_called_once()

    def test_unix_build_uses_semantic_gmake_target(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            (root / "out" / "build" / "gmake").mkdir(parents=True)
            args = blue.parse_build_args(
                [
                    "BlueTests",
                    "--config=Release",
                    "--platform=x64_DLL",
                    "--toolchain=clang",
                    "--memory-backend=mimalloc",
                ],
                command="build",
            )

            with (
                mock.patch.object(
                    build_module,
                    "require_gnu_make",
                    return_value="/usr/bin/gmake",
                ),
                mock.patch.object(
                    build_module,
                    "prepare_unix_toolchain_environment",
                    return_value={"PATH": "/toolchain"},
                ),
                mock.patch.object(build_module, "run_premake", return_value=0) as run_premake,
                mock.patch.object(build_module, "run_command", return_value=0) as run_command,
            ):
                result = blue.run_build(root, "linux", args)

        self.assertEqual(result, 0)
        self.assertIn("gmake", run_premake.call_args.args[2])
        self.assertIn("--blue-build-platforms=x64_DLL", run_premake.call_args.args[2])
        self.assertEqual(
            run_command.call_args.args[0],
            [
                "/usr/bin/gmake",
                "-C",
                str(root / "out" / "build" / "gmake"),
                "config=release_x64_dll",
                "BlueTests",
            ],
        )

    def test_unix_build_supports_semantic_ninja_target(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            build_root = root / "out" / "build" / "ninja"
            build_root.mkdir(parents=True)
            args = blue.parse_build_args(
                [
                    "BlueRunTests",
                    "--config=Debug",
                    "--platform=x64",
                    "--backend=ninja",
                    "--toolchain=clang",
                ],
                command="build",
            )
            env = {"PATH": "/toolchain"}

            with (
                mock.patch.object(build_module, "require_command", return_value="/usr/bin/ninja"),
                mock.patch.object(
                    build_module,
                    "prepare_unix_toolchain_environment",
                    return_value=env,
                ) as prepare_environment,
                mock.patch.object(build_module, "run_premake", return_value=0) as run_premake,
                mock.patch.object(build_module, "run_command", return_value=0) as run_command,
            ):
                result = blue.run_build(root, "macos", args)

        self.assertEqual(result, 0)
        prepare_environment.assert_called_once_with(root, "macos", "clang")
        run_premake.assert_called_once_with(
            root,
            "macos",
            [
                "ninja",
                "--toolchain=clang",
                "--blue-platforms=macos",
                "--blue-build-platforms=x64",
                "--memory-backend=system",
            ],
            env=env,
        )
        run_command.assert_called_once_with(
            [
                "/usr/bin/ninja",
                "-C",
                str(build_root),
                "BlueRunTests_Debug_x64",
            ],
            cwd=root,
            env=env,
        )

    def test_unix_clean_is_configuration_scoped_for_ninja_and_gmake(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            ninja_root = root / "out" / "build" / "ninja"
            gmake_root = root / "out" / "build" / "gmake"
            ninja_root.mkdir(parents=True)
            gmake_root.mkdir(parents=True)

            ninja_args = blue.parse_build_args(
                ["--config=Release", "--backend=ninja", "--toolchain=clang"],
                command="clean",
            )
            with (
                mock.patch.object(build_module, "require_command", return_value="/usr/bin/ninja"),
                mock.patch.object(build_module, "prepare_unix_toolchain_environment", return_value={}),
                mock.patch.object(build_module, "run_premake", return_value=0),
                mock.patch.object(build_module, "run_command", return_value=0) as run_command,
            ):
                result = blue.run_build(root, "macos", ninja_args, clean=True)

            self.assertEqual(result, 0)
            self.assertEqual(
                run_command.call_args.args[0],
                [
                    "/usr/bin/ninja",
                    "-C",
                    str(ninja_root),
                    "-t",
                    "clean",
                    "Release_x64",
                ],
            )

            gmake_args = blue.parse_build_args(
                [
                    "--config=Release",
                    "--platform=x64_DLL",
                    "--backend=gmake",
                    "--toolchain=clang",
                ],
                command="clean",
            )
            with (
                mock.patch.object(build_module, "require_gnu_make", return_value="/usr/bin/gmake"),
                mock.patch.object(build_module, "prepare_unix_toolchain_environment", return_value={}),
                mock.patch.object(build_module, "run_premake", return_value=0),
                mock.patch.object(build_module, "run_command", return_value=0) as run_command,
            ):
                result = blue.run_build(root, "macos", gmake_args, clean=True)

            self.assertEqual(result, 0)
            self.assertEqual(
                run_command.call_args.args[0],
                [
                    "/usr/bin/gmake",
                    "-C",
                    str(gmake_root),
                    "config=release_x64_dll",
                    "clean",
                ],
            )

    def test_ninja_build_rejects_x64_dll_platform(self) -> None:
        args = blue.parse_build_args(
            [
                "BlueRunTests",
                "--platform=x64_DLL",
                "--backend=ninja",
                "--toolchain=clang",
            ],
            command="build",
        )

        with self.assertRaisesRegex(blue.BlueCliError, "Ninja only supports the static x64"):
            blue.resolve_build_request("macos", args)

    def test_format_file_collection_is_cross_platform_and_ignores_vendor_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            (root / ".clang-format").write_text("BasedOnStyle: LLVM\n", encoding="utf-8")
            (root / ".clang-format-ignore").write_text("third_party/**\n", encoding="utf-8")
            for relative in (
                "modules/BlueSystem/src/Test.cpp",
                "third_party/vendor/Vendor.cpp",
                "build/framework/test.lua",
                "scripts/blue_cli/test.py",
                "benchmarks/tool.py",
            ):
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("x", encoding="utf-8")

            files = blue.collect_format_files(root)

        self.assertEqual([path.name for path in files.cxx], ["Test.cpp"])
        self.assertEqual([path.name for path in files.lua], ["test.lua"])
        self.assertEqual(sorted(path.name for path in files.python), ["test.py", "tool.py"])

    def test_format_returns_nonzero_when_any_formatter_fails(self) -> None:
        root = Path("/repo")
        files = formatting_module.FormatFiles((), (), ())
        tools = formatting_module.FormatTools(("clang-format",), ("stylua",), ("black",))
        args = blue.parse_format_args([], command="format-check")

        with (
            mock.patch.object(formatting_module, "assert_single_root_clang_format"),
            mock.patch.object(formatting_module, "collect_format_files", return_value=files),
            mock.patch.object(formatting_module, "resolve_format_tools", return_value=tools),
            mock.patch.object(
                formatting_module,
                "run_formatter_for_files",
                side_effect=[1, 0, 2],
            ) as run_formatter,
            mock.patch("builtins.print"),
        ):
            result = blue.run_format(root, "linux", "check", args)

        self.assertEqual(result, 1)
        self.assertEqual(run_formatter.call_count, 3)

    def test_format_dispatch_does_not_round_trip_through_premake(self) -> None:
        root = Path("/repo")
        with (
            mock.patch.object(cli_module, "run_format", return_value=0) as run_format,
            mock.patch.object(cli_module, "run_premake", return_value=99) as run_premake,
        ):
            result = blue.dispatch(["format-check"], root=root, host="linux")

        self.assertEqual(result, 0)
        run_format.assert_called_once()
        run_premake.assert_not_called()


if __name__ == "__main__":
    unittest.main()
