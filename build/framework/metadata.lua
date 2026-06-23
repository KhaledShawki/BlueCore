function bb.generate_metadata(filename)
    local lines = { "{", '  "workspace": "' .. bb.registry.workspace.name .. '",', '  "projects": [' }
    local first = true
    for name, node in pairs(bb.registry.projects) do
        if not first then
            table.insert(lines, ",")
        end
        first = false
        table.insert(lines, "    {")
        table.insert(lines, '      "name": "' .. name .. '",')
        table.insert(lines, '      "kind": "' .. node.kind .. '"')
        table.insert(lines, "    }")
    end
    table.insert(lines, "")
    table.insert(lines, "  ]")
    table.insert(lines, "}")
    bb.fs.write_file(path.join(BLUE_ROOT, filename), table.concat(lines, "\n"))
end
