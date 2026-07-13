cmake_minimum_required(VERSION 3.15)
project(ActionFlowEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(ActionFlowEngine main.cpp)

target_include_directories(ActionFlowEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/shaders
)

target_link_libraries(ActionFlowEngine PRIVATE
    SDL2::SDL2 SDL2::SDL2main
    bgfx::bgfx bgfx::bx bgfx::bimg
    imgui::imgui
)