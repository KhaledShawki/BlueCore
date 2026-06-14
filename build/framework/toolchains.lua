local VISUAL_STUDIO_MSVC_DEFAULTS = {
    vs2019 = {
        toolset = "msc-v142",
        projectVersion = "16.0",
    },
    vs2022 = {
        toolset = "msc-v143",
        projectVersion = "17.0",
    },
    vs2026 = {
        toolset = "msc-v145",
        projectVersion = "18.0",
    },
}


local function trim(value)
    return tostring(value or ""):gsub("^%s+", ""):gsub("%s+$", "")
end

local function capture_command(command)
    local pipe = io.popen(command, "r")
    if not pipe then
        return nil
    end

    local result = pipe:read("*a")
    pipe:close()

    result = trim(result)
    if result == "" then
        return nil
    end

    return result
end

local function get_macos_sdk_path()
    if bb.registry.macos_sdk_path_resolved then
        return bb.registry.macos_sdk_path
    end

    bb.registry.macos_sdk_path_resolved = true

    if os.host() ~= "macosx" then
        return nil
    end

    local sdkPath = capture_command("xcrun --sdk macosx --show-sdk-path 2>/dev/null")
    if not sdkPath then
        error("macOS SDK path could not be resolved. Install Xcode Command Line Tools with: xcode-select --install")
    end

    bb.registry.macos_sdk_path = sdkPath
    return sdkPath
end

local function get_macos_deployment_target()
    return _OPTIONS["macos-deployment-target"] or "11.0"
end

local function apply_macos_clang_sdk_policy()
    local sdkPath = get_macos_sdk_path()
    if not sdkPath then
        return
    end

    local deploymentTarget = get_macos_deployment_target()

    buildoptions {
        "-isysroot " .. sdkPath,
        "-stdlib=libc++",
        "-mmacosx-version-min=" .. deploymentTarget,
        "-pthread",
    }

    linkoptions {
        "-isysroot " .. sdkPath,
        "-stdlib=libc++",
        "-mmacosx-version-min=" .. deploymentTarget,
        "-pthread",
    }
end

local function normalize_msvc_toolset(value)
    value = tostring(value or "")
    if value == "" or value == "auto" or value == "default" or value == "native" then
        return nil
    end

    return value
end

local function get_visual_studio_defaults(actionName)
    return VISUAL_STUDIO_MSVC_DEFAULTS[tostring(actionName or "")]
end

function bb.get_requested_msvc_toolset()
    return normalize_msvc_toolset(_OPTIONS["msvc-toolset"])
end

function bb.get_effective_msvc_toolset()
    if bb.registry.effective_msvc_toolset_resolved then
        return bb.registry.effective_msvc_toolset
    end

    bb.registry.effective_msvc_toolset_resolved = true

    local explicitToolset = bb.get_requested_msvc_toolset()
    if explicitToolset then
        bb.registry.effective_msvc_toolset = explicitToolset
        bb.registry.effective_msvc_toolset_source = "explicit"
        return explicitToolset
    end

    local defaults = get_visual_studio_defaults(_ACTION)
    local defaultToolset = defaults and defaults.toolset or nil
    bb.registry.effective_msvc_toolset = defaultToolset
    bb.registry.effective_msvc_toolset_source = defaultToolset and "visual-studio-action-default" or "toolchain-default"
    return defaultToolset
end

function bb.get_effective_visual_studio_project_version()
    local defaults = get_visual_studio_defaults(_ACTION)
    return defaults and defaults.projectVersion or nil
end

local function has_option_value(optionName)
    local value = _OPTIONS[optionName]
    return value ~= nil and value ~= ""
end

local function apply_msvc_toolset_policy()
    local effectiveToolset = bb.get_effective_msvc_toolset()
    if effectiveToolset then
        toolset(effectiveToolset)
    end

    if has_option_value("msvc-tools-version") then
        toolsversion(_OPTIONS["msvc-tools-version"])
    end

end

function bb.apply_toolchains()
    local selectedToolchain = _OPTIONS["toolchain"] or "default"

    if selectedToolchain == "msvc" then
        filter { "system:windows" }
            apply_msvc_toolset_policy()
    elseif selectedToolchain == "clang" then
        filter { "system:windows or system:linux or system:macosx" }
            toolset "clang"
    elseif selectedToolchain == "gcc" then
        filter { "system:linux" }
            toolset "gcc"
    end

    filter { "system:macosx" }
        apply_macos_clang_sdk_policy()

    filter { "options:toolchain=msvc", "system:windows" }
        warnings "Extra"
        conformancemode "On"
        exceptionhandling "Off"
        rtti "Off"
        defines {
            "_CRT_SECURE_NO_WARNINGS",
            "NOMINMAX",
            "WIN32_LEAN_AND_MEAN",
        }

    filter { "options:toolchain=clang or gcc" }
        warnings "Extra"
        exceptionhandling "Off"
        rtti "Off"
        buildoptions {
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-fno-exceptions",
            "-fno-rtti",
        }

    filter { "options:strict", "options:toolchain=msvc", "system:windows" }
        fatalwarnings { "All" }

    filter { "options:strict", "options:toolchain=clang or gcc" }
        buildoptions { "-Werror" }

    filter { "options:memory-backend=mimalloc" }
        defines { "BLUE_MEMORY_USE_MIMALLOC=1" }

    filter { "options:memory-backend=system" }
        defines { "BLUE_MEMORY_USE_MIMALLOC=0" }

    filter {}
end
