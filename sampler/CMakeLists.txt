#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_executable(sampler src/sampler.cpp)

target_compile_definitions(sampler
    PRIVATE SAMPLER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/asset")

target_link_libraries(sampler
    PUBLIC common)