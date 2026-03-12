cmake_minimum_required(VERSION 3.16)

#cmake_policy(SET CMP0135 NEW) # file timestamps of extracted data are updated to "now"
FetchContent_Declare(
    doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
    #GIT_REPOSITORY  https://github.com/doctest/doctest.git
    #GIT_TAG         v2.4.11
    #GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(doctest)

file(
    GLOB_RECURSE
        CLI11WRAPPER_UNIT_TEST_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_LIST_DIR}/*.test.cxx"
)

add_executable(
    cli11_wrapper_unit_tests
        "${CMAKE_CURRENT_LIST_DIR}/doctest_main.cxx"
        ${CLI11WRAPPER_UNIT_TEST_SOURCES}
)
target_link_libraries(
    cli11_wrapper_unit_tests
        PRIVATE
            doctest::doctest
            cli11_wrapper
)
target_compile_definitions(
    cli11_wrapper_unit_tests
        PRIVATE
            DOCTEST_CONFIG_SUPER_FAST_ASSERTS
            #DOCTEST_CONFIG_NO_TRY_CATCH_IN_ASSERTS
)
