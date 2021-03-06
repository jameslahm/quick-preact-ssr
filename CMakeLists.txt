cmake_minimum_required(VERSION 3.4)

project(qpreact LANGUAGES C)

include(GNUInstallDirs)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Building with ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} on ${CMAKE_SYSTEM}")

include(${CMAKE_SOURCE_DIR}/CMakeLists.quickjs.txt)

add_executable(qpreact
    quickjs/quickjs-libc.c
    tiny.c
    preact.c
    htm.c
)

set_target_properties(qpreact PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME qpreact-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>
)

target_compile_definitions(qpreact PRIVATE
    _GNU_SOURCE
)

target_link_libraries(qpreact qjs m pthread dl)
