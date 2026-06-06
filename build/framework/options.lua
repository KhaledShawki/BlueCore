function bb.load_options()
    newoption {
        trigger = "toolchain",
        value = "TOOLCHAIN",
        description = "Select compiler toolchain",
        default = "default",
        allowed = {
            { "default", "Use generator default" },
            { "msvc", "Microsoft Visual C++" },
            { "clang", "Clang / Apple Clang" },
            { "gcc", "GNU GCC" },
        }
    }

    newoption {
        trigger = "blue-platforms",
        value = "SET",
        description = "Select target OS/platform set. Auto picks the host/generator-safe target. Visual Studio uses canonical x64.",
        default = "auto",
        allowed = {
            { "auto", "Use framework defaults for the selected action/toolchain" },
            { "all", "Generate all declared platforms" },
            { "windows", "Generate Windows x64 only" },
            { "linux", "Generate Linux x64 only" },
            { "macos", "Generate macOS Universal only" },
        }
    }

    newoption {
        trigger = "msvc-toolset",
        value = "TOOLSET",
        description = "Explicitly override the Visual Studio MSVC platform toolset, for example v143, v145, msc-v143, or msc-v145. When omitted, Blue selects the default MSVC toolset for the requested Visual Studio generator."
    }

    newoption {
        trigger = "msvc-tools-version",
        value = "VERSION",
        description = "Optional exact MSVC tools version, for example 14.50. Use only when that version is installed."
    }

    newoption {
        trigger = "strict",
        description = "Treat warnings as errors"
    }

    newoption {
        trigger = "memory-backend",
        value = "BACKEND",
        description = "Select BlueMemory low-level backend",
        default = "system",
        allowed = {
            { "system", "Use system allocation backend" },
            { "mimalloc", "Use mimalloc backend" },
        }
    }

    newoption {
        trigger = "format-path",
        value = "PATH",
        description = "Optional path or command name for clang-format. If omitted, Blue resolves repo-local tools, LLVM install paths, then PATH."
    }


    newoption {
        trigger = "blue-startup",
        value = "PROJECT",
        description = "Override the generated solution startup project. Use BlueRunTests to run all tests from Visual Studio/IDE."
    }


    newoption {
        trigger = "regen-action",
        value = "ACTION",
        description = "Generation action whose build-graph token should be checked or updated, for example vs2026, vs2022, ninja, or gmake2."
    }
end
