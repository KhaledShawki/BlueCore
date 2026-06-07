bb.clion = bb.clion or {}

local TARGET_SYSTEM_BY_OS = {
    windows = "windows",
    linux = "linux",
    macos = "macosx",
}

local DEFAULT_CONFIG = "Debug"
local DEFAULT_PLATFORM = "x64"

local OPTION_DEFAULTS = {
    ["blue-platforms"] = "auto",
    ["toolchain"] = "default",
    ["memory-backend"] = "system",
    ["clion-config"] = "default",
    ["clion-platform"] = "default",
    ["clion-idea"] = "on",
    ["clion-run-targets"] = "default",
    ["clion-build-targets"] = "default",
}

local CONFIG_DEFINES = {
    Debug = {
        "BLUE_DEBUG=1",
        "BLUE_ENABLE_ASSERTS=1",
        "BLUE_ENABLE_LOGGING=1",
        "BLUE_ENABLE_MEMORY_TRACKING=1",
    },
    Release = {
        "BLUE_RELEASE=1",
        "NDEBUG",
        "BLUE_ENABLE_ASSERTS=0",
        "BLUE_ENABLE_LOGGING=1",
        "BLUE_ENABLE_MEMORY_TRACKING=0",
    },
    Profile = {
        "BLUE_PROFILE=1",
        "NDEBUG",
        "BLUE_ENABLE_ASSERTS=1",
        "BLUE_ENABLE_LOGGING=1",
        "BLUE_ENABLE_MEMORY_TRACKING=1",
        "BLUE_ENABLE_PROFILING=1",
    },
    Shipping = {
        "BLUE_SHIPPING=1",
        "NDEBUG",
        "BLUE_ENABLE_ASSERTS=0",
        "BLUE_ENABLE_LOGGING=0",
        "BLUE_ENABLE_MEMORY_TRACKING=0",
    },
}

local function option_value(name)
    local value = _OPTIONS and _OPTIONS[name] or nil
    if value == nil or value == "" then
        return OPTION_DEFAULTS[name]
    end

    return value
end

local function normalize_slashes(value)
    return tostring(value or ""):gsub("\\", "/")
end

local function quote_shell_argument(value)
    local text = tostring(value or "")
    text = text:gsub("\\", "\\\\"):gsub('"', '\\"')
    return '"' .. text .. '"'
end

local function json_escape(value)
    local text = tostring(value or "")
    text = text:gsub("\\", "\\\\")
    text = text:gsub('"', '\\"')
    text = text:gsub("\b", "\\b")
    text = text:gsub("\f", "\\f")
    text = text:gsub("\n", "\\n")
    text = text:gsub("\r", "\\r")
    text = text:gsub("\t", "\\t")
    return text
end

local function append_unique(target, values)
    local seen = {}
    for _, value in ipairs(target) do
        seen[tostring(value)] = true
    end

    for _, value in ipairs(bb.table.as_list(values)) do
        local key = tostring(value)
        if key ~= "" and not seen[key] then
            table.insert(target, value)
            seen[key] = true
        end
    end
end

local function list_contains(values, candidate)
    for _, value in ipairs(values or {}) do
        if value == candidate then
            return true
        end
    end

    return false
end

local function select_requested_values(optionName, available, defaultValue)
    local requested = option_value(optionName)
    if requested == nil or requested == "" or requested == "all" then
        return bb.table.copy_array(available)
    end

    if requested == "default" then
        return { defaultValue }
    end

    if not list_contains(available, requested) then
        error("Invalid " .. optionName .. " value for CLion generation: " .. tostring(requested))
    end

    return { requested }
end

local function get_target_system()
    local targetOs = bb.resolve_target_os and bb.resolve_target_os() or "linux"
    return TARGET_SYSTEM_BY_OS[targetOs] or targetOs
end

local function get_target_os_name(targetSystem)
    if targetSystem == "macosx" then
        return "macos"
    end

    return targetSystem
end

local function get_absolute_path(value)
    if value == nil or value == "" then
        return nil
    end

    local rootPath = bb.path.to_root_path(value)
    return normalize_slashes(path.getabsolute(rootPath))
end

