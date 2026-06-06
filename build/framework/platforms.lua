local function get_target_system_name(targetOs)
    if targetOs == "windows" then
        return "windows"
    end

    if targetOs == "linux" then
        return "linux"
    end

    if targetOs == "macos" then
        return "macosx"
    end

    return "windows"
end

function bb.apply_platforms()
    local targetOs = bb.resolve_target_os and bb.resolve_target_os() or "windows"
    local targetSystem = get_target_system_name(targetOs)

    filter { "platforms:x64 or x64_DLL" }
        architecture "x86_64"
        system(targetSystem)

    filter "system:linux"
        linkgroups "On"

    filter "platforms:x64_DLL"
        visibility "Hidden"
        defines {
            "BLUE_SHARED_LIBRARY=1",
        }

    filter { "platforms:x64_DLL", "system:linux" }
        linkoptions {
            "-Wl,-rpath,'$$ORIGIN'",
        }

    filter { "platforms:x64_DLL", "system:macosx" }
        linkoptions {
            "-Wl,-rpath,@loader_path",
        }

    filter {}
end
