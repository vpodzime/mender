include(CheckIncludeFileCXX)
add_library(common_tar STATIC)
find_package(LibArchive)
check_include_file_cxx("archive.h" HAVE_archive_h)
if (NOT LibArchive_FOUND OR NOT HAVE_archive_h)
  include(FetchContent)
  FetchContent_Declare(libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive/
    GIT_TAG v3.8.3)

  set(ENABLE_ZSTD OFF)
  FetchContent_MakeAvailable(libarchive)
  set(LibArchive_LIBRARIES archive_static)
endif()

target_link_libraries(common_tar PUBLIC
  ${LibArchive_LIBRARIES}
  common_error
  common_log
  common_io
)
target_sources(common_tar PRIVATE
  tar.cpp
  tar_errors.cpp
  platform/libarchive/tar.cpp
  platform/libarchive/wrapper.cpp
)
target_include_directories(common_tar PRIVATE )
target_include_directories(common_tar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform)
