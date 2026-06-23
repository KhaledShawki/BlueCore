-- Blue root Premake entry point.
-- This file intentionally patches Lua's module search path before loading the framework.

local rootDirectory = path.getabsolute(path.getdirectory(_SCRIPT))
BLUE_ROOT = rootDirectory

package.path = table.concat({
    rootDirectory .. "/?.lua",
    rootDirectory .. "/?/init.lua",
    rootDirectory .. "/build/framework/?.lua",
    rootDirectory .. "/build/framework/?/init.lua",
    package.path,
}, ";")

require("build.framework.init")

bb.load_options()
bb.load_actions()

include(path.join(BLUE_ROOT, "build.lua"))

bb.finalize()
