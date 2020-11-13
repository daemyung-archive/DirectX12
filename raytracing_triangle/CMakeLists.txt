#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_executable(raytracing_triangle src/raytracing_triangle.cpp)

target_compile_definitions(raytracing_triangle
    PRIVATE RAYTRACING_TRIANGLE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/asset")

target_link_libraries(raytracing_triangle
    PUBLIC common)