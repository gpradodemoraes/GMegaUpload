cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(GMegaUpload LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT VCPKG_ROOT AND DEFINED ENV{VCPKG_ROOT})
  set(VCPKG_ROOT "$ENV{VCPKG_ROOT}")
endif()

find_package(fmt CONFIG REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")

    configure_file(
      "${PROJECT_SOURCE_DIR}/git_hash.h.in"
      "${CMAKE_CURRENT_BINARY_DIR}/git_hash.h"
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(gen-version-h ALL
      COMMAND "${CMAKE_COMMAND}"
      "-D" "SRC_DIR=${PROJECT_SOURCE_DIR}"
      "-D" "BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}"
      "-P" "${PROJECT_SOURCE_DIR}/version.cmake"
      COMMENT "Generating git version file"
      BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/git_hash.h
    )
endif()

# --- SDK options: set BEFORE add_subdirectory ---
set(USE_FFMPEG     OFF CACHE BOOL "" FORCE)
set(USE_FREEIMAGE  OFF CACHE BOOL "" FORCE)
set(USE_MEDIAINFO  OFF CACHE BOOL "" FORCE)
set(USE_PDFIUM     OFF CACHE BOOL "" FORCE)
set(ENABLE_SDKLIB_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_SDKLIB_TESTS    OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/mega-sdk)
add_executable(gmegaupload main.cpp thread_test.cpp build_timestamp.cpp)

target_link_libraries(gmegaupload PRIVATE fmt::fmt-header-only)

# MSVC specifics
if(MSVC)
  target_compile_options(gmegaupload PRIVATE /W4 /MP)
  target_link_options(gmegaupload PRIVATE
    $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(gmegaupload PRIVATE third_party/mega-sdk/include)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_dependencies(gmegaupload gen-version-h)
	add_custom_target(file_toucher ALL
		COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/build_timestamp.cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
		BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/build_timestamp.cpp)

	add_dependencies(gmegaupload file_toucher)
endif()


############################
# Copy to source directory
add_custom_target(
    copy-compile-commands ALL
    DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/compile_commands.json
    )
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_LIST_DIR}/compile_commands.json
    DEPENDS
        # Unlike "proper" targets like executables and libraries, 
        # custom command / target pairs will not set up source
        # file dependencies, so we need to list file explicitly here
        generate-compile-commands
        ${CMAKE_BINARY_DIR}/compile_commands.json
    )

# Generate the compilation commands. Necessary so cmake knows where it came
# from and if for some reason you delete it.
add_custom_target(generate-compile-commands
    DEPENDS
        ${CMAKE_BINARY_DIR}/compile_commands.json
    )
############################
