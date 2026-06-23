local function is_visual_studio_action()
    return _ACTION == "vs2019" or _ACTION == "vs2022" or _ACTION == "vs2026"
end

function bb.resolve_target_os()
    local selected = _OPTIONS["blue-platforms"] or "auto"

    if selected == "windows" or selected == "linux" or selected == "macos" then
        return selected
    end

    local selectedToolchain = _OPTIONS["toolchain"] or "default"

    if selectedToolchain == "msvc" then
        return "windows"
    end

    if is_visual_studio_action() then
        return "windows"
    end

    local host = os.host()
    if host == "windows" then
        return "windows"
    elseif host == "macosx" then
        return "macos"
    end

    return "linux"
end

function bb.workspace(desc)
    assert(type(desc) == "table", "bb.workspace expects a table")
    assert(desc.name, "workspace requires name")

    if bb.registry.workspace then
        error("Only one workspace may be declared")
    end

    desc.targetOs = bb.resolve_target_os()
    desc.configurations = desc.configurations or bb.get_default_build_configurations()
    desc.platforms = desc.platforms or bb.get_default_build_platforms()
    bb.registry.workspace = desc
    bb.visual_studio.install_slnx_platform_axis_mapping(desc)

    workspace(desc.name)
    location(path.join(BLUE_ROOT, "out/build/" .. (_ACTION or "none")))
    startproject(_OPTIONS["blue-startup"] or desc.startproject or "")
    configurations(desc.configurations)
    platforms(desc.platforms)
    defaultplatform("x64")
    language("C++")
    cppdialect(desc.cppdialect or "C++20")
    staticruntime("Off")
    targetdir(path.join(BLUE_ROOT, "out/bin/%{cfg.system}/%{cfg.platform}/%{cfg.buildcfg}"))
    objdir(path.join(BLUE_ROOT, "out/obj/%{cfg.system}/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}"))

    bb.apply_platforms()
    bb.apply_profiles()
    bb.apply_toolchains()
    filter({})
end
