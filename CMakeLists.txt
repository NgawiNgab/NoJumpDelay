cmake_minimum_required(VERSION 3.22)

project(apollon_killaura LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- LeviLauncher package identity -----------------------------------------
set(MOD_ID "ApollonKillAura" CACHE STRING "LeviLauncher mod package directory name")
set(MOD_NAME "Apollon KillAura" CACHE STRING "LeviLauncher mod display name")
set(MOD_AUTHOR "NgawiNgab" CACHE STRING "LeviLauncher mod author")
set(MOD_VERSION "1.0.1" CACHE STRING "LeviLauncher mod version")
set(MOD_LIBRARY_NAME "ApollonKillAura" CACHE STRING
    "Native library name without lib prefix or .so suffix")
# Target game build (user constraint: manifest.json must target 1.26.60).
set(MOD_MINECRAFT_VERSIONS "[\"1.26.60\"]" CACHE STRING
    "Raw JSON array for manifest minecraft_versions")
set(MOD_ICON "" CACHE STRING "Optional package-relative icon path, such as icon.png")

# ---- dependencies (vendored — no network FetchContent) ----------------------
# third_party/fmt            fmt 11.2.0 (header + src)
# third_party/nlohmann       nlohmann/json.hpp v3.11.3 (single header)
# third_party/preloader      pl SDK headers (preloader-android 0.2.2 subset)
# third_party/android        android/log.h stub (host/CI builds)
# third_party/jni.h          JNI stub (host/CI builds)
add_library(preloader_sdk_headers INTERFACE)
target_include_directories(preloader_sdk_headers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/preloader/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
)

add_library(fmt STATIC
    third_party/fmt/src/format.cc
)
target_include_directories(fmt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include")
target_compile_features(fmt PUBLIC cxx_std_20)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---- native mod library -----------------------------------------------------
if(ANDROID)
    add_library(${MOD_LIBRARY_NAME} SHARED
        src/main.cpp
        src/kaura/AuraCore.cpp
        src/kaura/AuraConfig.cpp
        src/kaura/KillAuraMod.cpp
        src/kaura/re/Prologues.cpp
        src/kaura/re/Resolver.cpp
        src/kaura/re/GameAccess.cpp
        src/kaura/re/Hooks.cpp
    )
    set_target_properties(${MOD_LIBRARY_NAME} PROPERTIES
        OUTPUT_NAME "${MOD_LIBRARY_NAME}"
    )
    # User constraint flags (arm64-v8a, 16 KB page alignment).
    target_compile_options(${MOD_LIBRARY_NAME} PRIVATE
        -O2
        -fno-strict-aliasing
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )
    target_link_options(${MOD_LIBRARY_NAME} PRIVATE
        "-Wl,-z,max-page-size=16384"
    )
    target_link_libraries(${MOD_LIBRARY_NAME} PRIVATE
        log
        fmt
        nlohmann_json
        preloader_sdk_headers
    )
    if(TARGET preloader)
        # Preloader SDK built as part of this tree (Levi_LINK_PRELOADER flow).
        target_link_libraries(${MOD_LIBRARY_NAME} PRIVATE preloader)
    else()
        # SDK symbols are resolved at runtime by the launcher when the
        # preloader target is not linked directly.
        target_link_options(${MOD_LIBRARY_NAME} PRIVATE
            "-Wl,--unresolved-symbols=ignore-all"
        )
    endif()

    if(NOT CMAKE_STRIP)
        message(FATAL_ERROR "CMAKE_STRIP is required to strip the generated library.")
    endif()
    add_custom_command(TARGET ${MOD_LIBRARY_NAME} POST_BUILD
        COMMAND "${CMAKE_STRIP}" --strip-unneeded "$<TARGET_FILE:${MOD_LIBRARY_NAME}>"
        COMMENT "Stripping symbols from $<TARGET_FILE_NAME:${MOD_LIBRARY_NAME}>"
        VERBATIM
    )
else()
    # Host/CI build only compiles the engine + config sources for the unit
    # tests (no Android game bindings).
    add_library(kaura_host STATIC
        src/kaura/AuraCore.cpp
        src/kaura/AuraConfig.cpp
    )
    target_include_directories(kaura_host PUBLIC src "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
    target_compile_options(kaura_host PRIVATE -O2 -fno-strict-aliasing)
endif()

# ---- host unit tests (ASan/UBSan) -------------------------------------------
if(NOT ANDROID)
    enable_testing()
    add_executable(test_aura_core
        tests/test_aura_core.cpp
    )
    target_link_libraries(test_aura_core PRIVATE kaura_host)
    target_compile_options(test_aura_core PRIVATE
        -g -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(test_aura_core PRIVATE -fsanitize=address,undefined)
    add_test(NAME aura_core COMMAND test_aura_core)

    add_custom_target(run_tests
        COMMAND "$<TARGET_FILE:test_aura_core>"
        DEPENDS test_aura_core
        COMMENT "Running host unit tests (ASan/UBSan)"
        VERBATIM
    )
endif()

# ---- manifest + levipack packaging ------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
    @ONLY
)

if(ANDROID)
    set(LEVI_PACKAGE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/package")
    set(LEVI_PACKAGE_MOD_DIR "${LEVI_PACKAGE_ROOT}/${MOD_ID}")
    set(LEVI_PACKAGE_FILE
        "${CMAKE_CURRENT_BINARY_DIR}/${MOD_ID}-${MOD_VERSION}.levipack")

    add_custom_target(levi_package
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${LEVI_PACKAGE_ROOT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${LEVI_PACKAGE_MOD_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${MOD_LIBRARY_NAME}>"
                "${LEVI_PACKAGE_MOD_DIR}/lib${MOD_LIBRARY_NAME}.so"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
                "${LEVI_PACKAGE_MOD_DIR}/manifest.json"
        COMMAND "${CMAKE_COMMAND}" -E chdir "${LEVI_PACKAGE_ROOT}"
                "${CMAKE_COMMAND}" -E tar "cf" "${LEVI_PACKAGE_FILE}"
                --format=zip "${MOD_ID}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS ${MOD_LIBRARY_NAME}
        COMMENT "Packaging ${MOD_ID} for LeviLauncher"
        VERBATIM
    )
endif()
