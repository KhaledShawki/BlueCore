local SANITIZER_ORDER = {
    "asan",
    "ubsan",
    "tsan",
}

local SANITIZER_NAMES = {
    asan = true,
    ubsan = true,
    tsan = true,
}

local function trim(value)
    return value:match("^%s*(.-)%s*$")
end

function bb.parse_sanitizer_set(value)
    local normalized = trim((value or "none"):lower())

    if normalized == "none" then
        return {}
    end

    if normalized == "" then
        error("--sanitizer requires one or more values: asan, ubsan, tsan")
    end

    local sanitizers = {}

    for item in (normalized .. ","):gmatch("(.-),") do
        local name = trim(item)

        if name == "" then
            error("Sanitizer set contains an empty value")
        end

        if name == "none" then
            error("'none' cannot be combined with sanitizer names")
        end

        if not SANITIZER_NAMES[name] then
            error("Unknown sanitizer '" .. name .. "'. Supported sanitizers: asan, ubsan, tsan")
        end

        if sanitizers[name] then
            error("Duplicate sanitizer '" .. name .. "'")
        end

        sanitizers[name] = true
    end

    if sanitizers.tsan and sanitizers.asan then
        error(
            "ThreadSanitizer cannot be combined with AddressSanitizer in one build; "
                .. "use the Blue CLI to request separate sanitizer runs"
        )
    end

    return sanitizers
end

function bb.get_sanitizers()
    return bb.parse_sanitizer_set(_OPTIONS["sanitizer"] or "none")
end

function bb.sanitizer_value(sanitizers)
    local names = {}

    for _, name in ipairs(SANITIZER_ORDER) do
        if sanitizers[name] then
            table.insert(names, name)
        end
    end

    if #names == 0 then
        return "none"
    end

    return table.concat(names, ",")
end

function bb.sanitizer_output_name(sanitizers)
    return bb.sanitizer_value(sanitizers):gsub(",", "-")
end

function bb.is_sanitized_build()
    return bb.sanitizer_value(bb.get_sanitizers()) ~= "none"
end

function bb.get_build_output_root()
    local sanitizers = bb.get_sanitizers()

    if bb.sanitizer_value(sanitizers) == "none" then
        return path.join(BLUE_ROOT, "out")
    end

    return path.join(BLUE_ROOT, "out/sanitizers/" .. bb.sanitizer_output_name(sanitizers))
end

function bb.validate_sanitizers()
    local sanitizers = bb.get_sanitizers()

    if bb.sanitizer_value(sanitizers) == "none" then
        return
    end

    local targetOs = bb.resolve_target_os()
    local toolchain = _OPTIONS["toolchain"] or "default"

    if targetOs == "windows" then
        if toolchain ~= "msvc" then
            error("Sanitized Windows builds currently require --toolchain=msvc")
        end

        if sanitizers.ubsan or sanitizers.tsan then
            error("MSVC on Windows currently supports only AddressSanitizer")
        end

        return
    end

    if targetOs == "linux" or targetOs == "macos" then
        if toolchain ~= "clang" then
            error("Sanitized " .. targetOs .. " builds currently require --toolchain=clang")
        end

        return
    end

    error("Sanitizers are not configured for target OS: " .. tostring(targetOs))
end

function bb.apply_sanitizers()
    local sanitizers = bb.get_sanitizers()

    if bb.sanitizer_value(sanitizers) == "none" then
        return
    end

    bb.validate_sanitizers()

    local targetOs = bb.resolve_target_os()

    filter({})
    symbols("On")

    if targetOs == "windows" then
        filter({ "system:windows", "options:toolchain=msvc" })
        editandcontinue("Off")
        runtimechecks("Off")
        incrementallink("Off")
        sanitize({ "Address" })
        filter({})
        return
    end

    local enabled = {}

    if sanitizers.asan then
        table.insert(enabled, "Address")
    end

    if sanitizers.ubsan then
        table.insert(enabled, "UndefinedBehavior")
    end

    if sanitizers.tsan then
        table.insert(enabled, "Thread")
    end

    filter({
        "system:linux or system:macosx",
        "options:toolchain=clang",
    })

    sanitize(enabled)

    buildoptions({
        "-fno-omit-frame-pointer",
    })

    if sanitizers.ubsan then
        buildoptions({
            "-fno-sanitize-recover=undefined",
        })
    end

    filter({})
end
