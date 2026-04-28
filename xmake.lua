set_project("HeptaGenerator")
set_version("0.1.0")
set_languages("c99")
set_allowedplats("mingw", "linux", "macosx")

add_rules("mode.debug", "mode.release")

target("heptagenerator")
    set_kind("binary")
    add_files("main.c", "src/*.c")
    add_includedirs("include")
    set_warnings("all")

    if is_mode("release") then
        set_optimize("fastest")
    end

    if is_plat("linux", "macosx", "mingw") then
        add_cflags("-pthread", { tools = { "clang", "gcc" } })
        add_ldflags("-pthread", { force = true })
    end

    if is_plat("linux", "macosx") then
        add_syslinks("m")
    end

    if is_plat("mingw") then
        add_links("pthread")
    end
