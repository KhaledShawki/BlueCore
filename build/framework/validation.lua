local function collect_deps(node)
    local result = {}
    if not node or not node.deps then
        return result
    end
    bb.table.append_unique(result, node.deps.private)
    bb.table.append_unique(result, node.deps.public)
    bb.table.append_unique(result, node.deps.interface)
    return result
end

local function validate_workspace()
    if not bb.registry.workspace then
        error("No workspace declared")
    end
end

local function validate_references()
    local function validate_node(name, node)
        for _, depName in ipairs(collect_deps(node)) do
            if not bb.registry_has_node(depName) then
                error("Unknown dependency '" .. depName .. "' referenced by '" .. name .. "'")
            end
        end
    end

    for name, node in pairs(bb.registry.projects) do
        validate_node(name, node)
    end
    for name, node in pairs(bb.registry.dependencies) do
        validate_node(name, node)
    end
end

local function validate_no_cycles()
    local visiting = {}
    local visited = {}

    local function visit(name, stack)
        if visiting[name] then
            table.insert(stack, name)
            error("Dependency cycle detected: " .. table.concat(stack, " -> "))
        end
        if visited[name] then
            return
        end
        visiting[name] = true
        table.insert(stack, name)
        local node = bb.registry_get_node(name)
        for _, depName in ipairs(collect_deps(node)) do
            visit(depName, bb.table.copy_array(stack))
        end
        visiting[name] = nil
        visited[name] = true
    end

    for name, _ in pairs(bb.registry.projects) do
        visit(name, {})
    end
end

function bb.validate_all()
    validate_workspace()
    validate_references()
    validate_no_cycles()
end
