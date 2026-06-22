local function normalize_path(value)
    return (value or ""):gsub("\\", "/")
end

local function path_has_prefix(value, prefix)
    value = normalize_path(value)
    prefix = normalize_path(prefix)
    return value:sub(1, #prefix) == prefix
end

local function ensure_benchmark_source_policy(desc)
    local benchmarkRoot = normalize_path(desc.root .. "/benchmarks/")

    for _, file in ipairs(desc.files) do
        if type(file) == "string" and not path_has_prefix(file, benchmarkRoot) then
            error("Benchmark target '" .. desc.name .. "' includes file outside project-local benchmarks folder: " .. file)
        end
    end
end

local function register_benchmark(desc)
    if bb.registry.benchmarks_by_name[desc.name] then
        error("Duplicate benchmark executable registered: " .. desc.name)
    end

    local moduleName = desc.module or desc.name
    local groupName = desc.group or ("Benchmarks/" .. moduleName)

    local record = {
        name = desc.name,
        module = moduleName,
        root = desc.root,
        group = groupName,
        files = bb.table.copy_array(desc.files),
    }

    table.insert(bb.registry.benchmarks, record)
    bb.registry.benchmarks_by_name[desc.name] = record
end

function bb.benchmark_executable(desc)
    assert(type(desc) == "table", "bb.benchmark_executable expects table")
    assert(desc.name, "benchmark executable requires name")
    assert(desc.root, "benchmark executable requires root")

    desc.kind = "ConsoleApp"
    desc.default_files = false
    desc.files = bb.table.as_list(desc.files)

    local normalizedRoot = normalize_path(desc.root)
    desc.module = desc.module
        or normalizedRoot:match("^modules/([^/]+)")
        or normalizedRoot:match("^apps/([^/]+)")
        or desc.name

    desc.group = desc.group or ("Benchmarks/" .. desc.module)

    if #desc.files == 0 then
        desc.files = {
            desc.root .. "/benchmarks/" .. desc.name .. ".cpp",
        }
    end

    ensure_benchmark_source_policy(desc)
    register_benchmark(desc)

    bb.project(desc)
end

function bb.module_benchmarks(desc)
    assert(type(desc) == "table", "bb.module_benchmarks expects table")
    assert(desc.module, "bb.module_benchmarks requires module")
    assert(desc.root, "bb.module_benchmarks requires root")
    assert(type(desc.benchmarks) == "table", "bb.module_benchmarks requires benchmarks table")

    local defaultDeps = bb.table.as_list(desc.deps)

    for _, benchmarkEntry in ipairs(desc.benchmarks) do
        local benchmarkDesc

        if type(benchmarkEntry) == "string" then
            benchmarkDesc = {
                name = benchmarkEntry,
                files = {
                    desc.root .. "/benchmarks/" .. benchmarkEntry .. ".cpp",
                },
            }
        else
            assert(type(benchmarkEntry) == "table", "module benchmark entry must be string or table")
            assert(benchmarkEntry.name, "module benchmark table entry requires name")

            benchmarkDesc = benchmarkEntry
            benchmarkDesc.files = bb.table.as_list(benchmarkDesc.files)

            if #benchmarkDesc.files == 0 then
                benchmarkDesc.files = {
                    desc.root .. "/benchmarks/" .. benchmarkDesc.name .. ".cpp",
                }
            end
        end

        benchmarkDesc.root = benchmarkDesc.root or desc.root
        benchmarkDesc.module = benchmarkDesc.module or desc.module
        benchmarkDesc.group = benchmarkDesc.group or ("Benchmarks/" .. desc.module)

        benchmarkDesc.deps = benchmarkDesc.deps or {}
        benchmarkDesc.deps.private = bb.table.as_list(benchmarkDesc.deps.private)

        for _, dep in ipairs(defaultDeps) do
            if not bb.table.contains(benchmarkDesc.deps.private, dep) then
                table.insert(benchmarkDesc.deps.private, dep)
            end
        end

        benchmarkDesc.platform = benchmarkDesc.platform or desc.platform

        bb.benchmark_executable(benchmarkDesc)
    end
end

function bb.print_registered_benchmarks()
    if #bb.registry.benchmarks == 0 then
        bb.log.warn("No Blue benchmarks registered")
        return
    end

    bb.log.info("Registered Blue benchmarks:")

    for _, benchmark in ipairs(bb.registry.benchmarks) do
        print("  - " .. benchmark.group .. "/" .. benchmark.name)
    end
end

local function escape_json(value)
    value = tostring(value or "")
    value = value:gsub("\\", "\\\\")
    value = value:gsub('"', '\\"')
    value = value:gsub("\n", "\\n")
    return value
end

function bb.generate_benchmark_manifest(filename)
    local lines = {}

    table.insert(lines, "{")
    table.insert(lines, '  "benchmarks": [')

    for index, benchmark in ipairs(bb.registry.benchmarks) do
        local suffix = index == #bb.registry.benchmarks and "" or ","

        table.insert(lines, "    {")
        table.insert(lines, '      "name": "' .. escape_json(benchmark.name) .. '",')
        table.insert(lines, '      "module": "' .. escape_json(benchmark.module) .. '",')
        table.insert(lines, '      "group": "' .. escape_json(benchmark.group) .. '"')
        table.insert(lines, "    }" .. suffix)
    end

    table.insert(lines, "  ]")
    table.insert(lines, "}")

    bb.fs.write_file(path.join(BLUE_ROOT, filename), table.concat(lines, "\n"))
end
