add_executable(mender-auth main.cpp)
target_link_libraries(mender-auth common_json)
target_include_directories(mender-auth PUBLIC ../)
