local function quote(value)
    return '"' .. value .. '"'
end

local function run_test_script()
    local host = os.host()
    local command

    if host == "windows" then
        command = quote(path.join(BLUE_ROOT, "scripts/run-tests-windows.cmd"))
    elseif host == "macosx" then
        command = quote(path.join(BLUE_ROOT, "scripts/run-tests-macos.sh"))
    else
        command = quote(path.join(BLUE_ROOT, "scripts/run-tests-linux.sh"))
    end

    local result = os.execute(command)
    if result ~= true and result ~= 0 then
        error("Blue test script failed")
    end
end

function bb.load_actions()
    newaction {
        trigger = "validate",
        description = "Validate Blue build graph",
        execute = function()
            bb.validate_all()
            bb.log.info("Validation passed")
        end
    }

    newaction {
        trigger = "format",
        description = "Format all C/C++ files using clang-format",
        execute = function()
            bb.run_format_action("format")
            bb.log.info("Formatting completed")
        end
    }

    newaction {
        trigger = "check-format",
        description = "Check C/C++ formatting using clang-format",
        execute = function()
            bb.run_format_action("check")
            bb.log.info("Format check passed")
        end
    }

    newaction {
        trigger = "graph",
        description = "Generate dependency graph",
        execute = function()
            bb.validate_all()
            bb.generate_graph("generated/graphs/dependencies.dot")
            bb.log.info("Dependency graph generated")
        end
    }

    newaction {
        trigger = "metadata",
        description = "Generate project metadata",
        execute = function()
            bb.validate_all()
            bb.generate_metadata("generated/metadata/projects.json")
            bb.log.info("Metadata generated")
        end
    }

    newaction {
        trigger = "list-tests",
        description = "List registered Blue test executables",
        execute = function()
            bb.print_registered_tests()
        end
    }

    newaction {
        trigger = "test-metadata",
        description = "Generate Blue registered test metadata",
        execute = function()
            bb.generate_test_manifest("generated/tests/BlueTests.json")
            bb.log.info("Test metadata generated")
        end
    }

    newaction {
        trigger = "list-format-files",
        description = "List C/C++ files included in Blue formatting",
        execute = function()
            bb.run_format_action("list")
        end
    }

    newaction {
        trigger = "run-tests",
        description = "Generate, build, and run registered Blue tests for the host platform",
        execute = function()
            run_test_script()
        end
    }

    newaction {
        trigger = "build-graph-token",
        description = "Print the current Blue build graph regeneration token",
        execute = function()
            print(bb.regeneration.compute_token())
        end
    }

    newaction {
        trigger = "check-regeneration",
        description = "Check whether solution/project regeneration is required. Exit code 2 means regeneration is required.",
        execute = function()
            local required = bb.regeneration.print_status()
            if required then
                os.exit(2)
            end
            os.exit(0)
        end
    }

    newaction {
        trigger = "update-build-token",
        description = "Write the current Blue build graph regeneration token",
        execute = function()
            local token = bb.regeneration.write_current_token()
            bb.log.info("Updated build graph token: " .. token)
        end
    }

end
