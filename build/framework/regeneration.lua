bb.regeneration = bb.regeneration or {}

local function normalize_path(value)
    value = tostring(value or "")
    value = value:gsub("\\", "/")
    return value
end

local function quote_string(value)
    return tostring(value or ""):gsub("[^%w%._%-]+", "_")
end

local function read_file_binary(filename)
    local file = io.open(filename, "rb")
    if not file then
        return nil
    end

    local content = file:read("*a")
    file:close()
    return content or ""
end

local function fnv1a32(text, seed)
    local hash = seed or 2166136261
    for index = 1, #text do
        hash = (hash ~ text:byte(index)) & 0xffffffff
        hash = (hash * 16777619) & 0xffffffff
    end
    return hash
end

local function hash_manifest(text)
    -- Stable 64-bit token from two independent 32-bit FNV-1a passes.
    local high = fnv1a32(text, 2166136261)
    local low = fnv1a32(text, 2166136261 ~ 0x9e3779b9)
    return string.format("%08x%08x", high, low)
end

local function collect_files(patterns)
    local result = {}
    local seen = {}

    for _, pattern in ipairs(patterns) do
        for _, file in ipairs(os.matchfiles(path.join(BLUE_ROOT, pattern))) do
            local relative = normalize_path(path.getrelative(BLUE_ROOT, file))
            if relative ~= "" and not seen[relative] then
                seen[relative] = true
                table.insert(result, relative)
            end
        end
    end

    table.sort(result)
    return result
end

local function append_content_entries(lines, label, patterns)
    table.insert(lines, "[" .. label .. "]")

    for _, relative in ipairs(collect_files(patterns)) do
        local absolute = path.join(BLUE_ROOT, relative)
        local content = read_file_binary(absolute) or ""
        table.insert(lines, relative .. ":" .. hash_manifest(content))
    end

    table.insert(lines, "")
end

local function append_inventory_entries(lines, label, patterns)
    table.insert(lines, "[" .. label .. "]")

    for _, relative in ipairs(collect_files(patterns)) do
        table.insert(lines, relative)
    end

    table.insert(lines, "")
end

function bb.regeneration.get_generation_action()
    return _OPTIONS["regen-action"] or _ACTION or "none"
end

function bb.regeneration.get_token_name()
    local actionName = quote_string(bb.regeneration.get_generation_action())
    local platformName = quote_string(_OPTIONS["blue-platforms"] or "auto")
    local toolchainName = quote_string(_OPTIONS["toolchain"] or "default")
    local startupName = quote_string(_OPTIONS["blue-startup"] or "default")
    return actionName .. "-" .. platformName .. "-" .. toolchainName .. "-" .. startupName
end

function bb.regeneration.get_token_file()
    return path.join(BLUE_ROOT, "out/build/.blue/premake", bb.regeneration.get_token_name() .. ".token")
end

function bb.regeneration.build_manifest()
    local workspaceDesc = bb.registry.workspace or {}
    local lines = {}

    table.insert(lines, "BlueBuildGraphTokenVersion=1")
    table.insert(lines, "GenerationAction=" .. bb.regeneration.get_generation_action())
    table.insert(lines, "Host=" .. tostring(os.host()))
    table.insert(lines, "TargetPlatforms=" .. tostring(_OPTIONS["blue-platforms"] or "auto"))
    table.insert(lines, "Toolchain=" .. tostring(_OPTIONS["toolchain"] or "default"))
    table.insert(lines, "MsvcToolset=" .. tostring(_OPTIONS["msvc-toolset"] or "auto"))
    table.insert(lines, "MsvcToolsVersion=" .. tostring(_OPTIONS["msvc-tools-version"] or "auto"))
    table.insert(lines, "MemoryBackend=" .. tostring(_OPTIONS["memory-backend"] or "system"))
    table.insert(lines, "Strict=" .. tostring(_OPTIONS["strict"] and "true" or "false"))
    table.insert(lines, "Startup=" .. tostring(_OPTIONS["blue-startup"] or workspaceDesc.startproject or ""))
    table.insert(lines, "Workspace=" .. tostring(workspaceDesc.name or ""))
    table.insert(lines, "Configurations=" .. table.concat(workspaceDesc.configurations or {}, ";"))
    table.insert(lines, "Platforms=" .. table.concat(workspaceDesc.selectedPlatforms or {}, ";"))
    table.insert(lines, "")

    append_content_entries(lines, "BuildGraphFiles", {
        "premake5.lua",
        "build.lua",
        "build/framework/**/*.lua",
        "build/third_party/**/*.lua",
        "modules/**/project.lua",
        "apps/**/project.lua",
        "tests/**/project.lua",
        "scripts/premake-*",
        "scripts/regenerate-*",
    })

    append_inventory_entries(lines, "SourceInventory", {
        "modules/**/*.h",
        "apps/**/*.h",
        "tests/**/*.h",
        "modules/**/*.hpp",
        "apps/**/*.hpp",
        "tests/**/*.hpp",
        "modules/**/*.inl",
        "apps/**/*.inl",
        "tests/**/*.inl",
        "modules/**/*.c",
        "apps/**/*.c",
        "tests/**/*.c",
        "modules/**/*.cpp",
        "apps/**/*.cpp",
        "tests/**/*.cpp",
        "tools/**/*.h",
        "tools/**/*.hpp",
        "tools/**/*.cpp",
    })

    return table.concat(lines, "\n") .. "\n"
end

function bb.regeneration.compute_token()
    return hash_manifest(bb.regeneration.build_manifest())
end

function bb.regeneration.read_previous_token()
    local content = read_file_binary(bb.regeneration.get_token_file())
    if not content then
        return nil
    end

    return content:match("token=([0-9a-fA-F]+)")
end

function bb.regeneration.write_current_token()
    local token = bb.regeneration.compute_token()
    local content = table.concat({
        "version=1",
        "generationAction=" .. bb.regeneration.get_generation_action(),
        "platforms=" .. tostring(_OPTIONS["blue-platforms"] or "auto"),
        "toolchain=" .. tostring(_OPTIONS["toolchain"] or "default"),
        "startup=" .. tostring(_OPTIONS["blue-startup"] or "default"),
        "token=" .. token,
        "",
    }, "\n")

    bb.fs.write_file(bb.regeneration.get_token_file(), content)
    return token
end

function bb.regeneration.is_regeneration_required()
    local current = bb.regeneration.compute_token()
    local previous = bb.regeneration.read_previous_token()
    return previous ~= current, current, previous
end

function bb.regeneration.print_status()
    local required, current, previous = bb.regeneration.is_regeneration_required()
    print("[BlueBuild] Token file: " .. bb.regeneration.get_token_file())
    print("[BlueBuild] Previous token: " .. tostring(previous or "<none>"))
    print("[BlueBuild] Current token : " .. current)

    if required then
        print("[BlueBuild] Regeneration required.")
    else
        print("[BlueBuild] Build graph unchanged. Regeneration can be skipped.")
    end

    return required
end
