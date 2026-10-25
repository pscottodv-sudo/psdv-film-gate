cmake_minimum_required(VERSION 3.20)
project(psdv_film_gate VERSION 1.2.0 LANGUAGES CXX)

# psdv-film gate emulation - OpenFX plugin for DaVinci Resolve.
#   macOS: universal (arm64 + x86_64) bundle with Metal rendering.
#   Linux: CPU-only bundle, used for automated tests.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
if(APPLE)
    enable_language(OBJCXX)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    endif()
endif()

# ---- OpenFX SDK (headers + C++ Support library, BSD-3-Clause)
set(OPENFX_DIR "" CACHE PATH "Path to an OpenFX checkout (leave empty to download)")
if(NOT OPENFX_DIR)
    # Direct-argument form of FetchContent_Populate: does not add OpenFX's own
    # CMake project (which needs Conan), only downloads the sources.
    include(FetchContent)
    set(openfx_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/openfx-src)
    if(NOT EXISTS ${openfx_SOURCE_DIR}/include/ofxCore.h)
        FetchContent_Populate(openfx
            GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/openfx.git
            GIT_TAG c3f1c2dcf4f51762b794c4fc21181b4a501be1bf
            GIT_SHALLOW FALSE
            SOURCE_DIR ${openfx_SOURCE_DIR}
            BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/openfx-build
            SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/openfx-subbuild)
    endif()
    set(OPENFX_DIR "${openfx_SOURCE_DIR}")
endif()

add_library(ofxsupport STATIC
    ${OPENFX_DIR}/Support/Library/ofxsCore.cpp
    ${OPENFX_DIR}/Support/Library/ofxsImageEffect.cpp
    ${OPENFX_DIR}/Support/Library/ofxsInteract.cpp
    ${OPENFX_DIR}/Support/Library/ofxsLog.cpp
    ${OPENFX_DIR}/Support/Library/ofxsMultiThread.cpp
    ${OPENFX_DIR}/Support/Library/ofxsParams.cpp
    ${OPENFX_DIR}/Support/Library/ofxsProperty.cpp
    ${OPENFX_DIR}/Support/Library/ofxsPropertyValidation.cpp)
target_include_directories(ofxsupport PUBLIC ${OPENFX_DIR}/include ${OPENFX_DIR}/Support/include)

# ---- the plugin
set(PLUGIN_NAME psdv-film-gate)
add_library(${PLUGIN_NAME} MODULE src/PsdvFilmGate.cpp)
target_include_directories(${PLUGIN_NAME} PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(${PLUGIN_NAME} PRIVATE ofxsupport)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "" SUFFIX ".ofx")

if(APPLE)
    set(KERNEL_HDR ${CMAKE_CURRENT_BINARY_DIR}/generated/psdv_kernel_source.h)
    add_custom_command(OUTPUT ${KERNEL_HDR}
        COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src -DOUT=${KERNEL_HDR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_kernel.cmake
        DEPENDS src/core/psdv_prelude.h src/core/psdv_params.h src/core/psdv_core.h
                src/metal/psdv_kernel_body.metal cmake/embed_kernel.cmake
        COMMENT "Embedding Metal kernel source")
    target_sources(${PLUGIN_NAME} PRIVATE src/metal/PsdvMetal.mm ${KERNEL_HDR})
    set_source_files_properties(src/metal/PsdvMetal.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(${PLUGIN_NAME} PRIVATE "-framework Metal" "-framework Foundation")
    set(BUNDLE_ARCH_DIR MacOS)
elseif(UNIX)
    set(BUNDLE_ARCH_DIR Linux-x86-64)
else()
    set(BUNDLE_ARCH_DIR Win64)
endif()

# ---- OFX bundle layout: <name>.ofx.bundle/Contents/<arch>/<name>.ofx
set(BUNDLE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}.ofx.bundle)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packaging/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUNDLE_DIR}/Contents/${BUNDLE_ARCH_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PLUGIN_NAME}> ${BUNDLE_DIR}/Contents/${BUNDLE_ARCH_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/Info.plist ${BUNDLE_DIR}/Contents/Info.plist
    COMMENT "Assembling ${PLUGIN_NAME}.ofx.bundle")

# ---- CPU tests (not part of the plugin)
option(PSDV_BUILD_TESTS "Build the CPU reference renderer used by the tests" ON)
if(PSDV_BUILD_TESTS)
    add_executable(psdv_render tests/psdv_render.cpp)
    target_include_directories(psdv_render PRIVATE src)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(psdv_render PRIVATE OpenMP::OpenMP_CXX)
    endif()
endif()

# ---- GPU vs CPU comparison (Metal on macOS, CPU stub elsewhere)
option(PSDV_BUILD_GPU_TEST "Build psdv_gpu_compare" OFF)
if(PSDV_BUILD_GPU_TEST)
    if(APPLE)
        add_executable(psdv_gpu_compare tests/metal/gpu_compare.cpp tests/metal/gpu_backend_metal.mm
                       src/metal/PsdvMetal.mm ${KERNEL_HDR})
        set_source_files_properties(tests/metal/gpu_backend_metal.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
        target_link_libraries(psdv_gpu_compare PRIVATE "-framework Metal" "-framework Foundation")
    else()
        add_executable(psdv_gpu_compare tests/metal/gpu_compare.cpp tests/metal/gpu_backend_stub.cpp)
        find_package(Threads REQUIRED)
        target_link_libraries(psdv_gpu_compare PRIVATE Threads::Threads)
    endif()
    target_include_directories(psdv_gpu_compare PRIVATE src tests tests/metal ${CMAKE_CURRENT_BINARY_DIR}/generated)
endif()
