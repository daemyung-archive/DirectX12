//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#define DDSKTX_IMPLEMENT

#include <dds-ktx.h>
#include <common/window.h>
#include <common/example.h>
#include <common/uploader.h>
#include <iostream>
#include <memory>
#include <vector>
#include <array>

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT2 uv;
    XMFLOAT3 normal;
};

//----------------------------------------------------------------------------------------------------------------------

struct Constants {
    XMFLOAT4X4 projection;
    XMFLOAT4X4 view;
    XMFLOAT4X4 model;
    XMFLOAT3X4 normal;
    XMFLOAT3 view_direction;
    float light_distance;
    XMFLOAT3 light_position;
    float light_spot_power;
    XMFLOAT3 light_color;
    float padding0;
    XMFLOAT3 light_direction;
    INT mip_slice;
};

//----------------------------------------------------------------------------------------------------------------------

struct Options {
    float light_distance = 20.0f;
    std::array<float, 3> light_position = {0.0f, 0.0f, -5.0f};
    std::array<float, 3> light_color = {1.0f, 1.0f, 1.0f};
    float light_spot_power = 16.0f;
    std::array<float, 3> light_direction = {0.0f, 0.0f, 1.0f};
    INT mip_slice = 0;
};

//----------------------------------------------------------------------------------------------------------------------

inline auto BuildFilePath(const std::string &file_name) {
    std::filesystem::path file_path;

    file_path = fmt::format("{}/{}", TEXTURE_ASSET_DIR, file_name);
    if (file_path.has_filename()) {
        return file_path;
    }

    throw std::runtime_error(fmt::format("File isn't exist: {}.", file_name));
}

//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> kDescriptorCount = {
        {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         kSwapChainBufferCount},
        {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kImGuiFontBufferCount + 1}};

//----------------------------------------------------------------------------------------------------------------------

class Texture : public Example {
public:
    Texture() : Example("Texture", kDescriptorCount) {
        InitResources();
        InitPipelines();
    }

protected:
    void OnInit() override {
    }

    void OnTerm() override {
    }

    void OnResize(const Resolution &resolution) override {
        // Update a viewport.
        _viewport.Width = static_cast<float>(GetWidth(resolution));
        _viewport.Height = static_cast<float>(GetHeight(resolution));

        // Update a scissor rect.
        _scissor_rect.right = GetWidth(resolution);
        _scissor_rect.bottom = GetHeight(resolution);
    }

    void OnUpdate(UINT index) override {
        // Update ImGui.
        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Light distance", &_options.light_distance, 0.0f, 40.0f);
            ImGui::SliderFloat3("Light position", _options.light_position.data(), -15.0f, 15.0f);
            ImGui::SliderFloat("Light spot power", &_options.light_spot_power, 1.0f, 256.0f);
            ImGui::ColorPicker3("Light color", _options.light_color.data(),
                                ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_DisplayRGB);
            ImGui::SliderFloat3("Light direction", _options.light_direction.data(), -1.0f, 1.0f);
            ImGui::Separator();
            ImGui::SliderInt("Mip slice", &_options.mip_slice, 0, 9);
        }

        // Define constants.
        Constants constants;
        constants.projection = _camera.GetProjection();
        constants.view = _camera.GetView();
        XMStoreFloat4x4(&constants.model, XMMatrixRotationY(XM_PI));
        constants.normal = XMMatrixInverseTranspose(constants.model);
        constants.view_direction = _camera.GetForward();
        constants.light_distance = _options.light_distance;
        constants.light_position = XMFLOAT3(_options.light_position.data());
        constants.light_spot_power = _options.light_spot_power;
        constants.light_color = XMFLOAT3(_options.light_color.data());
        constants.light_direction = XMFLOAT3(_options.light_direction.data());
        constants.mip_slice = _options.mip_slice;

        // Update transformation.
        UpdateBuffer(_constant_buffers[index].Get(), &constants, sizeof(Constants));
    }

    void OnRender(UINT index) override {
        FLOAT clear_color[4] = {};
        D3D12_VIEWPORT viewport = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        D3D12_RECT scissor_rect = {};
        D3D12_RESOURCE_BARRIER resource_barrier = {};

        // Define a transition of a resource from PRESENT to RENDER_TARGET.
        resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                D3D12_RESOURCE_STATE_PRESENT,
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, &resource_barrier);

        // Define a clear color.
        clear_color[0] = 0.025f;
        clear_color[1] = 0.025f;
        clear_color[2] = 0.025f;
        clear_color[3] = 1.0f;

        // Record clearing render target view command.
        _command_list->ClearRenderTargetView(_swap_chain_views[index], clear_color, 0, nullptr);

        // Record commands to draw a triangle.
        _command_list->OMSetRenderTargets(1, &_swap_chain_views[index], true, nullptr);
        _command_list->RSSetViewports(1, &_viewport);
        _command_list->RSSetScissorRects(1, &_scissor_rect);
        _command_list->SetGraphicsRootSignature(_root_signature.Get());

        ID3D12DescriptorHeap *heap = _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get();
        _command_list->SetDescriptorHeaps(1, &heap);
        _command_list->SetGraphicsRootDescriptorTable(1,
                                                      _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());


        _command_list->SetGraphicsRootConstantBufferView(0, _constant_buffers[index]->GetGPUVirtualAddress());
        _command_list->SetPipelineState(_pipeline_state.Get());
        _command_list->IASetVertexBuffers(0, 1, &_vertex_buffer_view);
        _command_list->IASetIndexBuffer(&_index_buffer_view);
        _command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _command_list->DrawIndexedInstanced(6, 1, 0, 0, 0);

        RecordDrawImGuiCommands();

        // Define a transition of a resource from RENDER_TARGET to PRESENT.
        resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                D3D12_RESOURCE_STATE_PRESENT);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, &resource_barrier);
    }

