cmake_minimum_required(VERSION 3.18.0)

# Include file check macros honor CMAKE_REQUIRED_LIBRARIES.
cmake_policy(SET CMP0075 NEW)
# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW)

# **************************************************************************** #
#                                   PROJECT                                    #
# **************************************************************************** #

project(ssh-drop)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Static compilation of runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# Set c++ standard and force it into the cache
set(CMAKE_CXX_STANDARD 20 CACHE STRING "" FORCE)
# Force generation of a compile commands file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

# **************************************************************************** #
#                                  PROFILES                                    #
# **************************************************************************** #

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if (isMultiConfig)
    if (NOT "Asan" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Asan)
    endif ()
else ()
    set(allowedBuildTypes Asan Debug Release RelWithDebInfo MinSizeRel)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")

    if (CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif ()
endif ()

set(CMAKE_C_FLAGS_ASAN
        "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING
        "Flags used by the C compiler for Asan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_ASAN
        "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING
        "Flags used by the C++ compiler for Asan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_ASAN
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING
        "Linker flags to be used to create executables for Asan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_ASAN
        "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING
        "Linker flags to be used to create shared libraries for Asan build type." FORCE)

set(CMAKE_MODULE_LINKER_FLAGS_ASAN
        "${CMAKE_MODULE_LINKER_FLAGS_DEBUG}" CACHE STRING
        "Linker flags to be used to create module libraries for Asan build type." FORCE)

# Function to apply ASan flags if the build type is Asan
function(add_asan_flags target)
    if (CMAKE_BUILD_TYPE STREQUAL "Asan")
        if (MSVC)
            target_compile_options(${target} PRIVATE /fsanitize=address)
            target_link_options(${target} PRIVATE /fsanitize=address)
        else ()
            target_compile_options(${target} PRIVATE -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address -fsanitize=leak)
        endif ()
    endif ()
endfunction()

# Function to apply Warning flags
function(add_warn_flags target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -Wpedantic)
    endif ()
endfunction()

# **************************************************************************** #
#                                  EXECUTABLE                                  #
# **************************************************************************** #

add_executable(main)
#add_warn_flags(main)
add_asan_flags(main)

# Windows properties
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Make current executable the default startup project on VS
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT main)
    # Make the configuration build directory the working dir on vs
    set_property(TARGET main PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<CONFIG>)
    # Use the console subsystem for debug so we see the console output
    #set_property(TARGET main PROPERTY WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR
            CMAKE_BUILD_TYPE STREQUAL "Asan")
        set_target_properties(main PROPERTIES WIN32_EXECUTABLE FALSE)
    else ()
        set_target_properties(main PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()
endif ()


# Target properties
set_target_properties(main PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

# Copy runtime directories
foreach (dir config key secret)
    add_custom_command(TARGET main POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/${dir}
            ${dir}
    )
endforeach ()

add_subdirectory("external")
add_subdirectory("src")
