bb.scaffold = bb.scaffold or {}

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

local function dirname(value)
    value = normalize_path(value)
    local result = value:match("^(.*)/[^/]+$")
    return result or ""
end

local function has_wildcard(value)
    return tostring(value or ""):find("[%*%?]", 1, false) ~= nil
end

local function is_empty_directory(directory)
    local absolute = path.join(BLUE_ROOT, directory)
    if not os.isdir(absolute) then
        return true
    end

    local entries = os.matchfiles(path.join(absolute, "**"))
    return #entries == 0
end

local function read_file(filename)
    local file = io.open(filename, "rb")
    if not file then
        return nil
    end

    local content = file:read("*a")
    file:close()
    return content
end

local function write_file_if_missing(filename, content)
    if os.isfile(filename) then
        return false
    end

    bb.fs.write_file(filename, content)
    return true
end

local function license_header()
    return "// Copyright (c) BlueByte. All rights reserved.\n\n"
end

local function include_guard_path(projectName, headerName)
    return "#include <" .. projectName .. "/" .. headerName .. ">"
end

local function api_header_content(projectName)
    local suffix = bb.get_module_define_suffix(projectName)
    return license_header() .. table.concat({
        "#pragma once",
        "",
        "#if defined( _WIN32 ) || defined( __CYGWIN__ )",
        "#\tdefine " .. suffix .. "_API_EXPORT __declspec( dllexport )",
        "#\tdefine " .. suffix .. "_API_IMPORT __declspec( dllimport )",
        "#elif defined( __GNUC__ ) || defined( __clang__ )",
        "#\tdefine " .. suffix .. "_API_EXPORT __attribute__( ( visibility( \"default\" ) ) )",
        "#\tdefine " .. suffix .. "_API_IMPORT __attribute__( ( visibility( \"default\" ) ) )",
        "#else",
        "#\tdefine " .. suffix .. "_API_EXPORT",
        "#\tdefine " .. suffix .. "_API_IMPORT",
        "#endif",
        "",
        "#if defined( BLUE_SHARED_LIBRARY )",
        "#\tif defined( " .. bb.get_module_build_define(projectName) .. " ) || defined( " .. bb.get_module_export_define(projectName) .. " )",
        "#\t\tdefine " .. suffix .. "_API " .. suffix .. "_API_EXPORT",
        "#\telse",
        "#\t\tdefine " .. suffix .. "_API " .. suffix .. "_API_IMPORT",
        "#\tendif",
        "#else",
        "#\tdefine " .. suffix .. "_API",
        "#endif",
        "",
        "",
    }, "\n")
end

local function public_header_content(projectName)
    return license_header() .. table.concat({
        "#pragma once",
        "",
        include_guard_path(projectName, projectName .. "Api.h"),
        "",
        "",
    }, "\n")
end

local function private_header_content(projectName)
    return license_header() .. table.concat({
        "#pragma once",
        "",
        include_guard_path(projectName, projectName .. ".h"),
        "",
        "",
    }, "\n")
end

local function pch_header_content(projectName)
    return license_header() .. table.concat({
        "#pragma once",
        "",
        "#include \"" .. projectName .. "Private.h\"",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "",
    }, "\n")
end

local function pch_source_content()
    return license_header() .. "#include \"Pch.h\"\n"
end

local function cpp_source_content()
    return license_header() .. "#include \"Pch.h\"\n"
end

local function plain_header_content()
    return license_header() .. "#pragma once\n"
end

local function project_template(projectName, root, projectType)
    if projectType == "library" then
        return table.concat({
            "bb.module {",
            "    name = \"" .. projectName .. "\",",
            "    type = \"library\",",
            "    linkage = \"auto\",",
            "    root = \"" .. root .. "\",",
            "",
            "    files = {",
            "        public_headers = {",
            "            \"include/" .. projectName .. "/" .. projectName .. ".h\",",
            "            \"include/" .. projectName .. "/" .. projectName .. "Api.h\",",
            "        },",
            "",
            "        private_headers = {",
            "            \"src/" .. projectName .. "Private.h\",",
            "            \"src/Pch.h\",",
            "        },",
            "",
            "        sources = {",
            "            \"src/Pch.cpp\",",
            "        },",
            "",
            "        platform = {",
            "            windows = {",
            "                sources = {",
            "                },",
            "            },",
            "",
            "            linux = {",
            "                sources = {",
            "                },",
            "            },",
            "",
            "            macosx = {",
            "                sources = {",
            "                },",
            "            },",
            "        },",
            "    },",
            "}",
            "",
            "",
        }, "\n")
    end

    return table.concat({
        "bb.module {",
        "    name = \"" .. projectName .. "\",",
        "    type = \"executable\",",
        "    root = \"" .. root .. "\",",
        "",
        "    files = {",
        "        private_headers = {",
        "            \"src/Pch.h\",",
        "        },",
        "",
        "        sources = {",
        "            \"src/Pch.cpp\",",
        "            \"src/Main.cpp\",",
        "        },",
        "    },",
        "}",
        "",
        "",
    }, "\n")
end

local function detect_project_type(root)
    root = normalize_path(root)
    if root:match("^modules/") then
        return "library"
    end
    return "executable"
end

local function normalize_project_reference(reference)
    assert(type(reference) == "string", "bb.include_projects entries must be strings")

    local normalized = normalize_path(reference)
    if normalized:match("%.lua$") then
        return normalized, dirname(normalized)
    end

    return path.join(normalized, "project.lua"), normalized
end

