cmake_minimum_required(VERSION 3.16)

project(RetroDisc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(
    nlohmann_json
    REQUIRED
)


add_executable(
    RetroDisc

    src/main.cpp
    src/config.cpp
    src/filesystem.cpp
    src/runtime.cpp
    src/registry.cpp
    src/discord.cpp
)


target_include_directories(
    RetroDisc

    PRIVATE
    include
)


target_link_libraries(
    RetroDisc

    PRIVATE
    nlohmann_json::nlohmann_json
)