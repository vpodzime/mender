# We require CMake 3.14 for FetchContent_MakeAvailable, which is used
# to download BOOST when the user selects option MENDER_DOWNLOAD_BOOST.
# Without this option, it works with 3.13 and possibly older versions.
#
cmake_minimum_required(VERSION 3.14)

project(mender)

add_subdirectory(support)
add_subdirectory(Documentation)

set(CMAKE_SYSTEM_PROCESSOR "aarch64")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(can_use_assembler TRUE)
enable_language(CXX C ASM)

if(NOT "${MENDER_NO_BUILD}" STREQUAL "1")
  include(main.cmake)
endif()

