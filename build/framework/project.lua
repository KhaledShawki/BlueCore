local function normalize(desc)
    assert(type(desc) == "table", "project expects table")
    assert(desc.name, "project requires name")
    assert(desc.kind, "project requires kind")

    desc.root = desc.root or ("modules/" .. desc.name)
    desc.include_dirs = desc.include_dirs or {}
    desc.external_include_dirs = desc.external_include_dirs or {}
    desc.defines = desc.defines or {}
    desc.links = desc.links or {}
    desc.lib_dirs = desc.lib_dirs or {}
    desc.build_options = desc.build_options or {}
    desc.link_options = desc.link_options or {}
    desc.deps = desc.deps or {}

    desc.files = bb.table.as_list(desc.files)
    desc.remove_files = bb.table.as_list(desc.remove_files)
    desc.dependson = bb.table.as_list(desc.dependson)
    desc.prebuildcommands = bb.table.as_list(desc.prebuildcommands)
    desc.postbuildcommands = bb.table.as_list(desc.postbuildcommands)
    desc.debugargs = bb.table.as_list(desc.debugargs)
    desc.group = desc.group or nil
    desc.pch = desc.pch
    desc.pch_header = desc.pch_header or nil
    desc.pch_source = desc.pch_source or nil

    desc.include_dirs.public = bb.table.as_list(desc.public_include_dirs or desc.include_dirs.public)
    desc.include_dirs.private = bb.table.as_list(desc.private_include_dirs or desc.include_dirs.private)
    desc.include_dirs.interface = bb.table.as_list(desc.interface_include_dirs or desc.include_dirs.interface)
    desc.defines.public = bb.table.as_list(desc.public_defines or desc.defines.public)
    desc.defines.private = bb.table.as_list(desc.private_defines or desc.defines.private)
    desc.defines.interface = bb.table.as_list(desc.interface_defines or desc.defines.interface)
    desc.links.public = bb.table.as_list(desc.public_links or desc.links.public)
    desc.links.private = bb.table.as_list(desc.private_links or desc.links.private)
    desc.links.interface = bb.table.as_list(desc.interface_links or desc.links.interface)
    desc.deps.public = bb.table.as_list(desc.deps.public)
    desc.deps.private = bb.table.as_list(desc.deps.private)
    desc.deps.interface = bb.table.as_list(desc.deps.interface)
    return desc
end

local function emit_default_files(desc)
    files {
        path.join(BLUE_ROOT, desc.root .. "/include/**.h"),
        path.join(BLUE_ROOT, desc.root .. "/include/**.hpp"),
        path.join(BLUE_ROOT, desc.root .. "/include/**.inl"),
        path.join(BLUE_ROOT, desc.root .. "/src/**.h"),
        path.join(BLUE_ROOT, desc.root .. "/src/**.cpp"),
        path.join(BLUE_ROOT, desc.root .. "/src/**.c"),
    }
end

local function emit_project_files(desc)
    if desc.default_files ~= false then
        emit_default_files(desc)
    end

    if #desc.files > 0 then
        files(bb.path.to_root_paths(desc.files))
    end

    if #desc.remove_files > 0 then
        removefiles(bb.path.to_root_paths(desc.remove_files))
    end
end

local function emit_project_commands(desc)
    if #desc.dependson > 0 then
        dependson(desc.dependson)
    end

    if #desc.prebuildcommands > 0 then
        prebuildcommands(desc.prebuildcommands)
    end

    if #desc.postbuildcommands > 0 then
        postbuildcommands(desc.postbuildcommands)
    end

    if desc.debugcommand then
        debugcommand(desc.debugcommand)
    end

    if desc.debugdir then
        debugdir(desc.debugdir)
    end

    if #desc.debugargs > 0 then
        debugargs(desc.debugargs)
    end
end

