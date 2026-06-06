function bb.generate_graph(filename)
    local lines = { "digraph BlueDependencies {", "    rankdir=LR;" }
    for name, _ in pairs(bb.registry.projects) do
        table.insert(lines, string.format('    "%s" [shape=box];', name))
    end
    for name, _ in pairs(bb.registry.dependencies) do
        table.insert(lines, string.format('    "%s" [shape=ellipse];', name))
    end
    for name, node in pairs(bb.registry.projects) do
        for scope, deps in pairs(node.deps or {}) do
            for _, dep in ipairs(deps) do
                table.insert(lines, string.format('    "%s" -> "%s" [label="%s"];', name, dep, scope))
            end
        end
    end
    table.insert(lines, "}")
    bb.fs.write_file(path.join(BLUE_ROOT, filename), table.concat(lines, "\n"))
end
