add_rules("mode.debug", "mode.release")

set_warnings("all", "error")
set_languages("c++20")
add_cxflags("-Wno-maybe-uninitialized")

add_requires("fmt 9.1.0")
add_requires("spdlog 1.10.0", { configs = { fmt_external = true } })
add_requires("boost 1.78.0")

target("miniss")
    set_kind("static")
    add_includedirs("include", { public = true })
    add_files("src/**.cpp")
    add_packages("fmt", { public = true })
    add_packages("spdlog", { public = true })
    add_packages("boost", { public = true })

includes("test/")
