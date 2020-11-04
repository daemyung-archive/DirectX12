#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_library(common
    STATIC include/common/utility.h
           include/common/window.h
           include/common/example.h
           include/common/resource_uploader.h
           include/common/timer.h
           include/common/camera.h
           include/common/image_loader.h
           include/common/compiler.h
               src/utility.cpp
               src/window.cpp
               src/example.cpp
               src/resource_uploader.cpp
               src/timer.cpp
               src/camera.cpp
               src/image_loader.cpp
               src/compiler.cpp)

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
           dxguid
           dxgi
           d3dcompiler
           d3d12)
