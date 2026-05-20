cmake_minimum_required(VERSION 3.14)
project(LazySeqStream CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

message(STATUS "Fetching ADTLinearOrganization...")
FetchContent_Declare(
        ADTLinOrg
        GIT_REPOSITORY https://github.com/VenEraBle842/ADTLinearOrganization.git
        GIT_TAG        master
        SOURCE_SUBDIR  "_none_"
)
FetchContent_MakeAvailable(ADTLinOrg)

message(STATUS "Fetching GoogleTest...")
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(App app/main.cpp app/UI.cpp)

target_include_directories(App PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${adtlinorg_SOURCE_DIR}/src
)

enable_testing()
add_executable(Tests
        tests/test_Ordinal.cpp
        tests/test_LazySequence.cpp
        tests/test_Stream.cpp
        tests/test_SubstringFrequency.cpp
)

target_include_directories(Tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${adtlinorg_SOURCE_DIR}/src
)

target_link_libraries(Tests PRIVATE gtest_main)

include(GoogleTest)
gtest_discover_tests(Tests)
