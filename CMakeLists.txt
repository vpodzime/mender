cmake_minimum_required(VERSION 3.15)

# fail hard when some include doesn't 100% work
cmake_policy(SET CMP0111 NEW)

# update timestamps of downloaded files after extraction instead of keeping the timestamps from the archive
cmake_policy(SET CMP0135 NEW)

project(mender)

set(CMAKE_VERBOSE_MAKEFILE ON)
enable_testing()


### BEGIN taken from https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)

set(GTEST_VERSION 1.12.1)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-${GTEST_VERSION}.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
### END

add_library(mender_compiler_flags INTERFACE)
target_compile_features(mender_compiler_flags INTERFACE cxx_std_11)

add_executable(mender main.cpp lib.cpp)
target_link_libraries(mender PUBLIC mender_compiler_flags)

add_executable(main_test main_test.cpp lib.cpp)
target_link_libraries(main_test PUBLIC mender_compiler_flags)
target_link_libraries(main_test PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(main_test)

add_subdirectory(common)
add_subdirectory(mender-update)
add_subdirectory(mender-auth)
