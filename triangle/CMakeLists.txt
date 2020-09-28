#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_executable(triangle src/triangle.cpp)

target_compile_definitions(triangle
    PRIVATE TRIANGLE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/asset")

target_link_libraries(triangle
    PUBLIC common)