private:
    void InitResources() {
        // Define vertices.
        Vertex vertices[4] = {{XMFLOAT3(1.0f, -1.0f, 0.0f),  XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
                              {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
                              {XMFLOAT3(-1.0f, 1.0f, 0.0f),  XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
                              {XMFLOAT3(1.0f, 1.0f, 0.0f),   XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)}};

        // Device indices.
        UINT16 indices[6] = {1, 0, 3, 1, 3, 2};

        Uploader uploader(_device.Get());

        // Initialize a vertex buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(vertices), &_vertex_buffer));
        uploader.RecordCopyData(_vertex_buffer.Get(), vertices, sizeof(vertices));

        // Initialize an index buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(indices), &_index_buffer));
        uploader.RecordCopyData(_index_buffer.Get(), indices, sizeof(indices));

        // Read a ktx file.
        auto contents = ReadFile(BuildFilePath("metalplate01_rgba.ktx"));
        assert(contents.size());

        // Initialize a texture.
        ddsktx_texture_info texture_info;
        if (ddsktx_parse(&texture_info, contents.data(), static_cast<INT>(contents.size()), nullptr)) {
            ThrowIfFailed(CreateDefaultTexture2D(_device.Get(), texture_info.width, texture_info.height,
                                                 texture_info.num_mips, DXGI_FORMAT_R8G8B8A8_UNORM, &_texture));
            for (auto i = 0; i != texture_info.num_mips; ++i) {
                ddsktx_sub_data sub_data;
                ddsktx_get_sub(&texture_info, &sub_data, contents.data(), static_cast<INT>(contents.size()), 0, 0, i);
                uploader.RecordCopyData(_texture.Get(), i, sub_data.buff, sub_data.row_pitch_bytes * sub_data.height);
            }
        }

        uploader.Execute();

        // Initialize constant buffers.
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            ThrowIfFailed(CreateConstantBuffer(_device.Get(), sizeof(Constants), &_constant_buffers[i]));
        }

        // Initialize a vertex buffer view.
        _vertex_buffer_view.BufferLocation = _vertex_buffer->GetGPUVirtualAddress();
        _vertex_buffer_view.SizeInBytes = sizeof(vertices);
        _vertex_buffer_view.StrideInBytes = sizeof(Vertex);

        // Initialize an index buffer view.
        _index_buffer_view.BufferLocation = _index_buffer->GetGPUVirtualAddress();
        _index_buffer_view.SizeInBytes = sizeof(indices);
        _index_buffer_view.Format = DXGI_FORMAT_R16_UINT;

        // Initialize a texture view.
        _device->CreateShaderResourceView(_texture.Get(), nullptr,
                                          _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
    }

    void InitPipelines() {
        // Define an input layout.
        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        D3D12_DESCRIPTOR_RANGE descriptor_range = {};
        descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptor_range.NumDescriptors = 1;

        // Define a root parameter.
        CD3DX12_ROOT_PARAMETER root_parameters[2];
        root_parameters[0].InitAsConstantBufferView(0);
        root_parameters[1].InitAsDescriptorTable(1, &descriptor_range);

        // Define a static sampler.
        D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
        sampler_desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler_desc.MipLODBias = 0;
        sampler_desc.MinLOD = 0.0f;
        sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
        sampler_desc.ShaderRegister = 0;
        sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Define a root signature.
        CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(2, root_parameters, 1, &sampler_desc,
                                                        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // Create a root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_root_signature));

        // Compile a vertex shader.
        ComPtr<ID3DBlob> vertex_shader;
        ThrowIfFailed(CompileShader(BuildFilePath("lighting.hlsl"), "VSMain", "vs_5_0", &vertex_shader));

        // Compile a pixel shader.
        ComPtr<ID3DBlob> pixel_shader;
        ThrowIfFailed(CompileShader(BuildFilePath("lighting.hlsl"), "PSMain", "ps_5_0", &pixel_shader));

        // Define a graphics pipeline state.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = {input_layout.data(), static_cast<UINT>(input_layout.size())};
        desc.pRootSignature = _root_signature.Get();
        desc.VS = {reinterpret_cast<BYTE *>(vertex_shader->GetBufferPointer()), vertex_shader->GetBufferSize()};
        desc.PS = {reinterpret_cast<BYTE *>(pixel_shader->GetBufferPointer()), pixel_shader->GetBufferSize()};
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = false;
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = kSwapChainFormat;
        desc.SampleDesc = {1, 0};

        // Create a graphics pipeline state.
        ThrowIfFailed(_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&_pipeline_state)));
    }

private:
    Options _options;
    ComPtr<ID3D12Resource> _vertex_buffer;
    ComPtr<ID3D12Resource> _index_buffer;
    ComPtr<ID3D12Resource> _texture;
    FrameResource<ID3D12Resource> _constant_buffers;
    D3D12_VERTEX_BUFFER_VIEW _vertex_buffer_view = {};
    D3D12_INDEX_BUFFER_VIEW _index_buffer_view = {};
    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline_state;
    D3D12_VIEWPORT _viewport = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    D3D12_RECT _scissor_rect = {0, 0, 0, 0};
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        auto example = std::make_unique<Texture>();
        Window::GetInstance()->MainLoop(example.get());
    }
    catch (const std::exception &exception) {
        std::cerr << exception.what() << std::endl;
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------