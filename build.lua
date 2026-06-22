bb.workspace {
    name = "Blue",
    startproject = "BlueTests",
    cppdialect = "C++20",

    configurations = bb.get_default_build_configurations(),
    platforms = bb.get_default_build_platforms(),
}

bb.include_dependencies {
    "build/third_party/mimalloc.lua",
    "build/third_party/googletest.lua",
    "build/third_party/googlebenchmark.lua",
}

bb.include_projects {
    "modules/BlueSystem/project.lua",
    "modules/BlueMemory/project.lua",
    "modules/BlueContainer/project.lua",
    "modules/BlueJobSystem/project.lua",
    "tests/BlueTests/project.lua",
    "apps/BlueBenchmarks/project.lua",
}
