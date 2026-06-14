local function camel_to_define_suffix(value)
    value = tostring(value or "")
    value = value:gsub("([A-Z]+)([A-Z][a-z])", "%1_%2")
    value = value:gsub("([a-z0-9])([A-Z])", "%1_%2")
    value = value:gsub("[^A-Za-z0-9]+", "_")
    value = value:gsub("^_+", "")
    value = value:gsub("_+$", "")
    return value:upper()
end

local BUILD_PROFILES = {
    "Debug",
    "Release",
    "Profile",
    "Shipping",
}

local BUILD_PLATFORMS = {
    x64 = {
        architecture = "x86_64",
        output = "x64",
        shared = false,
    },
    x64_DLL = {
        architecture = "x86_64",
        output = "x64_DLL",
        shared = true,
    },
}


local function is_ninja_action()
    return _ACTION == "ninja"
end

local function selected_build_platforms_option()
    local selected = _OPTIONS and _OPTIONS["blue-build-platforms"] or nil
    if selected == nil or selected == "" then
        return "default"
    end

    return tostring(selected)
end

local function copy_selected_build_platforms(selected)
    if selected == "all" then
        return bb.get_build_platforms()
    end

    if BUILD_PLATFORMS[selected] ~= nil then
        return { selected }
    end

    error("Unknown Blue build platform selection: " .. tostring(selected))
end

function bb.get_build_profiles()
    return bb.table.copy_array(BUILD_PROFILES)
end

function bb.get_build_platforms()
    return {
        "x64",
        "x64_DLL",
    }
end

function bb.get_default_build_configurations()
    return bb.get_build_profiles()
end

function bb.get_default_build_platforms()
    local selected = selected_build_platforms_option()

    if selected ~= "default" then
        if is_ninja_action() and selected ~= "x64" then
            error("Premake Ninja generation currently supports only the x64 build platform. " ..
                  "Use --blue-build-platforms=x64 for Ninja, or use Visual Studio/gmake2 for x64_DLL/shared-library matrix builds.")
        end

        return copy_selected_build_platforms(selected)
    end

    if is_ninja_action() then
        return { "x64" }
    end

    return bb.get_build_platforms()
end

function bb.get_build_platform(platformName)
    return BUILD_PLATFORMS[tostring(platformName or "")]
end

function bb.get_build_platform_output(platformName)
    local platform = bb.get_build_platform(platformName)
    if platform ~= nil then
        return platform.output
    end

    return tostring(platformName or "")
end

function bb.is_shared_build_platform(platformName)
    local platform = bb.get_build_platform(platformName)
    return platform ~= nil and platform.shared == true
end

function bb.get_module_define_suffix(moduleName)
    return camel_to_define_suffix(moduleName)
end

function bb.get_module_build_define(moduleName)
    return "BLUE_BUILD_" .. bb.get_module_define_suffix(moduleName)
end

function bb.get_module_export_define(moduleName)
    return "BLUE_EXPORT_" .. tostring(moduleName)
end

function bb.supports_shared_linkage(desc)
    return desc.kind == "StaticLib" and desc.shared ~= false
end
