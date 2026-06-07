local function quote(value)
    return '"' .. value .. '"'
end

local function normalize_path(value)
    return (value or ""):gsub("\\", "/")
end

local function path_has_prefix(value, prefix)
    value = normalize_path(value)
    prefix = normalize_path(prefix)
    return value:sub(1, #prefix) == prefix
end

local function get_test_binary_path(testName, executableExtension)
    return path.join("%{cfg.targetdir}", testName .. executableExtension)
end

local function get_runner_binary_path(executableExtension)
    return path.join("%{cfg.targetdir}", "BlueRunTests" .. executableExtension)
end

local function get_executable_extension_for_platform(platformName)
    if platformName == "windows" then
        return ".exe"
    end

    return ""
end

local function make_runner_arguments(executableExtension)
    local args = {}

    for _, test in ipairs(bb.registry.tests) do
        table.insert(args, get_test_binary_path(test.name, executableExtension))
    end

    return args
end

local function make_runner_command(executableExtension)
    local parts = {
        quote(get_runner_binary_path(executableExtension)),
    }

    for _, testPath in ipairs(make_runner_arguments(executableExtension)) do
        table.insert(parts, quote(testPath))
    end

    return table.concat(parts, " ")
end

local function ensure_test_source_policy(desc)
    local testRoot = normalize_path(desc.root .. "/tests/")

    for _, file in ipairs(desc.files) do
        if type(file) == "string" and not path_has_prefix(file, testRoot) then
            error("Test target '" .. desc.name .. "' includes file outside project-local tests folder: " .. file)
        end
    end
end

local function register_test(desc)
    if bb.registry.tests_by_name[desc.name] then
        error("Duplicate test executable registered: " .. desc.name)
    end

    local moduleName = desc.module or desc.name
    local groupName = desc.group or ("Tests/" .. moduleName)

    local record = {
        name = desc.name,
        module = moduleName,
        root = desc.root,
        group = groupName,
        files = bb.table.copy_array(desc.files),
    }

    table.insert(bb.registry.tests, record)
    bb.registry.tests_by_name[desc.name] = record
end

function bb.test_executable(desc)
    assert(type(desc) == "table", "bb.test_executable expects table")
    assert(desc.name, "test executable requires name")
    assert(desc.root, "test executable requires root")

    desc.kind = "ConsoleApp"
    desc.default_files = false
    desc.files = bb.table.as_list(desc.files)
    local normalizedRoot = normalize_path(desc.root)
    desc.module = desc.module or normalizedRoot:match("^modules/([^/]+)") or normalizedRoot:match("^apps/([^/]+)") or normalizedRoot:match("^tests/([^/]+)") or desc.name
    desc.group = desc.group or ("Tests/" .. desc.module)

    if #desc.files == 0 then
        desc.files = {
            desc.root .. "/tests/" .. desc.name .. ".cpp",
        }
    end

    ensure_test_source_policy(desc)
    register_test(desc)

    bb.project(desc)
end

function bb.module_tests(desc)
    assert(type(desc) == "table", "bb.module_tests expects table")
    assert(desc.module, "bb.module_tests requires module")
    assert(desc.root, "bb.module_tests requires root")
    assert(type(desc.tests) == "table", "bb.module_tests requires tests table")

    local defaultDeps = bb.table.as_list(desc.deps)

    for _, testEntry in ipairs(desc.tests) do
        local testDesc

        if type(testEntry) == "string" then
            testDesc = {
                name = testEntry,
                files = {
                    desc.root .. "/tests/" .. testEntry .. ".cpp",
                },
            }
        else
            assert(type(testEntry) == "table", "module test entry must be string or table")
            assert(testEntry.name, "module test table entry requires name")
            testDesc = testEntry
            testDesc.files = bb.table.as_list(testDesc.files)

            if #testDesc.files == 0 then
                testDesc.files = {
                    desc.root .. "/tests/" .. testDesc.name .. ".cpp",
                }
            end
        end

        testDesc.root = testDesc.root or desc.root
        testDesc.module = testDesc.module or desc.module
        testDesc.group = testDesc.group or ("Tests/" .. desc.module)

        testDesc.deps = testDesc.deps or {}
        testDesc.deps.private = bb.table.as_list(testDesc.deps.private)
        for _, dep in ipairs(defaultDeps) do
            if not bb.table.contains(testDesc.deps.private, dep) then
                table.insert(testDesc.deps.private, dep)
            end
        end

        testDesc.platform = testDesc.platform or desc.platform
        bb.test_executable(testDesc)
    end
end

function bb.print_registered_tests()
    if #bb.registry.tests == 0 then
        bb.log.warn("No Blue tests registered")
        return
    end

    bb.log.info("Registered Blue tests:")
    for _, test in ipairs(bb.registry.tests) do
        print("  - " .. test.group .. "/" .. test.name)
    end
end

local function escape_json(value)
    value = tostring(value or "")
    value = value:gsub("\\", "\\\\")
    value = value:gsub('"', '\\"')
    value = value:gsub("\n", "\\n")
    return value
end

function bb.generate_test_manifest(filename)
    local lines = {}
    table.insert(lines, "{")
    table.insert(lines, '  "tests": [')

    for index, test in ipairs(bb.registry.tests) do
        local suffix = index == #bb.registry.tests and "" or ","
        table.insert(lines, "    {")
        table.insert(lines, '      "name": "' .. escape_json(test.name) .. '",')
        table.insert(lines, '      "module": "' .. escape_json(test.module) .. '",')
        table.insert(lines, '      "group": "' .. escape_json(test.group) .. '"')
        table.insert(lines, "    }" .. suffix)
    end

    table.insert(lines, "  ]")
    table.insert(lines, "}")

    bb.fs.write_file(path.join(BLUE_ROOT, filename), table.concat(lines, "\n"))
end

function bb.emit_test_runner_project()
    if bb.registry.test_runner_emitted then
        return
    end

    bb.registry.test_runner_emitted = true

    if #bb.registry.tests == 0 then
        return
    end

    local testNames = {}
    for _, test in ipairs(bb.registry.tests) do
        table.insert(testNames, test.name)
    end

    local platformRules = {}
    for _, platformName in ipairs({ "windows", "linux", "macosx" }) do
        local extension = get_executable_extension_for_platform(platformName)
        platformRules[platformName] = {
            debugargs = make_runner_arguments(extension),
            postbuildcommands = {
                make_runner_command(extension),
            },
        }
    end

    bb.executable {
        name = "BlueRunTests",
        root = "tools/BlueTestRunner",
        group = "Tests/Runner",
        default_files = false,
        files = {
            "tools/BlueTestRunner/BlueTestRunner.cpp",
        },
        private_include_dirs = {
            "modules/BlueSystem/include",
        },
        dependson = testNames,
        debugdir = BLUE_ROOT,
        platform = platformRules,
        ide = {
            run = {
                enabled = false,
            },
        },
    }
end
