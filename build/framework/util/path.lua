bb.path = bb.path or {}

local function starts_with(value, prefix)
    return string.sub(value, 1, string.len(prefix)) == prefix
end

function bb.path.is_absolute(value)
    if type(value) ~= "string" then
        return false
    end

    if value:match("^%a:[/\\]") then
        return true
    end

    if starts_with(value, "/") or starts_with(value, "\\\\") then
        return true
    end

    return false
end

function bb.path.is_premake_token_path(value)
    if type(value) ~= "string" then
        return false
    end

    return starts_with(value, "%{")
end

function bb.path.to_root_path(value)
    if value == nil then
        return nil
    end

    if type(value) ~= "string" then
        return value
    end

    if value == "" then
        return value
    end

    if bb.path.is_absolute(value) or bb.path.is_premake_token_path(value) then
        return value
    end

    return path.join(BLUE_ROOT, value)
end

function bb.path.to_root_paths(values)
    local result = {}

    for _, value in ipairs(bb.table.as_list(values)) do
        table.insert(result, bb.path.to_root_path(value))
    end

    return result
end
