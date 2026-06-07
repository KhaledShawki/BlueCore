bb.files = bb.files or {}

local CATEGORIES = {
    "public_headers",
    "private_headers",
    "sources",
    "resources",
    "natvis",
}

local function normalize_path(value)
    value = tostring(value or "")
    value = value:gsub("\\", "/")
    value = value:gsub("/+", "/")
    value = value:gsub("^%./", "")
    return value
end

local function is_structured_manifest(value)
    if type(value) ~= "table" then
        return false
    end

    for _, key in ipairs(CATEGORIES) do
        if value[key] ~= nil then
            return true
        end
    end

    return value.platform ~= nil
end

local function append_manifest_category(desc, manifest, category, target, seen)
    for _, entry in ipairs(bb.table.as_list(manifest[category])) do
        local projectRelative = bb.scaffold.validate_manifest_path(desc, entry, category)
        local rootRelative = normalize_path(path.join(desc.root, projectRelative))
        local key = rootRelative:lower()

        if seen[key] then
            error("Project '" .. desc.name .. "' lists duplicate file: " .. rootRelative)
        end

        seen[key] = true
        bb.scaffold.create_manifest_file(desc, rootRelative, projectRelative, category)
        table.insert(target, rootRelative)
    end
end

local function flatten_structured_manifest(desc, manifest)
    local result = {}
    local seen = {}

    for _, category in ipairs(CATEGORIES) do
        append_manifest_category(desc, manifest, category, result, seen)
    end

    return result, seen
end

local function flatten_platform_manifests(desc, manifest, baseSeen)
    local result = {}

    for platformName, platformManifest in pairs(manifest.platform or {}) do
        if not result[platformName] then
            result[platformName] = {}
        end

        local platformFiles, platformSeen = flatten_structured_manifest(desc, platformManifest)
        for _, file in ipairs(platformFiles) do
            local key = file:lower()
            if baseSeen[key] then
                error("Project '" .. desc.name .. "' lists file both as common and platform-specific: " .. file)
            end
            table.insert(result[platformName], file)
        end
    end

    return result
end

function bb.files.prepare_project_files(desc)
    local files = desc.files
    desc._project_files = {}
    desc._platform_project_files = {}
    desc._structured_files = false

    if files == nil then
        return
    end

    if is_structured_manifest(files) then
        desc._structured_files = true
        local baseFiles, baseSeen = flatten_structured_manifest(desc, files)
        desc._project_files = baseFiles
        desc._platform_project_files = flatten_platform_manifests(desc, files, baseSeen)
        return
    end

    desc._project_files = bb.table.as_list(files)
end

function bb.files.project_files(desc)
    return desc._project_files or {}
end

function bb.files.platform_files(desc, platformName)
    local byPlatform = desc._platform_project_files or {}
    return byPlatform[platformName] or {}
end

function bb.files.is_structured(desc)
    return desc._structured_files == true
end
