add_executable(mender-update main.cpp)
target_link_libraries(mender-update common_json)
target_include_directories(mender-update PUBLIC ../)
