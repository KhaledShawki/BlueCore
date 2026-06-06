local function normalize(desc)
    assert(type(desc) == "table", "dependency expects table")
    assert(desc.name, "dependency requires name")

    desc.kind = desc.kind or "External"
    desc.include_dirs = desc.include_dirs or {}
    desc.external_include_dirs = desc.external_include_dirs or {}
    desc.defines = desc.defines or {}
    desc.lib_dirs = desc.lib_dirs or {}
    desc.links = desc.links or {}
    desc.build_options = desc.build_options or {}
    desc.link_options = desc.link_options or {}
    desc.deps = desc.deps or {}

    desc.include_dirs.interface = bb.table.as_list(desc.include_dirs.interface)
    desc.external_include_dirs.interface = bb.table.as_list(desc.external_include_dirs.interface)
    desc.defines.interface = bb.table.as_list(desc.defines.interface)
    desc.lib_dirs.interface = bb.table.as_list(desc.lib_dirs.interface)
    desc.links.interface = bb.table.as_list(desc.links.interface)
    desc.build_options.interface = bb.table.as_list(desc.build_options.interface)
    desc.link_options.interface = bb.table.as_list(desc.link_options.interface)
    desc.deps.interface = bb.table.as_list(desc.deps.interface)

    return desc
end

function bb.dependency(desc)
    desc = normalize(desc)
    bb.registry_add_dependency(desc)

    -- External dependencies are expanded into consumers instead of emitted as solution projects.
end
