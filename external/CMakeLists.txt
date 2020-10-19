#
# This file is part of the "DirectX12" project
# See "LICENSE" for license information.
#

add_library(external
    STATIC include/d3dx12.h
           include/imconfig.h
           include/imgui.h
           include/imgui_impl_dx12.h
           include/imgui_impl_win32.h
           include/dds-ktx.h
           include/stb_image.h
           include/stb_perlin.h
           include/fmt/core.h
           include/fmt/format.h
           include/fmt/format-inl.h
           include/nlohmann/json.hpp
               src/imgui_internal.h
               src/imstb_truetype.h
               src/imstb_textedit.h
               src/imstb_rectpack.h
               src/imgui.cpp
               src/imgui_draw.cpp
               src/imgui_widgets.cpp
               src/imgui_demo.cpp
               src/imgui_impl_dx12.cpp
               src/imgui_impl_win32.cpp
               src/format.cc)

target_include_directories(external
    PUBLIC include)