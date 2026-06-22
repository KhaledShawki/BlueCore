bb.commands = bb.commands or {}

local function normalize_path(value)
    value = tostring(value or "")
    value = value:gsub("\\", "/")
    value = value:gsub("/+", "/")
    value = value:gsub("^%./", "")
    value = value:gsub("/$", "")
    return value
end

local function basename(value)
    value = normalize_path(value)
    return value:match("([^/]+)$") or value
end

local function quote_lua_string(value)
    local text = tostring(value or "")
    text = text:gsub("\\", "\\\\")
    text = text:gsub('"', '\\"')
    return '"' .. text .. '"'
end

local function read_file(filename)
    local file, errorMessage = io.open(filename, "rb")
    if not file then
        error("Failed to read file '" .. filename .. "': " .. tostring(errorMessage))
    end

    local content = file:read("*a")
    file:close()
    return content
end

local function write_file(filename, content)
    bb.fs.write_file(filename, content)
end

local function split_lines(content)
    local normalized = tostring(content or ""):gsub("\r\n", "\n"):gsub("\r", "\n")
    local lines = {}

    for line in (normalized .. "\n"):gmatch("(.-)\n") do
        table.insert(lines, line)
    end

    if #lines > 0 and lines[#lines] == "" then
        table.remove(lines, #lines)
    end

    return lines
end

local function join_lines(lines)
    return table.concat(lines, "\n") .. "\n"
end

local function validate_identifier(value, label)
    local text = tostring(value or "")
    if text == "" or not text:match("^[A-Z][A-Za-z0-9_]*$") then
        error(label .. " must be a PascalCase identifier: " .. text)
    end
    return text
end

local function require_option(name, label)
    local value = _OPTIONS[name]
    if value == nil or value == "" then
        error("Missing required option --" .. name .. " for " .. label)
    end
    return value
end

local function find_project(projectName)
    local desc = bb.registry.projects[projectName]
    if not desc then
        error("Project not found in build graph: " .. tostring(projectName))
    end
    return desc
end

local function normalize_project_relative_path(projectRelativeFile)
    local normalized = normalize_path(projectRelativeFile)

    if normalized == "" then
        error("File path must not be empty")
    end

    if bb.path.is_absolute(normalized) or bb.path.is_premake_token_path(normalized) then
        error("File path must be relative to the project root: " .. projectRelativeFile)
    end

    if normalized:find("^%.%./") or normalized:find("/%.%./") then
        error("File path must not escape the project root: " .. projectRelativeFile)
    end

    if normalized:find("[%*%?]") then
        error("File path must not contain wildcards: " .. projectRelativeFile)
    end

    return normalized
end

local function with_prefix(prefix, value)
    value = normalize_project_relative_path(value)
    if value:sub(1, #prefix + 1) == prefix .. "/" then
        return value
    end
    return prefix .. "/" .. value
end

local function normalize_named_target(value, folderName, label)
    local normalized = normalize_project_relative_path(value)

    local folderPrefix = folderName .. "/"
    if normalized:sub(1, #folderPrefix) == folderPrefix then
        normalized = normalized:sub(#folderPrefix + 1)
    end

    normalized = normalized:gsub("%.cpp$", "")
    normalized = normalized:gsub("%.cc$", "")
    normalized = normalized:gsub("%.cxx$", "")

    if normalized == "" then
        error(label .. " name must not be empty")
    end

    if normalized:find("/") then
        error(label .. " name must not contain directories: " .. value)
    end

    if not normalized:match("^[A-Za-z_][A-Za-z0-9_]*$") then
        error("Invalid " .. label .. " name: " .. normalized)
    end

    return normalized
end

local function normalize_test_name(value)
    return normalize_named_target(value, "tests", "Test")
end

local function normalize_benchmark_name(value)
    return normalize_named_target(value, "benchmarks", "Benchmark")
end


local function classify_file(projectName, kind, value)
    local normalizedKind = tostring(kind or ""):lower():gsub("_", "-")
    local relative = value
    local sectionPath
    local category
    local manifestEntry

    if normalizedKind == "source" or normalizedKind == "src" then
        relative = with_prefix("src", relative)
        sectionPath = { "files", "sources" }
        category = "sources"
    elseif normalizedKind == "public-header" or normalizedKind == "public_header" or normalizedKind == "header" then
        relative = normalize_project_relative_path(relative)
        if relative:sub(1, 8) ~= "include/" then
            relative = path.join("include", projectName, relative)
        end
        relative = normalize_project_relative_path(relative)
        sectionPath = { "files", "public_headers" }
        category = "public_headers"
    elseif normalizedKind == "private-header" or normalizedKind == "private_header" then
        relative = with_prefix("src", relative)
        sectionPath = { "files", "private_headers" }
        category = "private_headers"
    elseif normalizedKind == "windows-source" or normalizedKind == "windows_source" then
        relative = with_prefix("src/Platform/Windows", relative)
        sectionPath = { "files", "platform", "windows", "sources" }
        category = "sources"
    elseif normalizedKind == "linux-source" or normalizedKind == "linux_source" then
        relative = with_prefix("src/Platform/Linux", relative)
        sectionPath = { "files", "platform", "linux", "sources" }
        category = "sources"
    elseif normalizedKind == "macos-source" or normalizedKind == "macos_source" or normalizedKind == "macosx-source" or normalizedKind == "macosx_source" then
        relative = with_prefix("src/Platform/MacOS", relative)
        sectionPath = { "files", "platform", "macosx", "sources" }
        category = "sources"
    elseif normalizedKind == "posix-source" or normalizedKind == "posix_source" then
        relative = with_prefix("src/Platform/POSIX", relative)
        sectionPath = { "files", "platform", "linux", "sources" }
        category = "sources"
    elseif normalizedKind == "test" or normalizedKind == "tests" or normalizedKind == "unit-test" or normalizedKind == "unit_test" then
        local testName = normalize_test_name(relative)
        relative = "tests/" .. testName .. ".cpp"
        sectionPath = { "tests" }
        category = "tests"
        manifestEntry = testName
    elseif normalizedKind == "benchmark" or normalizedKind == "benchmarks" or normalizedKind == "bench" or normalizedKind == "perf" then
        local benchmarkName = normalize_benchmark_name(relative)
        relative = "benchmarks/" .. benchmarkName .. ".cpp"
        sectionPath = { "benchmarks" }
        category = "benchmarks"
        manifestEntry = benchmarkName
    else
        error("Unsupported file kind: " .. tostring(kind))
    end

    local sectionPaths = { sectionPath }

    if normalizedKind == "posix-source" or normalizedKind == "posix_source" then
        sectionPaths = {
            { "files", "platform", "linux", "sources" },
            { "files", "platform", "macosx", "sources" },
        }
    end

    return {
        kind = normalizedKind,
        project_relative = normalize_project_relative_path(relative),
        manifest_entry = manifestEntry or normalize_project_relative_path(relative),
        section_path = sectionPath,
        section_paths = sectionPaths,
        category = category,
    }
end

local function leading_space(line)
    return tostring(line or ""):match("^(%s*)") or ""
end

local function close_line_for_block(lines, openLine)
    local depth = 0
    for lineIndex = openLine, #lines do
        local line = lines[lineIndex]
        local opens = select(2, line:gsub("{", ""))
        local closes = select(2, line:gsub("}", ""))
        depth = depth + opens - closes
        if lineIndex > openLine and depth <= 0 then
            return lineIndex
        end
    end
    error("Could not find closing brace for table block at line " .. tostring(openLine))
end

local function find_nested_block(lines, sectionPath)
    local searchStart = 1
    local blockOpen = nil
    local blockClose = #lines

    for _, sectionName in ipairs(sectionPath) do
        local found = nil
        for lineIndex = searchStart, blockClose do
            if lines[lineIndex]:match("^%s*" .. sectionName .. "%s*=%s*{%s*$") then
                found = lineIndex
                break
            end
        end

        if not found then
            error("Manifest section not found: " .. table.concat(sectionPath, ".") .. " (missing " .. sectionName .. ")")
        end

        blockOpen = found
        blockClose = close_line_for_block(lines, blockOpen)
        searchStart = blockOpen + 1
    end

    return blockOpen, blockClose
end

local function entry_pattern(projectRelativeFile)
    local quoted = quote_lua_string(projectRelativeFile):gsub("([^%w])", "%%%1")
    return "^%s*" .. quoted .. "%s*,%s*$"
end

local function find_entry_line(lines, openLine, closeLine, projectRelativeFile)
    local pattern = entry_pattern(projectRelativeFile)
    for lineIndex = openLine + 1, closeLine - 1 do
        if lines[lineIndex]:match(pattern) then
            return lineIndex
        end
    end
    return nil
end

local function collect_entries(lines, openLine, closeLine)
    local entries = {}
    for lineIndex = openLine + 1, closeLine - 1 do
        local value = lines[lineIndex]:match('^%s*"([^"]+)"%s*,%s*$')
        if value then
            table.insert(entries, { line = lineIndex, value = value })
        end
    end
    return entries
end

local function insert_manifest_entry(filename, sectionPath, projectRelativeFile)
    local lines = split_lines(read_file(filename))
    local openLine, closeLine = find_nested_block(lines, sectionPath)

    if find_entry_line(lines, openLine, closeLine, projectRelativeFile) then
        bb.log.info("Manifest already contains " .. projectRelativeFile)
        return false
    end

    local entries = collect_entries(lines, openLine, closeLine)
    local insertLine = closeLine
    for _, entry in ipairs(entries) do
        if projectRelativeFile < entry.value then
            insertLine = entry.line
            break
        end
    end

    local indent = leading_space(lines[closeLine]) .. "    "
    table.insert(lines, insertLine, indent .. quote_lua_string(projectRelativeFile) .. ",")
    write_file(filename, join_lines(lines))
    return true
end

local function remove_manifest_entry(filename, sectionPath, projectRelativeFile)
    local lines = split_lines(read_file(filename))
    local openLine, closeLine = find_nested_block(lines, sectionPath)
    local entryLine = find_entry_line(lines, openLine, closeLine, projectRelativeFile)
    if not entryLine then
        error("Manifest entry not found: " .. projectRelativeFile)
    end

    table.remove(lines, entryLine)
    write_file(filename, join_lines(lines))
end

local function replace_manifest_entry(filename, sectionPath, oldRelativeFile, newRelativeFile)
    local lines = split_lines(read_file(filename))
    local openLine, closeLine = find_nested_block(lines, sectionPath)
    local oldLine = find_entry_line(lines, openLine, closeLine, oldRelativeFile)
    if not oldLine then
        error("Manifest entry not found: " .. oldRelativeFile)
    end

    if find_entry_line(lines, openLine, closeLine, newRelativeFile) then
        error("Manifest already contains target path: " .. newRelativeFile)
    end

    lines[oldLine] = leading_space(lines[oldLine]) .. quote_lua_string(newRelativeFile) .. ","
    write_file(filename, join_lines(lines))
end

local function project_lua_path(desc)
    return path.join(BLUE_ROOT, desc.root, "project.lua")
end

local function root_relative_path(desc, projectRelativeFile)
    return normalize_path(path.join(desc.root, projectRelativeFile))
end

local function absolute_path_from_root_relative(rootRelativeFile)
    return path.join(BLUE_ROOT, rootRelativeFile)
end

local function should_create_files()
    return _OPTIONS["blue-no-create"] == nil
end

local function should_delete_files()
    return _OPTIONS["blue-delete-file"] ~= nil
end

local function ensure_physical_file(desc, classified)
    local rootRelativeFile = root_relative_path(desc, classified.project_relative)
    local absolute = absolute_path_from_root_relative(rootRelativeFile)

    if os.isfile(absolute) then
        return
    end

    if not should_create_files() then
        return
    end

    bb.scaffold.create_authored_file(desc, rootRelativeFile, classified.project_relative, classified.category)
end

local function rename_physical_file(desc, oldProjectRelative, newProjectRelative)
    local oldRootRelative = root_relative_path(desc, oldProjectRelative)
    local newRootRelative = root_relative_path(desc, newProjectRelative)
    local oldAbsolute = absolute_path_from_root_relative(oldRootRelative)
    local newAbsolute = absolute_path_from_root_relative(newRootRelative)

    if not os.isfile(oldAbsolute) then
        error("Cannot rename missing file: " .. oldRootRelative)
    end

    if os.isfile(newAbsolute) then
        error("Cannot rename to existing file: " .. newRootRelative)
    end

    os.mkdir(path.getdirectory(newAbsolute))
    local ok, errorMessage = os.rename(oldAbsolute, newAbsolute)
    if not ok then
        error("Failed to rename '" .. oldRootRelative .. "' to '" .. newRootRelative .. "': " .. tostring(errorMessage))
    end

    bb.log.info("Renamed " .. oldRootRelative .. " -> " .. newRootRelative)
end

function bb.commands.add_file()
    local projectName = require_option("blue-project", "blue-add-file")
    local kind = require_option("blue-kind", "blue-add-file")
    local filePath = require_option("blue-path", "blue-add-file")
    local desc = find_project(projectName)
    local classified = classify_file(desc.name, kind, filePath)
    local manifest = project_lua_path(desc)

    bb.scaffold.validate_manifest_path(desc, classified.project_relative, classified.category)
    for _, sectionPath in ipairs(classified.section_paths) do
        insert_manifest_entry(manifest, sectionPath, classified.manifest_entry)
    end
    ensure_physical_file(desc, classified)
    bb.log.info("Added " .. root_relative_path(desc, classified.project_relative) .. " to " .. desc.name)
end

function bb.commands.remove_file()
    local projectName = require_option("blue-project", "blue-remove-file")
    local kind = require_option("blue-kind", "blue-remove-file")
    local filePath = require_option("blue-path", "blue-remove-file")
    local desc = find_project(projectName)
    local classified = classify_file(desc.name, kind, filePath)
    local manifest = project_lua_path(desc)

    for _, sectionPath in ipairs(classified.section_paths) do
        remove_manifest_entry(manifest, sectionPath, classified.manifest_entry)
    end

    if should_delete_files() then
        local rootRelativeFile = root_relative_path(desc, classified.project_relative)
        local absolute = absolute_path_from_root_relative(rootRelativeFile)
        if os.isfile(absolute) then
            os.remove(absolute)
            bb.log.info("Deleted " .. rootRelativeFile)
        end
    end

    bb.log.info("Removed " .. root_relative_path(desc, classified.project_relative) .. " from " .. desc.name)
end

function bb.commands.rename_file()
    local projectName = require_option("blue-project", "blue-rename-file")
    local kind = require_option("blue-kind", "blue-rename-file")
    local fromPath = require_option("blue-from", "blue-rename-file")
    local toPath = require_option("blue-to", "blue-rename-file")
    local desc = find_project(projectName)
    local oldFile = classify_file(desc.name, kind, fromPath)
    local newFile = classify_file(desc.name, kind, toPath)
    local manifest = project_lua_path(desc)

    if #oldFile.section_paths ~= #newFile.section_paths then
        error("Cannot rename between file kinds with different platform manifest coverage")
    end

    for index, sectionPath in ipairs(oldFile.section_paths) do
        replace_manifest_entry(manifest, sectionPath, oldFile.manifest_entry, newFile.manifest_entry)
    end

    if should_create_files() then
        rename_physical_file(desc, oldFile.project_relative, newFile.project_relative)
    end
    bb.log.info("Updated manifest rename in " .. desc.name)
end

local function include_project_line(projectPath)
    return "    " .. quote_lua_string(projectPath) .. ","
end

local function find_include_projects_block(lines)
    for index = 1, #lines do
        if lines[index]:match("^%s*bb%.include_projects%s*{%s*$") then
            local closeLine = close_line_for_block(lines, index)
            return index, closeLine
        end
    end
    error("Could not find bb.include_projects block in build.lua")
end

local function add_project_include(projectFile)
    local filename = path.join(BLUE_ROOT, "build.lua")
    local lines = split_lines(read_file(filename))
    local openLine, closeLine = find_include_projects_block(lines)
    local line = include_project_line(projectFile)

    for index = openLine + 1, closeLine - 1 do
        if lines[index] == line or lines[index]:match('^%s*"' .. projectFile:gsub("([^%w])", "%%%1") .. '"%s*,%s*$') then
            bb.log.info("Project include already exists: " .. projectFile)
            return false
        end
    end

    table.insert(lines, closeLine, line)
    write_file(filename, join_lines(lines))
    return true
end

function bb.commands.add_project()
    local projectName = validate_identifier(require_option("blue-project", "blue-add-project"), "Project name")
    local projectType = _OPTIONS["blue-type"] or "library"
    local linkage = _OPTIONS["blue-linkage"] or "auto"

    if projectType ~= "library" and projectType ~= "executable" then
        error("Unsupported project type: " .. projectType)
    end

    local root
    if projectType == "library" then
        root = normalize_path(path.join("modules", projectName))
    else
        root = normalize_path(path.join("apps", projectName))
    end

    local projectFile = normalize_path(path.join(root, "project.lua"))
    add_project_include(projectFile)

    if should_create_files() then
        bb.scaffold.create_project(projectName, root, projectType, linkage)
    end

    bb.log.info("Added project " .. projectName .. " at " .. projectFile)
end
