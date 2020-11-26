#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_executable(depth_test src/depth_test.cpp)

target_compile_definitions(depth_test
    PRIVATE DEPTH_TEST_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/asset")

target_link_libraries(depth_test
    PUBLIC common)