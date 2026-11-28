-- Independent port. Adapted to the upstream BedrockTools
-- preloader package layout; do not use the legacy root CMake build.
add_rules("mode.debug", "mode.release")

package("preloader")
    add_urls("https://github.com/LiteLDev/preloader-android.git")
    add_versions("research-pin", "cc08b40a31471d3e2dedb267f5f17b28da2b0604")
    add_deps("cmake")
    on_install("android", function (package)
        import("package.tools.cmake").install(package)
    end)
package_end()

add_requires("preloader research-pin")
add_requires("nlohmann_json v3.11.3")
add_requires("fmt 11.2.0")

target("ApollonKillAura")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/*.cpp", "native/*.cpp")
    add_includedirs("src", "native")
    add_packages("preloader", "nlohmann_json", "fmt")
    if is_plat("android") then
        set_runtimes("c++_static")
        add_cxxflags("-O2", "-fno-strict-aliasing")
        -- Disable the driver's implicit libc++ selection and resolve the archives
        -- before the preloader shared library, as in build_native.sh.
        add_shflags("-static-libstdc++", "-nostdlib++", "-Wl,--exclude-libs,libc++_static.a",
                    "-Wl,--exclude-libs,libc++abi.a", "-Wl,-z,max-page-size=16384",
                    "-Wl,--start-group", "-l:libc++_static.a", "-l:libc++abi.a", "-Wl,--end-group",
                    {force = true})
        add_syslinks("log", "dl")
    end

    after_build(function (target)
        if target:is_plat("android") then
            os.vrunv("python3", {path.join(os.projectdir(), "scripts", "check_elf.py"), target:targetfile()})
        end
    end)
