bb.fs = bb.fs or {}

function bb.fs.write_file(filename, content)
    local directory = path.getdirectory(filename)
    if directory and directory ~= "" then
        os.mkdir(directory)
    end

    local file, errorMessage = io.open(filename, "w")
    if not file then
        error("Failed to write file '" .. filename .. "': " .. tostring(errorMessage))
    end

    file:write(content)
    file:close()
end
