bb = bb or {}

local function import(relativePath)
    local filename = path.join(BLUE_ROOT, relativePath)
    if not os.isfile(filename) then
        error("Blue build framework file not found: " .. filename)
    end
    return dofile(filename)
end

bb.import = import

bb.import("build/framework/util/table.lua")
bb.import("build/framework/util/path.lua")
bb.import("build/framework/util/fs.lua")
bb.import("build/framework/util/log.lua")
bb.import("build/framework/registry.lua")
bb.import("build/framework/options.lua")
bb.import("build/framework/platforms.lua")
bb.import("build/framework/profiles.lua")
bb.import("build/framework/linkage.lua")
bb.import("build/framework/scaffold.lua")
bb.import("build/framework/files.lua")
bb.import("build/framework/commands.lua")
bb.import("build/framework/visual_studio.lua")
bb.import("build/framework/toolchains.lua")
bb.import("build/framework/usage.lua")
bb.import("build/framework/workspace.lua")
bb.import("build/framework/dependency.lua")
bb.import("build/framework/project.lua")
bb.import("build/framework/testing.lua")
bb.import("build/framework/regeneration.lua")
bb.import("build/framework/build_system.lua")
bb.import("build/framework/validation.lua")
bb.import("build/framework/graph.lua")
bb.import("build/framework/metadata.lua")
bb.import("build/framework/formatting.lua")
bb.import("build/framework/clion.lua")
bb.import("build/framework/actions.lua")

function bb.include_files(files)
    assert(type(files) == "table", "expected table")
    for _, file in ipairs(files) do
        include(path.join(BLUE_ROOT, file))
    end
end

function bb.include_projects(files)
    assert(type(files) == "table", "expected table")
    for _, projectFile in ipairs(files) do
        include(path.join(BLUE_ROOT, bb.scaffold.ensure_included_project(projectFile)))
    end
end

function bb.include_dependencies(files)
    bb.include_files(files)
end

function bb.finalize()
    bb.emit_build_system_projects()
    bb.emit_formatting_projects()
    bb.emit_test_runner_project()
    bb.validate_all()
end
