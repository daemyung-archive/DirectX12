#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_executable(texture src/texture.cpp)

target_compile_definitions(texture
        PRIVATE TEXTURE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/asset")

target_link_libraries(texture
    PUBLIC common)