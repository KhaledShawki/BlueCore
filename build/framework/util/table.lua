bb.table = bb.table or {}

function bb.table.as_list(value)
    if value == nil then
        return {}
    end

    if type(value) == "string" then
        return { value }
    end

    assert(type(value) == "table", "expected string, table, or nil")
    return value
end

function bb.table.contains(list, value)
    if list == nil then
        return false
    end

    for _, item in ipairs(list) do
        if item == value then
            return true
        end
    end

    return false
end

function bb.table.append_unique(target, source)
    target = target or {}
    for _, value in ipairs(bb.table.as_list(source)) do
        if not bb.table.contains(target, value) then
            table.insert(target, value)
        end
    end
    return target
end

function bb.table.copy_array(source)
    local result = {}
    for _, value in ipairs(source or {}) do
        table.insert(result, value)
    end
    return result
end
