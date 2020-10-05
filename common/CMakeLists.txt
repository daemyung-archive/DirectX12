#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_library(common
    STATIC include/common/utility.h
           include/common/window.h
           include/common/example.h
           include/common/uploader.h
           include/common/timer.h
               src/utility.cpp
               src/window.cpp
               src/example.cpp
               src/uploader.cpp
               src/timer.cpp)

target_include_directories(common
    PUBLIC  include
    PRIVATE include/common)

target_compile_features(common
    PUBLIC cxx_std_20)

target_compile_definitions(common
    PUBLIC NOMINMAX
           UNICODE
           WIN32_LEAN_AND_MEAN)

target_link_libraries(common
    PUBLIC external
           dxguid.lib
           dxgi.lib
           d3dcompiler.lib
           d3d12.lib)
