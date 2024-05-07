add_rules("mode.debug", "mode.release")

target("my_muduo")
    --add_defines("M_DEBUG")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("example/*.cpp")
    add_includedirs("include")