local function emit_platform_rule(rule)
    if rule.files then files(bb.path.to_root_paths(rule.files)) end
    if rule.remove_files then removefiles(bb.path.to_root_paths(rule.remove_files)) end
    if rule.defines then defines(rule.defines) end
    if rule.include_dirs then includedirs(bb.path.to_root_paths(rule.include_dirs)) end
    if rule.external_include_dirs then externalincludedirs(bb.path.to_root_paths(rule.external_include_dirs)) end
    if rule.lib_dirs then libdirs(bb.path.to_root_paths(rule.lib_dirs)) end
    if rule.private_links then links(rule.private_links) end
    if rule.links then links(rule.links) end
    if rule.build_options then buildoptions(rule.build_options) end
    if rule.link_options then linkoptions(rule.link_options) end
    if rule.prebuildcommands then prebuildcommands(rule.prebuildcommands) end
    if rule.postbuildcommands then postbuildcommands(rule.postbuildcommands) end
    if rule.debugcommand then debugcommand(rule.debugcommand) end
    if rule.debugdir then debugdir(rule.debugdir) end
    if rule.debugargs then debugargs(rule.debugargs) end
end

local function find_project_pch(desc)
    if desc.pch == false then
        return nil
    end

    local candidates = {
        {
            header = desc.pch_header or path.join(desc.root, "src/Pch.h"),
            source = desc.pch_source or path.join(desc.root, "src/Pch.cpp"),
            include_dir = path.join(desc.root, "src"),
        },
        {
            header = desc.pch_header or path.join(desc.root, "Pch.h"),
            source = desc.pch_source or path.join(desc.root, "Pch.cpp"),
            include_dir = desc.root,
        },
    }

    for _, candidate in ipairs(candidates) do
        local header = path.join(BLUE_ROOT, candidate.header)
        local source = path.join(BLUE_ROOT, candidate.source)
        if os.isfile(header) and os.isfile(source) then
            return {
                header = candidate.header,
                source = candidate.source,
                include_dir = candidate.include_dir,
            }
        end
    end

    return nil
end

local function emit_project_pch(desc)
    local pch = find_project_pch(desc)
    if not pch then
        return
    end

    files {
        path.join(BLUE_ROOT, pch.header),
        path.join(BLUE_ROOT, pch.source),
    }
    includedirs { path.join(BLUE_ROOT, pch.include_dir) }
    forceincludes { "Pch.h" }
    pchheader "Pch.h"
    pchsource(path.join(BLUE_ROOT, pch.source))
end

function bb.project(desc)
    desc = normalize(desc)
    bb.registry_add_project(desc)

    if desc.group then
        group(desc.group)
    end

    project(desc.name)
        kind(desc.kind)
        if bb.supports_shared_linkage(desc) then
            filter "platforms:x64_DLL"
                kind "SharedLib"
                defines {
                    bb.get_module_build_define(desc.name) .. "=1",
                    bb.get_module_export_define(desc.name) .. "=1",
                }
            filter "platforms:x64"
                kind "StaticLib"
            filter {}
        end
        language "C++"
        location(path.join(BLUE_ROOT, "out/build/" .. (_ACTION or "none") .. "/" .. desc.name))

        emit_project_files(desc)
        emit_project_pch(desc)
        emit_project_commands(desc)

        -- Apply the target's own requirements before exporting usage blocks.
        bb.emit_usage_fields(desc, "private")
        bb.emit_usage_fields(desc, "public")

    -- Platform rules must be emitted in project scope, before usage scopes.
    for platformName, filterName in pairs({ windows = "system:windows", linux = "system:linux", macosx = "system:macosx" }) do
        local rule = desc.platform and desc.platform[platformName]
        if rule then
            filter(filterName)
            emit_platform_rule(rule)
        end
    end

    bb.emit_filters(desc)
    filter {}

    usage "PRIVATE"
        bb.emit_usage_fields(desc, "private")

    usage "PUBLIC"
        bb.emit_usage_fields(desc, "public")

    usage "INTERFACE"
        bb.emit_usage_fields(desc, "interface")
        if desc.kind == "StaticLib" or desc.kind == "SharedLib" then
            links { desc.name }
        end

    filter {}

    if desc.group then
        group ""
    end
end

function bb.static_library(desc)
    desc.kind = "StaticLib"
    bb.project(desc)
end

function bb.shared_library(desc)
    desc.kind = "SharedLib"
    bb.project(desc)
end

function bb.executable(desc)
    desc.kind = desc.kind or "ConsoleApp"
    bb.project(desc)
end
