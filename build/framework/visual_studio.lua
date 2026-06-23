bb.visual_studio = bb.visual_studio or {}

local VISUAL_STUDIO_ACTIONS = {
    vs2019 = true,
    vs2022 = true,
    vs2026 = true,
}

local STANDARD_MSVC_PROJECT_PLATFORMS = {
    x64 = true,
    Win32 = true,
    ARM64 = true,
}

local function is_visual_studio_action(actionName)
    return VISUAL_STUDIO_ACTIONS[tostring(actionName or "")] == true
end

local function escape_lua_pattern(value)
    return tostring(value or ""):gsub("([%%%^%$%(%)%.%[%]%*%+%-%?])", "%%%1")
end

local function read_file(filename)
    local file, errorMessage = io.open(filename, "rb")
    if not file then
        error("Failed to read file '" .. tostring(filename) .. "': " .. tostring(errorMessage))
    end

    local content = file:read("*a")
    file:close()
    return content
end

local function write_file_if_changed(filename, content)
    local oldContent = read_file(filename)
    if oldContent == content then
        return false
    end

    local file, errorMessage = io.open(filename, "wb")
    if not file then
        error("Failed to write file '" .. tostring(filename) .. "': " .. tostring(errorMessage))
    end

    file:write(content)
    file:close()
    return true
end

local function get_shared_platforms(workspaceDesc)
    local result = {}

    for _, platformName in ipairs(workspaceDesc.platforms or {}) do
        if bb.is_shared_build_platform(platformName) then
            table.insert(result, platformName)
        end
    end

    return result
end

local function has_shared_platforms(workspaceDesc)
    return #get_shared_platforms(workspaceDesc) > 0
end

local function project_file_path(project)
    local location = project.location
        or path.join(BLUE_ROOT, "out/build/" .. (_ACTION or "none") .. "/" .. project.name)
    local filename = project.filename or project.name
    return path.join(location, filename .. ".vcxproj")
end

local function normalize_vcxproj_ide_version(projectFile, required)
    if not os.isfile(projectFile) then
        if required then
            error("Visual Studio project metadata target does not exist: " .. tostring(projectFile))
        end

        return false
    end

    local projectVersion = bb.get_effective_visual_studio_project_version
            and bb.get_effective_visual_studio_project_version()
        or nil
    if not projectVersion then
        return false
    end

    local content = read_file(projectFile)
    local updated = content
    local replacements = 0

    updated, replacements = updated:gsub(
        "<VCProjectVersion>[^<]+</VCProjectVersion>",
        "<VCProjectVersion>" .. projectVersion .. "</VCProjectVersion>"
    )

    if replacements == 0 then
        updated, replacements = updated:gsub(
            '(<PropertyGroup Label="Globals">\r\n)',
            "%1    <VCProjectVersion>" .. projectVersion .. "</VCProjectVersion>\r\n",
            1
        )
    end

    if replacements == 0 then
        updated, replacements = updated:gsub(
            '(<PropertyGroup Label="Globals">\n)',
            "%1    <VCProjectVersion>" .. projectVersion .. "</VCProjectVersion>\n",
            1
        )
    end

    if replacements == 0 then
        error("Failed to locate Visual Studio Globals property group in: " .. tostring(projectFile))
    end

    if updated == content then
        return false
    end

    write_file_if_changed(projectFile, updated)
    return true
end

local function solution_file_path(workspaceDesc)
    return path.join(BLUE_ROOT, "out/build/" .. (_ACTION or "none"), tostring(workspaceDesc.name) .. ".slnx")
end

local function build_project_configuration_mappings(workspaceDesc)
    local lines = {}

    -- Map solution-level shared platforms to native MSVC x64 project configurations.
    for _, buildProfile in ipairs(workspaceDesc.configurations or {}) do
        for _, sharedPlatform in ipairs(get_shared_platforms(workspaceDesc)) do
            local solutionPair = buildProfile .. "|" .. sharedPlatform
            local projectBuildType = buildProfile .. " " .. sharedPlatform

            table.insert(
                lines,
                '    <BuildType Solution="' .. solutionPair .. '" Project="' .. projectBuildType .. '" />'
            )
            table.insert(lines, '    <Platform Solution="' .. solutionPair .. '" Project="x64" />')
        end
    end

    return table.concat(lines, "\n")
