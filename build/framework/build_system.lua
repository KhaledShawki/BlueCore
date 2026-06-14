
local function sanitize_project_name(value)
    local text = tostring(value or "")
    text = text:gsub("[^%w%._%-]+", "")
    if text == "" then
        return "Workspace"
    end
    return text
end

function bb.get_workspace_build_target_name()
    local workspaceDesc = bb.registry.workspace or {}
    return sanitize_project_name(tostring(workspaceDesc.name or "Workspace") .. "Workspace")
end

local function collect_workspace_build_dependencies()
    local dependencies = {}

    for _, desc in pairs(bb.registry.projects or {}) do
        if desc.kind ~= "Utility" then
            table.insert(dependencies, desc.name)
        end
    end

    table.sort(dependencies)
    return dependencies
end

local function quote(value)
    return '"' .. tostring(value or "") .. '"'
end

local function option_value(name, fallback)
    local value = _OPTIONS[name]
    if value == nil or value == "" then
        return fallback
    end
    return value
end

local function command_argument(value)
    local text = tostring(value or "")
    if text == "" then
        return '""'
    end

    if text:find('[%s"]') then
        text = text:gsub("\\", "\\\\"):gsub('"', '\\"')
        return '"' .. text .. '"'
    end

    return text
end

local function build_option_arguments(options)
    options = options or {}

    local args = {}

    local function add(name, value)
        if value and value ~= "" then
            table.insert(args, "--" .. name .. "=" .. value)
        end
    end

    local function add_flag(name)
        table.insert(args, "--" .. name)
    end

    add("toolchain", option_value("toolchain", "default"))
    add("blue-platforms", option_value("blue-platforms", "auto"))
    add("blue-build-platforms", option_value("blue-build-platforms", "default"))
    add("memory-backend", option_value("memory-backend", "system"))
    add("blue-startup", option_value("blue-startup", (bb.registry.workspace and bb.registry.workspace.startproject) or ""))
    add("msvc-toolset", _OPTIONS["msvc-toolset"])
    add("msvc-tools-version", _OPTIONS["msvc-tools-version"])

    if options.include_clion_options == true then
        add("clion-config", option_value("clion-config", "default"))
        add("clion-platform", option_value("clion-platform", "default"))
        add("clion-idea", option_value("clion-idea", "on"))
        add("clion-run-targets", option_value("clion-run-targets", "default"))
        add("clion-build-targets", option_value("clion-build-targets", "default"))
    end

    if _OPTIONS["strict"] then
        add_flag("strict")
    end

    if _OPTIONS["blue-scaffold"] or options.force_scaffold == true then
        add_flag("blue-scaffold")
    end

    return table.concat(args, " ")
end

local function command_for_script(scriptName, extra)
    local command = quote(path.join(BLUE_ROOT, "scripts", scriptName))
    if extra and extra ~= "" then
        command = command .. " " .. extra
    end
    return command
end

local function premake_action_command(actionName, options)
    options = options or {}
    if actionName == "clion" then
        options.include_clion_options = true
    end

    local optionArgs = build_option_arguments(options)
    local args = actionName
    if optionArgs ~= "" then
        args = optionArgs .. " " .. actionName
    end

    return {
        windows = command_for_script("premake-windows.cmd", args),
        linux = command_for_script("premake-linux.sh", args),
        macosx = command_for_script("premake-macos.sh", args),
    }
end

local function regenerate_command(options)
    local actionName = _ACTION or "vs2022"
    local args = actionName
    local optionArgs = build_option_arguments(options)
    if optionArgs ~= "" then
        args = args .. " " .. optionArgs
    end

    return {
        windows = command_for_script("regenerate-windows.cmd", args),
        linux = command_for_script("regenerate-linux.sh", args),
        macosx = command_for_script("regenerate-macos.sh", args),
    }
end

local function platform_postbuild(commandByPlatform)
    return {
        windows = { postbuildcommands = { commandByPlatform.windows } },
        linux = { postbuildcommands = { commandByPlatform.linux } },
        macosx = { postbuildcommands = { commandByPlatform.macosx } },
    }
end

function bb.emit_build_system_projects()
    if bb.registry.build_system_projects_emitted then
        return
    end

    bb.registry.build_system_projects_emitted = true

    -- Ninja is used as a fast command-line backend. Visual Studio-style utility
    -- projects generate warnings and are not useful there; keep helper commands
    -- as Premake actions/scripts for Ninja and emit only real build targets.
    if _ACTION == "ninja" then
        return
    end

    bb.project {
        name = bb.get_workspace_build_target_name(),
        kind = "Utility",
        root = ".",
        group = "Build System",
        default_files = false,
        files = {
            "premake5.lua",
            "build.lua",
        },
        dependson = collect_workspace_build_dependencies(),
    }

    bb.project {
        name = "BlueBuildSystemFiles",
        kind = "Utility",
        root = ".",
        group = "Build System",
        default_files = false,
        files = {
            "premake5.lua",
            "build.lua",
            "build/**/*.lua",
            "modules/**/project.lua",
            "apps/**/project.lua",
            "tests/**/project.lua",
            "scripts/**.cmd",
            "scripts/**.ps1",
            "scripts/**.sh",
            "docs/**/*.md",
            ".clang-format",
            ".editorconfig",
        },
    }

    bb.project {
        name = "BlueRegenerateSolution",
        kind = "Utility",
        root = ".",
        group = "Build System",
        default_files = false,
        files = {
            "premake5.lua",
            "build.lua",
            "build/framework/**/*.lua",
            "modules/**/project.lua",
            "apps/**/project.lua",
            "tests/**/project.lua",
            "scripts/regenerate-*",
        },
        platform = platform_postbuild(regenerate_command()),
    }

    bb.project {
        name = "BlueScaffoldSolution",
        kind = "Utility",
        root = ".",
        group = "Build System",
        default_files = false,
        files = {
            "premake5.lua",
            "build.lua",
            "build/framework/**/*.lua",
            "modules/**/project.lua",
            "apps/**/project.lua",
            "tests/**/project.lua",
            "scripts/regenerate-*",
        },
        platform = platform_postbuild(regenerate_command({ force_scaffold = true })),
    }

    bb.project {
        name = "BlueValidateBuildGraph",
        kind = "Utility",
        root = ".",
        group = "Build System",
        default_files = false,
        files = {
            "build/framework/**/*.lua",
            "modules/**/project.lua",
            "apps/**/project.lua",
            "tests/**/project.lua",
        },
        platform = platform_postbuild(premake_action_command("validate")),
    }



    bb.project {
        name = "BlueListTests",
        kind = "Utility",
        root = ".",
        group = "Build System",
        default_files = false,
        files = {
            "build/framework/**/*.lua",
            "modules/**/project.lua",
            "apps/**/project.lua",
            "tests/**/project.lua",
            "modules/**/tests/*.cpp",
        },
        platform = platform_postbuild(premake_action_command("list-tests")),
    }
end
