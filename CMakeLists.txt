cmake_minimum_required(VERSION 3.16)

project(
    cli11_wrapper
        VERSION
            0.1.0 # beta
        HOMEPAGE_URL
            "https://github.com/Ruzovej/cli11_wrapper"
        DESCRIPTION
            "Small wrapper to ease use of the CLI11 library"
        LANGUAGES
            CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    cli11
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.6.2.tar.gz
    # GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    # GIT_TAG        v2.6.2
)
FetchContent_MakeAvailable(cli11)

# Check if this is being used as a subdirectory via FetchContent, add_subdirectory or similar ...
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    set(CLI11WRAPPER_TOP_LEVEL ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
else()
    set(CLI11WRAPPER_TOP_LEVEL OFF)
endif()

if (CLI11WRAPPER_TOP_LEVEL)
    # TODO all interesting ones, e.g.:
    # - https://github.com/google/sanitizers
    # - https://hpc-wiki.info/hpc/Compiler_Sanitizers
    # - https://clang.llvm.org/docs/index.html (search for `sanitizer`)
    # - https://clang.llvm.org/docs/UsersManual.html#command-line-options (or here ...)
    # - https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html (ditto)
    option(CLI11WRAPPER_ASAN "Enable AddressSanitizer" OFF)
    option(CLI11WRAPPER_MSAN "Enable MemorySanitizer" OFF) # Why this one doesn't work?! Maybe it's only for `clang`?
    option(CLI11WRAPPER_TSAN "Enable ThreadSanitizer" OFF)
    option(CLI11WRAPPER_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

    if (CLI11WRAPPER_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fno-omit-frame-pointer)
    endif()

    if (CLI11WRAPPER_MSAN)
        add_compile_options(-fsanitize=memory -fno-omit-frame-pointer)
        add_link_options(-fsanitize=memory -fno-omit-frame-pointer)
    endif()

    if (CLI11WRAPPER_TSAN)
        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread -fno-omit-frame-pointer)
    endif()

    if (CLI11WRAPPER_UBSAN)
        add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined -fno-omit-frame-pointer)
    endif()
endif()

# TODO all interesting ones, e.g.:
# - https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
# - https://clang.llvm.org/docs/DiagnosticsReference.html
#add_compile_options(-Wall -Wextra -Wpedantic)

# core library; source files:
# https://cmake.org/cmake/help/latest/command/file.html#filesystem
file(
    GLOB_RECURSE
        CLI11WRAPPER_SOURCES
            CONFIGURE_DEPENDS
            "${cli11_wrapper_SOURCE_DIR}/src/*.cxx"
)

add_library(
    cli11_wrapper
        SHARED
            ${CLI11WRAPPER_SOURCES}
)
target_include_directories(
    cli11_wrapper
        PUBLIC
            "${cli11_wrapper_SOURCE_DIR}/include"
)
target_link_libraries(
    cli11_wrapper
        PUBLIC
            CLI11::CLI11
)

add_library(
    cli11_wrapper::cli11_wrapper
        ALIAS
            cli11_wrapper
)

if (CLI11WRAPPER_TOP_LEVEL)
    add_subdirectory(tests/unit)
endif()