end

local function project_has_custom_mapping(projectBody, workspaceDesc)
    for _, sharedPlatform in ipairs(get_shared_platforms(workspaceDesc)) do
        if projectBody:find('<BuildType[^>]-Solution="[^"]*|' .. escape_lua_pattern(sharedPlatform) .. '"') then
            return true
        end

        if projectBody:find('<Platform[^>]-Solution="[^"]*|' .. escape_lua_pattern(sharedPlatform) .. '"') then
            return true
        end
    end

    return false
end

local function add_project_mapping_to_open_project(projectOpenTag, projectBody, projectCloseTag, workspaceDesc)
    if project_has_custom_mapping(projectBody, workspaceDesc) then
        return projectOpenTag .. projectBody .. projectCloseTag
    end

    local mappings = build_project_configuration_mappings(workspaceDesc)
    if mappings == "" then
        return projectOpenTag .. projectBody .. projectCloseTag
    end

    if projectBody:match("^%s*$") then
        return projectOpenTag .. "\n" .. mappings .. "\n  " .. projectCloseTag
    end

    return projectOpenTag .. "\n" .. mappings .. projectBody .. projectCloseTag
end

local function add_project_mapping_to_self_closing_project(projectTag, workspaceDesc)
    local mappings = build_project_configuration_mappings(workspaceDesc)
    if mappings == "" then
        return projectTag
    end

    local openTag = projectTag:gsub("%s*/>%s*$", ">")
    return openTag .. "\n" .. mappings .. "\n  </Project>"
end

local function normalize_slnx_project_mappings(workspaceDesc)
    if not has_shared_platforms(workspaceDesc) then
        return false
    end

    local filename = solution_file_path(workspaceDesc)
    if not os.isfile(filename) then
        error("VS2026 SLNX file was not generated: " .. tostring(filename))
    end

    local content = read_file(filename)
    local updated = content
    local replacements = 0
    local count = 0

    updated, count = updated:gsub("(<Project [^>]-/>%s*)", function(projectTag)
        if projectTag:find("<Configuration", 1, true) then
            return projectTag
        end

        replacements = replacements + 1
        return add_project_mapping_to_self_closing_project(projectTag, workspaceDesc)
    end)

    updated, count = updated:gsub(
        "(<Project [^>]->)(.-)(</Project>)",
        function(projectOpenTag, projectBody, projectCloseTag)
            local normalized =
                add_project_mapping_to_open_project(projectOpenTag, projectBody, projectCloseTag, workspaceDesc)
            if normalized ~= projectOpenTag .. projectBody .. projectCloseTag then
                replacements = replacements + 1
            end
            return normalized
        end
    )

    if replacements == 0 then
        return false
    end

    return write_file_if_changed(filename, updated)
end

function bb.visual_studio.is_action(actionName)
    return is_visual_studio_action(actionName or _ACTION)
end

function bb.visual_studio.uses_nonstandard_platforms(platforms)
    for _, platformName in ipairs(platforms or {}) do
        if not STANDARD_MSVC_PROJECT_PLATFORMS[platformName] then
            return true
        end
    end

    return false
end

function bb.visual_studio.install_slnx_platform_axis_mapping(workspaceDesc)
    if _ACTION ~= "vs2026" then
        return
    end

    if not bb.visual_studio.uses_nonstandard_platforms(workspaceDesc.platforms) then
        return
    end

    if bb.registry.slnx_platform_axis_mapping_installed then
        return
    end

    local action = premake.action.current()
    local originalOnProject = action.onProject
    local originalOnEnd = action.onEnd

    assert(type(originalOnProject) == "function", "VS2026 action does not expose onProject hook")

    action.onProject = function(project)
        originalOnProject(project)

        if normalize_vcxproj_ide_version(project_file_path(project), true) then
            bb.log.info("Applied Visual Studio project metadata for " .. project.name)
        end
    end

    action.onEnd = function(...)
        if type(originalOnEnd) == "function" then
            originalOnEnd(...)
        end

        if normalize_slnx_project_mappings(workspaceDesc) then
            bb.log.info("Mapped VS2026 SLNX custom linkage platforms to native C++ project configurations")
        end
    end

    bb.registry.slnx_platform_axis_mapping_installed = true
end
