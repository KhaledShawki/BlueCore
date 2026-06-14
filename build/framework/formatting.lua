local function quote(value)
    return '"' .. tostring(value) .. '"'
end

local function command_argument(value)
    local text = tostring(value or "")
    if text == "" then
        return '""'
    end

    if text:find('[%s"]') then
        text = text:gsub('\\', '\\\\'):gsub('"', '\\"')
        return '"' .. text .. '"'
    end

    return text
end

local function get_host_format_script(mode)
    local host = os.host()

    if host == "windows" then
        if mode == "check" then
            return path.join(BLUE_ROOT, "scripts/format-check-windows.cmd")
        elseif mode == "list" then
            return path.join(BLUE_ROOT, "scripts/list-format-files-windows.cmd")
        end

        return path.join(BLUE_ROOT, "scripts/format-windows.cmd")
    end

    if host == "macosx" then
        if mode == "check" then
            return path.join(BLUE_ROOT, "scripts/format-check-macos.sh")
        elseif mode == "list" then
            return path.join(BLUE_ROOT, "scripts/list-format-files-macos.sh")
        end

        return path.join(BLUE_ROOT, "scripts/format-macos.sh")
    end

    if mode == "check" then
        return path.join(BLUE_ROOT, "scripts/format-check-linux.sh")
    elseif mode == "list" then
        return path.join(BLUE_ROOT, "scripts/list-format-files-linux.sh")
    end

    return path.join(BLUE_ROOT, "scripts/format-linux.sh")
end

local function get_explicit_clang_format()
    if not _OPTIONS then
        return nil
    end

    local explicit = _OPTIONS["format-path"]
    if explicit == nil or explicit == "" then
        return nil
    end

    return explicit
end

local function make_format_command(mode)
    local script = get_host_format_script(mode)
    local command = quote(script)
    local explicitClangFormat = get_explicit_clang_format()

    if explicitClangFormat then
        if os.host() == "windows" then
            command = "set " .. quote("BLUE_CLANG_FORMAT=" .. explicitClangFormat) .. " && " .. command
        else
            command = "BLUE_CLANG_FORMAT=" .. command_argument(explicitClangFormat) .. " " .. command
        end
    end

    return command
end

function bb.run_format_action(mode)
    assert(mode == "format" or mode == "check" or mode == "list", "unknown format action mode")

    local script = get_host_format_script(mode)
    if not os.isfile(script) then
        error("Blue format script not found: " .. script)
    end

    local command = make_format_command(mode)
    local result = os.execute(command)
    if result ~= true and result ~= 0 then
        error("Blue formatting command failed: " .. command)
    end
end

-- Backward-compatible entry point used by older actions.lua versions.
function bb.run_clang_format(checkOnly)
    bb.run_format_action(checkOnly and "check" or "format")
end


local function collect_build_system_files()
    local patterns = {
        ".clang-format",
        ".clang-format-ignore",
        ".editorconfig",
        ".vscode/settings.json",
        ".vscode/extensions.json",
        "build/**/*.lua",
        "scripts/format-*.cmd",
        "scripts/format-*.ps1",
        "scripts/format-*.sh",
        "scripts/list-format-files-*.cmd",
        "scripts/list-format-files-*.sh",
        "docs/FORMATTING.md",
        "docs/IDE_FORMAT_ON_SAVE.md",
    }

    local files = {}
    for _, pattern in ipairs(patterns) do
        for _, file in ipairs(os.matchfiles(path.join(BLUE_ROOT, pattern))) do
            table.insert(files, file)
        end
    end

    return files
end

local function emit_format_utility_project(name, mode, description)
    group("Build System/Formatting")

    project(name)
        kind "Utility"
        location(path.join(BLUE_ROOT, "out/build/" .. (_ACTION or "none") .. "/" .. name))
        files(collect_build_system_files())
        postbuildmessage(description)
        postbuildcommands {
            make_format_command(mode),
        }

    group("")
end

function bb.emit_formatting_projects()
    if bb.registry.formatting_projects_emitted then
        return
    end

    bb.registry.formatting_projects_emitted = true

    -- Ninja should not emit IDE utility projects. Formatting remains available
    -- through `premake format`, `premake check-format`, and platform scripts.
    if _ACTION == "ninja" then
        return
    end

    emit_format_utility_project("BlueFormat", "format", "Formatting Blue C/C++ sources")
    emit_format_utility_project("BlueFormatCheck", "check", "Checking Blue C/C++ formatting")
    emit_format_utility_project("BlueListFormatFiles", "list", "Listing Blue C/C++ format files")
end