local function apply(fn, values)
    values = bb.table.as_list(values)
    if #values > 0 then
        fn(values)
    end
end

local function apply_paths(fn, values)
    values = bb.path.to_root_paths(values)
    if #values > 0 then
        fn(values)
    end
end

local function emit_external_dependency(desc, visited)
    visited = visited or {}

    if visited[desc.name] then
        return
    end

    visited[desc.name] = true

    if desc.deps and desc.deps.interface then
        bb.emit_dependency_list(desc.deps.interface, visited)
    end

    if desc.include_dirs and desc.include_dirs.interface then
        apply_paths(includedirs, desc.include_dirs.interface)
    end

    if desc.external_include_dirs and desc.external_include_dirs.interface then
        apply_paths(externalincludedirs, desc.external_include_dirs.interface)
    end

    if desc.defines and desc.defines.interface then
        apply(defines, desc.defines.interface)
    end

    if desc.lib_dirs and desc.lib_dirs.interface then
        apply_paths(libdirs, desc.lib_dirs.interface)
    end

    if desc.links and desc.links.interface then
        apply(links, desc.links.interface)
    end

    if desc.build_options and desc.build_options.interface then
        apply(buildoptions, desc.build_options.interface)
    end

    if desc.link_options and desc.link_options.interface then
        apply(linkoptions, desc.link_options.interface)
    end

    bb.emit_filters(desc, visited)
end

function bb.emit_dependency_reference(name, visited)
    visited = visited or {}

    if bb.registry.projects[name] then
        uses { name }
        return
    end

    local dependency = bb.registry.dependencies[name]

    if dependency then
        emit_external_dependency(dependency, visited)
        return
    end

    error("Unknown dependency: " .. tostring(name))
end

function bb.emit_dependency_list(values, visited)
    for _, name in ipairs(bb.table.as_list(values)) do
        bb.emit_dependency_reference(name, visited)
    end
end

function bb.emit_usage_fields(desc, scope)
    if desc.include_dirs and desc.include_dirs[scope] then
        apply_paths(includedirs, desc.include_dirs[scope])
    end

    if desc.external_include_dirs and desc.external_include_dirs[scope] then
        apply_paths(externalincludedirs, desc.external_include_dirs[scope])
    end

    if desc.defines and desc.defines[scope] then
        apply(defines, desc.defines[scope])
    end

    if desc.lib_dirs and desc.lib_dirs[scope] then
        apply_paths(libdirs, desc.lib_dirs[scope])
    end

    if desc.links and desc.links[scope] then
        apply(links, desc.links[scope])
    end

    if desc.build_options and desc.build_options[scope] then
        apply(buildoptions, desc.build_options[scope])
    end

    if desc.link_options and desc.link_options[scope] then
        apply(linkoptions, desc.link_options[scope])
    end

    if desc.deps and desc.deps[scope] then
        bb.emit_dependency_list(desc.deps[scope])
    end
end

function bb.emit_filters(desc, visited)
    for _, rule in ipairs(desc.filters or {}) do
        assert(rule.when, "filter rule requires 'when'")

        filter(rule.when)

        apply_paths(includedirs, rule.include_dirs)
        apply_paths(externalincludedirs, rule.external_include_dirs)
        apply(defines, rule.defines)
        apply_paths(libdirs, rule.lib_dirs)
        apply(links, rule.links)
        apply(buildoptions, rule.build_options)
        apply(linkoptions, rule.link_options)
        bb.emit_dependency_list(rule.deps, visited)
    end

    filter {}
end
