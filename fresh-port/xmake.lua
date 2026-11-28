-- Independent, inactive prototype. Adapted to the upstream BedrockTools
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
        add_cxxflags("-O2", "-fno-strict-aliasing")
        add_shflags("-Wl,-z,max-page-size=16384")
        add_syslinks("log", "dl")
    end