local function create_project_files(projectName, root, projectType)
    local files = {}

    if projectType == "library" then
        files[path.join(root, "include", projectName, projectName .. ".h")] = public_header_content(projectName)
        files[path.join(root, "include", projectName, projectName .. "Api.h")] = api_header_content(projectName)
        files[path.join(root, "src", projectName .. "Private.h")] = private_header_content(projectName)
    else
        files[path.join(root, "src", "Main.cpp")] = cpp_source_content()
    end

    files[path.join(root, "src", "Pch.h")] = projectType == "library" and pch_header_content(projectName) or plain_header_content()
    files[path.join(root, "src", "Pch.cpp")] = pch_source_content()

    for relative, content in pairs(files) do
        local absolute = path.join(BLUE_ROOT, relative)
        if write_file_if_missing(absolute, content) then
            bb.log.info("Created " .. relative)
        end
    end
end

local function manifest_file_content(projectName, projectRelativeFile)
    local fileName = basename(projectRelativeFile)

    if fileName == projectName .. "Api.h" then
        return api_header_content(projectName)
    end

    if fileName == projectName .. ".h" then
        return public_header_content(projectName)
    end

    if fileName == projectName .. "Private.h" then
        return private_header_content(projectName)
    end

    if fileName == "Pch.h" then
        return pch_header_content(projectName)
    end

    if fileName == "Pch.cpp" then
        return pch_source_content()
    end

    if projectRelativeFile:match("%.c$") or projectRelativeFile:match("%.cpp$") or projectRelativeFile:match("%.cxx$") or projectRelativeFile:match("%.cc$") then
        return cpp_source_content()
    end

    if projectRelativeFile:match("%.h$") or projectRelativeFile:match("%.hpp$") or projectRelativeFile:match("%.inl$") then
        return plain_header_content()
    end

    return license_header()
end

function bb.scaffold.create_authored_file(desc, rootRelativeFile, projectRelativeFile, category)
    local absolute = path.join(BLUE_ROOT, rootRelativeFile)
    if os.isfile(absolute) then
        return false
    end

    bb.fs.write_file(absolute, manifest_file_content(desc.name, projectRelativeFile))
    bb.log.info("Created " .. rootRelativeFile .. " from " .. tostring(category) .. " template")
    return true
end

function bb.scaffold.create_project(projectName, root, projectType, linkage)
    projectName = tostring(projectName or "")
    root = normalize_path(root)
    projectType = projectType or detect_project_type(root)
    linkage = linkage or "auto"

    local projectFile = path.join(root, "project.lua")
    local absoluteProjectFile = path.join(BLUE_ROOT, projectFile)

    if os.isfile(absoluteProjectFile) then
        bb.log.info("Project file already exists: " .. projectFile)
        return false
    end

    if os.isdir(path.join(BLUE_ROOT, root)) and not is_empty_directory(root) then
        error("Cannot scaffold project into non-empty directory without project.lua: " .. root)
    end

    bb.log.info("Scaffolding missing " .. projectType .. " project: " .. root)
    create_project_files(projectName, root, projectType)
    bb.fs.write_file(absoluteProjectFile, project_template(projectName, root, projectType):gsub('linkage = "auto"', 'linkage = "' .. linkage .. '"'))
    bb.log.info("Created " .. projectFile)
    return true
end

function bb.scaffold.is_enabled()
    return _OPTIONS["blue-scaffold"] ~= nil
end

function bb.scaffold.ensure_included_project(reference)
    local projectFile, root = normalize_project_reference(reference)
    local absoluteProjectFile = path.join(BLUE_ROOT, projectFile)

    if os.isfile(absoluteProjectFile) then
        return projectFile
    end

    if not bb.scaffold.is_enabled() then
        error("Included project does not exist: " .. projectFile .. "\nRun with --blue-scaffold to create missing included projects from Blue templates.")
    end

    if os.isdir(path.join(BLUE_ROOT, root)) and not is_empty_directory(root) then
        error("Cannot scaffold project into non-empty directory without project.lua: " .. root)
    end

    local projectName = basename(root)
    local projectType = detect_project_type(root)

    bb.scaffold.create_project(projectName, root, projectType, "auto")
    return projectFile
end

function bb.scaffold.create_manifest_file(desc, rootRelativeFile, projectRelativeFile, category)
    local absolute = path.join(BLUE_ROOT, rootRelativeFile)
    if os.isfile(absolute) then
        return false
    end

    if not bb.scaffold.is_enabled() then
        error("Project '" .. desc.name .. "' lists missing file: " .. rootRelativeFile .. "\nRun with --blue-scaffold to create missing listed files from Blue templates.")
    end

    return bb.scaffold.create_authored_file(desc, rootRelativeFile, projectRelativeFile, category)
end

function bb.scaffold.validate_manifest_path(desc, projectRelativeFile, category)
    if type(projectRelativeFile) ~= "string" or projectRelativeFile == "" then
        error("Project '" .. desc.name .. "' has invalid file entry in " .. tostring(category))
    end

    if bb.path.is_absolute(projectRelativeFile) or bb.path.is_premake_token_path(projectRelativeFile) then
        error("Project '" .. desc.name .. "' strict file manifest uses non-relative path: " .. projectRelativeFile)
    end

    if has_wildcard(projectRelativeFile) then
        error("Project '" .. desc.name .. "' strict file manifest uses wildcard path: " .. projectRelativeFile)
    end

    local normalized = normalize_path(projectRelativeFile)
    if normalized:find("^%.%./") or normalized:find("/%.%./") then
        error("Project '" .. desc.name .. "' strict file manifest escapes project root: " .. projectRelativeFile)
    end

    return normalized
end