local function get_relative_path(value)
    local absoluteRoot = normalize_slashes(path.getabsolute(BLUE_ROOT))
    local absoluteValue = normalize_slashes(path.getabsolute(value))
    local prefix = absoluteRoot .. "/"

    if absoluteValue:sub(1, #prefix) == prefix then
        return absoluteValue:sub(#prefix + 1)
    end

    return absoluteValue
end

local function is_source_file(filename)
    local lower = tostring(filename or ""):lower()
    return lower:match("%.c$") ~= nil
        or lower:match("%.cc$") ~= nil
        or lower:match("%.cpp$") ~= nil
        or lower:match("%.cxx$") ~= nil
        or lower:match("%.m$") ~= nil
        or lower:match("%.mm$") ~= nil
end

local function is_c_source_file(filename)
    local lower = tostring(filename or ""):lower()
    return lower:match("%.c$") ~= nil
end

local function expand_pattern(patternValue)
    local filename = bb.path.to_root_path(patternValue)
    local matches = os.matchfiles(filename)

    if #matches > 0 then
        return matches
    end

    if os.isfile(filename) then
        return { filename }
    end

    return {}
end

local function add_source_patterns(result, patterns)
    for _, patternValue in ipairs(bb.table.as_list(patterns)) do
        for _, filename in ipairs(expand_pattern(patternValue)) do
            if is_source_file(filename) then
                result[normalize_slashes(path.getabsolute(filename))] = true
            end
        end
    end
end

local function remove_source_patterns(result, patterns)
    for _, patternValue in ipairs(bb.table.as_list(patterns)) do
        for _, filename in ipairs(expand_pattern(patternValue)) do
            result[normalize_slashes(path.getabsolute(filename))] = nil
        end
    end
end

local function collect_default_sources(desc, result)
    local root = tostring(desc.root or "")
    add_source_patterns(result, {
        root .. "/src/**.c",
        root .. "/src/**.cc",
        root .. "/src/**.cpp",
        root .. "/src/**.cxx",
        root .. "/src/**.m",
        root .. "/src/**.mm",
    })
end

local function split_or_terms(value)
    local text = tostring(value or "")
    local result = {}
    local startIndex = 1

    while true do
        local splitStart, splitEnd = text:find("%s+or%s+", startIndex)
        if not splitStart then
            break
        end

        table.insert(result, text:sub(startIndex, splitStart - 1))
        startIndex = splitEnd + 1
    end

    table.insert(result, text:sub(startIndex))
    return result
end

local function matches_single_filter_term(context, term)
    term = tostring(term or "")

    local optionName, optionValue = term:match("^options:([^=]+)=(.+)$")
    if optionName then
        return tostring(option_value(optionName)) == optionValue
    end

    optionName = term:match("^options:(.+)$")
    if optionName then
        return _OPTIONS and _OPTIONS[optionName] ~= nil and _OPTIONS[optionName] ~= false
    end

    local systemName = term:match("^system:(.+)$")
    if systemName then
        return context.system == systemName
    end

    local platformName = term:match("^platforms:(.+)$")
    if platformName then
        return context.platform == platformName
    end

    local configurationName = term:match("^configurations:(.+)$")
    if configurationName then
        return context.configuration == configurationName
    end

    return false
end

local function matches_filter_term(context, term)
    local prefix, values = tostring(term or ""):match("^([^:]+):(.+)$")
    if not prefix or not values or not values:find("%s+or%s+") then
        return matches_single_filter_term(context, term)
    end

    for _, value in ipairs(split_or_terms(values)) do
        if matches_single_filter_term(context, prefix .. ":" .. value) then
            return true
        end
    end

    return false
end

local function is_filter_active(context, filterExpression)
    if filterExpression == nil then
        return true
    end

    if type(filterExpression) == "string" then
        return matches_filter_term(context, filterExpression)
    end

    for _, term in ipairs(bb.table.as_list(filterExpression)) do
        if not matches_filter_term(context, term) then
            return false
        end
    end

    return true
end

local function append_rule_fields(fields, rule)
    if not rule then
        return
    end

    append_unique(fields.include_dirs, rule.include_dirs)
    append_unique(fields.external_include_dirs, rule.external_include_dirs)
    append_unique(fields.defines, rule.defines)
    append_unique(fields.build_options, rule.build_options)
end

local function append_scoped_fields(fields, desc, scope)
    if not desc then
        return
    end

    if desc.include_dirs and desc.include_dirs[scope] then
        append_unique(fields.include_dirs, desc.include_dirs[scope])
    end

    if desc.external_include_dirs and desc.external_include_dirs[scope] then
        append_unique(fields.external_include_dirs, desc.external_include_dirs[scope])
    end

    if desc.defines and desc.defines[scope] then
        append_unique(fields.defines, desc.defines[scope])
    end

    if desc.build_options and desc.build_options[scope] then
        append_unique(fields.build_options, desc.build_options[scope])
    end
end

local function append_active_filter_fields(fields, desc, context)
    for _, rule in ipairs(desc.filters or {}) do
        if is_filter_active(context, rule.when) then
            append_rule_fields(fields, rule)
        end
    end
end

local function append_dependency_names(result, desc, context, scopes)
    for _, scope in ipairs(scopes) do
        if desc.deps and desc.deps[scope] then
            append_unique(result, desc.deps[scope])
        end
    end

    for _, rule in ipairs(desc.filters or {}) do
        if is_filter_active(context, rule.when) then
            append_unique(result, rule.deps)
        end
    end
end

local function collect_dependency_usage(name, fields, context, visited)
    visited = visited or {}
    if visited[name] then
        return
    end

    visited[name] = true

    local desc = bb.registry_get_node(name)
    if not desc then
        error("Unknown CLion dependency: " .. tostring(name))
    end

    append_scoped_fields(fields, desc, "public")
    append_scoped_fields(fields, desc, "interface")
    append_active_filter_fields(fields, desc, context)

    local childDeps = {}
    append_dependency_names(childDeps, desc, context, { "public", "interface" })

    for _, childName in ipairs(childDeps) do
        collect_dependency_usage(childName, fields, context, visited)
    end
end

local function collect_project_dependencies(desc, context)
    local deps = {}
    append_dependency_names(deps, desc, context, { "private", "public" })
    return deps
end

local function collect_project_fields(desc, context)
    local fields = {
        include_dirs = {},
        external_include_dirs = {},
        defines = {},
        build_options = {},
    }

    append_scoped_fields(fields, desc, "private")
    append_scoped_fields(fields, desc, "public")
    append_rule_fields(fields, desc.platform and desc.platform[context.system])
    append_active_filter_fields(fields, desc, context)

    for _, dependencyName in ipairs(collect_project_dependencies(desc, context)) do
        collect_dependency_usage(dependencyName, fields, context, {})
    end

    return fields
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
        if os.isfile(path.join(BLUE_ROOT, candidate.header)) and os.isfile(path.join(BLUE_ROOT, candidate.source)) then
            return candidate
        end
    end

    return nil
end

local function collect_project_sources(desc, context)
    local result = {}

    if desc.default_files ~= false then
        collect_default_sources(desc, result)
    end

    add_source_patterns(result, desc.files)
    remove_source_patterns(result, desc.remove_files)

    local platformRule = desc.platform and desc.platform[context.system]
    if platformRule then
        add_source_patterns(result, platformRule.files)
        remove_source_patterns(result, platformRule.remove_files)
    end

    for _, rule in ipairs(desc.filters or {}) do
        if is_filter_active(context, rule.when) then
            add_source_patterns(result, rule.files)
            remove_source_patterns(result, rule.remove_files)
        end
    end

    local sources = {}
    for filename in pairs(result) do
        table.insert(sources, filename)
    end
    table.sort(sources)

    return sources
end

local function append_defines(target, defines)
    append_unique(target, defines)
end

local function get_global_defines(desc, context)
    local defines = {}

    append_defines(defines, CONFIG_DEFINES[context.configuration])

    if option_value("memory-backend") == "mimalloc" then
        table.insert(defines, "BLUE_MEMORY_USE_MIMALLOC=1")
    else
        table.insert(defines, "BLUE_MEMORY_USE_MIMALLOC=0")
    end

    if context.platform == "x64_DLL" then
        table.insert(defines, "BLUE_SHARED_LIBRARY=1")
        if bb.supports_shared_linkage(desc) then
            table.insert(defines, bb.get_module_build_define(desc.name) .. "=1")
            table.insert(defines, bb.get_module_export_define(desc.name) .. "=1")
        end
    end

    if context.system == "windows" then
        table.insert(defines, "_CRT_SECURE_NO_WARNINGS")
        table.insert(defines, "NOMINMAX")
        table.insert(defines, "WIN32_LEAN_AND_MEAN")
    end

    return defines
end

local function get_compiler(context, isCFile)
    local selectedToolchain = option_value("toolchain")

    if context.system == "windows" then
        if selectedToolchain == "clang" then
            return isCFile and "clang-cl" or "clang-cl"
        end

        return "cl.exe"
    end

    if selectedToolchain == "gcc" then
        return isCFile and "gcc" or "g++"
    end

    return isCFile and "clang" or "clang++"
end

local function make_msvc_arguments(desc, fields, context, sourceFile)
    local args = {
        get_compiler(context, is_c_source_file(sourceFile)),
        is_c_source_file(sourceFile) and "/TC" or "/TP",
        "/nologo",
        "/c",
        "/std:c++20",
        "/W4",
        "/GR-",
        "/EHs-c-",
    }

    if context.configuration == "Debug" then
        table.insert(args, "/Od")
        table.insert(args, "/Zi")
        table.insert(args, "/MDd")
    elseif context.configuration == "Shipping" then
        table.insert(args, "/O2")
        table.insert(args, "/MD")
    else
        table.insert(args, "/O2")
        table.insert(args, "/Zi")
        table.insert(args, "/MD")
    end

    if _OPTIONS and _OPTIONS["strict"] then
        table.insert(args, "/WX")
    end

    for _, includeDir in ipairs(fields.include_dirs) do
        table.insert(args, "/I" .. quote_shell_argument(get_absolute_path(includeDir)))
    end

    for _, includeDir in ipairs(fields.external_include_dirs) do
        table.insert(args, "/I" .. quote_shell_argument(get_absolute_path(includeDir)))
    end

    local defines = get_global_defines(desc, context)
    append_defines(defines, fields.defines)
    for _, defineValue in ipairs(defines) do
        table.insert(args, "/D" .. defineValue)
    end

    for _, option in ipairs(fields.build_options) do
        table.insert(args, option)
    end

    local pch = find_project_pch(desc)
    if pch then
        table.insert(args, "/I" .. quote_shell_argument(get_absolute_path(pch.include_dir)))
        table.insert(args, "/FI" .. quote_shell_argument(pch.header:match("([^/\\]+)$")))
    end

    table.insert(args, quote_shell_argument(sourceFile))
    return args
end

local function make_posix_arguments(desc, fields, context, sourceFile)
    local args = {
        get_compiler(context, is_c_source_file(sourceFile)),
        "-c",
    }

    if not is_c_source_file(sourceFile) then
        table.insert(args, "-std=c++20")
    end

    table.insert(args, "-Wall")
    table.insert(args, "-Wextra")
    table.insert(args, "-Wpedantic")
    table.insert(args, "-fno-exceptions")
    table.insert(args, "-fno-rtti")
    table.insert(args, "-m64")

    if context.platform == "x64_DLL" then
        table.insert(args, "-fPIC")
    end

    if context.configuration == "Debug" then
        table.insert(args, "-O0")
        table.insert(args, "-g")
    elseif context.configuration == "Shipping" then
        table.insert(args, "-O3")
    else
        table.insert(args, "-O2")
        table.insert(args, "-g")
    end

    if _OPTIONS and _OPTIONS["strict"] then
        table.insert(args, "-Werror")
    end

    for _, includeDir in ipairs(fields.include_dirs) do
        table.insert(args, "-I" .. quote_shell_argument(get_absolute_path(includeDir)))
    end

    for _, includeDir in ipairs(fields.external_include_dirs) do
        table.insert(args, "-I" .. quote_shell_argument(get_absolute_path(includeDir)))
    end

    local defines = get_global_defines(desc, context)
    append_defines(defines, fields.defines)
    for _, defineValue in ipairs(defines) do
        table.insert(args, "-D" .. defineValue)
    end

    for _, option in ipairs(fields.build_options) do
        table.insert(args, option)
    end

    local pch = find_project_pch(desc)
    if pch then
        table.insert(args, "-I" .. quote_shell_argument(get_absolute_path(pch.include_dir)))
        table.insert(args, "-include")
        table.insert(args, quote_shell_argument(pch.header:match("([^/\\]+)$")))
    end

    table.insert(args, quote_shell_argument(sourceFile))
    return args
end

local function make_compile_command(desc, fields, context, sourceFile)
    local args
    if context.system == "windows" then
        args = make_msvc_arguments(desc, fields, context, sourceFile)
    else
        args = make_posix_arguments(desc, fields, context, sourceFile)
    end

    return table.concat(args, " ")
end

local function make_entry(desc, fields, context, sourceFile)
    local command = make_compile_command(desc, fields, context, sourceFile)

    return {
        directory = normalize_slashes(path.getabsolute(BLUE_ROOT)),
        file = sourceFile,
        output = normalize_slashes(path.join(BLUE_ROOT, "out/obj", context.system, context.platform, context.configuration, desc.name, get_relative_path(sourceFile) .. ".o")),
        command = command,
    }
end

local function append_project_entries(entries, desc, context)
    if desc.kind == "Utility" then
        return
    end

    local sources = collect_project_sources(desc, context)
    if #sources == 0 then
        return
    end

    local fields = collect_project_fields(desc, context)
    for _, sourceFile in ipairs(sources) do
        table.insert(entries, make_entry(desc, fields, context, sourceFile))
    end
end

local function get_projects_sorted_by_name()
    local projects = {}
    for _, desc in pairs(bb.registry.projects or {}) do
        table.insert(projects, desc)
    end

    table.sort(projects, function(left, right)
        return tostring(left.name) < tostring(right.name)
    end)

    return projects
end

local function encode_compile_commands(entries)
    local lines = {}
    table.insert(lines, "[")

    for index, entry in ipairs(entries) do
        local suffix = index == #entries and "" or ","
        table.insert(lines, "  {")
        table.insert(lines, '    "directory": "' .. json_escape(entry.directory) .. '",')
        table.insert(lines, '    "file": "' .. json_escape(entry.file) .. '",')
        table.insert(lines, '    "output": "' .. json_escape(entry.output) .. '",')
        table.insert(lines, '    "command": "' .. json_escape(entry.command) .. '"')
        table.insert(lines, "  }" .. suffix)
    end

    table.insert(lines, "]")
    table.insert(lines, "")
    return table.concat(lines, "\n")
end

local function generate_database(context)
    local entries = {}

    for _, desc in ipairs(get_projects_sorted_by_name()) do
        append_project_entries(entries, desc, context)
    end

    table.sort(entries, function(left, right)
        if left.file == right.file then
            return left.command < right.command
        end

        return left.file < right.file
    end)

    return encode_compile_commands(entries), #entries
end

local EXECUTABLE_KINDS = {
    ConsoleApp = true,
    WindowedApp = true,
}

local function xml_escape(value)
    local text = tostring(value or "")
    text = text:gsub("&", "&amp;")
    text = text:gsub('"', "&quot;")
    text = text:gsub("'", "&apos;")
    text = text:gsub("<", "&lt;")
    text = text:gsub(">", "&gt;")
    return text
end

local function sanitize_file_name(value)
    local text = tostring(value or "")
    text = text:gsub("[^%w%._%-]+", "_")
    text = text:gsub("_+", "_")
    text = text:gsub("^_+", "")
    text = text:gsub("_+$", "")
    if text == "" then
        return "Configuration"
    end

    return text
end

local function fnv1a32(text, seed)
    local hash = seed or 2166136261
    for index = 1, #text do
        hash = (hash ~ text:byte(index)) & 0xffffffff
        hash = (hash * 16777619) & 0xffffffff
    end
    return hash
end

local function stable_uuid(namespace, value)
    local text = tostring(namespace or "") .. ":" .. tostring(value or "")
    local a = fnv1a32(text, 2166136261)
    local b = fnv1a32(text, 2166136261 ~ 0x9e3779b9)
    local c = fnv1a32(text, 2166136261 ~ 0x85ebca6b)
    local d = fnv1a32(text, 2166136261 ~ 0xc2b2ae35)

    return string.format(
        "%08x-%04x-%04x-%04x-%012x",
        a,
        (b >> 16) & 0xffff,
        b & 0xffff,
        (c >> 16) & 0xffff,
        ((c & 0xffff) << 32) | d
    )
end

local function split_csv(value)
    local result = {}
    for item in tostring(value or ""):gmatch("[^,]+") do
        item = item:gsub("^%s+", ""):gsub("%s+$", "")
        if item ~= "" then
            table.insert(result, item)
        end
    end

    return result
end

local function get_workspace_name()
    local workspaceDesc = bb.registry.workspace or {}
    return tostring(workspaceDesc.name or "Project")
end

local function get_workspace_start_project()
    local workspaceDesc = bb.registry.workspace or {}
    local startupOverride = _OPTIONS and _OPTIONS["blue-startup"] or nil
    if startupOverride ~= nil and startupOverride ~= "" then
        return startupOverride
    end

    return workspaceDesc.startproject
end

local function get_workspace_build_target_name()
    if bb.get_workspace_build_target_name then
        return bb.get_workspace_build_target_name()
    end

    return sanitize_file_name(get_workspace_name() .. "Workspace")
end

local function get_generated_file_prefix()
    return sanitize_file_name(get_workspace_name())
end

local function is_executable_project(desc)
    return desc ~= nil and EXECUTABLE_KINDS[desc.kind] == true
end

local function get_project_by_name(name)
    if name == nil or name == "" then
        return nil
    end

    return bb.registry.projects and bb.registry.projects[name] or nil
end

local function resolve_clion_working_directory(desc)
    local value = desc and desc.debugdir or nil
    if value == nil or value == "" then
        return "$PROJECT_DIR$"
    end

    value = normalize_slashes(value)
    if value:find("%$PROJECT_DIR%$") or value:find("%$ProjectFileDir%$") then
        return value
    end

    if path.isabsolute and path.isabsolute(value) then
        return normalize_slashes(value)
    end

    return "$PROJECT_DIR$/" .. normalize_slashes(value)
end

local function get_runnable_projects_by_name()
    local result = {}
    for _, desc in pairs(bb.registry.projects or {}) do
        if is_executable_project(desc) then
            result[desc.name] = {
                desc = desc,
                display_name = desc.name,
                working_dir = resolve_clion_working_directory(desc),
                program_args = bb.table.as_list(desc.debugargs),
            }
        end
    end

    return result
end

local function sorted_project_names(targets)
    local names = {}
    for name, _ in pairs(targets or {}) do
        table.insert(names, name)
    end
    table.sort(names)
    return names
end

local function append_run_target(result, seen, targets, name, required)
    if name == nil or name == "" then
        return
    end

    if not targets[name] then
        if required then
            error("CLion run target is not a runnable Premake executable project: " .. tostring(name))
        end
        return
    end

    if not seen[name] then
        table.insert(result, name)
        seen[name] = true
    end
end

local function collect_default_run_targets(targets)
    local result = {}
    local seen = {}

    append_run_target(result, seen, targets, get_workspace_start_project(), false)

    local allNames = sorted_project_names(targets)
    if #result == 0 and #allNames == 1 then
        append_run_target(result, seen, targets, allNames[1], false)
    end

    table.sort(result)
    return result
end

local function collect_requested_run_targets()
    local mode = option_value("clion-run-targets")
    local targets = get_runnable_projects_by_name()
    local names = {}
    local seen = {}

    if mode == "none" or mode == "off" then
        return {}, targets
    end

    if mode == "default" then
        return collect_default_run_targets(targets), targets
    end

    if mode == "all" then
        return sorted_project_names(targets), targets
    end

    for _, name in ipairs(split_csv(mode)) do
        append_run_target(names, seen, targets, name, true)
    end

    table.sort(names)
    return names, targets
end

local function get_executable_extension(context)
    if context.system == "windows" then
        return ".exe"
    end

    return ""
end

local function get_binary_output_system_name(targetSystem)
    return targetSystem
end

local function get_run_path(context, targetName)
    return "$PROJECT_DIR$/out/bin/" .. get_binary_output_system_name(context.system) .. "/" .. context.platform .. "/" .. context.configuration .. "/" .. targetName .. get_executable_extension(context)
end

local function get_idea_script_path(context, kind)
    if context.system == "windows" then
        return "$ProjectFileDir$/scripts/clion-" .. kind .. "-windows.cmd"
    end

    return "$ProjectFileDir$/scripts/clion-" .. kind .. "-unix.sh"
end

local BUILDABLE_PROJECT_KINDS = {
    StaticLib = true,
    SharedLib = true,
    ConsoleApp = true,
    WindowedApp = true,
}

local function get_clion_target_name()
    return sanitize_file_name(get_workspace_name())
end

local function get_clion_project_configuration_name(targetName, targetRecord, configurationName, platformName)
    local displayName = targetRecord and targetRecord.display_name or targetName
    return displayName .. " " .. configurationName .. " " .. platformName
end

local function get_workspace_configuration_name(configurationName, platformName)
    return get_workspace_name() .. " Workspace " .. configurationName .. " " .. platformName
end

local function get_external_tool_name(kind, configName)
    return get_workspace_name() .. ": " .. kind .. " " .. configName
end

local function get_external_tool_action_id(kind, configName)
    return "Tool_External Tools_" .. get_external_tool_name(kind, configName)
end

local function get_external_tool_parameters(targetName, configurationName, platformName)
    return targetName .. " " .. configurationName .. " " .. platformName
end

local function make_build_configuration_record(name, targetName, context)
    return {
        name = name,
        target_name = targetName,
        clion_target_name = get_clion_target_name(),
        context = context,
        platform = context.platform,
        configuration = context.configuration,
        build_tool_name = get_external_tool_name("Build", name),
        clean_tool_name = get_external_tool_name("Clean", name),
        build_action_id = get_external_tool_action_id("Build", name),
        clean_action_id = get_external_tool_action_id("Clean", name),
        parameters = get_external_tool_parameters(targetName, context.configuration, context.platform),
    }
end

local function is_buildable_project(desc)
    return desc ~= nil and BUILDABLE_PROJECT_KINDS[desc.kind] == true
end

local function get_buildable_projects_by_name()
    local result = {}
    for _, desc in pairs(bb.registry.projects or {}) do
        if is_buildable_project(desc) then
            result[desc.name] = {
                desc = desc,
                display_name = desc.name,
            }
        end
    end

    return result
end

local function make_workspace_build_configuration(context)
    local name = get_workspace_configuration_name(context.configuration, context.platform)
    return make_build_configuration_record(name, get_workspace_build_target_name(), context)
end

local function make_project_build_configuration(targetName, targetRecord, context)
    local name = get_clion_project_configuration_name(targetName, targetRecord, context.configuration, context.platform)
    return make_build_configuration_record(name, targetName, context)
end

local function append_build_target_name(result, seen, targets, name, required)
    if name == nil or name == "" then
        return
    end

    if not targets[name] then
        if required then
            error("CLion build target is not a buildable Premake project: " .. tostring(name))
        end
        return
    end

    if not seen[name] then
        table.insert(result, name)
        seen[name] = true
    end
end

local function collect_requested_build_target_names(defaultRunTargetNames)
    local mode = option_value("clion-build-targets")
    local targets = get_buildable_projects_by_name()
    local names = {}
    local seen = {}

    if mode == "none" or mode == "off" or mode == "workspace" then
        return names, targets
    end

    if mode == "default" then
        for _, name in ipairs(defaultRunTargetNames or {}) do
            append_build_target_name(names, seen, targets, name, false)
        end
        table.sort(names)
        return names, targets
    end

    if mode == "all" then
        return sorted_project_names(targets), targets
    end

    for _, name in ipairs(split_csv(mode)) do
        append_build_target_name(names, seen, targets, name, true)
    end

    table.sort(names)
    return names, targets
end

local function collect_build_configurations(targetSystem, platforms, configurations, defaultRunTargetNames)
    local result = {}
    local selectedTargetNames, buildableTargets = collect_requested_build_target_names(defaultRunTargetNames)

    for _, platformName in ipairs(platforms) do
        for _, configurationName in ipairs(configurations) do
            local context = {
                system = targetSystem,
                platform = platformName,
                configuration = configurationName,
            }

            table.insert(result, make_workspace_build_configuration(context))

            for _, targetName in ipairs(selectedTargetNames) do
                table.insert(result, make_project_build_configuration(targetName, buildableTargets[targetName], context))
            end
        end
    end

    return result
end

local function collect_run_configurations(targetSystem, targetNames, targetRecords, buildConfigurationsByKey, platforms, configurations)
    local result = {}
    local filePrefix = get_generated_file_prefix()

    for _, targetName in ipairs(targetNames) do
        local targetRecord = targetRecords[targetName]
        for _, platformName in ipairs(platforms) do
            for _, configurationName in ipairs(configurations) do
                local context = {
                    system = targetSystem,
                    platform = platformName,
                    configuration = configurationName,
                }
                local name = get_clion_project_configuration_name(targetName, targetRecord, configurationName, platformName)
                local buildConfiguration = buildConfigurationsByKey[name]
                if not buildConfiguration then
                    error("Missing CLion build configuration for run target: " .. tostring(name))
                end

                table.insert(result, {
                    name = name,
                    file_name = filePrefix .. "_" .. sanitize_file_name(name) .. ".xml",
                    project_name = get_clion_target_name(),
                    target_name = get_clion_target_name(),
                    config_name = buildConfiguration.name,
                    platform = platformName,
                    configuration = configurationName,
                    context = context,
                    run_path = get_run_path(context, targetName),
                    working_dir = targetRecord.working_dir or "$PROJECT_DIR$",
                    program_args = targetRecord.program_args or {},
                    build_configuration = buildConfiguration,
                })
            end
        end
    end

    table.sort(result, function(left, right)
        return left.name < right.name
    end)

    return result
end

local function index_build_configurations(buildConfigurations)
    local result = {}
    for _, config in ipairs(buildConfigurations or {}) do
        if result[config.name] then
            error("Duplicate CLion build configuration generated: " .. tostring(config.name))
        end
        result[config.name] = config
    end
    return result
end

local function write_external_tools_file(buildConfigurations)
    local lines = {
        '<toolSet name="External Tools">',
    }

    for _, config in ipairs(buildConfigurations) do
        for _, tool in ipairs({
            { name = config.build_tool_name, description = "Build " .. config.name, command = get_idea_script_path(config.context, "build") },
            { name = config.clean_tool_name, description = "Clean " .. config.name, command = get_idea_script_path(config.context, "clean") },
        }) do
            table.insert(lines, '  <tool name="' .. xml_escape(tool.name) .. '" description="' .. xml_escape(tool.description) .. '" showInMainMenu="false" showInEditor="false" showInProject="false" showInSearchPopup="false" disabled="false" useConsole="true" showConsoleOnStdOut="true" showConsoleOnStdErr="true" synchronizeAfterRun="true">')
            table.insert(lines, "    <exec>")
            table.insert(lines, '      <option name="COMMAND" value="' .. xml_escape(tool.command) .. '" />')
            table.insert(lines, '      <option name="PARAMETERS" value="' .. xml_escape(config.parameters) .. '" />')
            table.insert(lines, '      <option name="WORKING_DIRECTORY" value="$ProjectFileDir$" />')
            table.insert(lines, "    </exec>")
            table.insert(lines, "  </tool>")
        end
    end

    table.insert(lines, "</toolSet>")
    table.insert(lines, "")

    bb.fs.write_file(path.join(BLUE_ROOT, ".idea/tools/External Tools.xml"), table.concat(lines, "\n"))
end

local function build_custom_targets_component(buildConfigurations)
    local clionTargetName = get_clion_target_name()
    local lines = {
        '  <component name="CLionExternalBuildManager">',
        '    <target id="' .. stable_uuid("clion-target", clionTargetName) .. '" name="' .. xml_escape(clionTargetName) .. '" defaultType="TOOL">',
    }

    for _, config in ipairs(buildConfigurations) do
        table.insert(lines, '      <configuration id="' .. stable_uuid("clion-configuration", clionTargetName .. ":" .. config.name) .. '" name="' .. xml_escape(config.name) .. '">')
        table.insert(lines, '        <build type="TOOL">')
        table.insert(lines, '          <tool actionId="' .. xml_escape(config.build_action_id) .. '" />')
        table.insert(lines, '        </build>')
        table.insert(lines, '        <clean type="TOOL">')
        table.insert(lines, '          <tool actionId="' .. xml_escape(config.clean_action_id) .. '" />')
        table.insert(lines, '        </clean>')
        table.insert(lines, '      </configuration>')
    end

    table.insert(lines, '    </target>')
    table.insert(lines, '  </component>')
    return table.concat(lines, "\n")
end

local function write_custom_targets_file(buildConfigurations)
    local lines = {
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<project version="4">',
        build_custom_targets_component(buildConfigurations),
        '</project>',
        '',
    }

    bb.fs.write_file(path.join(BLUE_ROOT, ".idea/customTargets.xml"), table.concat(lines, "\n"))
end

local function read_text_file(filename)
    local file = io.open(filename, "r")
    if not file then
        return nil
    end

    local content = file:read("*a")
    file:close()
    return content
end

local function normalize_project_xml_document(content)
    local text = tostring(content or "")
    if text == "" then
        return '<?xml version="1.0" encoding="UTF-8"?>\n<project version="4">\n</project>\n'
    end

    if not text:find('<project%s+version="4"%s*>') then
        return '<?xml version="1.0" encoding="UTF-8"?>\n<project version="4">\n</project>\n'
    end

    if not text:find("</project>%s*$") then
        text = text .. "\n</project>\n"
    end

    return text
end

local function replace_component(document, componentName, componentContent)
    local text = normalize_project_xml_document(document)
    local escapedName = componentName:gsub("([^%w])", "%%%1")
    local pattern = '%s*<component%s+name="' .. escapedName .. '".-</component>'

    local replaced
    text, replaced = text:gsub(pattern, "\n" .. componentContent, 1)
    if replaced > 0 then
        return text
    end

    return text:gsub("%s*</project>%s*$", "\n" .. componentContent .. "\n</project>\n", 1)
end

local function build_cmake_run_configuration_manager_component()
    return table.concat({
        '  <component name="CMakeRunConfigurationManager">',
        '    <generated />',
        '  </component>',
    }, "\n")
end

local function build_disabled_cmake_settings_component()
    return table.concat({
        '  <component name="CMakeSettings">',
        '    <configurations>',
        '      <configuration PROFILE_NAME="Debug" ENABLED="false" CONFIG_NAME="Debug" />',
        '    </configurations>',
        '  </component>',
    }, "\n")
end

local function write_clion_workspace_bridge_file(buildConfigurations)
    local workspacePath = path.join(BLUE_ROOT, ".idea/workspace.xml")
    local document = read_text_file(workspacePath)

    document = replace_component(document, "CLionExternalBuildManager", build_custom_targets_component(buildConfigurations))
    document = replace_component(document, "CMakeRunConfigurationManager", build_cmake_run_configuration_manager_component())
    document = replace_component(document, "CMakeSettings", build_disabled_cmake_settings_component())

    bb.fs.write_file(workspacePath, document)
end

local function write_clion_project_name_file()
    bb.fs.write_file(path.join(BLUE_ROOT, ".idea/.name"), get_workspace_name() .. "\n")
end

local function clear_generated_run_configurations()
    local runConfigDir = path.join(BLUE_ROOT, ".idea/runConfigurations")
    local prefix = get_generated_file_prefix() .. "_"
    for _, file in ipairs(os.matchfiles(path.join(runConfigDir, prefix .. "*.xml"))) do
        os.remove(file)
    end
end

local function encode_program_arguments(args)
    return table.concat(bb.table.as_list(args), " ")
end

local function write_run_configuration_file(config)
    local lines = {
        '<component name="ProjectRunConfigurationManager">',
        '  <configuration default="false" name="' .. xml_escape(config.name) .. '" type="CLionExternalRunConfiguration" factoryName="Application" REDIRECT_INPUT="false" ELEVATE="false" USE_EXTERNAL_CONSOLE="false" EMULATE_TERMINAL="false" PASS_PARENT_ENVS_2="true" PROJECT_NAME="' .. xml_escape(config.project_name) .. '" TARGET_NAME="' .. xml_escape(config.target_name) .. '" CONFIG_NAME="' .. xml_escape(config.config_name) .. '" RUN_PATH="' .. xml_escape(config.run_path) .. '">',
        '    <option name="WORKING_DIRECTORY" value="' .. xml_escape(config.working_dir) .. '" />',
    }

    local programArguments = encode_program_arguments(config.program_args)
    if programArguments ~= "" then
        table.insert(lines, '    <option name="PROGRAM_ARGUMENTS" value="' .. xml_escape(programArguments) .. '" />')
    end

    table.insert(lines, '    <method v="2">')
    table.insert(lines, '      <option name="CLION.EXTERNAL.BUILD" enabled="true" />')
    table.insert(lines, '    </method>')
    table.insert(lines, '  </configuration>')
    table.insert(lines, '</component>')
    table.insert(lines, '')

    bb.fs.write_file(path.join(BLUE_ROOT, ".idea/runConfigurations", config.file_name), table.concat(lines, "\n"))
end

local function write_run_configuration_files(configurations)
    clear_generated_run_configurations()
    for _, config in ipairs(configurations) do
        write_run_configuration_file(config)
    end
end

local function write_clion_idea_files(targetSystem, platforms, configurations)
    if option_value("clion-idea") == "off" then
        return {
            build_configurations = 0,
            run_configurations = 0,
        }
    end

    local runTargetNames, runTargetRecords = collect_requested_run_targets()
    local buildConfigurations = collect_build_configurations(targetSystem, platforms, configurations, runTargetNames)
    local buildConfigurationsByKey = index_build_configurations(buildConfigurations)
    local runConfigurations = collect_run_configurations(targetSystem, runTargetNames, runTargetRecords, buildConfigurationsByKey, platforms, configurations)

    write_external_tools_file(buildConfigurations)
    write_custom_targets_file(buildConfigurations)
    write_clion_workspace_bridge_file(buildConfigurations)
    write_clion_project_name_file()
    write_run_configuration_files(runConfigurations)

    return {
        build_configurations = #buildConfigurations,
        run_configurations = #runConfigurations,
    }
end

local function write_clion_readme(outputRoot, targetSystem, generatedDatabases, ideaStats)
    local lines = {
        "# " .. get_workspace_name() .. " CLion Project Files",
        "",
        "This folder contains generated compilation databases for CLion and other clang tooling.",
        "Do not edit these files manually. Regenerate them through the Premake `clion` action.",
        "",
        "Target system: `" .. targetSystem .. "`",
        "",
        "## Databases",
        "",
    }

    for _, database in ipairs(generatedDatabases) do
        table.insert(lines, "- `" .. database.relativePath .. "` (`" .. database.entries .. "` translation units)")
    end

    table.insert(lines, "")
    table.insert(lines, "The repository root `compile_commands.json` is a copy of the default Debug/x64 database when that database is generated.")
    table.insert(lines, "")
    local buildConfigurationCount = ideaStats and ideaStats.build_configurations or 0
    local runConfigurationCount = ideaStats and ideaStats.run_configurations or 0

    table.insert(lines, "## CLion IDE integration")
    table.insert(lines, "")
    table.insert(lines, "Generated custom build target: `" .. get_clion_target_name() .. "`")
    table.insert(lines, "Generated build configurations: `" .. tostring(buildConfigurationCount) .. "`")
    table.insert(lines, "Generated run configurations: `" .. tostring(runConfigurationCount) .. "`")
    table.insert(lines, "")
    table.insert(lines, "The generated custom target is named after the Premake workspace. Its configurations build the workspace and any selected Premake projects. The exporter writes the same external-build target model to `customTargets.xml` and `workspace.xml` so CLion can resolve run configurations reliably after project reload.")
    table.insert(lines, "")

    bb.fs.write_file(path.join(outputRoot, "README.md"), table.concat(lines, "\n"))
end

function bb.clion.generate()
    bb.validate_all()

    local targetSystem = get_target_system()
    local targetOsName = get_target_os_name(targetSystem)
    local workspaceDesc = bb.registry.workspace or {}
    local configurations = select_requested_values("clion-config", workspaceDesc.configurations or bb.get_default_build_configurations(), DEFAULT_CONFIG)
    local platforms = select_requested_values("clion-platform", workspaceDesc.platforms or bb.get_default_build_platforms(), DEFAULT_PLATFORM)
    local outputRoot = path.join(BLUE_ROOT, "out/ide/clion", targetOsName)
    local generatedDatabases = {}
    local defaultDatabaseContent = nil
    local defaultDatabasePath = nil

    for _, platformName in ipairs(platforms) do
        for _, configurationName in ipairs(configurations) do
            local context = {
                system = targetSystem,
                platform = platformName,
                configuration = configurationName,
            }

            local content, entryCount = generate_database(context)
            local databasePath = path.join(outputRoot, platformName, configurationName, "compile_commands.json")
            bb.fs.write_file(databasePath, content)

            table.insert(generatedDatabases, {
                relativePath = normalize_slashes(path.getrelative(outputRoot, databasePath)),
                entries = entryCount,
            })

            if platformName == DEFAULT_PLATFORM and configurationName == DEFAULT_CONFIG then
                defaultDatabaseContent = content
                defaultDatabasePath = databasePath
            end
        end
    end

    if not defaultDatabaseContent and #platforms > 0 and #configurations > 0 then
        local fallbackPath = path.join(outputRoot, platforms[1], configurations[1], "compile_commands.json")
        local fallbackFile = io.open(fallbackPath, "r")
        if fallbackFile then
            defaultDatabaseContent = fallbackFile:read("*a")
            fallbackFile:close()
            defaultDatabasePath = fallbackPath
        end
    end

    if defaultDatabaseContent then
        bb.fs.write_file(path.join(BLUE_ROOT, "compile_commands.json"), defaultDatabaseContent)
    end

    local ideaStats = write_clion_idea_files(targetSystem, platforms, configurations)
    write_clion_readme(outputRoot, targetSystem, generatedDatabases, ideaStats)

    bb.log.info("CLion compilation databases generated under " .. normalize_slashes(outputRoot))
    if option_value("clion-idea") ~= "off" then
        bb.log.info("CLion custom targets and run configurations generated under " .. normalize_slashes(path.join(BLUE_ROOT, ".idea")))
    end
    if defaultDatabasePath then
        bb.log.info("Root compile_commands.json copied from " .. normalize_slashes(defaultDatabasePath))
    end
end
