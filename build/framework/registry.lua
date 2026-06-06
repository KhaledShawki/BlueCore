bb.registry = bb.registry or {
    workspace = nil,
    projects = {},
    dependencies = {},
    tests = {},
    tests_by_name = {},
    test_runner_emitted = false,
}

function bb.registry_has_node(name)
    return bb.registry.projects[name] ~= nil or bb.registry.dependencies[name] ~= nil
end

function bb.registry_get_node(name)
    return bb.registry.projects[name] or bb.registry.dependencies[name]
end

function bb.registry_add_project(desc)
    if bb.registry.projects[desc.name] then
        error("Duplicate project registered: " .. desc.name)
    end

    if bb.registry.dependencies[desc.name] then
        error("Project name conflicts with dependency: " .. desc.name)
    end

    bb.registry.projects[desc.name] = desc
end

function bb.registry_add_dependency(desc)
    if bb.registry.dependencies[desc.name] then
        error("Duplicate dependency registered: " .. desc.name)
    end

    if bb.registry.projects[desc.name] then
        error("Dependency name conflicts with project: " .. desc.name)
    end

    bb.registry.dependencies[desc.name] = desc
end
